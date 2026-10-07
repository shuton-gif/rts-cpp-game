#ifndef ARROW_H
#define ARROW_H

#include "Vector2D.h"

const float arrowSpeed = 1.0f;
const float arrowRange = 800.0f;
const float arrowLife = 2000.0f;
// combat: a shot arrow flies straight at arrowSpeed map px per ms for arrowRange map px (raised from 400 to 800), then lies still until arrowLife ms after the shot

class arrow
{

private:
    Vector2D<float> pos = {0.0f, 0.0f};
    Vector2D<float> dir = {1.0f, 0.0f};
    float height = 0.0f;
    float traveled = 0.0f;
    float age = 0.0f;

public:
    arrow(Vector2D<float> pos, Vector2D<float> dir, float height);

    void update(float deltaTime);

    Vector2D<float> getPos() const;
    float getFeet() const;
    double getAngle() const;
    bool isGone() const;
};
// combat: the bow shoots arrows; pos is the center of the sprite in map px, dir a unit vector, height how far pos is above the ground so getFeet gives the y it sorts by, and getAngle turns arrow.png (drawn pointing right) along dir

#endif
