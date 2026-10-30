#include "Wall/wall.h"

#include <algorithm>
#include <utility>

#include "Tree/tree.h"

wall::wall(Vector2D<float> pos, Vector2D<float> size, std::string image, std::shared_ptr<WallGroup> group)
    : GameObject(pos, size, size.x, true, std::move(image)), group(std::move(group))
{
}
// economy: a post is one tile wide and posts in a line must leave no gap; its fixed oval has radius size.x (twice the post), so ovals of posts a tile apart sideways, up or down overlap or touch, and main.cpp only adds it to the collision list once the post is built

void wall::join()
{
    if (this->group->posts == 0) this->group->first = this->getPos();
    ++this->group->posts;
    this->group->feetSum.x += this->getPos().x;
    this->group->feetSum.y += this->getPos().y;
}

bool wall::isGroupFirst() const { return this->getPos() == this->group->first; }

Vector2D<float> wall::getGroupMiddle() const
{
    const float posts = static_cast<float>(std::max(this->group->posts, 1));
    return {this->group->feetSum.x / posts, this->group->feetSum.y / posts};
}
// economy-wall-group: a group gets one bar at its middle; join records each placed post's feet and the first post, which main.cpp uses to draw that bar once

bool wall::deliver()
{
    if (this->hasAllPlanks()) return false;
    ++this->group->delivered;
    return true;
}

bool wall::hasAllPlanks() const { return this->group->delivered >= wallCost; }
int wall::getDelivered() const { return this->group->delivered; }
// economy-wall-group: a plan takes planks for its whole group until it has wallCost; a plank brought after that is refused so the caller keeps it

bool wall::build(float deltaTime)
{
    if (this->group->built || !this->hasAllPlanks()) return false;
    this->group->progress = std::min(this->group->progress + deltaTime, wallBuildTime);
    if (this->group->progress < wallBuildTime) return false;
    this->group->built = true;
    return true;
}

float wall::getProgress() const { return this->group->progress / wallBuildTime; }
// economy-wall-group: building only starts with every plank in; work on any post adds up to wallBuildTime ms for the group, from any number of builders in the same frame, and all its posts stand on the frame it is reached

bool wall::damage(int amount)
{
    if (!this->group->built || this->isDestroyed()) return false;
    this->group->hp = std::max(this->group->hp - amount, 0);
    return this->isDestroyed();
}

Collider wall::getHitbox() const
{
    Collider hitbox = this->getCollider();
    hitbox.radius *= chopScale;
    return hitbox;
}
// economy-wall-group: only a standing wall can be hit; a hit on any post takes the group's hp, never below 0, a plan or a destroyed wall ignores hits, and the hitbox uses the tree's chopScale so an axe reaches it the same way

int wall::getHp() const { return this->group->hp; }
int wall::getMaxHp() const { return wallHp; }
bool wall::isBuilt() const { return this->group->built && this->group->hp > 0; }
bool wall::isDestroyed() const { return this->group->built && this->group->hp <= 0; }
// economy-wall-group: a whole group must disappear when broken; main.cpp draws, collides and removes walls by state, which every post reads from its group, so isBuilt is a standing group and isDestroyed one that was built and lost all hp
