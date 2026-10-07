#include "Map/chunk.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <set>
#include <utility>

struct ChunkRect {
    float x;
    float y;
    float w;
    float h;
};
// map: chunk code needs a box test without SDL_FRect; x, y is the top left in map px

struct TreeSpot {
    int local;
    Vector2D<float> feet;
    ChunkRect rect;
};
// map: tree spots are all drawn before any tree is made so walls cannot change the random sequence; a spot keeps its slot number, snapped feet and ground rect

static std::int64_t chunkKey(const int x, const int y)
{
    return (static_cast<std::int64_t>(x) << 32) ^ static_cast<std::uint32_t>(y);
}
// map: chunks are looked up by coords; x and y are packed in one int64 like the collision grid's cellKey

static bool overlaps(const ChunkRect& a, const ChunkRect& b)
{
    return a.x <= b.x + b.w && b.x <= a.x + a.w && a.y <= b.y + b.h && b.y <= a.y + a.h;
}
// map: clumps and trees use the same box test as main.cpp; edges touching count as overlap

static bool contains(const ChunkRect& outer, const ChunkRect& inner)
{
    return inner.x >= outer.x && inner.y >= outer.y && inner.x + inner.w <= outer.x + outer.w && inner.y + inner.h <= outer.y + outer.h;
}
// map: a clump or tree must lie wholly in its chunk and the map; true when inner is inside outer, edges included

static ChunkRect clumpBound(const GrassClump& clump, const ChunkLook& look)
{
    const float w = clump.size.x * look.clumpScale + look.clumpMargin;
    const float h = clump.size.y * look.clumpScale + look.clumpMargin;
    return {clump.pos.x - w / 2.0f, clump.pos.y - h / 2.0f, w, h};
}
// map: same bound as main.cpp's clumpRect; the clump size in sprite px times the scale plus the margin, centered on the clump

static void chunkSpan(const float lo, const float hi, const float mapSize, int& first, int& last)
{
    const int count = static_cast<int>(std::ceil(mapSize / chunkSize));
    first = std::max(0, static_cast<int>(std::floor(lo / chunkSize)));
    last = std::min(count - 1, static_cast<int>(std::floor(hi / chunkSize)));
}
// map: an area touches a run of chunks on each axis; first and last are the chunk coords under lo and hi clamped to the map, and first > last means none

static void addArea(std::set<std::int64_t>& keys, const float x0, const float y0, const float x1, const float y1, const Vector2D<float> mapSize)
{
    int cx0 = 0;
    int cx1 = 0;
    int cy0 = 0;
    int cy1 = 0;
    chunkSpan(x0, x1, mapSize.x, cx0, cx1);
    chunkSpan(y0, y1, mapSize.y, cy0, cy1);
    for (int cy = cy0; cy <= cy1; ++cy) {
        for (int cx = cx0; cx <= cx1; ++cx) keys.insert(chunkKey(cx, cy));
    }
}
// map: the load and keep areas are boxes around the view and the npcs; every map chunk a box overlaps is added to the set

chunkStore::chunkStore(std::uint32_t seed, const ChunkLook& look)
    : seed(seed), look(look)
{
}
// map: the store starts empty; the first update loads the chunks around the view and npcs

bool chunkStore::inMap(int cx, int cy) const
{
    const int countX = static_cast<int>(std::ceil(this->look.mapSize.x / chunkSize));
    const int countY = static_cast<int>(std::ceil(this->look.mapSize.y / chunkSize));
    return cx >= 0 && cy >= 0 && cx < countX && cy < countY;
}
// map: only chunks on the map are loaded; coords run from 0 to ceil(map size / chunkSize) - 1 on each axis

void chunkStore::unload(std::int64_t key, std::vector<tree>& trees, std::vector<int>& remap)
{
    for (std::size_t i = 0; i < trees.size(); ++i) {
        if (this->owner[i] != key) continue;
        if (trees[i].getHp() < trees[i].getMaxHp()) {
            this->damaged[key][this->local[i]] = trees[i].getHp();
        } else {
            const auto chunk = this->damaged.find(key);
            if (chunk != this->damaged.end()) chunk->second.erase(this->local[i]);
        }
        remap[i] = -1;
    }
    this->grass.erase(key);
}
// map: a freed chunk must come back as it was left; trees below max hp have their hp stored by chunk and slot, all its trees are marked -1 in remap for update to erase, and its grass is dropped

