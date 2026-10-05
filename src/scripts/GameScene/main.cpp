#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <vector>

#include "Collision/collision.h"
#include "GameObject/gameObject.h"
#include "Grass/grass.h"
#include "Item/item.h"
#include "NPC/npc.h"
#include "Vector2D.h"
// core: Vector2D is shared with npc.cpp; definition moved to src/scripts/Vector2D.h and included here

struct IntervalTimer {
    Uint64 last = SDL_GetTicksNS();

    float tick()
    {
        const Uint64 now = SDL_GetTicksNS();
        const float deltaTime = static_cast<float>(now - last) / 1'000'000.0f;
        last = now;
        return deltaTime;
    }
};
// core: camera speed must not depend on frame rate; tick() returns ms since the previous call from the ns clock and resets

const Vector2D<float> bgSize = {4000.0f, 4000.0f};
const float edgeMargin = 8.0f;

Vector2D<float> camera = {0.0f, 0.0f};
float cameraSpeed = 1.0f;

Vector2D<float> mouse = {0.0f, 0.0f};
// camera: sketch values made real; camera is the top-left of the view in map px, cameraSpeed is px per ms (1.0 = 1000 px/s)

const float spriteScale = 4.0f;
const float footprintGap = 24.0f;
const float footprintLife = 1000.0f;

struct Sprites {
    SDL_Texture* npcUp = nullptr;
    SDL_Texture* npcLeft = nullptr;
    SDL_Texture* npcRight = nullptr;
    SDL_Texture* footprint = nullptr;
    SDL_Texture* cross = nullptr;
    SDL_Texture* selectedArrow = nullptr;
};

const float arrowGap = 20.0f;
const float arrowBobHeight = 4.0f;
const float arrowSize = 0.5f;
const Uint8 selectFillAlpha = 153;
const float grassSize = 0.75f;
const int clumpCount = 60;
const int clumpTries = 30;
const float clumpMargin = 24.0f;
const float groundArriveRad = 50.0f;
const int arriveTries = 30;
const int renderMargin = 100;
const int tileRatioX = 10;
const int tileRatioY = 10;
// map: the map is laid out in tiles; one tile is tileRatioX x tileRatioY map px (10 x 10), and the selection box snaps to them
const float holdGap = 5.0f;
const float holdHeight = 8.0f;
// unit_item: a held item was too close and overlapped the npc, then 15 and 10 were too far so the gap is 5; its center bottom sits holdHeight map px above the npc's feet, and holdGap map px of space stays between the npc's side and the item's side
const Vector2D<float> tempObjectSize = {300.0f, 300.0f};
// core: a temporary object to try clicking and gathering around, made 6 times bigger; arriveTries caps redraws of a spot that overlaps the object, tempObjectSize is its sprite area in map px (300 x 300), which also sets its collider (radius 150) and the arrive spread (rad 300)

struct Footprint {
    Vector2D<float> pos;
    float age;
    double angle;
};
// render: pixel sprites are tiny at real size and footprints must fade; every sprite is drawn at spriteScale, a footprint is dropped each footprintGap px walked and lives footprintLife ms

static Vector2D<float> cameraPx()
{
    return {std::floor(camera.x), std::floor(camera.y)};
}

float density = 1.0f;
float zoom = 1.0f;
float wheelTotal = 0.0f;
const float zoomMaxPerDensity = 4.0f;
// render: sprites were blurry because the window drew at half resolution and zoom was fractional; density is real screen pixels per window point (2 on Retina), and zoom is a whole number of real screen pixels per map px, so every sprite pixel covers whole screen pixels

static Vector2D<float> toPixels(const float x, const float y)
{
    return {x * density, y * density};
}
// input: mouse events arrive in window points but the game now works in real pixels; a point is multiplied by density

static Vector2D<float> viewSize(const Vector2D<float> viewport)
{
    return {viewport.x / zoom, viewport.y / zoom};
}
// camera: zoom changes how much map fits on screen; the default zoom equals density, and the visible map area is the window size in real pixels divided by zoom

static Vector2D<float> toMap(const Vector2D<float> screen)
{
    const Vector2D<float> cam = cameraPx();
    return {screen.x / zoom + cam.x, screen.y / zoom + cam.y};
}
// camera: clicks arrive in screen px but npcs live in map px; map point = screen point / zoom + whole-px camera, the inverse of how drawing places things

static void clampCamera(const Vector2D<float> viewport)
{
    const Vector2D<float> view = viewSize(viewport);
    camera.x = std::clamp(camera.x, 0.0f, std::max(0.0f, bgSize.x - view.x));
    camera.y = std::clamp(camera.y, 0.0f, std::max(0.0f, bgSize.y - view.y));
}

static void moveCamera(const Vector2D<float> viewport, const float deltaTime)
{
    const float step = cameraSpeed * density * deltaTime / zoom;
    const float margin = edgeMargin * density;

    if (mouse.x <= margin) camera.x -= step;
    if (mouse.x >= viewport.x - 1.0f - margin) camera.x += step;
    if (mouse.y <= margin) camera.y -= step;
    if (mouse.y >= viewport.y - 1.0f - margin) camera.y += step;

    clampCamera(viewport);
}
// camera: edge scroll like League of Legends; cursor within edgeMargin points of a window edge moves that axis by cameraSpeed * deltaTime (times density, divided by zoom, so screen speed is the same on any display and zoom), then camera is locked inside the map

