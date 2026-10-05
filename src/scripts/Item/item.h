#ifndef ITEM_H
#define ITEM_H

#include <string>

#include "GameObject/gameObject.h"
#include "NPC/npc.h"
#include "Vector2D.h"

const float useRise = 20.0f;
const float useDip = 5.0f;
const float useTurn = 90.0f;

class item : public GameObject
{

private:
    int holder = -1;
    int fetcher = -1;
    // unit_item: an item lies on the ground or is held; holder is the id of the npc holding it and fetcher the id of the npc walking to pick it up, -1 for none

    float useDuration = 300.0f;
    float useElapsed = -1.0f;
    // animation: a use plays over useDuration ms; useElapsed is the ms played so far, -1 when not in use

public:
    item(Vector2D<float> pos, Vector2D<float> size, std::string image, float useDuration = 300.0f);

    virtual void use(npc& user, GameObject* target);
    // economy: a tool acts on what is in front of the npc; target is the object within reach when the swing ends, nullptr on a miss

    bool startUse();
    bool updateUse(float deltaTime);
    bool isUsing() const;
    float getUseOffset() const;
    float getUseAngle() const;
    // animation: the swing also turns the item from upright to flat; getUseAngle is 0 to useTurn degrees, unsigned, and the caller picks the side
    // animation: using a held item shows a swing; startUse begins it, updateUse advances it and is true on the frame it ends (when use runs), and getUseOffset is the sprite's y shift from useRise map px up to useDip map px down

    void pickUp(int id);
    void drop(Vector2D<float> pos);
    void setFetcher(int id);
    void moveTo(Vector2D<float> pos);

    int getHolder() const;
    int getFetcher() const;
    bool isHeld() const;
};
// unit_item: items are picked up and used by npcs; an item is a GameObject that is never solid, pickUp hands it to an npc, moveTo makes it follow the holder's hand, and each item type overrides use with its own action

#endif
