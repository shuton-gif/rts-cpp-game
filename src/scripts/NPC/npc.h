#ifndef NPC_H
#define NPC_H

#include "Vector2D.h"

/**
 * npc sprite
 * width: 6
 * height: 10
 */

enum class Facing {
    Up, // assets/npc-u.png
    Left, // assets/npc-l.png
    Right, // assets/npc-r.png
    Down, // npc-l.png or npc-r.png, the side last faced
    UpLeft, UpRight, // assets/npc-u.png
    DownLeft, // assets/npc-l.png
    DownRight // assets/npc-r.png
};

class npc
{

private:
    Vector2D<float> pos = {0.0f, 0.0f};
    Vector2D<float> size = {6.0f, 10.0f};
    Vector2D<float> targetedPos = {0.0f, 0.0f};

    Facing facing = Facing::Down;
    bool facingLeft = false;

    bool selected = false;

    float speed = 0.2f;

public:
    int id;

    npc(int id, Vector2D<float> pos);
    ~npc();

    void update(float deltaTime);
    void moveX(float step);
    void moveY(float step);

    void setDestinaion(Vector2D<float> clicked);
    void setSelected(bool selected);

    Vector2D<float> getPos() const;
    Vector2D<float> getSize() const;
    Vector2D<float> getTargetedPos() const;
    Facing getFacing() const;
    bool isFacingLeft() const;
    bool isMoving() const;
    bool isSelected() const;
};
// unit: main.cpp could not create or draw an npc while the class lived inside npc.cpp; declaration is now a header, pos is the sprite center in map px, speed is px per ms, and getters expose state without SDL

#endif
