#ifndef NPC_H
#define NPC_H

#include <cstddef>
#include <vector>

#include "Collision/collision.h"
#include "Vector2D.h"

const std::size_t maxDestinations = 3;
const float colliderRadius = 10.0f;
const float arriveSlack = 20.0f;

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
    std::vector<Vector2D<float>> queuedPos;
    Vector2D<float> targetedMark = {0.0f, 0.0f};
    std::vector<Vector2D<float>> queuedMark;
    // ui: npcs sent together walk to their own spread-out points but should show one cross at the clicked point; each destination keeps a mark (where the cross is drawn) next to its pos (where the npc walks)
    // movement: shift right click adds destinations to visit in order; queuedPos holds the ones after targetedPos, and with targetedPos there are at most maxDestinations

    Facing facing = Facing::Down;
    bool facingLeft = false;

    bool selected = false;

    float speed = 0.2f;

    int player = 0;
    // unit: npcs belong to a player and are drawn in that player's color; player is the owner index, 0 = P1

    Vector2D<float> aim = {1.0f, 0.0f};
    // combat: a bow shoots where its npc aims; aim is a unit vector from the feet, right until set

public:
    int id;

    npc(int id, Vector2D<float> pos, int player = 0);
    ~npc();

    void update(float deltaTime);
    void moveX(float step);
    void moveY(float step);

    void setDestinaion(Vector2D<float> clicked, Vector2D<float> mark);
    void addDestination(Vector2D<float> clicked, Vector2D<float> mark);
    void setSelected(bool selected);
    void pushTo(Vector2D<float> pushed);
    void setAim(Vector2D<float> point);
    // combat: E aims every selected npc at the cursor; setAim turns the direction from the feet to point into a unit vector, and a point on the feet keeps the old aim
    void faceAim();
    // combat: a shooting npc turns to its aim; faceAim sets facing to the nearest of the 8 facings, until the next step turns it again

    Collider getCollider() const;
    // movement: npcs must not walk through or stand on each other; getCollider gives the solid oval at the feet (20 x 10 map px) and pushTo takes the position collision moved it to
    Vector2D<float> getPos() const;
    Vector2D<float> getSize() const;
    Vector2D<float> getTargetedPos() const;
    const std::vector<Vector2D<float>>& getQueuedPos() const;
    Vector2D<float> getTargetedMark() const;
    const std::vector<Vector2D<float>>& getQueuedMark() const;
    Facing getFacing() const;
    bool isFacingLeft() const;
    bool isMoving() const;
    bool isSelected() const;
    int getPlayer() const;
    Vector2D<float> getAim() const;
};
// unit: main.cpp could not create or draw an npc while the class lived inside npc.cpp; declaration is now a header, pos is the center bottom (feet) of the sprite in map px, speed is px per ms, and getters expose state without SDL

#endif
