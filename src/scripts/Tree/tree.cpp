#include "Tree/tree.h"

#include <utility>

tree::tree(Vector2D<float> pos, Vector2D<float> size, std::string image, int maxHp)
    : GameObject(pos, size, size.x / 2.0f, true, std::move(image))
{
    this->maxHp = maxHp;
    this->hp = maxHp;
}
// economy: a tree starts at full hp; it is solid and never pushed, with an oval as wide as the tree

bool tree::chop(int damage)
{
    if (this->isFelled()) return false;
    this->hp -= damage;
    if (this->hp < 0) this->hp = 0;
    return this->isFelled();
}
// economy: each chop takes damage off hp, never below 0; it is true only on the chop that brings hp to 0, and a felled tree ignores more chops

Collider tree::getChopHitbox() const
{
    Collider hitbox = this->getCollider();
    hitbox.radius *= chopScale;
    return hitbox;
}
// economy: an axe needs a bigger target than the solid oval; the hitbox is centered on the tree like its collider with radius x chopScale (half height still radius / 2)

int tree::getHp() const { return this->hp; }
int tree::getMaxHp() const { return this->maxHp; }
bool tree::isFelled() const { return this->hp <= 0; }
// economy: main.cpp draws the hp bar and removes a felled tree; read-only getters
