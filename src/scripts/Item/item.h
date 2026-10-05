#ifndef ITEM_H
#define ITEM_H

#include <string>

#include "GameObject/gameObject.h"
#include "NPC/npc.h"
#include "Vector2D.h"

class item : public GameObject
{

private:
    int holder = -1;
    int fetcher = -1;
    // unit_item: an item lies on the ground or is held; holder is the id of the npc holding it and fetcher the id of the npc walking to pick it up, -1 for none

public:
    item(Vector2D<float> pos, Vector2D<float> size, std::string image);

    virtual void use(npc& user);

    void pickUp(int id);
    void setFetcher(int id);
    void moveTo(Vector2D<float> pos);

    int getHolder() const;
    int getFetcher() const;
    bool isHeld() const;
};
// unit_item: items are picked up and used by npcs; an item is a GameObject that is never solid, pickUp hands it to an npc, moveTo makes it follow the holder's hand, and each item type overrides use with its own action

#endif
