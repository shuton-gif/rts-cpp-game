#ifndef GRASS_H
#define GRASS_H

#include <random>
#include <vector>

#include "Vector2D.h"

struct GrassStrip {
    int sprite;
    Vector2D<float> offset;
};

struct GrassClump {
    Vector2D<float> pos;
    Vector2D<float> size;
    std::vector<GrassStrip> strips;
};
// map: clumps are plotted at random and must not land on each other; size is the clump's bound in sprite px so the caller can test clumps against each other and against the view

GrassClump makeGrassClump(std::mt19937& rng, Vector2D<float> pos, int spriteCount);
// map: a clump is built from single grass strips; pos is the clump center in map px, each strip holds which strip sprite it uses and its offset from the center in sprite px, and the caller passes the random generator so a fixed seed can be used later

#endif
