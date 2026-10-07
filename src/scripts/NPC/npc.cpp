#include "NPC/npc.h"

#include <cmath>
#include <utility>

npc::npc(int id, Vector2D<float> pos, int player)
{
    this->id = id;
    this->pos = pos;
    this->targetedPos = pos;
    this->player = player;
}
// unit: targetedPos was uninitialized so a new npc walked toward garbage; it now starts at its own position and stands still

npc::~npc() = default;
// unit: npc owns no heap memory or SDL handle, only plain values, so there is nothing to release; compiler-generated destructor frees the members

void npc::update(float deltaTime)
{
    if (!this->isMoving() && !this->queuedPos.empty()) {
        this->targetedPos = this->queuedPos.front();
        this->targetedMark = this->queuedMark.front();
        this->queuedPos.erase(this->queuedPos.begin());
        this->queuedMark.erase(this->queuedMark.begin());
        this->replan();
    }
    // movement: after arriving the npc must go on to the next added destination; when it stands on targetedPos and the queue is not empty, the oldest queued point becomes targetedPos
    // pathfinding: the old path led to the old destination; popping a queued destination calls replan so main computes a path to the new targetedPos

    while (!this->route.empty() && this->route.front() == this->pos) this->route.erase(this->route.begin());
    const Vector2D<float> walkTo = this->route.empty() ? this->targetedPos : this->route.front();
    const float dx = walkTo.x - this->pos.x;
    const float dy = walkTo.y - this->pos.y;
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

    const float step = this->speed * deltaTime;
    const float dist = std::hypot(dx, dy);
    if (dist <= step) {
        this->pos = walkTo;
        if (!this->route.empty()) this->route.erase(this->route.begin());
    } else {
        this->pos.x += dx / dist * step;
        this->pos.y += dy / dist * step;
    }
}
// pathfinding: npcs must walk the path around trees and walls segment by segment at the same speed on any frame rate; the walking target is the first route waypoint or targetedPos when the route is empty, waypoints already stood on are dropped, facing is picked from the sign of x and y toward it (8 directions), and the npc steps speed * deltaTime straight along the vector, snapping onto the target and popping the waypoint when within one step

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

void npc::setDestinaion(Vector2D<float> clicked, Vector2D<float> mark)
{
    this->targetedPos = clicked;
    this->targetedMark = mark;
    this->queuedPos.clear();
    this->queuedMark.clear();
    this->replan();
}
// movement: a plain right click is a new order; it replaces the destination and drops every queued one
// pathfinding: the old path led to the old destination; a new order calls replan so main computes a path to the new targetedPos

void npc::addDestination(Vector2D<float> clicked, Vector2D<float> mark)
{
    if (!this->isMoving() && this->queuedPos.empty()) {
        this->targetedPos = clicked;
        this->targetedMark = mark;
        this->replan();
    } else if (this->queuedPos.size() + 1 < maxDestinations) {
        this->queuedPos.push_back(clicked);
        this->queuedMark.push_back(mark);
    }
}
// movement: shift right click adds a destination after the current ones, 3 at most; a standing npc takes it as its destination, a moving one appends it to the queue, and it is ignored when targetedPos plus the queue already hold maxDestinations
// pathfinding: a standing npc given a destination needs a path to it; only the branch that sets targetedPos calls replan, a queued destination gets its path when popped in update

Collider npc::getCollider() const
{
    return {this->pos, colliderRadius, false};
}

void npc::pushTo(Vector2D<float> pushed)
{
    const bool standing = !this->isMoving() && this->queuedPos.empty();
    this->pos = pushed;

    const float left = std::hypot(this->targetedPos.x - this->pos.x, this->targetedPos.y - this->pos.y);
    if (standing || left <= arriveSlack) {
        this->targetedPos = this->pos;
        this->route.clear();
    }
}
// movement: a pushed npc must not fight the push; a standing npc stays where it was pushed, and a walking one that is pushed within arriveSlack of its destination counts as arrived there, so npcs sent to nearly the same spot settle side by side instead of shoving forever
// pathfinding: an npc that counts as arrived must not walk leftover waypoints; setting targetedPos to pos also clears the route

bool npc::needsRoute() const { return this->isMoving() && !this->routed; }
const std::vector<Vector2D<float>>& npc::getRoute() const { return this->route; }
// pathfinding: main computes a path only when an npc walks without one and draws the route as a white line; read-only, needsRoute is true while moving and not routed