static float zoomLowest(const Vector2D<float> viewport)
{
    return std::max({1.0f, std::ceil(viewport.x / bgSize.x), std::ceil(viewport.y / bgSize.y)});
}

static void zoomCamera(const Vector2D<float> viewport, const Vector2D<float> cursor, const float wheel)
{
    wheelTotal += wheel;
    const float levels = std::trunc(wheelTotal);
    wheelTotal -= levels;
    if (levels == 0.0f) return;

    const float before = zoom;
    zoom = std::clamp(zoom + levels, zoomLowest(viewport), std::max(zoomLowest(viewport), zoomMaxPerDensity * density));

    camera.x += cursor.x / before - cursor.x / zoom;
    camera.y += cursor.y / before - cursor.y / zoom;
    clampCamera(viewport);
}
// camera: mouse scroll zooms in and out in whole steps so pixels stay sharp; wheel movement is added up and each full notch moves zoom by 1, between 1 (or more, so the view is never bigger than the map) and 4 x density, and the camera shifts so the map point under the cursor stays under it

static bool drawBackground(SDL_Renderer* renderer)
{
    const Vector2D<float> cam = cameraPx();
    const SDL_FRect map = {-cam.x, -cam.y, bgSize.x, bgSize.y};

    if (!SDL_SetRenderDrawColor(renderer, 97, 210, 141, 255)) return false;
    return SDL_RenderFillRect(renderer, &map);
}
// render: checkerboard was a placeholder; the whole map is now one #61d28d rectangle shifted by the whole-px camera, and SDL clips the part that is off screen

static SDL_Texture* loadSprite(SDL_Renderer* renderer, const char* name)
{
    const char* base = SDL_GetBasePath();
    const std::string path = std::string(base ? base : "") + "assets/" + name;

    SDL_Surface* surface = SDL_LoadPNG(path.c_str());
    if (!surface) return nullptr;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) return nullptr;

    if (!SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST)) {
        SDL_DestroyTexture(texture);
        return nullptr;
    }
    return texture;
}
// assets: sprites must load from any working directory and stay sharp when scaled; path is the executable folder + assets/, PNG is read by SDL3 itself and the texture uses nearest-neighbour scaling

static double facingAngle(const Facing facing)
{
    switch (facing) {
    case Facing::Up: return 0.0;
    case Facing::UpRight: return 45.0;
    case Facing::Right: return 90.0;
    case Facing::DownRight: return 135.0;
    case Facing::Down: return 180.0;
    case Facing::DownLeft: return 225.0;
    case Facing::Left: return 270.0;
    case Facing::UpLeft: return 315.0;
    }
    return 0.0;
}
// render: footprints must point the way the npc walked; foot-print.png is drawn walking up, so each facing maps to a clockwise angle in degrees from up

Vector2D<float> renderView = {0.0f, 0.0f};

static bool inRenderArea(const SDL_FRect& rect)
{
    const float margin = static_cast<float>(renderMargin) / zoom;
    const Vector2D<float> cam = cameraPx();
    const float left = cam.x - margin;
    const float top = cam.y - margin;
    const float right = cam.x + renderView.x + margin;
    const float bottom = cam.y + renderView.y + margin;
    return rect.x <= right && rect.x + rect.w >= left && rect.y <= bottom && rect.y + rect.h >= top;
}
// render: objects far off screen were still drawn; the render area is the visible map (renderView) grown by renderMargin screen px on every side (renderMargin / zoom in map px), and a map-px rect counts as inside when any part touches it

static bool drawSprite(SDL_Renderer* renderer, SDL_Texture* texture, const Vector2D<float> center, const Uint8 alpha, const double angle = 0.0, const float size = 1.0f, const SDL_FlipMode flip = SDL_FLIP_NONE)
{
    float w = 0.0f;
    float h = 0.0f;
    if (!SDL_GetTextureSize(texture, &w, &h)) return false;
    w *= spriteScale * size;
    h *= spriteScale * size;

    const SDL_FRect area = {std::floor(center.x - w / 2.0f), std::floor(center.y - h / 2.0f), w, h};
    if (!inRenderArea(area)) return true;
    // render: every sprite goes through here, so one check covers all current and future objects; a sprite outside the render area is skipped, which is not an error

    const Vector2D<float> cam = cameraPx();
    const SDL_FRect dst = {area.x - cam.x, area.y - cam.y, w, h};
    // render: the arrow needs to be smaller than other sprites; size multiplies spriteScale for this one draw (1 = normal) and the sprite stays centered on its point
    if (!SDL_SetTextureAlphaMod(texture, alpha)) return false;
    return SDL_RenderTextureRotated(renderer, texture, nullptr, &dst, angle, nullptr, flip);
}
// render: an item held on the left must face left; flip mirrors the sprite (SDL_FLIP_HORIZONTAL), default is no flip
// render: npc, cross and footprint all draw the same way; texture is centered on a map point, scaled by spriteScale, shifted by the camera, faded by alpha and rotated around its center by angle (0 for npc and cross)

