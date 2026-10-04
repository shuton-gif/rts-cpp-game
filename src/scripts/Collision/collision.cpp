#include "Collision/collision.h"

#include <cmath>
#include <cstddef>

const float ovalRatio = 2.0f;
const int passes = 4;

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

void resolveCollisions(std::vector<Collider>& colliders)
{
    for (int pass = 0; pass < passes; ++pass) {
        bool moved = false;
        for (std::size_t i = 0; i < colliders.size(); ++i) {
            for (std::size_t j = i + 1; j < colliders.size(); ++j) {
                if (separate(colliders[i], colliders[j])) moved = true;
            }
        }
        if (!moved) return;
    }
}
// movement: pushing one pair apart can push one of them into a third; every pair is checked, and the whole check repeats up to `passes` times or until nothing moves
