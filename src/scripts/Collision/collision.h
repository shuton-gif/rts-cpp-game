#ifndef COLLISION_H
#define COLLISION_H

#include <vector>

#include "Vector2D.h"

struct Collider {
    Vector2D<float> pos;
    float radius;
    bool fixed;
};
// movement: npcs and later rocks, buildings and other objects all need to be solid in the same way; a collider is an oval on the ground, pos its center in map px, radius its half width (half height is radius / 2), and fixed marks things that are never moved by a collision

bool separate(Collider& a, Collider& b);
void resolveCollisions(std::vector<Collider>& colliders);
// movement: collision knows nothing about npcs or SDL; the caller hands in one collider per solid thing, overlapping ones are pushed apart in place, and the caller copies the moved positions back

#endif
