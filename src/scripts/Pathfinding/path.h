#ifndef PATH_H
#define PATH_H

#include <cstdint>
#include <vector>

#include "Collision/collision.h"
#include "Vector2D.h"

const float navCell = 10.0f;
const int goalSearchCells = 30;
const float navMargin = 600.0f;
const int navMaxCells = 1000;
// pathfinding: a grid over the whole 40000 map would be 4000 x 4000 cells (~350 MB of buffers); navCell is the cell size in map px, each search grids only a window around from and to grown by navMargin map px per side and capped at navMaxCells cells per side, and goalSearchCells bounds the search for a free cell around a blocked goal

struct navWindow {
    Vector2D<float> origin;
    Vector2D<float> size;
};

navWindow navWindowFor(Vector2D<float> from, Vector2D<float> to, Vector2D<float> mapSize);
// pathfinding: the caller needs the rect to build the grid over before each search; navWindowFor returns from and to's bounding box grown by navMargin, clamped to the map, snapped outward to whole navCell cells so cells line up in every window, and a side over navMaxCells cells is cut to navMaxCells keeping from with navMargin room and reaching toward to as far as possible

class navGrid
{
private:
    int width = 0;
    int height = 0;
    Vector2D<float> origin = {0.0f, 0.0f};
    std::vector<unsigned char> blocked;
    mutable std::vector<float> cost;
    mutable std::vector<int> parent;
    mutable std::vector<std::uint32_t> stamp;
    mutable std::vector<std::uint32_t> closed;
    mutable std::uint32_t generation = 0;
    mutable std::vector<int> component;
    mutable bool labeled = false;
    // pathfinding: a search must not reallocate or clear its buffers each call; origin is the window's top left in map px, the buffers only grow to the largest window seen, cost and parent are valid only where stamp equals the current generation, closed marks expanded cells the same way, a new call just bumps generation, and component holds connected areas of free cells, labeled lazily after each build

    int cellOf(Vector2D<float> point) const;
    bool isFree(int cell, int start) const;
    bool lineOfSight(Vector2D<float> a, Vector2D<float> b, int skipCell) const;
    int nearestFree(int goal, int start) const;
    void label() const;
    // pathfinding: shared helpers of findPath; cellOf clamps a map point into the grid, isFree treats the start cell as free, lineOfSight samples a segment, nearestFree finds the free cell closest to a blocked goal, and label flood fills the free cells into connected areas

public:
    void build(const std::vector<Collider>& solids, Vector2D<float> origin, Vector2D<float> size);
    // pathfinding: the grid must match the standing trees and built walls but only around the trip; build covers the window at origin of size map px, clears it and blocks every cell where an npc standing at the cell center would overlap a solid's oval

    std::vector<Vector2D<float>> findPath(Vector2D<float> from, Vector2D<float> to) const;
    // pathfinding: npcs got stuck when a solid stood between them and their destination; findPath returns waypoints after from that always end with exactly to, {to} when the straight line is clear, else an A* path pulled tight into straight segments
};
// pathfinding: a full map grid no longer fits in memory; one grid of blocked cells over a window of the map, rebuilt from the solid colliders around each trip and searched by the npc that needs a way around them

#endif
