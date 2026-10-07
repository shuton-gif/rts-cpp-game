#include "Collision/collision.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

const float ovalRatio = 2.0f;
const int passes = 4;
const float gridCell = 40.0f;
// movement: colliders are looked up by area instead of all pairs; a grid cell is gridCell map px square (4 x 4 map tiles), about two npc widths

bool separate(Collider& a, Collider& b)
{
    if (a.fixed && b.fixed) return false;

    float dx = b.pos.x - a.pos.x;
    float dy = (b.pos.y - a.pos.y) * ovalRatio;
    float distance = std::hypot(dx, dy);

    const float overlap = a.radius + b.radius - distance;
    if (overlap <= 0.0f) return false;

    if (distance == 0.0f) {
        dx = 1.0f;
        dy = 0.0f;
        distance = 1.0f;
    }
    const float pushX = dx / distance * overlap;
    const float pushY = dy / distance * overlap / ovalRatio;

    const float shareA = a.fixed ? 0.0f : b.fixed ? 1.0f : 0.5f;
    const float shareB = b.fixed ? 0.0f : a.fixed ? 1.0f : 0.5f;

    a.pos.x -= pushX * shareA;
    a.pos.y -= pushY * shareA;
    b.pos.x += pushX * shareB;
    b.pos.y += pushY * shareB;
    return true;
}
// movement: two solid things must not share ground; y is stretched by ovalRatio so both 1 : 2 ovals become circles, the overlap is radius sum minus distance, and they are pushed apart along the line between them, half each, or all on the free one when the other is fixed (two on the same point split sideways)

using Grid = std::unordered_map<std::int64_t, std::vector<std::size_t>>;

static std::int64_t cellKey(const int x, const int y)
{
    return (static_cast<std::int64_t>(x) << 32) ^ static_cast<std::uint32_t>(y);
}

static void cellSpan(const Collider& c, int& x0, int& y0, int& x1, int& y1)
{
    x0 = static_cast<int>(std::floor((c.pos.x - c.radius) / gridCell));
    x1 = static_cast<int>(std::floor((c.pos.x + c.radius) / gridCell));
    y0 = static_cast<int>(std::floor((c.pos.y - c.radius / ovalRatio) / gridCell));
    y1 = static_cast<int>(std::floor((c.pos.y + c.radius / ovalRatio) / gridCell));
}

static void insert(Grid& grid, const std::vector<Collider>& colliders, const std::size_t i)
{
    int x0, y0, x1, y1;
    cellSpan(colliders[i], x0, y0, x1, y1);
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) grid[cellKey(x, y)].push_back(i);
    }
}

void resolveCollisions(std::vector<Collider>& colliders)
{
    Grid fixedGrid;
    for (std::size_t i = 0; i < colliders.size(); ++i) {
        if (colliders[i].fixed) insert(fixedGrid, colliders, i);
    }
    std::vector<std::size_t> seen(colliders.size(), colliders.size());

    for (int pass = 0; pass < passes; ++pass) {
        Grid freeGrid;
        for (std::size_t i = 0; i < colliders.size(); ++i) {
            if (!colliders[i].fixed) insert(freeGrid, colliders, i);
        }
        std::fill(seen.begin(), seen.end(), colliders.size());

        bool moved = false;
        for (std::size_t i = 0; i < colliders.size(); ++i) {
            if (colliders[i].fixed) continue;
            int x0, y0, x1, y1;
            cellSpan(colliders[i], x0, y0, x1, y1);
            for (int y = y0; y <= y1; ++y) {
                for (int x = x0; x <= x1; ++x) {
                    for (const Grid* grid : {&fixedGrid, &freeGrid}) {
                        const auto cell = grid->find(cellKey(x, y));
                        if (cell == grid->end()) continue;
                        for (const std::size_t j : cell->second) {
                            if (j == i || seen[j] == i) continue;
                            if (!colliders[j].fixed && j < i) continue;
                            seen[j] = i;
                            if (separate(colliders[i], colliders[j])) moved = true;
                        }
                    }
                }
            }
        }
        if (!moved) return;
    }
}
// movement: checking every pair grew with the square of built wall posts; fixed colliders go in a grid of gridCell map px once, free ones are re-gridded each pass, and each free collider is checked only against what shares a cell with its oval's bounding box (fixed ones, and free ones with a higher index so a pair is checked once), repeated up to `passes` times or until nothing moves
