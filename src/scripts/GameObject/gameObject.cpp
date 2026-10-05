#include "GameObject/gameObject.h"

#include <utility>

GameObject::GameObject(Vector2D<float> pos, Vector2D<float> size, float colliderRadius, bool fixed, std::string image)
{
    this->pos = pos;
    this->size = size;
    this->colliderRadius = colliderRadius;
    this->fixed = fixed;
    this->image = std::move(image);
}
// core: every game object starts with its place, size, solid oval and sprite set once; the image name is moved in, not copied

GameObject::~GameObject() = default;
// core: derived types are deleted through a GameObject pointer; the virtual destructor runs the derived one first, the base owns no SDL handle so nothing else is freed

void GameObject::onClick()
{
}
// core: a plain object does nothing when clicked; each type overrides onClick with its own action

void GameObject::setClicked(bool clicked)
{
    this->clicked = clicked;
}

Collider GameObject::getCollider() const
{
    return {this->pos, this->colliderRadius, this->fixed};
}
// core: collision must see the object where it is now; the collider is built from pos on every call, so it never goes stale

Vector2D<float> GameObject::getPos() const { return this->pos; }
Vector2D<float> GameObject::getSize() const { return this->size; }
const std::string& GameObject::getImage() const { return this->image; }
bool GameObject::isClicked() const { return this->clicked; }
// core: main.cpp needs object state to draw and hit test it; read-only getters, no SDL
