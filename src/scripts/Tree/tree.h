#ifndef TREE_H
#define TREE_H

#include <string>

#include "GameObject/gameObject.h"
#include "Vector2D.h"

class tree : public GameObject
{

private:
    int hp = 5;
    int maxHp = 5;

public:
    tree(Vector2D<float> pos, Vector2D<float> size, std::string image, int maxHp = 5);

    bool chop(int damage);

    int getHp() const;
    int getMaxHp() const;
    bool isFelled() const;
};
// economy: trees are chopped down for wood; a tree is a fixed solid GameObject with hp, chop takes damage off it and is true on the chop that fells it, and a felled tree (hp 0) is gone from the map

#endif
