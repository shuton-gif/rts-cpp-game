#ifndef CHUNK_H
#define CHUNK_H

#include <cstdint>
#include <map>
#include <vector>

#include "Grass/grass.h"
#include "Tree/tree.h"
#include "Wall/wall.h"
#include "Vector2D.h"

const float chunkSize = 2000.0f;
const int clumpsPerChunk = 75;
const int clumpTriesPerChunk = 30;
const int treesPerChunk = 8;
const int treeTriesPerChunk = 30;
const float spawnClear = 1200.0f;
// map: a 40000 map cannot hold all its grass and trees at once; it is cut into chunkSize squares that each get 75 clumps (the 300 per 4000 map set in main before chunks) and 8 trees, each tried up to 30 random spots, and no tree is made within spawnClear of the center where the npcs start

struct ChunkLook {
    Vector2D<float> mapSize;
    Vector2D<float> treeSize;
    Vector2D<float> tile;
    float clumpScale;
    float clumpMargin;
    int grassSprites;
};
// map: chunks are made without SDL or main.cpp globals; the look carries the map size, tree size and tile size in map px, the sprite px to map px scale and margin of a clump bound, and how many grass sprites there are

class chunkStore
{

private:
    std::uint32_t seed = 0;
    ChunkLook look;
    std::map<std::int64_t, std::vector<GrassClump>> grass;
    std::vector<std::int64_t> owner;
    std::vector<int> local;
    std::map<std::int64_t, std::map<int, int>> damaged;
    // map: loaded chunks are the keys of grass, each holding its clumps; owner and local run parallel to the trees list and name the chunk and slot of each tree (-1 for trees the store did not add), and damaged keeps the hp of hurt or felled trees by chunk and slot after their chunk is freed

    bool inMap(int cx, int cy) const;
    void unload(std::int64_t key, std::vector<tree>& trees, std::vector<int>& remap);
    void load(int cx, int cy, std::vector<tree>& trees, const std::vector<wall>& walls);
    // map: shared steps of update; inMap tells whether chunk coords lie on the map, unload stores hurt trees and erases the chunk's trees and grass, and load regenerates a chunk from the seed and adds its trees

public:
    chunkStore(std::uint32_t seed, const ChunkLook& look);

    std::vector<int> update(Vector2D<float> viewMin, Vector2D<float> viewMax, const std::vector<Vector2D<float>>& npcFeet, float npcReach, std::vector<tree>& trees, const std::vector<wall>& walls);
    // map: only chunks near the view or an npc should exist; update frees chunks out of the wider keep area, then loads missing chunks of the nearer load area, appends their trees and returns old index to new index (-1 erased) when trees were erased, else empty

    std::vector<const GrassClump*> clumps() const;
    // map: main.cpp draws grass of every loaded chunk; clumps lists pointers to them, valid until the next update
};
// map: grass and trees exist only in chunks near the view or an npc; a chunk is rebuilt the same from the world seed and its coords, and hurt or felled trees are remembered while it is freed

#endif
