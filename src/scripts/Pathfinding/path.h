#ifndef PATH_H
#define PATH_H

#include <cstdint>
#include <vector>

#include "Collision/collision.h"
#include "Vector2D.h"

const float navCell = 10.0f;
const int goalSearchCells = 30;
// pathfinding: npcs walk around trees and walls on a grid instead of pushing into them; navCell is the cell size in map px (a 4000 map is 400 x 400 cells), and goalSearchCells bounds the search for a free cell around a blocked goal

class navGrid
{
private:
    int width = 0;
    int height = 0;
    std::vector<unsigned char> blocked;
    mutable std::vector<float> cost;
    mutable std::vector<int> parent;
    mutable std::vector<std::uint32_t> stamp;
    mutable std::vector<std::uint32_t> closed;
    mutable std::uint32_t generation = 0;
    mutable std::vector<int> component;
    mutable bool labeled = false;
    // pathfinding: a search must not reallocate or clear 160k cells each call; cost and parent are valid only where stamp equals the current generation, closed marks expanded cells the same way, a new call just bumps generation, and component holds connected areas of free cells, labeled lazily after each build

    int cellOf(Vector2D<float> point) const;
    bool isFree(int cell, int start) const;
    bool lineOfSight(Vector2D<float> a, Vector2D<float> b, int skipCell) const;
    int nearestFree(int goal, int start) const;
    void label() const;
    // pathfinding: shared helpers of findPath; cellOf clamps a map point into the grid, isFree treats the start cell as free, lineOfSight samples a segment, nearestFree finds the free cell closest to a blocked goal, and label flood fills the free cells into connected areas

public:
    void build(const std::vector<Collider>& solids, Vector2D<float> mapSize);
    // pathfinding: the grid must match the standing trees and built walls; build clears it and blocks every cell where an npc standing at the cell center would overlap a solid's oval

    std::vector<Vector2D<float>> findPath(Vector2D<float> from, Vector2D<float> to) const;
    // pathfinding: npcs got stuck when a solid stood between them and their destination; findPath returns waypoints after from that always end with exactly to, {to} when the straight line is clear, else an A* path pulled tight into straight segments
};
// pathfinding: one grid of blocked cells for the whole map, built from the solid colliders and searched by every npc that needs a way around them

#endif
