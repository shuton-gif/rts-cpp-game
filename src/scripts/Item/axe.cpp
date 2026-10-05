#include "Item/axe.h"

#include <utility>

#include "Tree/tree.h"

axe::axe(Vector2D<float> pos, Vector2D<float> size, std::string image)
    : item(pos, size, std::move(image))
{
}

void axe::use(npc&, GameObject* target)
{
    tree* chopped = dynamic_cast<tree*>(target);
    if (chopped) chopped->chop(chopDamage);
}
// economy: a swing that ends next to a tree chops it; the target is checked to be a tree, which loses chopDamage hp, and a miss or any other object is left alone