static SDL_Texture* spriteFor(const Sprites& sprites, const npc& unit)
{
    switch (unit.getFacing()) {
    case Facing::Up:
    case Facing::UpLeft:
    case Facing::UpRight:
        return sprites.npcUp;
    case Facing::Left:
    case Facing::DownLeft:
        return sprites.npcLeft;
    case Facing::Right:
    case Facing::DownRight:
        return sprites.npcRight;
    case Facing::Down:
        break;
    }
    return unit.isFacingLeft() ? sprites.npcLeft : sprites.npcRight;
}
// unit: 3 sprites cover 8 facings; every up direction uses npc-u, left and down-left use npc-l, right and down-right use npc-r, straight down keeps the side last faced

static SDL_FRect npcRect(const npc& unit)
{
    const Vector2D<float> pos = unit.getPos();
    const Vector2D<float> size = unit.getSize();
    return {
        pos.x - size.x * spriteScale / 2.0f,
        pos.y - size.y * spriteScale,
        size.x * spriteScale,
        size.y * spriteScale,
    };
}
// unit: npc pos is now its center bottom (feet) like grass; the npc's rect for clicking and for placing the arrow runs from pos up by the sprite height

static SDL_FRect boxBetween(const Vector2D<float> a, const Vector2D<float> b)
{
    const float tileW = static_cast<float>(tileRatioX);
    const float tileH = static_cast<float>(tileRatioY);
    const float left = std::floor(std::min(a.x, b.x) / tileW) * tileW;
    const float top = std::floor(std::min(a.y, b.y) / tileH) * tileH;
    const float right = (std::floor(std::max(a.x, b.x) / tileW) + 1.0f) * tileW;
    const float bottom = (std::floor(std::max(a.y, b.y) / tileH) + 1.0f) * tileH;
    return {left, top, right - left, bottom - top};
}
// input: the selection box follows the map tiles; it covers every whole tileRatioX x tileRatioY tile from the one under the drag start to the one under the cursor, so a click is one 10 x 10 tile and a drag grows in steps of 10 on a grid fixed to the map

static bool overlaps(const SDL_FRect& a, const SDL_FRect& b)
{
    return a.x <= b.x + b.w && b.x <= a.x + a.w && a.y <= b.y + b.h && b.y <= a.y + a.h;
}
// input: click and drag select use one test; both are a box in map px (a click is a zero-size box) checked against the npc rect, edges included

static bool drawOutline(SDL_Renderer* renderer, SDL_FRect rect)
{
    const Vector2D<float> cam = cameraPx();
    rect.x = std::floor(rect.x) - cam.x;
    rect.y = std::floor(rect.y) - cam.y;
    if (!SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND)) return false;
    if (!SDL_SetRenderDrawColor(renderer, 255, 255, 255, selectFillAlpha)) return false;
    if (!SDL_RenderFillRect(renderer, &rect)) return false;

    if (!SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)) return false;
    return SDL_RenderRect(renderer, &rect);
}
// ui: the drag select area needs to be visible on the green map; a map-px rect is filled white at 0.6 opacity with alpha blending, then outlined with a solid white 1 px rectangle

static std::vector<SDL_Texture*> loadGrassSprites(SDL_Renderer* renderer)
{
    SDL_Texture* texture = loadSprite(renderer, "grassv2.png");
    if (!texture) return {};
    return {texture};
}
// assets: the grass strip has a new sprite; grassv2.png (1 x 4) is the single grass sprite loaded, and the result is empty if it fails

struct Drawable {
    SDL_Texture* texture;
    Vector2D<float> center;
    float size;
    float feet;
    SDL_FlipMode flip;
    double angle;
};
// animation: a swinging item is drawn turned; every drawable carries an angle in degrees clockwise, 0 unless set after it is added

static bool addDrawable(std::vector<Drawable>& drawables, SDL_Texture* texture, const Vector2D<float> feet, const float size, const SDL_FlipMode flip = SDL_FLIP_NONE)
{
    float w = 0.0f;
    float h = 0.0f;
    if (!SDL_GetTextureSize(texture, &w, &h)) return false;

    drawables.push_back({texture, {feet.x, feet.y - h * spriteScale * size / 2.0f}, size, feet.y, flip, 0.0});
    return true;
}
// render: a held item is drawn mirrored when on the left; every drawable carries a flip passed on to drawSprite, none unless given
// render: in this game the lower a thing stands on the map, the more in front it shows, and everything standing on the map is positioned by its center bottom (feet); a thing is queued with its sprite half its height above the feet point, then the list is sorted by feet y and drawn top to bottom

static bool addGrassClump(std::vector<Drawable>& drawables, const std::vector<SDL_Texture*>& grassSprites, const GrassClump& clump)
{
    for (const GrassStrip& strip : clump.strips) {
        const Vector2D<float> feet = {
            clump.pos.x + strip.offset.x * spriteScale * grassSize,
            clump.pos.y + strip.offset.y * spriteScale * grassSize,
        };
        if (!addDrawable(drawables, grassSprites[strip.sprite], feet, grassSize)) return false;
    }
    return true;
}
// map: a clump is data only and needs drawing, with strips smaller than other sprites; a strip's offset is its center bottom (where the blade meets the ground), scaled from sprite px to map px, so strips of any height share a ground line

