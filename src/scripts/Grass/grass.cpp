#include "Grass/grass.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

const int minW = 40;
const int maxW = 150;
const int stripWidth = 2;
const int stripHeight = 5;
const float fillMin = 0.08f;
const float fillMax = 0.2f;
// map: a 60% fill packed the whole oval and drew its outline; strips now cover a random 8% to 20% of the bound, each strip still reserving 2 x 5 (1 px blade + 1 px gap) so none overlap
const int triesPerStrip = 50;
const int restartsPerCount = 20;

static Vector2D<int> randomSpot(std::mt19937& rng, const int width, const int height)
{
    std::uniform_real_distribution<float> turn(0.0f, 2.0f * 3.14159265f);
    std::uniform_real_distribution<float> reach(0.0f, 1.0f);

    const float angle = turn(rng);
    const float radius = reach(rng);
    return {
        static_cast<int>(std::lround(std::cos(angle) * radius * static_cast<float>(width) / 2.0f)),
        static_cast<int>(std::lround(std::sin(angle) * radius * static_cast<float>(height) / 2.0f)),
    };
}
// map: the oval is only a bound and must not show as a shape; a spot is a random direction and a random distance from the center, which lands more strips near the middle and fewer toward the edge, so the clump fades out instead of ending on a line

static bool placeStrips(std::mt19937& rng, const int strips, const int width, const int height, std::vector<Vector2D<int>>& placed)
{
    placed.clear();
    for (int i = 0; i < strips; ++i) {
        bool found = false;
        for (int attempt = 0; attempt < triesPerStrip && !found; ++attempt) {
            const Vector2D<int> spot = randomSpot(rng, width, height);
            found = std::none_of(placed.begin(), placed.end(), [&](const Vector2D<int>& other) {
                return std::abs(spot.x - other.x) < stripWidth && std::abs(spot.y - other.y) < stripHeight;
            });
            if (found) placed.push_back(spot);
        }
        if (!found) return false;
    }
    return true;
}
// map: strips must not overlap inside the bound; each strip tries random spots and keeps the first whose stripWidth x stripHeight box touches no placed box, and the whole clump reports failure if a strip finds no room

GrassClump makeGrassClump(std::mt19937& rng, Vector2D<float> pos, int spriteCount)
{
    std::uniform_int_distribution<int> boundWidth(minW, maxW);
    std::uniform_real_distribution<float> fill(fillMin, fillMax);
    std::uniform_int_distribution<int> sprite(0, spriteCount - 1);

    GrassClump clump;
    clump.pos = pos;

    const int width = boundWidth(rng);
    const int height = std::max(stripHeight, width / 2);
    clump.size = {static_cast<float>(width), static_cast<float>(height)};
    const float ovalArea = 3.14159265f / 4.0f * static_cast<float>(width * height);
    int strips = std::max(1, static_cast<int>(std::lround(ovalArea * fill(rng) / static_cast<float>(stripWidth * stripHeight))));
    // map: clump sizes and densities should vary; the bound is an oval of random width in minW..maxW with height : width = 1 : 2, and the strip count is the oval area times a random fill

    std::vector<Vector2D<int>> placed;
    for (int restart = 0; !placeStrips(rng, strips, width, height, placed); ++restart) {
        if (restart % restartsPerCount == restartsPerCount - 1 && strips > 1) --strips;
    }
    // map: the dense middle can jam before every strip fits; placement restarts from empty, and after every 20 failed restarts one strip is dropped so the bound keeps its size and it always finishes

    for (const Vector2D<int>& spot : placed) {
        clump.strips.push_back({sprite(rng), {static_cast<float>(spot.x), static_cast<float>(spot.y)}});
    }

    std::sort(clump.strips.begin(), clump.strips.end(), [](const GrassStrip& a, const GrassStrip& b) {
        return a.offset.y < b.offset.y;
    });
    return clump;
}
// map: every clump should look different; a random number of strips each get a random strip sprite and a non-overlapping whole sprite-px spot chosen at random inside the 1 : 2 oval bound, stored as an offset from its center and sorted by y so lower ones draw over higher ones