void npc::setRoute(std::vector<Vector2D<float>> points)
{
    if (!points.empty() && points.back() == this->targetedPos) points.pop_back();
    this->route = std::move(points);
    this->routed = true;
    this->stuckTime = 0.0f;
}
// pathfinding: findPath ends its points with targetedPos, which update already walks to last; that point is dropped, the rest become the route, routed stops main asking again, and stuckTime restarts

void npc::replan()
{
    this->routed = false;
    this->route.clear();
}
// pathfinding: a new destination or a stuck npc needs a fresh path; replan clears the route and routed so needsRoute asks main for one

bool npc::trackProgress(Vector2D<float> before, float deltaTime)
{
    if (!this->isMoving()) {
        this->stuckTime = 0.0f;
        return false;
    }

    const float moved = std::hypot(this->pos.x - before.x, this->pos.y - before.y);
    if (moved < this->speed * deltaTime * 0.25f) {
        this->stuckTime += deltaTime;
    } else {
        this->stuckTime = 0.0f;
    }

    if (this->stuckTime < stuckLimit) return false;
    this->stuckTime = 0.0f;
    this->replan();
    return true;
}
// pathfinding: an npc blocked by something the path missed must not push forever; main passes the pos from before this frame, a frame moving under a quarter of a full step adds deltaTime to stuckTime, any other frame or standing resets it, and reaching stuckLimit ms resets it, replans and returns true

void npc::setAim(Vector2D<float> point)
{
    const Vector2D<float> delta = {point.x - this->pos.x, point.y - this->pos.y};
    const float length = std::hypot(delta.x, delta.y);
    if (length <= 0.0f) return;
    this->aim = {delta.x / length, delta.y / length};
}
// combat: the bow needs a direction, not a point; the offset from the feet to point is divided by its length, and a zero offset has no direction so the last aim stays

void npc::faceAim()
{
    const int octant = static_cast<int>(std::lround(std::atan2(this->aim.y, this->aim.x) / 0.78539816f));
    switch (octant) {
    case 0: this->facing = Facing::Right; break;
    case 1: this->facing = Facing::DownRight; break;
    case 2: this->facing = Facing::Down; break;
    case 3: this->facing = Facing::DownLeft; break;
    case -1: this->facing = Facing::UpRight; break;
    case -2: this->facing = Facing::Up; break;
    case -3: this->facing = Facing::UpLeft; break;
    default: this->facing = Facing::Left; break;
    }
    if (octant == 3 || octant == -3 || octant == 4 || octant == -4) this->facingLeft = true;
    if (octant == 0 || octant == 1 || octant == -1) this->facingLeft = false;
}
// combat: a shooting npc must look where it shoots; the aim angle is rounded to the nearest of 8 facings (45 degree steps, map y down), left-side facings set facingLeft and right-side ones clear it, and straight up or down keep the last side like walking does

void npc::setSelected(bool selected)
{
    this->selected = selected;
}

Vector2D<float> npc::getPos() const { return this->pos; }
Vector2D<float> npc::getSize() const { return this->size; }
Vector2D<float> npc::getTargetedPos() const { return this->targetedPos; }
const std::vector<Vector2D<float>>& npc::getQueuedPos() const { return this->queuedPos; }
Vector2D<float> npc::getTargetedMark() const { return this->targetedMark; }
const std::vector<Vector2D<float>>& npc::getQueuedMark() const { return this->queuedMark; }
// ui: the cross is drawn at the clicked point, not at the npc's own spread-out point; every destination stores its mark alongside its pos, set, queued and popped together
Facing npc::getFacing() const { return this->facing; }
bool npc::isFacingLeft() const { return this->facingLeft; }
bool npc::isMoving() const { return this->pos != this->targetedPos; }
bool npc::isSelected() const { return this->selected; }
int npc::getPlayer() const { return this->player; }
// unit: main.cpp picks the npc's sprite color from its owner; read-only getter
Vector2D<float> npc::getAim() const { return this->aim; }
// combat: the bow shoots along its holder's aim; read-only getter
// unit: main.cpp needs npc state to draw and select it; read-only getters, isMoving is an exact compare because moveX/moveY snap onto the target
