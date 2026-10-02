#include "Grass/grass.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

const int stripsMin = 13;
const int stripsMax = 17;
const int stripWidth = 2;
const int stripHeight = 5;
const float fill = 0.6f;
// map: strips should sit closer and denser; only the straight strip is used (1 px blade), so each strip reserves 2 x 5 (blade + 1 px gap) instead of the full 3 x 5 sprite, and the boxes fill about 60% of the spawn rect
const int triesPerStrip = 50;
const int restartsPerSize = 20;

static bool placeStrips(std::mt19937& rng, const int strips, const int width, const int height, std::vector<Vector2D<int>>& placed)
{
    std::uniform_int_distribution<int> left(0, width - stripWidth);
    std::uniform_int_distribution<int> top(0, height - stripHeight);

    placed.clear();
    for (int i = 0; i < strips; ++i) {
        bool found = false;
        for (int attempt = 0; attempt < triesPerStrip && !found; ++attempt) {
            const Vector2D<int> spot = {left(rng), top(rng)};
            found = std::none_of(placed.begin(), placed.end(), [&](const Vector2D<int>& other) {
                return std::abs(spot.x - other.x) < stripWidth && std::abs(spot.y - other.y) < stripHeight;
            });
            if (found) placed.push_back(spot);
        }
        if (!found) return false;
    }
    return true;
}
// map: strips must not overlap inside the spawn rect; each strip tries random spots and keeps the first whose stripWidth x stripHeight box touches no placed box, and the whole clump reports failure if a strip finds no room

GrassClump makeGrassClump(std::mt19937& rng, Vector2D<float> pos, int spriteCount)
{
    std::uniform_int_distribution<int> count(stripsMin, stripsMax);
    std::uniform_int_distribution<int> sprite(0, spriteCount - 1);

    GrassClump clump;
    clump.pos = pos;

    const int strips = count(rng);
    const float stripArea = static_cast<float>(strips * stripWidth * stripHeight);
    int height = std::max(stripHeight, static_cast<int>(std::ceil(std::sqrt(stripArea / (2.0f * fill)))));
    // map: clumps were too sparse and spread in a line; the spawn area is now a rect with height : width = 1 : 2, sized so the strip boxes fill about `fill` of it

    std::vector<Vector2D<int>> placed;
    for (int restart = 0; !placeStrips(rng, strips, height * 2, height, placed); ++restart) {
        if (restart % restartsPerSize == restartsPerSize - 1) ++height;
    }
    const int width = height * 2;
    // map: a dense rect can jam before every strip fits; placement restarts from empty, and after every 20 failed restarts the rect grows by one px of height so it always finishes

    for (const Vector2D<int>& spot : placed) {
        const float x = static_cast<float>(spot.x - (width - stripWidth) / 2);
        const float y = static_cast<float>(spot.y - (height - stripHeight) / 2);
        clump.strips.push_back({sprite(rng), {x, y}});
    }

    std::sort(clump.strips.begin(), clump.strips.end(), [](const GrassStrip& a, const GrassStrip& b) {
        return a.offset.y < b.offset.y;
    });
    return clump;
}
// map: every clump should look different; a random stripsMin..stripsMax strips (10 more than the first 3..7) each get a random strip sprite and a non-overlapping whole sprite-px spot in the 1 : 2 rect, stored as an offset from the rect center and sorted by y so lower ones draw over higher ones
