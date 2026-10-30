#include "Fog/fog.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>

fogMap::fogMap(Vector2D<float> mapSize)
{
    this->cellsX = static_cast<int>(std::ceil(mapSize.x / static_cast<float>(fogCell)));
    this->cellsY = static_cast<int>(std::ceil(mapSize.y / static_cast<float>(fogCell)));
    this->blocksX = (this->cellsX + fogBlockCells - 1) / fogBlockCells;
    this->blocksY = (this->cellsY + fogBlockCells - 1) / fogBlockCells;
    this->blocks.resize(static_cast<std::size_t>(this->blocksX) * static_cast<std::size_t>(this->blocksY));
}
// fog: the map is cut into blocks up front, but every block stays empty until the team sees into it

void fogMap::stampRow(int cy, int fromX, int toX)
{
    const int blockY = cy / fogBlockCells;
    const int insideY = cy % fogBlockCells;
    for (int cx = fromX; cx <= toX;) {
        const int blockX = cx / fogBlockCells;
        const int insideX = cx % fogBlockCells;
        const int count = std::min(toX - cx + 1, fogBlockCells - insideX);
        const int index = blockY * this->blocksX + blockX;
        std::unique_ptr<Block>& block = this->blocks[static_cast<std::size_t>(index)];
        if (!block) {
            block = std::make_unique<Block>();
            block->fill(static_cast<std::uint8_t>(FogState::Unknown));
        }
        if (std::find(this->lit.begin(), this->lit.end(), index) == this->lit.end()) this->lit.push_back(index);
        std::memset(block->data() + insideY * fogBlockCells + insideX, static_cast<int>(FogState::Sight), static_cast<std::size_t>(count));
        cx += count;
    }
}
// fog: one row of a circle can cross blocks; each stretch inside a block is set to Sight in one go, making the block and listing it in lit the first time

void fogMap::stampCircle(Vector2D<float> feet)
{
    const float cell = static_cast<float>(fogCell);
    const int fromY = std::max(0, static_cast<int>(std::floor((feet.y - viewField) / cell)));
    const int toY = std::min(this->cellsY - 1, static_cast<int>(std::floor((feet.y + viewField) / cell)));
    for (int cy = fromY; cy <= toY; ++cy) {
        const float away = (static_cast<float>(cy) + 0.5f) * cell - feet.y;
        const float square = viewField * viewField - away * away;
        if (square < 0.0f) continue;
        const float half = std::sqrt(square);
        const int fromX = std::max(0, static_cast<int>(std::floor((feet.x - half) / cell)));
        const int toX = std::min(this->cellsX - 1, static_cast<int>(std::floor((feet.x + half) / cell)));
        if (fromX <= toX) this->stampRow(cy, fromX, toX);
    }
}
// fog: the view field is a circle; each cell row gets the circle's width at that row's center height

void fogMap::update(const std::vector<Vector2D<float>>& teamFeet, const std::vector<tree>& trees, const std::vector<wall>& walls)
{
    for (const int index : this->lit) {
        Block& block = *this->blocks[static_cast<std::size_t>(index)];
        for (std::uint8_t& value : block) {
            if (value == static_cast<std::uint8_t>(FogState::Sight)) value = static_cast<std::uint8_t>(FogState::Discovered);
        }
    }
    this->lit.clear();
    for (const Vector2D<float>& feet : teamFeet) this->stampCircle(feet);

    std::erase_if(this->seen, [this](const Seen& entry) { return this->state(entry.pos) == FogState::Sight; });
    for (std::size_t t = 0; t < trees.size(); ++t) {
        if (trees[t].isFelled() || this->state(trees[t].getPos()) != FogState::Sight) continue;
        Seen entry;
        entry.kind = Seen::Kind::Tree;
        entry.pos = trees[t].getPos();
        entry.index = static_cast<int>(t);
        this->seen.push_back(entry);
    }
    for (std::size_t w = 0; w < walls.size(); ++w) {
        if (walls[w].isDestroyed() || this->state(walls[w].getPos()) != FogState::Sight) continue;
        Seen entry;
        entry.kind = Seen::Kind::Wall;
        entry.pos = walls[w].getPos();
        entry.index = static_cast<int>(w);
        entry.built = walls[w].isBuilt();
        entry.progress = walls[w].getProgress();
        this->seen.push_back(entry);
    }
}
// fog: last frame's sight becomes discovered and this frame's circles are stamped over it; every entry in sight is dropped and rebuilt from the live lists, so felled trees and destroyed walls leave the snapshot and the rest keep this frame's state and tree index

FogState fogMap::cellState(int cx, int cy) const
{
    if (cx < 0 || cy < 0 || cx >= this->cellsX || cy >= this->cellsY) return FogState::Unknown;
    const int index = (cy / fogBlockCells) * this->blocksX + cx / fogBlockCells;
    const std::unique_ptr<Block>& block = this->blocks[static_cast<std::size_t>(index)];
    if (!block) return FogState::Unknown;
    return static_cast<FogState>((*block)[static_cast<std::size_t>((cy % fogBlockCells) * fogBlockCells + cx % fogBlockCells)]);
}

FogState fogMap::state(Vector2D<float> point) const
{
    const float cell = static_cast<float>(fogCell);
    return this->cellState(static_cast<int>(std::floor(point.x / cell)), static_cast<int>(std::floor(point.y / cell)));
}

const std::vector<Seen>& fogMap::getSeen() const { return this->seen; }

const Seen* fogMap::seenAt(Seen::Kind kind, Vector2D<float> pos) const
{
    for (const Seen& entry : this->seen) {
        if (entry.kind == kind && entry.pos == pos) return &entry;
    }
    return nullptr;
}
// fog: a script's remembered target is matched to its snapshot entry by its exact feet, which never change for a tree or wall
