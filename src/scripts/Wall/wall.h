#ifndef WALL_H
#define WALL_H

#include <memory>
#include <string>
#include <vector>

#include "GameObject/gameObject.h"
#include "Vector2D.h"

const int wallGroupSize = 5;
const int wallCost = wallGroupSize;
const float wallBuildTime = 2000.0f * wallGroupSize;
const int wallHp = 100 * 2;
// economy-wall-group: walls are paid and broken in groups of wallGroupSize posts, not one post at a time; a group takes one plank per post of a full group and builds like wallGroupSize posts even with fewer planned, and its one hp pool is worth 2 posts

struct WallGroup {
    int delivered = 0;
    float progress = 0.0f;
    bool built = false;
    int hp = wallHp;
    int posts = 0;
    Vector2D<float> first = {0.0f, 0.0f};
    Vector2D<float> feetSum = {0.0f, 0.0f};
    std::vector<int> haulers;
};
// economy-wall-group: the posts of a group must fill, build and die together; each post shares one WallGroup holding planks, build work and hp, its posts' feet so the bar can sit at their middle, and the npc ids hauling a plank to it

class wall : public GameObject
{

private:
    std::shared_ptr<WallGroup> group;

public:
    wall(Vector2D<float> pos, Vector2D<float> size, std::string image, std::shared_ptr<WallGroup> group);

    void join();
    bool isGroupFirst() const;
    Vector2D<float> getGroupMiddle() const;
    // economy-wall-group: a candidate post only counts once placed; join adds it to its group, the group's first post draws the one bar, and getGroupMiddle is the average of the posts' feet

    bool deliver();
    bool hasAllPlanks() const;
    int getDelivered() const;
    bool build(float deltaTime);
    float getProgress() const;
    bool claim(int npcId);
    void release(int npcId);
    // economy-wall-group: every npc on a group hauled a plank however many were left, so extras arrived late; claim is true while delivered planks plus other npcs' claims leave a plank for npcId, and release gives the claim back
    // economy-wall-group: a plan's group fills up one plank per deliver (false once it has wallCost), then build adds deltaTime ms of work and is true on the frame it finishes; getProgress is 0 to 1

    bool damage(int amount);
    Collider getHitbox() const;
    // economy-wall-group: built walls are hit by arrows and axes; damage takes amount off the group's hp, never below 0, and is true on the hit that destroys the whole group, and getHitbox is the solid oval x chopScale like a tree's

    int getHp() const;
    int getMaxHp() const;
    bool isBuilt() const;
    bool isDestroyed() const;
};
// economy: players build walls of single posts; a wall starts as a plan that is not solid, is built from planks by npcs, and is then a fixed solid GameObject with hp that is gone at 0

#endif
