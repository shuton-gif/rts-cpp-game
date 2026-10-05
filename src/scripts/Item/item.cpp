#include "Item/item.h"

#include <utility>

item::item(Vector2D<float> pos, Vector2D<float> size, std::string image, float useDuration)
    : GameObject(pos, size, 0.0f, false, std::move(image))
{
    this->useDuration = useDuration;
}
// unit_item: npcs walk onto items to pick them up, so an item has no solid oval; radius 0, not fixed, and it is never put in the collision list
// animation: item types swing at their own speed; useDuration is set per item, 300 ms unless given

void item::use(npc&)
{
}
// unit_item: a plain item does nothing when used; each item type (axe, apple, ...) overrides use with its own action on the npc using it

bool item::startUse()
{
    if (!this->isHeld() || this->isUsing()) return false;
    this->useElapsed = 0.0f;
    return true;
}
// animation: only a held item that is not already swinging starts a use; elapsed restarts at 0

bool item::updateUse(float deltaTime)
{
    if (!this->isUsing()) return false;
    this->useElapsed += deltaTime;
    if (this->useElapsed < this->useDuration) return false;
    this->useElapsed = -1.0f;
    return true;
}
// animation: the swing advances by deltaTime ms; once it reaches useDuration it stops and returns true for that one frame so the caller runs use

bool item::isUsing() const
{
    return this->useElapsed >= 0.0f;
}

float item::getUseOffset() const
{
    if (!this->isUsing()) return 0.0f;
    const float t = this->useElapsed / this->useDuration;
    return -useRise + (useRise + useDip) * t * t;
}
// animation: the sprite starts useRise map px above its held spot and ends useDip map px below it; t is squared (ease-in) so it starts slow and speeds up like a swing, and it is 0 when not in use

float item::getUseAngle() const
{
    if (!this->isUsing()) return 0.0f;
    const float t = this->useElapsed / this->useDuration;
    return useTurn * t * t;
}
// animation: the item turns from upright (0) to flat (useTurn degrees) with the same t squared as the drop, so turning and coming down stay in step; 0 when not in use

void item::pickUp(int id)
{
    this->holder = id;
    this->fetcher = -1;
}
// unit_item: the fetching npc arrived; the item now belongs to that npc and no one is walking to it any more

void item::drop(Vector2D<float> pos)
{
    this->holder = -1;
    this->pos = pos;
    this->useElapsed = -1.0f;
}
// unit_item: an npc lets go of its item; the item is on the ground again at pos and can be picked up by anyone, and a swing in progress is cancelled without use

void item::setFetcher(int id)
{
    this->fetcher = id;
}

void item::moveTo(Vector2D<float> pos)
{
    this->pos = pos;
}
// unit_item: a held item moves with its holder; main.cpp sets pos to the holder's hand every frame

int item::getHolder() const { return this->holder; }
int item::getFetcher() const { return this->fetcher; }
bool item::isHeld() const { return this->holder >= 0; }
// unit_item: main.cpp needs to know who holds or fetches an item to move and draw it; read-only getters