void chunkStore::load(int cx, int cy, std::vector<tree>& trees, const std::vector<wall>& walls)
{
    const std::int64_t key = chunkKey(cx, cy);
    std::seed_seq seq{this->seed, static_cast<std::uint32_t>(cx), static_cast<std::uint32_t>(cy)};
    std::mt19937 rng(seq);

    const ChunkRect mapRect = {0.0f, 0.0f, this->look.mapSize.x, this->look.mapSize.y};
    const float left = static_cast<float>(cx) * chunkSize;
    const float top = static_cast<float>(cy) * chunkSize;
    const float right = std::min(left + chunkSize, this->look.mapSize.x);
    const float bottom = std::min(top + chunkSize, this->look.mapSize.y);
    const ChunkRect chunkRect = {left, top, right - left, bottom - top};

    std::vector<GrassClump>& clumps = this->grass[key];
    for (int i = 0; i < clumpsPerChunk; ++i) {
        GrassClump clump = makeGrassClump(rng, {0.0f, 0.0f}, this->look.grassSprites);
        const ChunkRect bound = clumpBound(clump, this->look);
        if (bound.w > chunkRect.w || bound.h > chunkRect.h) continue;
        std::uniform_real_distribution<float> x(left + bound.w / 2.0f, right - bound.w / 2.0f);
        std::uniform_real_distribution<float> y(top + bound.h / 2.0f, bottom - bound.h / 2.0f);

        for (int attempt = 0; attempt < clumpTriesPerChunk; ++attempt) {
            clump.pos = {std::floor(x(rng)), std::floor(y(rng))};
            const ChunkRect rect = clumpBound(clump, this->look);
            if (!contains(chunkRect, rect)) continue;
            const bool free = std::none_of(clumps.begin(), clumps.end(), [&](const GrassClump& other) {
                return overlaps(rect, clumpBound(other, this->look));
            });
            if (free) {
                clumps.push_back(clump);
                break;
            }
        }
    }
    // map: grass is drawn first so trees never shift it; each clump tries clumpTriesPerChunk floored spots until its bound lies in the chunk and touches no earlier clump of this chunk, and is skipped otherwise

    const float tileW = this->look.tile.x;
    const float tileH = this->look.tile.y;
    const int treeTilesX = static_cast<int>(std::lround(this->look.treeSize.x / tileW));
    const Vector2D<float> center = {this->look.mapSize.x / 2.0f, this->look.mapSize.y / 2.0f};
    std::vector<TreeSpot> spots;
    if (this->look.treeSize.x <= chunkRect.w && this->look.treeSize.y <= chunkRect.h) {
        std::uniform_real_distribution<float> x(left + this->look.treeSize.x / 2.0f, right - this->look.treeSize.x / 2.0f);
        std::uniform_real_distribution<float> y(top + this->look.treeSize.y, bottom);
        for (int k = 0; k < treesPerChunk; ++k) {
            for (int attempt = 0; attempt < treeTriesPerChunk; ++attempt) {
                const float spotX = x(rng);
                const float spotY = y(rng);
                const float centerX = treeTilesX % 2 == 0 ? std::round(spotX / tileW) * tileW : std::floor(spotX / tileW) * tileW + tileW / 2.0f;
                const float feetY = std::round(spotY / tileH) * tileH;
                const ChunkRect rect = {centerX - this->look.treeSize.x / 2.0f, feetY - this->look.treeSize.y, this->look.treeSize.x, this->look.treeSize.y};
                if (!contains(mapRect, rect) || !contains(chunkRect, rect)) continue;
                if (std::hypot(centerX - center.x, feetY - center.y) < spawnClear) continue;
                const bool free = std::none_of(spots.begin(), spots.end(), [&](const TreeSpot& other) {
                    return overlaps(rect, other.rect);
                });
                if (free) {
                    spots.push_back({k, {centerX, feetY}, rect});
                    break;
                }
            }
        }
    }
    // map: trees are snapped like main.cpp's fixed trees and kept wholly in the chunk so chunks stay independent; candidate k tries treeTriesPerChunk spots off the spawn clearing and off earlier spots, and keeps slot k only if one fits

    const std::size_t fixedEnd = trees.size();
    for (const TreeSpot& spot : spots) {
        const bool onWall = std::any_of(walls.begin(), walls.end(), [&](const wall& post) {
            const Vector2D<float> pos = post.getPos();
            return !post.isDestroyed() && pos.x >= spot.rect.x && pos.x <= spot.rect.x + spot.rect.w && pos.y >= spot.rect.y && pos.y <= spot.rect.y + spot.rect.h;
        });
        if (onWall) continue;
        bool onFixed = false;
        for (std::size_t i = 0; i < fixedEnd && !onFixed; ++i) {
            if (this->owner[i] != -1) continue;
            const Vector2D<float> pos = trees[i].getPos();
            const Vector2D<float> size = trees[i].getSize();
            onFixed = overlaps(spot.rect, {pos.x - size.x / 2.0f, pos.y - size.y, size.x, size.y});
        }
        if (onFixed) continue;

        trees.emplace_back(spot.feet, this->look.treeSize, "tree.png");
        this->owner.push_back(key);
        this->local.push_back(spot.local);
        const auto chunk = this->damaged.find(key);
        if (chunk == this->damaged.end()) continue;
        const auto hurt = chunk->second.find(spot.local);
        if (hurt != chunk->second.end()) trees.back().chop(trees.back().getMaxHp() - hurt->second);
    }
    // map: walls and fixed trees change during play, so they only filter after every random draw; a spot over a standing wall post or a fixed tree is skipped, the rest are appended with their chunk and slot, and a stored hp is put back by chopping the difference
}
// map: a chunk must regenerate the same every time; its generator is seeded from the world seed and the chunk coords, grass is drawn first, then tree spots, and only then are trees made

