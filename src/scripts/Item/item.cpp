#include "Item/item.h"

#include <utility>

item::item(Vector2D<float> pos, Vector2D<float> size, std::string image)
    : GameObject(pos, size, 0.0f, false, std::move(image))
{
}
// unit_item: npcs walk onto items to pick them up, so an item has no solid oval; radius 0, not fixed, and it is never put in the collision list

void item::use(npc&)
{
}
// unit_item: a plain item does nothing when used; each item type (axe, apple, ...) overrides use with its own action on the npc using it

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
}
// unit_item: an npc lets go of its item; the item is on the ground again at pos and can be picked up by anyone

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
