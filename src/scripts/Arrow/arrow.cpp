#include "Arrow/arrow.h"

#include <algorithm>
#include <cmath>

arrow::arrow(Vector2D<float> pos, Vector2D<float> dir, float height)
{
    this->pos = pos;
    this->dir = dir;
    this->height = height;
}
// combat: an arrow starts at the bow, flying along the shooter's aim at the bow's height

void arrow::update(float deltaTime)
{
    this->age += deltaTime;
    const float step = std::min(arrowSpeed * deltaTime, arrowRange - this->traveled);
    if (step <= 0.0f) return;
    this->pos = {this->pos.x + this->dir.x * step, this->pos.y + this->dir.y * step};
    this->traveled += step;
}
// combat: the arrow ages by deltaTime ms and moves arrowSpeed x deltaTime map px along dir, cut short so it stops exactly at arrowRange and stays there

Vector2D<float> arrow::getPos() const { return this->pos; }
float arrow::getFeet() const { return this->pos.y + this->height; }
double arrow::getAngle() const { return std::atan2(this->dir.y, this->dir.x) * 180.0 / 3.14159265358979; }
bool arrow::isGone() const { return this->age >= arrowLife; }
// combat: main.cpp draws arrows sorted with everything else and removes them once gone; the angle is degrees clockwise from right because map y points down
