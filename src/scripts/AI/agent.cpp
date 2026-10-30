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
    for (const auto& [name, slot] : this->slots) {
        if (slot.kind == Slot::Kind::Wall && slot.index >= 0 && static_cast<std::size_t>(slot.index) < world.walls.size()) world.walls[slot.index].release(this->npcId);
    }
    this->slots.clear();
}
// economy-wall-group: a stopped behavior must not keep items or plank claims from other npcs; every ground item this npc was fetching is freed, its wall slots give back their claims and the slots are cleared

void agent::forget(const item* thing)
{
    if (!thing) return;
    std::erase_if(this->slots, [thing](const auto& entry) { return entry.second.thing == thing; });
}
// ai_script: a slot must not point at an erased item; main calls forget before erasing, and every slot holding that item is dropped

void agent::remapTrees(const std::vector<int>& newIndex)
{
    for (auto& [name, slot] : this->slots) {
        if (slot.kind != Slot::Kind::Tree) continue;
        const bool known = slot.index >= 0 && static_cast<std::size_t>(slot.index) < newIndex.size();
        slot.index = known ? newIndex[slot.index] : -1;
    }
}
// ai_script: an unloaded chunk erases its trees and shifts the rest; every tree slot takes its index from main's old to new table, and an index outside the table becomes -1 so valid is false and find picks a new tree

void agent::run(World& world, const std::map<std::string, Script>& scripts)
{
    if (!this->active) return;
    const auto found = scripts.find(this->script);
    if (found == scripts.end()) return;
    this->settle(world);
    this->runBlock(found->second.body, world);
}
// ai_script: the behavior is re-decided every frame from the current scene; run settles remembered slots, plays the script's body from the top, and does nothing when stopped or the script is not loaded

const fogMap& agent::fog(World& world) const
{
    const int last = static_cast<int>(world.fogs.size()) - 1;
    return world.fogs[static_cast<std::size_t>(std::clamp(world.npcs[this->npcId].getPlayer(), 0, last))];
}

void agent::settle(World& world)
{
    const fogMap& team = this->fog(world);
    for (auto& [name, slot] : this->slots) {
        if (slot.kind != Slot::Kind::Remembered || team.state(slot.point) != FogState::Sight) continue;
        const Seen* entry = team.seenAt(slot.seen, slot.point);
        slot.kind = !entry ? Slot::Kind::None : entry->kind == Seen::Kind::Tree ? Slot::Kind::Tree : Slot::Kind::Wall;
        slot.index = entry ? entry->index : -1;
    }
}
// fog: a found object might be gone on arrival; once its spot is in sight the snapshot is current, so the slot takes the live index if the entry is still there, and otherwise goes None so valid is false and the next find picks something else

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
    case Slot::Kind::Remembered:
        return true;
    default:
        return false;
    }
}
// fog: out of sight the team cannot know a remembered target is gone, so a Remembered slot stays valid until settle turns it
// ai_script: a slot goes stale as the scene changes; a point is always valid, an item while no other npc holds or fetches it, a tree while standing, slot plan while still a plan, slot wall while built, and any other wall slot until destroyed

const GameObject* agent::object(const Slot& slot, World& world) const
{
    if (slot.kind == Slot::Kind::Tree) return &world.trees[slot.index];
    if (slot.kind == Slot::Kind::Wall) return &world.walls[slot.index];
    return nullptr;
}

Vector2D<float> agent::position(const Slot& slot, World& world) const
{
    if (slot.kind == Slot::Kind::Point || slot.kind == Slot::Kind::Remembered) return slot.point;
    if (slot.kind == Slot::Kind::Item) return slot.thing->getPos();
    return this->object(slot, world)->getCollider().pos;
}
// ai_script: skills walk to and aim at a slot's ground spot; a point or remembered spot is itself, an item its pos, and a tree or wall the center of its solid oval

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
    const fogMap& team = this->fog(world);
    Slot found;
    float best = 0.0f;
    auto consider = [&](const Slot& candidate, Vector2D<float> pos) {
        const float away = distance(feet, pos);
        if (found.kind != Slot::Kind::None && away >= best) return;
        found = candidate;
        best = away;
    };
    auto considerSeen = [&](const Seen& entry) {
        Slot candidate;
        if (team.state(entry.pos) == FogState::Sight) {
            candidate.kind = entry.kind == Seen::Kind::Tree ? Slot::Kind::Tree : Slot::Kind::Wall;
            candidate.index = entry.index;
        } else {
            candidate.kind = Slot::Kind::Remembered;
            candidate.seen = entry.kind;
            candidate.point = entry.pos;
        }
        consider(candidate, entry.pos);
    };
    if (name == "tree") {
        for (const Seen& entry : team.getSeen()) {
            if (entry.kind == Seen::Kind::Tree) considerSeen(entry);
        }
    } else if (name == "plan" || name == "wall") {
        for (const Seen& entry : team.getSeen()) {
            if (entry.kind == Seen::Kind::Wall && entry.built == (name == "wall")) considerSeen(entry);
        }
    } else {
        for (const std::unique_ptr<item>& thing : world.items) {
            if (thing->isHeld() || kindOf(*thing) != name || this->consumed(thing.get(), world)) continue;
            if (thing->getFetcher() != -1 && thing->getFetcher() != this->npcId) continue;
            if (team.state(thing->getPos()) != FogState::Sight) continue;
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
// fog: find uses what the team knows, not a range around the npc; trees, plans and walls come from the snapshot, as live slots in sight and Remembered slots elsewhere, and ground items only from sight
// ai_script: find keeps a still valid slot, else picks the nearest candidate (standing tree, unbuilt plan, built wall, or a free ground item of that kind), reserves a found item and frees the old one, and fails and drops the slot when the team knows of none

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

    if (slot.kind == Slot::Kind::Remembered) {
        if (!hasMark(unit, slot.point) || !unit.isMoving()) unit.setDestinaion(slot.point, slot.point);
        return Status::Running;
    }
    // fog: a remembered target has no live object to walk around yet; the npc heads for its feet, and the slot settles once the view field reaches it, well before arrival

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
        world.walls[plan->second.index].release(this->npcId);
        world.consumed.push_back(held);
        return Status::Done;
    }
    if (held->isUsing()) return Status::Running;
    if (!held->startUse()) return Status::Running;
    if (!held->swings()) held->use(world.npcs[this->npcId], nullptr);
    return Status::Done;
}
// economy-wall-group: use acts with the held item; a plank is delivered to the plan beside the npc, gives back the npc's claim and is put in consumed, a tool is Running while it swings and starts the next use when idle, and a bow fires at once like E (an axe's hit comes from main's item loop when the swing ends)

Status agent::full(const std::string& name, World& world)
{
    if (!this->valid(name, world)) return Status::Failed;
    const Slot& slot = this->slots[name];
    if (slot.kind != Slot::Kind::Wall) return Status::Failed;
    return world.walls[slot.index].claim(this->npcId) ? Status::Failed : Status::Done;
}
// economy-wall-group: only the npcs a group still needs may haul; full is Failed for an npc that holds a plank claim, so it hauls, and Done for one with no plank left to claim, so it goes to the plan and builds once every plank is in

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
