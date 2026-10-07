#include "AI/agent.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

#include "Collision/collision.h"
#include "GameObject/gameObject.h"
#include "Movement/arrive.h"

static std::string kindOf(const item& thing)
{
    std::string kind = thing.getImage();
    const std::string suffix = ".png";
    if (kind.size() >= suffix.size() && kind.compare(kind.size() - suffix.size(), suffix.size(), suffix) == 0) kind.erase(kind.size() - suffix.size());
    return kind;
}
// ai_script: scripts name items by kind; the kind is the image name without ".png" (plank.png is plank)

static float distance(Vector2D<float> a, Vector2D<float> b)
{
    return std::hypot(a.x - b.x, a.y - b.y);
}

static bool overlaps(Collider hitbox, Collider body)
{
    hitbox.fixed = false;
    body.fixed = false;
    return separate(hitbox, body);
}
// ai_script: beside tests two ovals without moving anything; overlaps runs separate on copies made not fixed, since two fixed ovals never count as overlapping

agent::agent(int npcId)
{
    this->npcId = npcId;
}

void agent::start(const std::string& script, std::map<std::string, Slot> slots, World& world)
{
    this->stop(world);
    this->script = script;
    this->slots = std::move(slots);
    this->active = true;
    for (const auto& [name, slot] : this->slots) {
        if (slot.kind == Slot::Kind::Item && slot.thing && !slot.thing->isHeld()) slot.thing->setFetcher(this->npcId);
    }
}
// ai_script: a new order replaces the running behavior; the old one is stopped first, then the script and slots are set and every ground item in a slot is reserved for this npc

void agent::stop(World& world)
{
    this->active = false;
    for (const std::unique_ptr<item>& thing : world.items) {
        if (thing->getFetcher() == this->npcId && !thing->isHeld()) thing->setFetcher(-1);
    }
    this->slots.clear();
}
// ai_script: a stopped behavior must not keep items from other npcs; every ground item this npc was fetching is freed and the slots are cleared

void agent::forget(const item* thing)
{
    if (!thing) return;
    std::erase_if(this->slots, [thing](const auto& entry) { return entry.second.thing == thing; });
}
// ai_script: a slot must not point at an erased item; main calls forget before erasing, and every slot holding that item is dropped

void agent::run(World& world, const std::map<std::string, Script>& scripts)
{
    if (!this->active) return;
    const auto found = scripts.find(this->script);
    if (found == scripts.end()) return;
    this->runBlock(found->second.body, world);
}
// ai_script: the behavior is re-decided every frame from the current scene; run plays the script's body from the top, and does nothing when stopped or the script is not loaded

bool agent::runBlock(const std::vector<Node>& block, World& world)
{
    for (const Node& node : block) {
        const Status status = this->call(node, world);
        if (!this->active) return false;
        if (node.isIf) {
            const bool pass = (status == Status::Done) != node.negate;
            if (!this->runBlock(pass ? node.then : node.otherwise, world)) return false;
            continue;
        }
        if (status != Status::Done) return false;
    }
    return true;
}
// ai_script: a statement that is Running or Failed halts the frame and Done goes on; an if is true when its call is Done (flipped by not) and runs its then or else block, and stop halts at once

Status agent::call(const Node& node, World& world)
{
    const std::string& arg = node.arg;
    if (node.call == "find") return this->find(arg, world);
    if (node.call == "holding") return this->holding(arg, world);
    if (node.call == "moveTo") return this->moveTo(arg, world);
    if (node.call == "pickup") return this->pickup(arg, world);
    if (node.call == "target") return this->target(arg, world);
    if (node.call == "use") return this->use(arg, world);
    if (node.call == "full") return this->full(arg, world);
    if (node.call == "build") return this->build(arg, world);
    if (node.call == "drop") return this->drop(world);
    if (node.call == "stop") {
        this->stop(world);
        return Status::Done;
    }
    return Status::Failed;
}
// ai_script: each script call name maps to one skill; stop ends the behavior, and an unknown call fails