static Vector2D<float> fuzzyArrive(std::mt19937& rng, const Vector2D<float> target, const float rad = groundArriveRad)
{
    std::uniform_real_distribution<float> turn(0.0f, 2.0f * 3.14159265f);
    std::uniform_real_distribution<float> reach(0.0f, 1.0f);

    const float angle = turn(rng);
    const float radius = std::sqrt(reach(rng));
    return {
        std::clamp(std::floor(target.x + std::cos(angle) * radius * rad), 0.0f, bgSize.x),
        std::clamp(std::floor(target.y + std::sin(angle) * radius * rad / 2.0f), 0.0f, bgSize.y),
    };
}
// movement: the spread around a target depends on what was clicked; rad is the oval's horizontal radius in map px (vertical rad / 2, the 1 : 2 oval), and a random point inside it is returned, evenly spread over the oval and kept inside the map

static SDL_FRect objectRect(const GameObject& object)
{
    const Vector2D<float> pos = object.getPos();
    const Vector2D<float> size = object.getSize();
    return {pos.x - size.x / 2.0f, pos.y - size.y, size.x, size.y};
}
// core: a game object is clicked and gets its arrow by its sprite area; size is already map px, so the rect runs from the feet up by the height, centered on x

static Vector2D<float> arriveAround(std::mt19937& rng, const GameObject& object)
{
    const Vector2D<float> pos = object.getPos();
    const Vector2D<float> size = object.getSize();
    const float rad = std::max(size.x, size.y);

    Vector2D<float> spot = fuzzyArrive(rng, pos, rad);
    for (int attempt = 0; attempt < arriveTries; ++attempt) {
        Collider solid = object.getCollider();
        Collider unit = {spot, colliderRadius, false};
        if (!separate(solid, unit)) break;
        spot = fuzzyArrive(rng, pos, rad);
    }
    return spot;
}
// movement: npcs sent to an object spread around it with rad = max(w, h); a spot where an npc would overlap the object's solid oval could never be reached, so it is drawn again (up to arriveTries times) until the npc fits beside the object

static bool handsFree(const std::vector<item>& items, const int id)
{
    return std::none_of(items.begin(), items.end(), [id](const item& thing) {
        return thing.getHolder() == id || thing.getFetcher() == id;
    });
}

static bool hasMark(const npc& unit, const Vector2D<float> mark)
{
    const std::vector<Vector2D<float>>& queued = unit.getQueuedMark();
    return unit.getTargetedMark() == mark || std::find(queued.begin(), queued.end(), mark) != queued.end();
}
// unit_item: an npc carries one item at a time and keeps fetching only while the item is still among its destinations; hands are free when no item is held or fetched by that npc id, and hasMark checks the current and queued marks

static SDL_FRect clumpRect(const GrassClump& clump)
{
    const float w = clump.size.x * spriteScale * grassSize + clumpMargin;
    const float h = clump.size.y * spriteScale * grassSize + clumpMargin;
    return {clump.pos.x - w / 2.0f, clump.pos.y - h / 2.0f, w, h};
}

static std::vector<GrassClump> scatterGrassClumps(std::mt19937& rng, const int spriteCount)
{
    std::vector<GrassClump> clumps;
    for (int i = 0; i < clumpCount; ++i) {
        GrassClump clump = makeGrassClump(rng, {0.0f, 0.0f}, spriteCount);
        const SDL_FRect bound = clumpRect(clump);
        std::uniform_real_distribution<float> x(bound.w / 2.0f, bgSize.x - bound.w / 2.0f);
        std::uniform_real_distribution<float> y(bound.h / 2.0f, bgSize.y - bound.h / 2.0f);

        for (int attempt = 0; attempt < clumpTries; ++attempt) {
            clump.pos = {std::floor(x(rng)), std::floor(y(rng))};
            const SDL_FRect rect = clumpRect(clump);
            const bool free = std::none_of(clumps.begin(), clumps.end(), [&](const GrassClump& other) {
                return overlaps(rect, clumpRect(other));
            });
            if (free) {
                clumps.push_back(clump);
                break;
            }
        }
    }
    return clumps;
}
// map: clumps are plotted at random over the whole map; each of clumpCount clumps is made, then given random spots inside the map until its bound touches no other clump, and is skipped if clumpTries spots all collide

static int fail(const char* what)
{
    SDL_Log("%s: %s", what, SDL_GetError());
    SDL_Quit();
    return 1;
}

