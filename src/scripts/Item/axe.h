#ifndef AXE_H
#define AXE_H

#include <string>

#include "GameObject/gameObject.h"
#include "Item/item.h"
#include "NPC/npc.h"
#include "Vector2D.h"

const int chopDamage = 1;

class axe : public item
{

public:
    axe(Vector2D<float> pos, Vector2D<float> size, std::string image);

    void use(npc& user, GameObject* target) override;
};
// economy: the axe is the tool that chops trees; it is an item whose use takes chopDamage hp off a tree target and does nothing to anything else

#endif