bool agent::consumed(const item* thing, World& world) const
{
    return std::find(world.consumed.begin(), world.consumed.end(), thing) != world.consumed.end();
}

item* agent::held(World& world) const
{
    for (const std::unique_ptr<item>& thing : world.items) {
        if (thing->getHolder() == this->npcId && !this->consumed(thing.get(), world)) return thing.get();
    }
    return nullptr;
}
// ai_script: a plank delivered this frame is still held until main erases it; held skips items in consumed so it is not used twice

bool agent::valid(const std::string& name, World& world) const
{
    const auto found = this->slots.find(name);
    if (found == this->slots.end()) return false;
    const Slot& slot = found->second;
    switch (slot.kind) {
    case Slot::Kind::Point:
        return true;
    case Slot::Kind::Item: {
        const item* thing = slot.thing;
        if (!thing || this->consumed(thing, world)) return false;
        if (thing->isHeld() && thing->getHolder() != this->npcId) return false;
        return thing->getFetcher() == -1 || thing->getFetcher() == this->npcId;
    }
    case Slot::Kind::Tree:
        if (slot.index < 0 || static_cast<std::size_t>(slot.index) >= world.trees.size()) return false;
        return !world.trees[slot.index].isFelled();
    case Slot::Kind::Wall: {
        if (slot.index < 0 || static_cast<std::size_t>(slot.index) >= world.walls.size()) return false;
        const wall& segment = world.walls[slot.index];
        if (name == "wall") return segment.isBuilt();
        if (name == "plan") return !segment.isBuilt() && !segment.isDestroyed();
        return !segment.isDestroyed();
    }
    default:
        return false;
    }
}
// ai_script: a slot goes stale as the scene changes; a point is always valid, an item while no other npc holds or fetches it, a tree while standing, slot plan while still a plan, slot wall while built, and any other wall slot until destroyed

const GameObject* agent::object(const Slot& slot, World& world) const
{
    if (slot.kind == Slot::Kind::Tree) return &world.trees[slot.index];
    if (slot.kind == Slot::Kind::Wall) return &world.walls[slot.index];
    return nullptr;
}

Vector2D<float> agent::position(const Slot& slot, World& world) const
{
    if (slot.kind == Slot::Kind::Point) return slot.point;
    if (slot.kind == Slot::Kind::Item) return slot.thing->getPos();
    return this->object(slot, world)->getCollider().pos;
}
// ai_script: skills walk to and aim at a slot's ground spot; a point is itself, an item its pos, and a tree or wall the center of its solid oval

bool agent::beside(const std::string& name, World& world) const
{
    const auto found = this->slots.find(name);
    if (found == this->slots.end()) return false;
    const Slot& slot = found->second;
    const npc& unit = world.npcs[this->npcId];
    if (slot.kind == Slot::Kind::Tree) return overlaps(world.trees[slot.index].getChopHitbox(), unit.getCollider());
    if (slot.kind != Slot::Kind::Wall) return false;
    const wall& segment = world.walls[slot.index];
    if (overlaps(segment.getHitbox(), unit.getCollider())) return true;
    return !segment.isBuilt() && !unit.isMoving() && distance(unit.getPos(), segment.getPos()) <= buildReach;
}
// ai_script: chopping and building need the npc at the object; beside is the npc's oval touching the tree's chop hitbox or the wall's hitbox, and a plan, which is not solid, also counts from buildReach once the npc stands still

