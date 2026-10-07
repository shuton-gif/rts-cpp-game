#ifndef BOW_H
#define BOW_H

#include <string>
#include <vector>

#include "Arrow/arrow.h"
#include "GameObject/gameObject.h"
#include "Item/item.h"
#include "NPC/npc.h"
#include "Vector2D.h"

const float bowCooldown = 800.0f;
// combat: a bow can shoot once per bowCooldown ms (raised from 500 to 800); it is the bow's useDuration, during which E is ignored

class bow : public item
{

private:
    std::vector<arrow>& arrows;

public:
    bow(Vector2D<float> pos, Vector2D<float> size, std::string image, std::vector<arrow>& arrows);

    bool swings() const override;
    void use(npc& user, GameObject* target) override;
};
// combat: the bow shoots arrows; it fires when used instead of swinging, and use adds an arrow to the scene's list, which must outlive the bow

#endif