int main(int, char**)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) return fail("SDL_Init");

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("rts", 0, 0, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIGH_PIXEL_DENSITY, &window, &renderer)) {
        return fail("SDL_CreateWindowAndRenderer");
    }
    SDL_SetRenderVSync(renderer, 1);
    SDL_SetWindowMouseGrab(window, true);
    // render: game opens full screen; borderless desktop full screen with vsync, cursor kept inside the window so it can reach the edges

    int windowW = 0;
    int windowH = 0;
    if (!SDL_GetWindowSizeInPixels(window, &windowW, &windowH)) return fail("SDL_GetWindowSizeInPixels");
    Vector2D<float> viewport = {static_cast<float>(windowW), static_cast<float>(windowH)};

    density = SDL_GetWindowPixelDensity(window);
    if (density <= 0.0f) density = 1.0f;
    zoom = std::max(zoomLowest(viewport), std::round(density));
    // render: the window is drawn at the screen's real pixels instead of being stretched; viewport is in real pixels, and zoom starts at density so the game looks the same size as before

    const Vector2D<float> view = viewSize(viewport);
    camera = {(bgSize.x - view.x) / 2.0f, (bgSize.y - view.y) / 2.0f};
    // camera: starts at the middle of the map; view is centered by offsetting half of (map - visible map area)

    Sprites sprites;
    sprites.npcUp = loadSprite(renderer, "npc-u.png");
    sprites.npcLeft = loadSprite(renderer, "npc-l.png");
    sprites.npcRight = loadSprite(renderer, "npc-r.png");
    sprites.footprint = loadSprite(renderer, "foot-print.png");
    sprites.cross = loadSprite(renderer, "cross.png");
    sprites.selectedArrow = loadSprite(renderer, "selectedArrow.png");
    if (!sprites.npcUp || !sprites.npcLeft || !sprites.npcRight || !sprites.footprint || !sprites.cross || !sprites.selectedArrow) {
        return fail("loadSprite");
    }

    const std::vector<SDL_Texture*> grassSprites = loadGrassSprites(renderer);
    if (grassSprites.empty()) return fail("loadGrassSprites");

    std::mt19937 rng(std::random_device{}());
    const std::vector<GrassClump> clumps = scatterGrassClumps(rng, static_cast<int>(grassSprites.size()));

    std::vector<Drawable> drawables;
    std::vector<Collider> colliders;

    std::vector<npc> npcs;
    npcs.emplace_back(0, Vector2D<float>{bgSize.x / 2.0f, bgSize.y / 2.0f});
    npcs.emplace_back(1, Vector2D<float>{bgSize.x / 2.0f - 60.0f, bgSize.y / 2.0f + 20.0f});
    npcs.emplace_back(2, Vector2D<float>{bgSize.x / 2.0f + 60.0f, bgSize.y / 2.0f + 20.0f});
    // unit: two more npcs are in the scene from the start; they stand 60 map px left and right of the first one, slightly lower

    GameObject tempObject({bgSize.x / 2.0f + 250.0f, bgSize.y / 2.0f}, tempObjectSize, tempObjectSize.x / 2.0f, true, "tree.png");
    SDL_Texture* tempObjectSprite = loadSprite(renderer, tempObject.getImage().c_str());
    if (!tempObjectSprite) return fail("loadSprite tempObject");
    float tempObjectW = 0.0f;
    float tempObjectH = 0.0f;
    if (!SDL_GetTextureSize(tempObjectSprite, &tempObjectW, &tempObjectH)) return fail("SDL_GetTextureSize tempObject");
    const float tempObjectDrawSize = tempObjectSize.x / (tempObjectW * spriteScale);
    // core: the temporary object is a tree, 250 map px right of the npcs so its bigger solid oval does not cover them at start; it is a fixed solid oval as wide as the object, its sprite comes from its image name (tree.png) and is scaled to the object's width

    struct ItemSpawn {
        const char* image;
        Vector2D<float> offset;
    };
    const ItemSpawn itemSpawns[] = {
        {"axe.png", {-140.0f, 20.0f}},
        {"sword.png", {-140.0f, 80.0f}},
        {"sheild.png", {-60.0f, 90.0f}},
        {"pick-axe.png", {-220.0f, 20.0f}},
        {"bow.png", {-220.0f, 80.0f}},
        {"hoe.png", {-140.0f, 140.0f}},
    };
    // unit_item: more tools to try picking up; a pick-axe, a bow and a hoe lie further left and below, next to the axe, sword and shield
    std::vector<item> items;
    std::vector<SDL_Texture*> itemSprites;
    for (const ItemSpawn& spawn : itemSpawns) {
        SDL_Texture* texture = loadSprite(renderer, spawn.image);
        if (!texture) return fail("loadSprite item");
        float w = 0.0f;
        float h = 0.0f;
        if (!SDL_GetTextureSize(texture, &w, &h)) return fail("SDL_GetTextureSize item");
        items.emplace_back(Vector2D<float>{bgSize.x / 2.0f + spawn.offset.x, bgSize.y / 2.0f + spawn.offset.y}, Vector2D<float>{w * spriteScale, h * spriteScale}, spawn.image);
        itemSprites.push_back(texture);
    }
    // unit_item: an axe, a sword and a shield lie on the ground left of and below the npcs to try picking up; each spawn is an image and an offset from the map center, its size is the png size at spriteScale, and itemSprites[i] is the sprite of items[i]

    std::vector<Vector2D<float>> lastFootprint;
    for (const npc& unit : npcs) lastFootprint.push_back(unit.getPos());
    std::vector<Footprint> footprints;

    bool dragging = false;
    Vector2D<float> dragStart = {0.0f, 0.0f};
    // unit: the first npc starts at the middle of the map, which is the middle of the screen because the camera starts centered; npcs sit in a vector so more can be added

    IntervalTimer timer;
    bool mouseSeen = false;
    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) running = false;
                if (event.key.key == SDLK_Q) {
                    for (item& thing : items) {
                        if (!thing.isHeld() || !npcs[thing.getHolder()].isSelected()) continue;
                        thing.drop({thing.getPos().x, npcs[thing.getHolder()].getPos().y});
                    }
                }
                // unit_item: Q makes every selected npc drop the item it holds; the item lands on the ground where it was held, at the npc's feet line
                if (event.key.key == SDLK_E) {
                    for (item& thing : items) {
                        if (thing.isHeld() && npcs[thing.getHolder()].isSelected()) thing.startUse();
                    }
                }
                // animation: E makes every selected npc use the item it holds; startUse ignores an item that is already swinging
                break;
            case SDL_EVENT_MOUSE_MOTION:
                mouse = toPixels(event.motion.x, event.motion.y);
                mouseSeen = true;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                mouse = toPixels(event.button.x, event.button.y);
                mouseSeen = true;
                if (event.button.button == SDL_BUTTON_LEFT) {
                    dragStart = toMap(mouse);
                    dragging = true;
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    const bool shift = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
                    const Vector2D<float> clicked = toMap(mouse);
                    const auto selected = std::count_if(npcs.begin(), npcs.end(), [](const npc& unit) {
                        return unit.isSelected();
                    });
                    const SDL_FRect point = {clicked.x, clicked.y, 0.0f, 0.0f};
                    int picked = -1;
                    for (size_t i = 0; i < items.size(); ++i) {
                        if (items[i].isHeld() || !overlaps(point, objectRect(items[i]))) continue;
                        picked = static_cast<int>(i);
                        break;
                    }
                    int fetcherId = -1;
                    if (picked >= 0) {
                        items[picked].setFetcher(-1);
                        for (const npc& unit : npcs) {
                            if (!unit.isSelected() || !handsFree(items, unit.id)) continue;
                            fetcherId = unit.id;
                            break;
                        }
                        items[picked].setFetcher(fetcherId);
                        if (fetcherId < 0) picked = -1;
                    }
                    // unit_item: right click on a ground item sends one npc to pick it up; the first selected npc with free hands becomes its fetcher, and with no free npc the click counts as a ground click

                    const bool onObject = picked < 0 && overlaps(point, objectRect(tempObject));
                    const Vector2D<float> mark = onObject ? tempObject.getPos() : clicked;
                    for (npc& unit : npcs) {
                        if (!unit.isSelected()) continue;
                        if (unit.id == fetcherId) {
                            const Vector2D<float> spot = items[picked].getPos();
                            if (shift) {
                                unit.addDestination(spot, spot);
                            } else {
                                unit.setDestinaion(spot, spot);
                            }
                            continue;
                        }
                        // unit_item: the fetcher walks exactly onto the item and its mark is the item's pos, so the arrow shows over the item instead of a cross; shift queues it like any destination
                        const Vector2D<float> target = onObject ? arriveAround(rng, tempObject)
                            : selected > 1 ? fuzzyArrive(rng, clicked, groundArriveRad) : clicked;
                        // movement: a right click on an object sends npcs around it instead of onto it; every selected npc, even a single one, gets an arriveAround spot with rad = max(w, h) of the object, and ground clicks are unchanged
                        if (shift) {
                            unit.addDestination(target, mark);
                        } else {
                            unit.setDestinaion(target, mark);
                        }
                    }
                    // ui: an object target shows the arrow on the object instead of a cross; its mark is the object's pos, which the draw step recognises, while a ground click keeps the clicked point as mark
                    // movement: several npcs sent to one point arrived on the same px; when more than one is selected each gets its own fuzzyArrive point around the click, with groundArriveRad for open ground (a clicked building will pass max(h, w) of the building instead), and a single npc still goes exactly to the click
                    // input: shift right click adds another cross to walk to after the current ones; with shift held the point is added to each selected npc's destinations, without it the point replaces them
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                mouse = toPixels(event.button.x, event.button.y);
                if (event.button.button == SDL_BUTTON_LEFT && dragging) {
                    const SDL_FRect box = boxBetween(dragStart, toMap(mouse));
                    for (npc& unit : npcs) unit.setSelected(overlaps(box, npcRect(unit)));
                    const bool objectHit = overlaps(box, objectRect(tempObject));
                    if (objectHit && !tempObject.isClicked()) tempObject.onClick();
                    tempObject.setClicked(objectHit);
                    // core: an object is selected the same way as npcs; release sets clicked when the box touches its sprite area, calls onClick when it was not clicked before, and clears it otherwise
                    dragging = false;
                }
                break;
            // input: left click or left drag selects, right click moves the selection; drag start is kept in map px so the box stays put while the camera scrolls, and release selects every npc the box touches
            case SDL_EVENT_MOUSE_WHEEL:
                mouse = toPixels(event.wheel.mouse_x, event.wheel.mouse_y);
                zoomCamera(viewport, mouse, event.wheel.y);
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                viewport = {static_cast<float>(event.window.data1), static_cast<float>(event.window.data2)};
                density = SDL_GetWindowPixelDensity(window);
                if (density <= 0.0f) density = 1.0f;
                zoom = std::max(zoom, zoomLowest(viewport));
                clampCamera(viewport);
                break;
            // render: moving to another display changes the real pixel size and density; viewport and density are re-read, zoom is raised if the view would be bigger than the map, and the camera is locked inside the map again
            default:
                break;
            }
        }
        // input: full screen has no close button and the cursor is 0,0 until it moves; Esc quits and edge scroll waits for the first mouse motion

        const float deltaTime = timer.tick();
        if (mouseSeen) moveCamera(viewport, deltaTime);

        for (Footprint& print : footprints) print.age += deltaTime;
        std::erase_if(footprints, [](const Footprint& print) { return print.age >= footprintLife; });

        for (npc& unit : npcs) unit.update(deltaTime);

        colliders.clear();
        for (const npc& unit : npcs) colliders.push_back(unit.getCollider());
        colliders.push_back(tempObject.getCollider());
        // core: npcs must not walk through the object; its fixed collider goes after the npcs, so colliders[i] still matches npcs[i] and only npcs are pushed
        resolveCollisions(colliders);
        for (size_t i = 0; i < npcs.size(); ++i) {
            const Vector2D<float> pushed = {
                std::clamp(colliders[i].pos.x, 0.0f, bgSize.x),
                std::clamp(colliders[i].pos.y, 0.0f, bgSize.y),
            };
            if (pushed != npcs[i].getPos()) npcs[i].pushTo(pushed);
        }
        // movement: npcs walked through each other; after all have moved, one collider per npc goes through resolveCollisions and every npc that was pushed takes its new position, kept inside the map. Other solid objects join by adding their collider to the same list

        for (size_t i = 0; i < npcs.size(); ++i) {
            const Vector2D<float> pos = npcs[i].getPos();
            if (std::hypot(pos.x - lastFootprint[i].x, pos.y - lastFootprint[i].y) >= footprintGap) {
                footprints.push_back({pos, 0.0f, facingAngle(npcs[i].getFacing())});
                lastFootprint[i] = pos;
            }
        }
        // movement: footprints follow the npc and fade after 1000 ms; each npc drops one at its feet every footprintGap px walked, every footprint ages by deltaTime and is erased at footprintLife

        for (item& thing : items) {
            const int fetcher = thing.getFetcher();
            if (fetcher >= 0) {
                const npc& unit = npcs[fetcher];
                const Vector2D<float> pos = unit.getPos();
                const Vector2D<float> spot = thing.getPos();
                if (!unit.isMoving() && std::hypot(pos.x - spot.x, pos.y - spot.y) <= arriveSlack) {
                    thing.pickUp(fetcher);
                } else if (!hasMark(unit, spot)) {
                    thing.setFetcher(-1);
                }
            }
            // unit_item: the fetcher picks the item up once it stands still within arriveSlack of it (a push near it still counts), and stops fetching when a new order removed the item from its destinations; npc id is its index in npcs

            if (!thing.isHeld()) continue;
            npc& holder = npcs[thing.getHolder()];
            if (thing.updateUse(deltaTime)) thing.use(holder);
            // animation: the swing plays over the item's useDuration and use runs once on the frame it ends
            const float side = holder.isFacingLeft() ? -1.0f : 1.0f;
            const float reach = holder.getSize().x * spriteScale / 2.0f + holdGap + thing.getSize().x / 2.0f;
            thing.moveTo({holder.getPos().x + side * reach, holder.getPos().y - holdHeight + thing.getUseOffset()});
            // animation: a swinging item is shifted up or down by getUseOffset from its held spot, 0 when idle
        }
        // unit_item: a held item stays at its holder's side after the npcs moved; its center is half the npc width + holdGap + half the item width from the npc's center, on the right, or on the left when the npc faces left, and up or down keep the last side because isFacingLeft only changes on a left or right step

        if (!SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)) return fail("SDL_SetRenderDrawColor");
        if (!SDL_RenderClear(renderer)) return fail("SDL_RenderClear");
        if (!SDL_SetRenderScale(renderer, zoom, zoom)) return fail("SDL_SetRenderScale");
        // camera: zoom must scale everything drawn; the renderer multiplies all coordinates by zoom, so draw code keeps working in map px
        renderView = viewSize(viewport);
        if (!drawBackground(renderer)) return fail("drawBackground");

        for (const Footprint& print : footprints) {
            const Uint8 alpha = static_cast<Uint8>(255.0f * (1.0f - print.age / footprintLife));
            if (!drawSprite(renderer, sprites.footprint, print.pos, alpha, print.angle)) return fail("drawSprite footprint");
        }
        bool objectTargeted = false;
        std::vector<bool> itemTargeted(items.size(), false);
        for (const npc& unit : npcs) {
            if (!unit.isMoving()) continue;
            std::vector<Vector2D<float>> marks = unit.getQueuedMark();
            marks.push_back(unit.getTargetedMark());
            for (const Vector2D<float>& mark : marks) {
                if (mark == tempObject.getPos()) {
                    objectTargeted = true;
                    continue;
                }
                bool onItem = false;
                for (size_t i = 0; i < items.size(); ++i) {
                    if (items[i].isHeld() || mark != items[i].getPos()) continue;
                    itemTargeted[i] = true;
                    onItem = true;
                }
                if (onItem) continue;
                if (!drawSprite(renderer, sprites.cross, mark, 255)) return fail("drawSprite cross");
            }
        }
        // ui: npcs sent together showed one cross each at their spread-out points; the cross is drawn at each destination's mark, the clicked point they share, so one click shows one cross until the last npc heading there arrives, and a mark on the object or on a ground item draws no cross but flags that object or item for the arrow

        drawables.clear();
        for (const GrassClump& clump : clumps) {
            if (!inRenderArea(clumpRect(clump))) continue;
            if (!addGrassClump(drawables, grassSprites, clump)) return fail("addGrassClump");
        }
        // render: the whole map holds clumps and most are off screen; only clumps whose bound touches the render area (screen + renderMargin) are queued for sorting and drawing
        for (const npc& unit : npcs) {
            if (!addDrawable(drawables, spriteFor(sprites, unit), unit.getPos(), 1.0f)) return fail("addDrawable npc");
        }
        if (!addDrawable(drawables, tempObjectSprite, tempObject.getPos(), tempObjectDrawSize)) return fail("addDrawable tempObject");
        // core: the object stands on the map like npcs and grass; it joins the feet-y sorted list so a lower npc shows in front of it
        for (size_t i = 0; i < items.size(); ++i) {
            const bool mirrored = items[i].isHeld() && npcs[items[i].getHolder()].isFacingLeft();
            if (!addDrawable(drawables, itemSprites[i], items[i].getPos(), 1.0f, mirrored ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE)) return fail("addDrawable item");
            if (items[i].isHeld()) drawables.back().feet = npcs[items[i].getHolder()].getPos().y + 0.01f;
            if (items[i].isUsing()) {
                const float turn = (mirrored ? -1.0f : 1.0f) * items[i].getUseAngle();
                const float radians = turn * 3.14159265f / 180.0f;
                const float half = items[i].getSize().y / 2.0f;
                const Vector2D<float> pivot = items[i].getPos();
                drawables.back().center = {pivot.x + std::sin(radians) * half, pivot.y - std::cos(radians) * half};
                drawables.back().angle = turn;
            }
            // animation: a swinging item turns outward around its handle, clockwise on the right and counter-clockwise on the left; its center is swung around the bottom pivot by the angle, then the sprite is turned by the same angle about that center
        }
        // unit_item: a ground item sorts by its own feet like everything else; a held item sorts just below its holder's feet so it always shows in front of that npc, and it is mirrored while held on the left
        std::stable_sort(drawables.begin(), drawables.end(), [](const Drawable& a, const Drawable& b) {
            return a.feet < b.feet;
        });
        for (const Drawable& item : drawables) {
            if (!drawSprite(renderer, item.texture, item.center, 255, item.angle, item.size, item.flip)) return fail("drawSprite drawable");
        }
        // render: grass always drew under the npc even when it stood lower on the map; grass strips and npcs now go in one list sorted by feet y, so whatever is lower on the map is drawn later and shows in front

        const float arrowBob = std::sin(static_cast<float>(SDL_GetTicks()) / 300.0f) * arrowBobHeight;
        for (const npc& unit : npcs) {
            if (!unit.isSelected()) continue;

            const SDL_FRect rect = npcRect(unit);
            const Vector2D<float> arrow = {unit.getPos().x, rect.y - arrowGap + arrowBob};
            if (!drawSprite(renderer, sprites.selectedArrow, arrow, 255, 0.0, arrowSize)) return fail("drawSprite selectedArrow");
        }
        // ui: selected npcs showed a green outline; they now show selectedArrow.png centered arrowGap px above the head, bobbing up and down by arrowBobHeight on a sine of the clock
        if (tempObject.isClicked() || objectTargeted) {
            const Vector2D<float> arrow = {tempObject.getPos().x, objectRect(tempObject).y - arrowGap + arrowBob};
            if (!drawSprite(renderer, sprites.selectedArrow, arrow, 255, 0.0, arrowSize)) return fail("drawSprite selectedArrow");
        }
        // ui: an object shows the arrow while it is clicked and while any npc is still walking to it (in place of the cross); the same bobbing arrow is drawn arrowGap px above the top of its sprite area
        for (size_t i = 0; i < items.size(); ++i) {
            if (!itemTargeted[i]) continue;
            const Vector2D<float> arrow = {items[i].getPos().x, objectRect(items[i]).y - arrowGap + arrowBob};
            if (!drawSprite(renderer, sprites.selectedArrow, arrow, 255, 0.0, arrowSize)) return fail("drawSprite selectedArrow");
        }
        // unit_item: a ground item an npc is walking to shows the bobbing arrow above it in place of the cross
        if (dragging && !drawOutline(renderer, boxBetween(dragStart, toMap(mouse)))) return fail("drawOutline");
        // render: draw order decides what covers what; background, then ground marks (footprints fading with age, cross at each moving npc's destination), then grass and npcs sorted by feet y, then selection arrows, drag box on top

        if (!SDL_RenderPresent(renderer)) return fail("SDL_RenderPresent");
    }

    SDL_DestroyTexture(sprites.npcUp);
    SDL_DestroyTexture(sprites.npcLeft);
    SDL_DestroyTexture(sprites.npcRight);
    SDL_DestroyTexture(sprites.footprint);
    SDL_DestroyTexture(sprites.cross);
    SDL_DestroyTexture(sprites.selectedArrow);
    SDL_DestroyTexture(tempObjectSprite);
    for (SDL_Texture* texture : itemSprites) SDL_DestroyTexture(texture);
    for (SDL_Texture* texture : grassSprites) SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