Status agent::find(const std::string& name, World& world)
{
    if (this->valid(name, world)) return Status::Done;
    const Vector2D<float> feet = world.npcs[this->npcId].getPos();
    Slot found;
    float best = 0.0f;
    auto consider = [&](const Slot& candidate, Vector2D<float> pos) {
        const float away = distance(feet, pos);
        if (away > sightRange || (found.kind != Slot::Kind::None && away >= best)) return;
        found = candidate;
        best = away;
    };
    if (name == "tree") {
        for (std::size_t t = 0; t < world.trees.size(); ++t) {
            if (world.trees[t].isFelled()) continue;
            Slot candidate;
            candidate.kind = Slot::Kind::Tree;
            candidate.index = static_cast<int>(t);
            consider(candidate, world.trees[t].getCollider().pos);
        }
    } else if (name == "plan" || name == "wall") {
        for (std::size_t w = 0; w < world.walls.size(); ++w) {
            const wall& segment = world.walls[w];
            const bool fits = name == "wall" ? segment.isBuilt() : !segment.isBuilt() && !segment.isDestroyed();
            if (!fits) continue;
            Slot candidate;
            candidate.kind = Slot::Kind::Wall;
            candidate.index = static_cast<int>(w);
            consider(candidate, segment.getCollider().pos);
        }
    } else {
        for (const std::unique_ptr<item>& thing : world.items) {
            if (thing->isHeld() || kindOf(*thing) != name || this->consumed(thing.get(), world)) continue;
            if (thing->getFetcher() != -1 && thing->getFetcher() != this->npcId) continue;
            Slot candidate;
            candidate.kind = Slot::Kind::Item;
            candidate.thing = thing.get();
            consider(candidate, thing->getPos());
        }
    }

    const auto old = this->slots.find(name);
    if (found.kind == Slot::Kind::None) {
        if (old != this->slots.end()) this->slots.erase(old);
        return Status::Failed;
    }
    if (old != this->slots.end() && old->second.kind == Slot::Kind::Item && old->second.thing) {
        item* previous = old->second.thing;
        if (previous->getFetcher() == this->npcId && !previous->isHeld()) previous->setFetcher(-1);
    }
    if (found.kind == Slot::Kind::Item) found.thing->setFetcher(this->npcId);
    this->slots[name] = found;
    return Status::Done;
}
// ai_script: find keeps a still valid slot, else picks the nearest candidate within sightRange of the feet (standing tree, unbuilt plan, built wall, or a free ground item of that kind), reserves a found item and frees the old one, and fails and drops the slot when nothing is in sight

Status agent::holding(const std::string& name, World& world)
{
    const item* held = this->held(world);
    if (!held) return Status::Failed;
    if (name.empty()) return Status::Done;
    const auto found = this->slots.find(name);
    if (found != this->slots.end() && found->second.kind == Slot::Kind::Item && found->second.thing == held) return Status::Done;
    return kindOf(*held) == name ? Status::Done : Status::Failed;
}
// ai_script: holding checks the npc's hand; empty arg is any item, else the item in that slot or an item of that kind

Status agent::moveTo(const std::string& name, World& world)
{
    if (!this->valid(name, world)) return Status::Failed;
    const Slot& slot = this->slots[name];
    npc& unit = world.npcs[this->npcId];
    const Vector2D<float> feet = unit.getPos();

    if (slot.kind == Slot::Kind::Point) {
        if (!hasMark(unit, slot.mark)) {
            unit.setDestinaion(slot.point, slot.mark);
            return Status::Running;
        }
        return unit.isMoving() ? Status::Running : Status::Done;
    }

    if (slot.kind == Slot::Kind::Item) {
        if (slot.thing->getHolder() == this->npcId) return Status::Done;
        const Vector2D<float> pos = slot.thing->getPos();
        if (!unit.isMoving() && distance(feet, pos) <= arriveSlack) return Status::Done;
        if (!hasMark(unit, pos) || !unit.isMoving()) unit.setDestinaion(pos, pos);
        return Status::Running;
    }

    const GameObject* object = this->object(slot, world);
    if (!object) return Status::Failed;
    const Vector2D<float> mark = object->getPos();
    if (this->beside(name, world)) {
        if (unit.isMoving()) unit.setDestinaion(feet, mark);
        return Status::Done;
    }
    if (!hasMark(unit, mark)) {
        unit.setDestinaion(arriveAround(world.rng, *object, world.mapSize), mark);
    } else if (!unit.isMoving()) {
        unit.setDestinaion(object->getCollider().pos, mark);
    }
    return Status::Running;
}
// ai_script: moveTo walks until arrived; a point is done once its order is walked, an item once the npc stands within arriveSlack of it (or holds it), and a tree or wall once beside it, going first to a free spot around it and then straight at its center so collision stops the npc against the hitbox, and the npc is halted there

