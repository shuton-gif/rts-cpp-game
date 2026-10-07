#include "Item/bow.h"

#include <cmath>
#include <utility>

bow::bow(Vector2D<float> pos, Vector2D<float> size, std::string image, std::vector<arrow>& arrows)
    : item(pos, size, std::move(image), bowCooldown), arrows(arrows)
{
}

bool bow::swings() const
{
    return false;
}

void bow::use(npc& user, GameObject*)
{
    user.faceAim();
    const float reach = std::abs(this->getPos().x - user.getPos().x);
    this->moveTo({user.getPos().x + (user.isFacingLeft() ? -reach : reach), this->getPos().y});
    // combat: the shooter turns to its aim before the shot; the bow jumps to the hand on the new side at once, so the arrow does not leave from the side it was held on before the turn
    const Vector2D<float> center = {this->getPos().x, this->getPos().y - this->getSize().y / 2.0f};
    const float height = user.getPos().y - center.y;
    this->arrows.emplace_back(center, user.getAim(), height);
}
// combat: a shot leaves from the middle of the bow along the holder's aim; its height above the holder's feet keeps it sorted in front of the shooter like the bow, and target is ignored because the arrow flies on its own
