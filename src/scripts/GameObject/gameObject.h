#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <string>

#include "Collision/collision.h"
#include "Vector2D.h"

class GameObject
{

protected:
    Vector2D<float> pos = {0.0f, 0.0f};
    Vector2D<float> size = {0.0f, 0.0f};
    float colliderRadius = 0.0f;
    bool fixed = false;
    std::string image;
    bool clicked = false;

public:
    GameObject(Vector2D<float> pos, Vector2D<float> size, float colliderRadius, bool fixed, std::string image);
    virtual ~GameObject();

    virtual void onClick();
    void setClicked(bool clicked);

    Collider getCollider() const;
    Vector2D<float> getPos() const;
    Vector2D<float> getSize() const;
    const std::string& getImage() const;
    bool isClicked() const;
};
// core: buildings, rocks and later npcs share position, size, a solid oval, a sprite and a click action; GameObject holds them without SDL, pos is the center bottom (feet) in map px, image is the sprite file name main.cpp loads and draws, and each type overrides onClick

#endif