Status agent::pickup(const std::string& name, World& world)
{
    if (!this->valid(name, world)) return Status::Failed;
    const Slot& slot = this->slots[name];
    if (slot.kind != Slot::Kind::Item) return Status::Failed;
    item* thing = slot.thing;
    if (thing->getHolder() == this->npcId) return Status::Done;
    const Vector2D<float> feet = world.npcs[this->npcId].getPos();
    if (distance(feet, thing->getPos()) > arriveSlack) return Status::Failed;
    for (const std::unique_ptr<item>& other : world.items) {
        if (other.get() == thing || other->getHolder() != this->npcId || this->consumed(other.get(), world)) continue;
        other->drop({other->getPos().x, feet.y});
    }
    thing->pickUp(this->npcId);
    return Status::Done;
}
// ai_script: an npc holds one item; pickup needs the npc within arriveSlack of the slot item, drops whatever else it holds under its hand at feet level (like Q), and takes the item

Status agent::target(const std::string& name, World& world)
{
    if (!this->valid(name, world)) return Status::Failed;
    npc& unit = world.npcs[this->npcId];
    unit.setAim(this->position(this->slots[name], world));
    unit.faceAim();
    return Status::Done;
}
// ai_script: a tool acts where the npc faces; target aims the npc at the slot's ground spot and turns it that way

Status agent::use(const std::string& name, World& world)
{
    if (this->holding(name, world) != Status::Done) return Status::Failed;
    item* held = this->held(world);
    if (kindOf(*held) == "plank") {
        const auto plan = this->slots.find("plan");
        if (plan == this->slots.end() || plan->second.kind != Slot::Kind::Wall) return Status::Failed;
        if (!this->valid("plan", world) || !this->beside("plan", world)) return Status::Failed;
        if (!world.walls[plan->second.index].deliver()) return Status::Failed;
        world.consumed.push_back(held);
        return Status::Done;
    }
    if (held->isUsing()) return Status::Running;
    if (!held->startUse()) return Status::Running;
    if (!held->swings()) held->use(world.npcs[this->npcId], nullptr);
    return Status::Done;
}
// ai_script: use acts with the held item; a plank is delivered to the plan beside the npc and put in consumed, a tool is Running while it swings and starts the next use when idle, and a bow fires at once like E (an axe's hit comes from main's item loop when the swing ends)

Status agent::full(const std::string& name, World& world)
{
    if (!this->valid(name, world)) return Status::Failed;
    const Slot& slot = this->slots[name];
    if (slot.kind != Slot::Kind::Wall) return Status::Failed;
    return world.walls[slot.index].hasAllPlanks() ? Status::Done : Status::Failed;
}
// ai_script: full tells hauling from building; done when the wall slot already has every plank it needs

Status agent::build(const std::string& name, World& world)
{
    const auto found = this->slots.find(name);
    if (found == this->slots.end() || found->second.kind != Slot::Kind::Wall) return Status::Failed;
    const int index = found->second.index;
    if (index < 0 || static_cast<std::size_t>(index) >= world.walls.size()) return Status::Failed;
    wall& segment = world.walls[index];
    if (segment.isBuilt()) return Status::Done;
    if (!this->valid(name, world) || !segment.hasAllPlanks() || !this->beside(name, world)) return Status::Failed;
    return segment.build(world.deltaTime) ? Status::Done : Status::Running;
}
// ai_script: build adds this frame's time to a full plan the npc is beside; Running until it is built, Done once it stands (also when another npc finished it)

Status agent::drop(World& world)
{
    item* held = this->held(world);
    if (held) held->drop({held->getPos().x, world.npcs[this->npcId].getPos().y});
    return Status::Done;
}
// ai_script: drop lets go of the held item under the hand at feet level, like Q

bool agent::isActive() const { return this->active; }
const std::string& agent::getScript() const { return this->script; }
// ai_script: main shows and replaces behaviors by state; read-only getters
