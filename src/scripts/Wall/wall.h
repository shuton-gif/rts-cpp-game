#ifndef WALL_H
#define WALL_H

#include <string>

#include "GameObject/gameObject.h"
#include "Vector2D.h"

const int wallCost = 1;
const float wallBuildTime = 2000.0f;
const int wallHp = 100;
// economy: a wall is a single post now, so it needs wallCost (1) plank brought to its plan, then wallBuildTime ms of building, and stands with wallHp hp

class wall : public GameObject
{

private:
    int delivered = 0;
    float progress = 0.0f;
    bool built = false;
    int hp = wallHp;
    int maxHp = wallHp;

public:
    wall(Vector2D<float> pos, Vector2D<float> size, std::string image);

    bool deliver();
    bool hasAllPlanks() const;
    int getDelivered() const;
    bool build(float deltaTime);
    float getProgress() const;
    // economy: a plan fills up one plank per deliver (false once it has wallCost), then build adds deltaTime ms of work and is true on the frame it finishes; getProgress is 0 to 1

    bool damage(int amount);
    Collider getHitbox() const;
    // combat: built walls are hit by arrows and axes; damage takes amount off hp, never below 0, and is true on the hit that destroys it, and getHitbox is the solid oval x chopScale like a tree's

    int getHp() const;
    int getMaxHp() const;
    bool isBuilt() const;
    bool isDestroyed() const;
};
// economy: players build walls of single posts; a wall starts as a plan that is not solid, is built from planks by npcs, and is then a fixed solid GameObject with hp that is gone at 0

#endif
