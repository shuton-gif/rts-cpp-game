#ifndef AGENT_H
#define AGENT_H

#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "AI/script.h"
#include "Item/item.h"
#include "NPC/npc.h"
#include "Tree/tree.h"
#include "Vector2D.h"
#include "Wall/wall.h"

const float sightRange = 1000.0f;
const float buildReach = 120.0f;
// ai_script: npcs act on what is around them, not across the map; find only looks sightRange map px from the feet, and a plan counts as beside within buildReach map px of its feet once the npc stands still

enum class Status { Running, Done, Failed };
// ai_script: a skill takes many frames; Running halts the script for this frame, Done goes on to the next line, Failed halts it (or is false in an if)

struct Slot {
    enum class Kind { None, Point, Item, Tree, Wall };
    Kind kind = Kind::None;
    Vector2D<float> point = {0.0f, 0.0f};
    Vector2D<float> mark = {0.0f, 0.0f};
    item* thing = nullptr;
    int index = -1;
};
// ai_script: skills pass what they act on by name; a slot is a ground point (point to walk to, mark to draw), an item pointer, or a tree or wall index, set by start or by find(name)

struct World {
    std::vector<npc>& npcs;
    std::vector<std::unique_ptr<item>>& items;
    std::vector<tree>& trees;
    std::vector<wall>& walls;
    std::mt19937& rng;
    Vector2D<float> mapSize;
    float deltaTime;
    std::vector<item*>& consumed;
};
// ai_script: skills read and change the scene without SDL; World holds references to main's lists, the frame time, and consumed, where use puts delivered planks for main to erase after calling forget

class agent
{

private:
    int npcId;
    std::string script;
    std::map<std::string, Slot> slots;
    bool active = false;
    // ai_script: each npc runs one behavior at a time; npcId is the npc's index in npcs, script the name of the running behavior, slots its named targets, and active is false once it stopped

    bool runBlock(const std::vector<Node>& block, World& world);
    Status call(const Node& node, World& world);
    // ai_script: the script is re-run from the top every frame; runBlock walks a block and is false once a statement is Running or Failed or the behavior stopped, and call dispatches one node to its skill

    bool valid(const std::string& name, World& world) const;
    bool consumed(const item* thing, World& world) const;
    item* held(World& world) const;
    const GameObject* object(const Slot& slot, World& world) const;
    Vector2D<float> position(const Slot& slot, World& world) const;
    bool beside(const std::string& name, World& world) const;
    // ai_script: skills share slot checks; valid is false for a slot that is gone (felled tree, item taken, plan built, wall destroyed), held is the npc's item not used up this frame, position is where a slot is on the ground, and beside is the npc touching a tree's or wall's hit oval (or standing within buildReach of a plan)

    Status find(const std::string& name, World& world);
    Status holding(const std::string& name, World& world);
    Status moveTo(const std::string& name, World& world);
    Status pickup(const std::string& name, World& world);
    Status target(const std::string& name, World& world);
    Status use(const std::string& name, World& world);
    Status full(const std::string& name, World& world);
    Status build(const std::string& name, World& world);
    Status drop(World& world);
    // ai_script: one member per script call; each acts on the slot named by the call's arg and returns Running, Done or Failed

public:
    explicit agent(int npcId);

    void start(const std::string& script, std::map<std::string, Slot> slots, World& world);
    void stop(World& world);
    void forget(const item* thing);
    void run(World& world, const std::map<std::string, Script>& scripts);
    // ai_script: main gives an npc a behavior on right click; start replaces the old one and reserves the slot items, stop ends it and frees every item it was fetching, forget drops slots of an item main is about to erase, and run plays one frame of the script

    bool isActive() const;
    const std::string& getScript() const;
};
// ai_script: npcs act like agents whose behaviors are text scripts chaining skills; an agent belongs to one npc, keeps the running script's name and slots, and runs it every frame until stop

#endif