std::vector<int> chunkStore::update(Vector2D<float> viewMin, Vector2D<float> viewMax, const std::vector<Vector2D<float>>& npcFeet, float npcReach, std::vector<tree>& trees, const std::vector<wall>& walls)
{
    this->owner.resize(trees.size(), -1);
    this->local.resize(trees.size(), -1);
    // map: trees main.cpp added itself (the fixed center trees) sit past the store's list; they get owner -1 and are never erased

    std::set<std::int64_t> wanted;
    std::set<std::int64_t> keep;
    addArea(wanted, viewMin.x - chunkSize, viewMin.y - chunkSize, viewMax.x + chunkSize, viewMax.y + chunkSize, this->look.mapSize);
    addArea(keep, viewMin.x - 2.0f * chunkSize, viewMin.y - 2.0f * chunkSize, viewMax.x + 2.0f * chunkSize, viewMax.y + 2.0f * chunkSize, this->look.mapSize);
    for (const Vector2D<float>& feet : npcFeet) {
        addArea(wanted, feet.x - npcReach, feet.y - npcReach, feet.x + npcReach, feet.y + npcReach, this->look.mapSize);
        const float reach = npcReach + chunkSize;
        addArea(keep, feet.x - reach, feet.y - reach, feet.x + reach, feet.y + reach, this->look.mapSize);
    }
    // map: chunks load one chunk out from the view and within npcReach of an npc, but are freed only a chunk further out, so walking along a border does not load and free the same chunk every frame

    std::vector<int> remap(trees.size());
    for (std::size_t i = 0; i < remap.size(); ++i) remap[i] = static_cast<int>(i);
    std::vector<std::int64_t> freed;
    for (const auto& [key, clumps] : this->grass) {
        if (!keep.count(key)) freed.push_back(key);
    }
    for (const std::int64_t key : freed) this->unload(key, trees, remap);
    // map: chunks out of the keep area are freed; unload marks their trees -1 in remap

    bool erased = false;
    std::size_t next = 0;
    for (std::size_t i = 0; i < remap.size(); ++i) {
        if (remap[i] < 0) {
            erased = true;
            continue;
        }
        if (next != i) {
            trees[next] = std::move(trees[i]);
            this->owner[next] = this->owner[i];
            this->local[next] = this->local[i];
        }
        remap[i] = static_cast<int>(next);
        ++next;
    }
    trees.erase(trees.begin() + static_cast<std::ptrdiff_t>(next), trees.end());
    this->owner.resize(next);
    this->local.resize(next);
    if (!erased) remap.clear();
    // map: main.cpp holds tree indices; the kept trees slide down in order with owner and local in step, and remap gives each old index its new one (-1 erased), or is empty when nothing was erased

    for (const std::int64_t key : wanted) {
        if (this->grass.count(key)) continue;
        const int cx = static_cast<int>(key >> 32);
        const int cy = static_cast<int>(static_cast<std::uint32_t>(key & 0xffffffff));
        if (this->inMap(cx, cy)) this->load(cx, cy, trees, walls);
    }
    // map: missing chunks of the load area are made after freeing; their trees are appended at the end so main.cpp sees them by the list growing

    return remap;
}
// map: one call per frame keeps the loaded chunks around the view and the npcs and keeps the trees list in step with them

std::vector<const GrassClump*> chunkStore::clumps() const
{
    std::vector<const GrassClump*> all;
    for (const auto& [key, clumps] : this->grass) {
        for (const GrassClump& clump : clumps) all.push_back(&clump);
    }
    return all;
}
// map: grass of every loaded chunk is drawn together; pointers into each chunk's clumps, valid until the next update frees or loads a chunk
