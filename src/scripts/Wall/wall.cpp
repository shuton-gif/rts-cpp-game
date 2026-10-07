#include "Wall/wall.h"

#include <algorithm>
#include <utility>

#include "Tree/tree.h"

wall::wall(Vector2D<float> pos, Vector2D<float> size, std::string image)
    : GameObject(pos, size, size.x, true, std::move(image))
{
}
// economy: a post is one tile wide and posts in a line must leave no gap; its fixed oval has radius size.x (twice the post), so ovals of posts a tile apart sideways, up or down overlap or touch, and main.cpp only adds it to the collision list once the post is built

bool wall::deliver()
{
    if (this->hasAllPlanks()) return false;
    ++this->delivered;
    return true;
}

bool wall::hasAllPlanks() const { return this->delivered >= wallCost; }
int wall::getDelivered() const { return this->delivered; }
// economy: a plan takes planks until it has wallCost; a plank brought after that is refused so the caller keeps it

bool wall::build(float deltaTime)
{
    if (this->built || !this->hasAllPlanks()) return false;
    this->progress = std::min(this->progress + deltaTime, wallBuildTime);
    if (this->progress < wallBuildTime) return false;
    this->built = true;
    return true;
}

float wall::getProgress() const { return this->progress / wallBuildTime; }
// economy: building only starts with every plank in; work adds up to wallBuildTime ms, from any number of builders in the same frame, and the wall is built on the frame it is reached

bool wall::damage(int amount)
{
    if (!this->built || this->isDestroyed()) return false;
    this->hp = std::max(this->hp - amount, 0);
    return this->isDestroyed();
}

Collider wall::getHitbox() const
{
    Collider hitbox = this->getCollider();
    hitbox.radius *= chopScale;
    return hitbox;
}
// combat: only a standing wall can be hit; hp never drops below 0, a plan or a destroyed wall ignores hits, and the hitbox uses the tree's chopScale so an axe reaches it the same way

int wall::getHp() const { return this->hp; }
int wall::getMaxHp() const { return this->maxHp; }
bool wall::isBuilt() const { return this->built && this->hp > 0; }
bool wall::isDestroyed() const { return this->built && this->hp <= 0; }
// economy: main.cpp draws, collides and removes walls by state; isBuilt is a standing wall, isDestroyed one that was built and lost all hp
