#ifndef ARRIVE_H
#define ARRIVE_H

#include <random>

#include "GameObject/gameObject.h"
#include "NPC/npc.h"
#include "Vector2D.h"

const float groundArriveRad = 50.0f;
const int arriveTries = 30;
// movement: arrive helpers are shared free functions that clamp to the mapSize passed in, and aroundRadius is the spread radius around an object's solid oval

Vector2D<float> fuzzyArrive(std::mt19937& rng, Vector2D<float> target, Vector2D<float> mapSize, float rad = groundArriveRad);
// movement: the spread around a target depends on what was clicked; rad is the oval's horizontal radius in map px (vertical rad / 2, the 1 : 2 oval), and a random point inside it is returned, evenly spread over the oval and kept inside the map

float aroundRadius(const GameObject& object);
// movement: aroundRadius is the spread radius around an object's solid oval; it is the oval's width plus one npc radius, so there is always a ring outside the oval where an npc fits

Vector2D<float> arriveAround(std::mt19937& rng, const GameObject& object, Vector2D<float> mapSize);
// movement: npcs sent to an object spread around its solid oval; a spot where an npc would overlap the object's solid oval could never be reached, so it is drawn again (up to arriveTries times) until the npc fits beside the object

bool hasMark(const npc& unit, Vector2D<float> mark);
// movement: hasMark checks if a unit has a mark as its current target or in its queued marks

#endif
