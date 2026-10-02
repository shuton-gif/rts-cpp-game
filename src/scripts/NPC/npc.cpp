#include "NPC/npc.h"

#include <cmath>

npc::npc(int id, Vector2D<float> pos)
{
    this->id = id;
    this->pos = pos;
    this->targetedPos = pos;
}
// unit: targetedPos was uninitialized so a new npc walked toward garbage; it now starts at its own position and stands still

npc::~npc() = default;
// unit: npc owns no heap memory or SDL handle, only plain values, so there is nothing to release; compiler-generated destructor frees the members

void npc::update(float deltaTime)
{
    const float dx = this->targetedPos.x - this->pos.x;
    const float dy = this->targetedPos.y - this->pos.y;
    if (dx == 0.0f && dy == 0.0f) return;

    if (dx < 0.0f) this->facingLeft = true;
    if (dx > 0.0f) this->facingLeft = false;

    if (dy < 0.0f) {
        this->facing = dx < 0.0f ? Facing::UpLeft : dx > 0.0f ? Facing::UpRight : Facing::Up;
    } else if (dy > 0.0f) {
        this->facing = dx < 0.0f ? Facing::DownLeft : dx > 0.0f ? Facing::DownRight : Facing::Down;
    } else {
        this->facing = dx < 0.0f ? Facing::Left : Facing::Right;
    }

    float step = this->speed * deltaTime;
    if (dx != 0.0f && dy != 0.0f) step *= 0.70710678f;

    this->moveX(step);
    this->moveY(step);
}
// movement: npc must walk at the same speed on any frame rate and face where it goes; each frame facing is picked from the sign of the remaining x and y (8 directions), then both axes step by speed * deltaTime, scaled by 1/sqrt(2) on diagonals

void npc::moveX(float step)
{
    const float remaining = this->targetedPos.x - this->pos.x;
    step = std::abs(step);

    if (std::abs(remaining) <= step) {
        this->pos.x = this->targetedPos.x;
    } else {
        this->pos.x += remaining > 0.0f ? step : -step;
    }
}
// movement: exact float compare never matched so npc overshot the target; x now steps toward targetedPos.x and snaps onto it when the remaining distance is within one step

void npc::moveY(float step)
{
    const float remaining = this->targetedPos.y - this->pos.y;
    step = std::abs(step);

    if (std::abs(remaining) <= step) {
        this->pos.y = this->targetedPos.y;
    } else {
        this->pos.y += remaining > 0.0f ? step : -step;
    }
}

void npc::setDestinaion(Vector2D<float> clicked)
{
    this->targetedPos = clicked;
}

void npc::setSelected(bool selected)
{
    this->selected = selected;
}

Vector2D<float> npc::getPos() const { return this->pos; }
Vector2D<float> npc::getSize() const { return this->size; }
Vector2D<float> npc::getTargetedPos() const { return this->targetedPos; }
Facing npc::getFacing() const { return this->facing; }
bool npc::isFacingLeft() const { return this->facingLeft; }
bool npc::isMoving() const { return this->pos != this->targetedPos; }
bool npc::isSelected() const { return this->selected; }
// unit: main.cpp needs npc state to draw and select it; read-only getters, isMoving is an exact compare because moveX/moveY snap onto the target
