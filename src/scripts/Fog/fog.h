#ifndef FOG_H
#define FOG_H

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "Tree/tree.h"
#include "Vector2D.h"
#include "Wall/wall.h"

const float viewField = 1000.0f;
const int fogCell = 10;
const int fogBlockCells = 200;
// fog: npcs see a plain circle and the veil has blocky edges; viewField is the circle's radius in map px, a veil cell is one tile (fogCell map px), and cells are stored in blocks of fogBlockCells a side, one chunk's worth

enum class FogState : std::uint8_t { Unknown, Discovered, Sight };
// fog: a spot is never discovered (solid veil), discovered without current sight (0.5 veil over the snapshot), or in sight (no veil)

struct Seen {
    enum class Kind { Tree, Wall };
    Kind kind = Kind::Tree;
    Vector2D<float> pos = {0.0f, 0.0f};
    int index = -1;
    bool built = false;
    float progress = 0.0f;
};
// fog: a discovered spot shows trees and walls as last seen, which may be out of date; a Seen entry is one of them, pos is its feet, index its place in main's list (a tree's only holds while in sight, since trees stream in and out; a wall's always holds), and built and progress are a wall's state when last seen

class fogMap
{

private:
    using Block = std::array<std::uint8_t, fogBlockCells * fogBlockCells>;

    int blocksX = 0;
    int blocksY = 0;
    int cellsX = 0;
    int cellsY = 0;
    std::vector<std::unique_ptr<Block>> blocks;
    std::vector<int> lit;
    std::vector<Seen> seen;
    // fog: a whole 40000 map of cells per team would take 16 MB each even when unexplored; a block is only made once the team has seen into it, and lit lists the blocks holding sight cells so only those are cleared each frame

    void stampRow(int cy, int fromX, int toX);
    void stampCircle(Vector2D<float> feet);
    // fog: sight is redrawn every frame; stampCircle marks the cells whose center is in the npc's view field, one row at a time through stampRow, which crosses blocks and makes missing ones

public:
    explicit fogMap(Vector2D<float> mapSize);

    void update(const std::vector<Vector2D<float>>& teamFeet, const std::vector<tree>& trees, const std::vector<wall>& walls);
    // fog: sight must follow the npcs and snapshots must follow sight; update turns last frame's sight into discovered, stamps a circle per npc, then forgets every Seen entry now in sight and records the standing trees and walls in sight again, so anything gone from a seen spot is gone from the snapshot

    FogState cellState(int cx, int cy) const;
    FogState state(Vector2D<float> point) const;
    const std::vector<Seen>& getSeen() const;
    const Seen* seenAt(Seen::Kind kind, Vector2D<float> pos) const;
    // fog: main draws from these and scripts search them; cellState reads one cell (Unknown off the map), state the cell under a map point, getSeen lists the snapshot, and seenAt finds the entry at exact feet, nullptr if none
};
// fog: one fogMap per team holds what that team sees and remembers; it has no SDL, so main draws it and the agents read it

#endif
