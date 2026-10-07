#include "Movement/arrive.h"

#include <algorithm>
#include <cmath>

#include "Collision/collision.h"

Vector2D<float> fuzzyArrive(std::mt19937& rng, Vector2D<float> target, Vector2D<float> mapSize, float rad)
{
    std::uniform_real_distribution<float> turn(0.0f, 2.0f * 3.14159265f);
    std::uniform_real_distribution<float> reach(0.0f, 1.0f);

    const float angle = turn(rng);
    const float radius = std::sqrt(reach(rng));
    return {
        std::clamp(std::floor(target.x + std::cos(angle) * radius * rad), 0.0f, mapSize.x),
        std::clamp(std::floor(target.y + std::sin(angle) * radius * rad / 2.0f), 0.0f, mapSize.y),
    };
}
// movement: the spread around a target depends on what was clicked; rad is the oval's horizontal radius in map px (vertical rad / 2, the 1 : 2 oval), and a random point inside it is returned, evenly spread over the oval and kept inside the map

float aroundRadius(const GameObject& object)
{
    const Collider body = object.getCollider();
    return std::max(body.radius * 2.0f, body.radius) + colliderRadius;
}
// movement: the spread radius around an object's solid oval is the oval's width plus one npc radius, so there is always a ring outside the oval where an npc fits

Vector2D<float> arriveAround(std::mt19937& rng, const GameObject& object, Vector2D<float> mapSize)
{
    const Collider body = object.getCollider();
    const Vector2D<float> pos = body.pos;
    const float rad = aroundRadius(object);

    Vector2D<float> spot = fuzzyArrive(rng, pos, mapSize, rad);
    for (int attempt = 0; attempt < arriveTries; ++attempt) {
        Collider solid = object.getCollider();
        Collider unit = {spot, colliderRadius, false};
        if (!separate(solid, unit)) break;
        spot = fuzzyArrive(rng, pos, mapSize, rad);
    }
    return spot;
}
// movement: npcs sent to an object spread around its solid oval; a spot where an npc would overlap the object's solid oval could never be reached, so it is drawn again (up to arriveTries times) until the npc fits beside the object

bool hasMark(const npc& unit, Vector2D<float> mark)
{
    const std::vector<Vector2D<float>>& queued = unit.getQueuedMark();
    return unit.getTargetedMark() == mark || std::find(queued.begin(), queued.end(), mark) != queued.end();
}
// movement: hasMark checks if a unit has a mark as its current target or in its queued marks
