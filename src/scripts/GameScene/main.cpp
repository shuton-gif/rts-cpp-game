#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "AI/agent.h"
#include "AI/script.h"
#include "Arrow/arrow.h"
#include "Collision/collision.h"
#include "Editor/editor.h"
#include "Fog/fog.h"
// ui_editor: npc scripts are edited in game; main.cpp owns one scriptEditor pop-up
#include "GameObject/gameObject.h"
#include "Grass/grass.h"
#include "Item/axe.h"
#include "Item/bow.h"
#include "Item/item.h"
#include "Map/chunk.h"
// map: a 40000 map cannot hold all its grass and trees; main.cpp owns one chunkStore that streams them in and out around the view and npcs
#include "Movement/arrive.h"
#include "NPC/npc.h"
// ai_script: npc behaviors moved out of main into text scripts; agent runs them, script loads them, and arrive holds the shared arrive helpers
#include "Pathfinding/path.h"
// pathfinding: npcs got stuck on trees and walls between them and their destination; main owns one navGrid and hands each npc a path around them
#include "Tree/tree.h"
#include "Vector2D.h"
// core: Vector2D is shared with npc.cpp; definition moved to src/scripts/Vector2D.h and included here
#include "Wall/wall.h"

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

const Vector2D<float> bgSize = {40000.0f, 40000.0f};
const float edgeMargin = 8.0f;

Vector2D<float> camera = {0.0f, 0.0f};
float cameraSpeed = 1.0f;

Vector2D<float> mouse = {0.0f, 0.0f};
// camera: sketch values made real; camera is the top-left of the view in map px, cameraSpeed is px per ms (1.0 = 1000 px/s)

const float spriteScale = 4.0f;
const float footprintGap = 24.0f;
const float footprintLife = 1000.0f;

const int playerCount = 8;
const int localPlayer = 0;
const Uint32 veilDiscovered = 128;
const Uint32 veilUnknown = 255;
// fog: the game has no teams yet, so each player is their own team and the screen shows P1's veil; never discovered cells are solid black and discovered ones 0.5 opacity
const SDL_Color bodyColor = {0x9b, 0xad, 0xb7, 255};
const SDL_Color playerColors[playerCount] = {
    {0xd0, 0x68, 0x68, 255},
    {0x68, 0x84, 0xd0, 255},
    {0xd0, 0xc0, 0x68, 255},
    {0xa0, 0x68, 0xd0, 255},
    {0xd0, 0x90, 0x68, 255},
    {0x68, 0xc0, 0xd0, 255},
    {0xd0, 0x68, 0xb0, 255},
    {0xe8, 0xe8, 0xe8, 255},
};
// unit: npcs show their owner; bodyColor is the gray body in the npc pngs, and playerColors[p] replaces it for player p (P1 red, P2 blue, then yellow, purple, orange, teal, pink, white), muted then made a bit brighter, up to 4v4

struct NpcSprites {
    SDL_Texture* up = nullptr;
    SDL_Texture* left = nullptr;
    SDL_Texture* right = nullptr;
};

struct Sprites {
    NpcSprites players[playerCount];
    SDL_Texture* footprint = nullptr;
    SDL_Texture* cross = nullptr;
    SDL_Texture* selectedArrow = nullptr;
};

const float arrowGap = 20.0f;
const float wallCountGap = 4.0f;
// economy-wall-group: the plank counter needs room above its post; it sits wallCountGap map px above the post height
const float arrowBobHeight = 4.0f;
const float arrowSize = 0.5f;
const Uint8 selectFillAlpha = 153;
const Uint8 hitboxAlpha = 179;
// test: dev hitbox view is red at 0.7 opacity
const float grassSize = 0.75f;
const float clumpMargin = 24.0f;
// map: clump counts and tries moved to Map/chunk.h, which makes grass per chunk; grassSize and clumpMargin stay as the clump look handed to the chunk store and used for culling
// ai_script: groundArriveRad and arriveTries were main's own; they moved to Movement/arrive.h with the arrive helpers that use them
const int renderMargin = 100;
const int tileRatioX = 10;
const int tileRatioY = 10;
// map: the map is laid out in tiles; one tile is tileRatioX x tileRatioY map px (10 x 10), and the selection box snaps to them
const float holdGap = 5.0f;
const float holdHeight = 8.0f;
// unit_item: a held item was too close and overlapped the npc, then 15 and 10 were too far so the gap is 5; its center bottom sits holdHeight map px above the npc's feet, and holdGap map px of space stays between the npc's side and the item's side
const float shoulderHeight = 24.0f;
// economy: a plank is carried on the shoulder; its center bottom sits shoulderHeight map px above the npc's feet, the top of the shoulder just under the eyes
const int plankCount = 3;
const float plankScatter = 40.0f;
const float plankScale = 3.0f;
// economy: planks were too small to see and click; a plank is plankScale times the sprite scale
const Vector2D<float> hpBarSize = {60.0f, 6.0f};
const float hpBarGap = 8.0f;
// economy: a felled tree drops plankCount planks within plankScatter map px, and the white hp bar is hpBarSize map px, hpBarGap below the tree's feet
const float hpBarShow = 3000.0f;
// combat: hp bars on every tree and post cluttered the map; a bar shows only for hpBarShow ms after the last damage
const float pathWidth = 4.0f;
// pathfinding: a 1 px path line was hard to see; it is pathWidth map px wide (3 px thicker)
const int treeTilesX = 30;
const int treeTilesY = 32;
const Vector2D<float> treeSize = {static_cast<float>(treeTilesX * tileRatioX), static_cast<float>(treeTilesY * tileRatioY)};
// economy: trees sit on the tile grid like walls; a tree covers treeTilesX x treeTilesY tiles (300 x 320 map px), the 15 x 16 sprite's shape, which also sets the arrive spread (rad 320), while its collider covers only the trunk (radius 50); arriveTries caps redraws of a spot that overlaps it
const Vector2D<float> treeSpots[] = {
    {250.0f, 0.0f},
    {650.0f, -250.0f},
    {-550.0f, -300.0f},
    {450.0f, 380.0f},
    {-650.0f, 300.0f},
};
// economy: one tree gave planks for only one wall; treeSpots are the feet of every tree as offsets from the map center, apart enough that their solid ovals and sprites do not cover each other, the npcs or the items
const int snapReach = 2;
const Uint8 planAlpha = 100;
// economy: walls are single posts, one tile each, and a new line joins the walls already there; a drag that starts or ends within snapReach tiles of a post or plan starts or ends on it; how close an npc must stand to haul or build is now buildReach in AI/agent.h; a plan and the placement ghost are drawn at planAlpha and a plan fills in to full while it is built

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

static SDL_FRect terminalButtonRect(const Vector2D<float> viewport)
{
    const float size = 16.0f * std::max(1.0f, std::round(3.0f * density));
    return {std::round(12.0f * density), std::round(viewport.y * 0.25f), size, size};
}
// ui-terminal: the Terminal needs a button at the screen's left; the 16 px icon scales by a whole 3 x density so its pixels stay sharp, its top is 25% down the screen, and it sits 12 points from the left edge, clear of the 8 point edge scroll

static SDL_Texture* loadRecolored(SDL_Renderer* renderer, const char* name, const SDL_Color from, const SDL_Color to)
{
    const char* base = SDL_GetBasePath();
    const std::string path = std::string(base ? base : "") + "assets/" + name;

    SDL_Surface* loaded = SDL_LoadPNG(path.c_str());
    if (!loaded) return nullptr;
    SDL_Surface* surface = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (!surface) return nullptr;

    if (!SDL_LockSurface(surface)) {
        SDL_DestroySurface(surface);
        return nullptr;
    }
    for (int y = 0; y < surface->h; ++y) {
        Uint8* row = static_cast<Uint8*>(surface->pixels) + y * surface->pitch;
        for (int x = 0; x < surface->w; ++x) {
            Uint8* px = row + x * 4;
            if (px[0] != from.r || px[1] != from.g || px[2] != from.b || px[3] != from.a) continue;
            px[0] = to.r;
            px[1] = to.g;
            px[2] = to.b;
        }
    }
    SDL_UnlockSurface(surface);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) return nullptr;

    if (!SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST)) {
        SDL_DestroyTexture(texture);
        return nullptr;
    }
    return texture;
}
// unit: one npc png serves every player; the png is converted to RGBA32 (bytes r, g, b, a), every opaque pixel exactly matching from (the gray body) becomes to (the player color), other pixels such as the eyes stay, and the texture is made sharp like loadSprite

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
    const NpcSprites& set = sprites.players[std::clamp(unit.getPlayer(), 0, playerCount - 1)];
    switch (unit.getFacing()) {
    case Facing::Up:
    case Facing::UpLeft:
    case Facing::UpRight:
        return set.up;
    case Facing::Left:
    case Facing::DownLeft:
        return set.left;
    case Facing::Right:
    case Facing::DownRight:
        return set.right;
    case Facing::Down:
        break;
    }
    return unit.isFacingLeft() ? set.left : set.right;
}
// unit: 3 sprites cover 8 facings; every up direction uses npc-u, left and down-left use npc-l, right and down-right use npc-r, straight down keeps the side last faced, all from the set in the npc's player color

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

static bool drawVeil(SDL_Renderer* renderer, SDL_Texture*& veil, const fogMap& fog)
{
    const Vector2D<float> cam = cameraPx();
    const float cell = static_cast<float>(fogCell);
    const int firstX = static_cast<int>(std::floor(cam.x / cell));
    const int firstY = static_cast<int>(std::floor(cam.y / cell));
    const int cols = static_cast<int>(std::ceil((cam.x + renderView.x) / cell)) - firstX;
    const int rows = static_cast<int>(std::ceil((cam.y + renderView.y) / cell)) - firstY;
    float textureW = 0.0f;
    float textureH = 0.0f;
    if (veil && !SDL_GetTextureSize(veil, &textureW, &textureH)) return false;
    if (!veil || textureW < static_cast<float>(cols) || textureH < static_cast<float>(rows)) {
        if (veil) SDL_DestroyTexture(veil);
        veil = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, cols, rows);
        if (!veil) return false;
        if (!SDL_SetTextureBlendMode(veil, SDL_BLENDMODE_BLEND)) return false;
        if (!SDL_SetTextureScaleMode(veil, SDL_SCALEMODE_NEAREST)) return false;
    }
    const SDL_Rect area = {0, 0, cols, rows};
    void* pixels = nullptr;
    int pitch = 0;
    if (!SDL_LockTexture(veil, &area, &pixels, &pitch)) return false;
    for (int y = 0; y < rows; ++y) {
        Uint32* line = reinterpret_cast<Uint32*>(static_cast<Uint8*>(pixels) + y * pitch);
        for (int x = 0; x < cols; ++x) {
            const FogState state = fog.cellState(firstX + x, firstY + y);
            const Uint32 alpha = state == FogState::Sight ? 0 : state == FogState::Discovered ? veilDiscovered : veilUnknown;
            line[x] = alpha << 24;
        }
    }
    SDL_UnlockTexture(veil);
    const SDL_FRect from = {0.0f, 0.0f, static_cast<float>(cols), static_cast<float>(rows)};
    const SDL_FRect to = {static_cast<float>(firstX) * cell - cam.x, static_cast<float>(firstY) * cell - cam.y, static_cast<float>(cols) * cell, static_cast<float>(rows) * cell};
    return SDL_RenderTexture(renderer, veil, &from, &to);
}
// fog: the view can hold tens of thousands of veil cells, too many for a rect each; each frame one texture pixel per visible cell gets its alpha, and the texture is drawn nearest-scaled so every cell is a hard-edged block, regrown when zooming out needs more cells

static bool drawHitbox(SDL_Renderer* renderer, const Collider& oval)
{
    const SDL_FRect bound = {oval.pos.x - oval.radius, oval.pos.y - oval.radius / 2.0f, oval.radius * 2.0f, oval.radius};
    if (!inRenderArea(bound)) return true;
    const Vector2D<float> cam = cameraPx();
    const SDL_FRect rect = {std::floor(bound.x) - cam.x, std::floor(bound.y) - cam.y, bound.w, bound.h};
    if (!SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND)) return false;
    if (!SDL_SetRenderDrawColor(renderer, 255, 0, 0, hitboxAlpha)) return false;
    return SDL_RenderFillRect(renderer, &rect);
}
// test: hitboxes are tuned by eye; an oval collider is shown as its bounding rect (2 x radius wide, radius high, centered on pos), filled red at hitboxAlpha, skipped outside the render area

static bool drawHpBar(SDL_Renderer* renderer, const Vector2D<float> feet, const float left, const float width = hpBarSize.x)
{
    const SDL_FRect bar = {std::floor(feet.x - width / 2.0f), std::floor(feet.y + hpBarGap), width, hpBarSize.y};
    if (!inRenderArea(bar)) return true;

    const Vector2D<float> cam = cameraPx();
    const SDL_FRect outline = {bar.x - cam.x, bar.y - cam.y, bar.w, bar.h};
    const SDL_FRect fill = {outline.x, outline.y, std::floor(bar.w * left), bar.h};
    if (!SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)) return false;
    if (!SDL_RenderFillRect(renderer, &fill)) return false;
    return SDL_RenderRect(renderer, &outline);
}
// economy: a tree's hp must be visible; a white bar hpBarSize map px sits hpBarGap below its feet, the 1 px outline is full hp and the white fill is hp / maxHp of it, skipped outside the render area
// economy: walls show hp and build progress with the same bar; the caller passes the feet and the filled part (0 to 1)

static bool drawWallCount(SDL_Renderer* renderer, const Vector2D<float> middle, const float height, const int delivered)
{
    const std::string text = std::to_string(delivered) + " / " + std::to_string(wallCost);
    const float glyph = static_cast<float>(SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE);
    const SDL_FRect area = {std::floor(middle.x - static_cast<float>(text.size()) * glyph / 2.0f), std::floor(middle.y - height - wallCountGap - glyph), static_cast<float>(text.size()) * glyph, glyph};
    if (!inRenderArea(area)) return true;

    const Vector2D<float> cam = cameraPx();
    if (!SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)) return false;
    return SDL_RenderDebugText(renderer, area.x - cam.x, area.y - cam.y, text.c_str());
}
// economy-wall-group: the planks in a plan must be visible to see npcs over-delivering; white `X / wallCost` debug text is centered wallCountGap map px above the post height over the group's middle, skipped outside the render area

static bool drawPathLine(SDL_Renderer* renderer, const std::vector<SDL_FPoint>& points)
{
    const float half = pathWidth / 2.0f;
    const SDL_FColor white = {1.0f, 1.0f, 1.0f, 1.0f};
    const int quad[6] = {0, 1, 2, 0, 2, 3};
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        const SDL_FPoint a = points[i];
        const SDL_FPoint b = points[i + 1];
        const float length = std::hypot(b.x - a.x, b.y - a.y);
        if (length <= 0.0f) continue;
        const float nx = -(b.y - a.y) / length * half;
        const float ny = (b.x - a.x) / length * half;
        const SDL_Vertex corners[4] = {
            {{a.x + nx, a.y + ny}, white, {0.0f, 0.0f}},
            {{b.x + nx, b.y + ny}, white, {0.0f, 0.0f}},
            {{b.x - nx, b.y - ny}, white, {0.0f, 0.0f}},
            {{a.x - nx, a.y - ny}, white, {0.0f, 0.0f}},
        };
        if (!SDL_RenderGeometry(renderer, nullptr, corners, 4, quad, 6)) return false;
    }
    if (!SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)) return false;
    for (const SDL_FPoint& joint : points) {
        const SDL_FRect square = {joint.x - half, joint.y - half, pathWidth, pathWidth};
        if (!SDL_RenderFillRect(renderer, &square)) return false;
    }
    return true;
}
// pathfinding: SDL lines are 1 px; each segment of the path (points already minus the camera) is a white quad pathWidth map px wide along its normal, and a pathWidth square on every point fills the gap where segments bend

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
    Uint8 alpha = 255;
};
// economy: wall plans are see-through; a drawable carries the alpha it is drawn with, opaque unless set after it is added
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

// ai_script: fuzzyArrive moved to Movement/arrive.h so agents share it; it now takes the map size to clamp to

static SDL_FRect objectRect(const GameObject& object)
{
    const Vector2D<float> pos = object.getPos();
    const Vector2D<float> size = object.getSize();
    return {pos.x - size.x / 2.0f, pos.y - size.y, size.x, size.y};
}
// core: a game object is clicked and gets its arrow by its sprite area; size is already map px, so the rect runs from the feet up by the height, centered on x

// ai_script: arriveAround moved to Movement/arrive.h so agents share it; it now takes the map size to clamp to

static bool handsFree(const std::vector<std::unique_ptr<item>>& items, const int id)
{
    return std::none_of(items.begin(), items.end(), [id](const std::unique_ptr<item>& thing) {
        return thing->getHolder() == id || thing->getFetcher() == id;
    });
}
// ai_script: an npc carries one item at a time; hands are free when no item is held by that npc id or reserved by its agent (fetcher), and hasMark moved to Movement/arrive.h

struct WallLook {
    SDL_Texture* posts[2] = {nullptr, nullptr};
    Vector2D<float> sizes[2] = {{0.0f, 0.0f}, {0.0f, 0.0f}};
    float drawSize = 1.0f;
};
const char* const postImages[2] = {"ww-single-3.png", "ww-single-4.png"};
// economy: a wall is one post, ww-single-3 (posts[0]) or ww-single-4 (posts[1]); both are drawn at drawSize so one sprite px is one tile wide, sizes are their map px sizes, and a wall's image names which one it is

struct PlanSpot {
    Vector2D<float> pos;
    int post;
};

static Vector2D<int> tileOf(const Vector2D<float> point)
{
    return {static_cast<int>(std::floor(point.x / static_cast<float>(tileRatioX))), static_cast<int>(std::floor(point.y / static_cast<float>(tileRatioY)))};
}

static Vector2D<float> tileFeet(const Vector2D<int> tile)
{
    return {static_cast<float>(tile.x * tileRatioX) + static_cast<float>(tileRatioX) / 2.0f, static_cast<float>((tile.y + 1) * tileRatioY)};
}
// economy: posts stand one per tile; tileOf is the tile a map point is in, and tileFeet is where a post on that tile stands, the middle of the tile's bottom edge

static int postOf(const wall& segment)
{
    return segment.getImage() == postImages[1] ? 1 : 0;
}

static int snapPost(const std::vector<wall>& walls, Vector2D<int>& tile)
{
    int nearest = -1;
    int best = 0;
    for (size_t i = 0; i < walls.size(); ++i) {
        if (walls[i].isDestroyed()) continue;
        const Vector2D<int> at = tileOf({walls[i].getPos().x, walls[i].getPos().y - 1.0f});
        const int distance = std::max(std::abs(at.x - tile.x), std::abs(at.y - tile.y));
        if (distance > snapReach || (nearest >= 0 && distance >= best)) continue;
        nearest = static_cast<int>(i);
        best = distance;
    }
    if (nearest >= 0) tile = tileOf({walls[nearest].getPos().x, walls[nearest].getPos().y - 1.0f});
    return nearest;
}
// economy: lines must connect to walls already there; the closest post or plan within snapReach tiles (counting diagonal steps as one) moves tile onto its own tile, and its index is returned, -1 when none is that close

static std::vector<PlanSpot> planLine(const Vector2D<float> from, const Vector2D<float> to, const std::vector<wall>& walls)
{
    Vector2D<int> tile = tileOf(from);
    Vector2D<int> end = tileOf(to);
    const int joined = snapPost(walls, tile);
    Vector2D<int> snappedEnd = end;
    if (snapPost(walls, snappedEnd) != joined || joined < 0) end = snappedEnd;
    // economy: a short drag near one post snapped both ends onto it and planned nothing; the end only snaps onto a different post than the start, so a line started on a post reaches the tile under the cursor
    const int firstPost = joined >= 0 ? postOf(walls[joined]) : 0;

    const int dx = std::abs(end.x - tile.x);
    const int dy = -std::abs(end.y - tile.y);
    const int stepX = tile.x < end.x ? 1 : -1;
    const int stepY = tile.y < end.y ? 1 : -1;
    int error = dx + dy;
    std::vector<PlanSpot> line;
    for (int i = 0;; ++i) {
        line.push_back({tileFeet(tile), (firstPost + i) % 2});
        if (tile == end) break;
        const int twice = 2 * error;
        if (twice >= dy) {
            error += dy;
            tile.x += stepX;
        }
        if (twice <= dx) {
            error += dx;
            tile.y += stepY;
        }
    }
    return line;
}
// economy: a drag plans a straight line of posts in any direction, like a pixel line; the start and end tiles snap onto nearby posts, the tiles between come from Bresenham's line so each post touches the next by a side or a corner, and posts alternate ww-single-3 and ww-single-4 from the start, continuing the alternation of the post the line starts on (that post itself is skipped later because its tile is taken)

static wall makeWall(const PlanSpot& spot, const WallLook& look, std::shared_ptr<WallGroup> group = std::make_shared<WallGroup>())
{
    return wall(spot.pos, look.sizes[spot.post], postImages[spot.post], std::move(group));
}
// economy-wall-group: every post belongs to a group; placement passes the line's current group, and a ghost post gets a throwaway one

static SDL_FRect wallFootprint(const wall& segment)
{
    const Vector2D<float> pos = segment.getPos();
    const float tileW = static_cast<float>(tileRatioX);
    const float tileH = static_cast<float>(tileRatioY);
    return {pos.x - tileW / 2.0f, pos.y - tileH, tileW, tileH};
}
// economy: walls block each other by the ground they stand on, not by their sprites; a post stands on its one tile

static bool addWallDrawables(std::vector<Drawable>& drawables, const WallLook& look, const PlanSpot& spot, const Uint8 alpha)
{
    if (!addDrawable(drawables, look.posts[spot.post], spot.pos, look.drawSize)) return false;
    drawables.back().alpha = alpha;
    return true;
}
// economy: a wall and its placement ghost are one post sprite at its feet, see-through by alpha

static bool canPlace(const wall& candidate, const std::vector<wall>& walls, const std::vector<tree>& trees)
{
    const SDL_FRect ground = wallFootprint(candidate);
    if (ground.x < 0.0f || ground.y < 0.0f || ground.x + ground.w > bgSize.x || ground.y + ground.h > bgSize.y) return false;
    const SDL_FRect rect = {ground.x + 1.0f, ground.y + 1.0f, ground.w - 2.0f, ground.h - 2.0f};
    for (const wall& other : walls) {
        if (!other.isDestroyed() && overlaps(rect, wallFootprint(other))) return false;
    }
    for (const tree& stand : trees) {
        if (stand.isFelled()) continue;
        Collider solid = stand.getCollider();
        Collider plan = candidate.getCollider();
        plan.fixed = false;
        if (separate(solid, plan)) return false;
    }
    return true;
}
// economy: a post cannot be planned on a tile another post or plan stands on, inside a standing tree's solid oval or off the map; the footprint is shrunk by 1 px so posts side by side only touch, and the tree test runs on copies

// ai_script: nearestPlan was only used by the wall job loop; the buildWall script finds the next plan with find(plan) instead

static SDL_FRect clumpRect(const GrassClump& clump)
{
    const float w = clump.size.x * spriteScale * grassSize + clumpMargin;
    const float h = clump.size.y * spriteScale * grassSize + clumpMargin;
    return {clump.pos.x - w / 2.0f, clump.pos.y - h / 2.0f, w, h};
}
// map: clumps are scattered per chunk by Map/chunk.h now, not over the whole map at start; clumpRect is left only for render culling, a clump's sprite bound in map px grown by clumpMargin

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
    for (int p = 0; p < playerCount; ++p) {
        NpcSprites& set = sprites.players[p];
        set.up = loadRecolored(renderer, "npc-u.png", bodyColor, playerColors[p]);
        set.left = loadRecolored(renderer, "npc-l.png", bodyColor, playerColors[p]);
        set.right = loadRecolored(renderer, "npc-r.png", bodyColor, playerColors[p]);
        if (!set.up || !set.left || !set.right) return fail("loadRecolored npc");
    }
    // unit: every player gets its own npc sprites; the 3 facing pngs are loaded once per palette color with the gray body swapped for it
    sprites.footprint = loadSprite(renderer, "foot-print.png");
    sprites.cross = loadSprite(renderer, "cross.png");
    sprites.selectedArrow = loadSprite(renderer, "selectedArrow.png");
    if (!sprites.footprint || !sprites.cross || !sprites.selectedArrow) {
        return fail("loadSprite");
    }

    const std::vector<SDL_Texture*> grassSprites = loadGrassSprites(renderer);
    if (grassSprites.empty()) return fail("loadGrassSprites");

    std::mt19937 rng(std::random_device{}());

    std::vector<Drawable> drawables;
    std::vector<Collider> colliders;

    std::vector<npc> npcs;
    npcs.emplace_back(0, Vector2D<float>{bgSize.x / 2.0f, bgSize.y / 2.0f}, 0);
    npcs.emplace_back(1, Vector2D<float>{bgSize.x / 2.0f - 60.0f, bgSize.y / 2.0f + 20.0f}, 0);
    npcs.emplace_back(2, Vector2D<float>{bgSize.x / 2.0f + 60.0f, bgSize.y / 2.0f + 20.0f}, 0);
    // unit: all three npcs belong to P1 for now, so they are red
    // unit: two more npcs are in the scene from the start; they stand 60 map px left and right of the first one, slightly lower

    std::vector<tree> trees;
    for (const Vector2D<float>& spot : treeSpots) {
        const float tileW = static_cast<float>(tileRatioX);
        const float tileH = static_cast<float>(tileRatioY);
        const float centerX = treeTilesX % 2 == 0 ? std::round((bgSize.x / 2.0f + spot.x) / tileW) * tileW : std::floor((bgSize.x / 2.0f + spot.x) / tileW) * tileW + tileW / 2.0f;
        const float feetY = std::round((bgSize.y / 2.0f + spot.y) / tileH) * tileH;
        trees.emplace_back(Vector2D<float>{centerX, feetY}, treeSize, "tree.png");
        // economy: a tree is snapped so its sides and feet fall on grid lines; with an even treeTilesX its center is on a grid line, with an odd one on a tile center like a wall
    }
    // economy: the single temporary tree became a list; one tree stands at each of treeSpots, all the same size, sharing one sprite
    std::uint32_t worldSeed = std::random_device{}();
    chunkStore chunks(worldSeed, ChunkLook{bgSize, treeSize, {static_cast<float>(tileRatioX), static_cast<float>(tileRatioY)}, spriteScale * grassSize, clumpMargin, static_cast<int>(grassSprites.size())});
    // map: a 40000 map cannot hold all its grass and trees, so nothing is scattered at start any more; the chunk store is made after the fixed treeSpots trees so they are never erased, and it regrows the same grass and trees per chunk from worldSeed as the view and npcs come near
    SDL_Texture* treeSprite = loadSprite(renderer, "tree.png");
    if (!treeSprite) return fail("loadSprite tree");
    float treeW = 0.0f;
    float treeH = 0.0f;
    if (!SDL_GetTextureSize(treeSprite, &treeW, &treeH)) return fail("SDL_GetTextureSize tree");
    const float treeDrawSize = treeSize.x / (treeW * spriteScale);
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
    std::vector<arrow> arrows;
    // combat: arrows shot by bows; declared before items so it outlives the bows that shoot into it
    std::vector<std::unique_ptr<item>> items;
    std::vector<SDL_Texture*> itemSprites;
    std::vector<SDL_Texture*> itemTextures;
    for (const ItemSpawn& spawn : itemSpawns) {
        SDL_Texture* texture = loadSprite(renderer, spawn.image);
        if (!texture) return fail("loadSprite item");
        itemTextures.push_back(texture);
        float w = 0.0f;
        float h = 0.0f;
        if (!SDL_GetTextureSize(texture, &w, &h)) return fail("SDL_GetTextureSize item");
        const Vector2D<float> pos = {bgSize.x / 2.0f + spawn.offset.x, bgSize.y / 2.0f + spawn.offset.y};
        const Vector2D<float> size = {w * spriteScale, h * spriteScale};
        if (std::string(spawn.image) == "axe.png") {
            items.push_back(std::make_unique<axe>(pos, size, spawn.image));
        } else if (std::string(spawn.image) == "bow.png") {
            items.push_back(std::make_unique<bow>(pos, size, spawn.image, arrows));
            // combat: the bow spawn is a `bow` so using it shoots into arrows
        } else {
            items.push_back(std::make_unique<item>(pos, size, spawn.image));
        }
        itemSprites.push_back(texture);
    }
    // unit_item: an axe, a sword and a shield lie on the ground left of and below the npcs to try picking up; each spawn is an image and an offset from the map center, its size is the png size at spriteScale, and itemSprites[i] is the sprite of items[i]
    // economy: the axe must run its own use, so items are held by pointer and the axe spawn is an `axe`; itemSprites[i] only points at a texture, itemTextures owns them and is destroyed once at exit

    SDL_Texture* plankSprite = loadSprite(renderer, "plank.png");
    if (!plankSprite) return fail("loadSprite plank");
    itemTextures.push_back(plankSprite);
    float plankW = 0.0f;
    float plankH = 0.0f;
    if (!SDL_GetTextureSize(plankSprite, &plankW, &plankH)) return fail("SDL_GetTextureSize plank");
    std::vector<bool> planksDropped(trees.size(), false);
    // economy: every tree drops its planks once; planksDropped[i] is set when trees[i] has dropped them
    std::vector<int> treeLastHp;
    for (const tree& stand : trees) treeLastHp.push_back(stand.getHp());
    std::vector<float> treeHitAge(trees.size(), hpBarShow);
    std::vector<int> wallLastHp;
    std::vector<float> wallHitAge;
    // combat: hp bars show only after damage; lastHp is the hp seen last frame and hitAge the ms since it last dropped, starting hidden at hpBarShow

    WallLook look;
    for (int i = 0; i < 2; ++i) {
        look.posts[i] = loadSprite(renderer, postImages[i]);
        if (!look.posts[i]) return fail("loadSprite post");
        itemTextures.push_back(look.posts[i]);
        float postW = 0.0f;
        float postH = 0.0f;
        if (!SDL_GetTextureSize(look.posts[i], &postW, &postH)) return fail("SDL_GetTextureSize post");
        look.drawSize = static_cast<float>(tileRatioX) / (postW * spriteScale);
        look.sizes[i] = {postW * spriteScale * look.drawSize, postH * spriteScale * look.drawSize};
    }
    // economy: walls are the two post sprites, scaled so a post is one tile wide (10 x 50 and 10 x 60 map px), owned by itemTextures
    std::vector<wall> walls;
    bool placing = false;
    bool showSolid = false;
    bool showChop = false;
    bool showVeil = true;
    SDL_Texture* veil = nullptr;
    // test: dev views, U shows every solid oval and I every chop hitbox
    // economy: walls are planned by the player and built by npcs; a segment is a horizontal wall or a vertical column of posts (see WallLook), walls keeps plans and built posts by index, npcs work on one through their agent's buildWall script, and placing is on between B and placing a row

    SDL_Texture* arrowSprite = loadSprite(renderer, "arrow.png");
    if (!arrowSprite) return fail("loadSprite arrow");
    itemTextures.push_back(arrowSprite);
    // combat: every arrow shares one sprite, owned by itemTextures like the plank
    // economy: a felled tree drops planks; the plank sprite is loaded once and shared by every plank, and planksDropped makes the drop happen only once

    std::vector<Vector2D<float>> lastFootprint;
    for (const npc& unit : npcs) lastFootprint.push_back(unit.getPos());
    std::vector<Footprint> footprints;

    bool dragging = false;
    Vector2D<float> dragStart = {0.0f, 0.0f};
    // unit: the first npc starts at the middle of the map, which is the middle of the screen because the camera starts centered; npcs sit in a vector so more can be added

    std::map<std::string, Script> scripts;
    const char* scriptBase = SDL_GetBasePath();
    const std::string scriptDir = std::string(scriptBase ? scriptBase : "") + "assets/scripts";
    for (const std::string& error : loadScripts(scriptDir, scripts)) SDL_Log("script: %s", error.c_str());
    scriptEditor editor;
    const std::string scriptSource = std::string(RTS_SOURCE_DIR) + "/assets/scripts";
    // ui_editor: npc scripts are edited in game; the editor saves to the repo's assets/scripts (scriptSource) and the build copy (scriptDir) that the game loads
    const auto openEditor = [&]() {
        std::vector<std::string> names;
        for (const auto& [name, script] : scripts) names.push_back(name);
        editor.open(window, names, scriptSource, scriptDir);
    };
    SDL_Texture* terminalIcon = loadSprite(renderer, "ui/icon/terminal-icon.png");
    if (!terminalIcon) return fail("loadSprite terminal icon");
    // ui-terminal: Shift+1 and the new button must open the Terminal the same way; openEditor gives it one tab per loaded script, and the button's icon loads with the other sprites
    std::vector<agent> agents;
    for (const npc& unit : npcs) agents.emplace_back(unit.id);
    std::vector<item*> consumed;
    std::vector<fogMap> fogs;
    for (int p = 0; p < playerCount; ++p) fogs.emplace_back(bgSize);
    const fogMap& localFog = fogs[localPlayer];
    auto inSight = [&](const Vector2D<float> point) { return !showVeil || localFog.state(point) == FogState::Sight; };
    // fog: each team keeps its own sight and snapshot; inSight is whether the screen shows the live state at a point, always true while the V dev key has the veil off
    World world{npcs, items, trees, walls, rng, bgSize, 0.0f, consumed, fogs};
    // ai_script: npc behaviors are text scripts so they change without recompiling; every .txt in assets/scripts next to the executable is parsed by name once here (errors logged), agents[id] runs npc id's behavior, and world hands the agents main's lists, the frame time and consumed, where delivered planks wait to be erased
    navGrid nav;
    std::vector<Collider> solids;
    size_t solidCount = SIZE_MAX;
    // pathfinding: the grid must follow trees being felled and walls being built; solids holds this frame's standing trees and built walls, and solidCount is how many the grid was last built from, SIZE_MAX so the first frame builds it

    IntervalTimer timer;
    bool mouseSeen = false;
    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (editor.isOpen() && editor.handleEvent(event, density)) {
                if (event.type == SDL_EVENT_MOUSE_MOTION) mouse = toPixels(event.motion.x, event.motion.y);
                continue;
            }
            // ui_editor: npc scripts are edited in game; while open the editor takes every key, text input and mouse event inside its panel and the game skips them, quit and window events always reach the game, and a consumed motion still moves the cursor used by edge scroll
            switch (event.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) {
                    if (placing) {
                        placing = false;
                        dragging = false;
                    } else {
                        running = false;
                    }
                }
                if (event.key.key == SDLK_U) showSolid = !showSolid;
                if (event.key.key == SDLK_I) showChop = !showChop;
                // test: U and I toggle the dev hitbox views
                if (event.key.key == SDLK_V) showVeil = !showVeil;
                // fog: V shows the whole live map for testing; only drawing and clicks change, while sight, snapshots and find keep working
                if (event.key.key == SDLK_F5) {
                    for (const std::string& error : loadScripts(scriptDir, scripts)) SDL_Log("script: %s", error.c_str());
                }
                // ai_script: scripts are edited while the game runs; F5 parses assets/scripts again into the same map, a broken file keeps its old version and its error is logged
                if (event.key.scancode == SDL_SCANCODE_1 && (event.key.mod & SDL_KMOD_SHIFT) != 0 && !editor.isOpen()) openEditor();
                // ui_editor: npc scripts are edited in game; Shift+1 (by scancode, since SDL3 keycodes follow shift) opens the editor with one tab per loaded script, and Esc inside it closes it before the game sees the key
                if (event.key.key == SDLK_B) {
                    placing = !placing;
                    dragging = false;
                }
                // economy: B starts or stops wall placement and drops a selection drag in progress; Esc cancels placement first and only quits when not placing
                if (event.key.key == SDLK_Q) {
                    for (const std::unique_ptr<item>& thing : items) {
                        if (!thing->isHeld() || !npcs[thing->getHolder()].isSelected()) continue;
                        thing->drop({thing->getPos().x, npcs[thing->getHolder()].getPos().y});
                    }
                }
                // unit_item: Q makes every selected npc drop the item it holds; the item lands on the ground where it was held, at the npc's feet line
                if (event.key.key == SDLK_E) {
                    for (npc& unit : npcs) {
                        if (unit.isSelected()) unit.setAim(toMap(mouse));
                    }
                    for (const std::unique_ptr<item>& thing : items) {
                        if (!thing->isHeld() || !npcs[thing->getHolder()].isSelected()) continue;
                        if (thing->startUse() && !thing->swings()) thing->use(npcs[thing->getHolder()], nullptr);
                    }
                }
                // combat: E aims every selected npc at the cursor first; an item that does not swing (the bow) is used right away when its use starts, and useDuration then runs as a cooldown in which E is ignored
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
                    const SDL_FRect button = terminalButtonRect(viewport);
                    if (mouse.x >= button.x && mouse.x < button.x + button.w && mouse.y >= button.y && mouse.y < button.y + button.h) {
                        if (!editor.isOpen()) openEditor();
                        break;
                    }
                    // ui-terminal: a click on the button must open the Terminal and not select on the map; it opens it if closed, and either way the game does not see the click
                    dragStart = toMap(mouse);
                    dragging = true;
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    if (placing) {
                        placing = false;
                        dragging = false;
                        break;
                    }
                    // economy: right click while placing cancels it and gives no order
                    const bool shift = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
                    const Vector2D<float> clicked = toMap(mouse);
                    const auto selected = std::count_if(npcs.begin(), npcs.end(), [](const npc& unit) {
                        return unit.isSelected();
                    });
                    const SDL_FRect point = {clicked.x, clicked.y, 0.0f, 0.0f};
                    int picked = -1;
                    for (size_t i = 0; i < items.size(); ++i) {
                        if (items[i]->isHeld() || !inSight(items[i]->getPos()) || !overlaps(point, objectRect(*items[i]))) continue;
                        picked = static_cast<int>(i);
                        break;
                    }
                    int fetcherId = -1;
                    if (picked >= 0) {
                        items[picked]->setFetcher(-1);
                        for (const npc& unit : npcs) {
                            if (!unit.isSelected() || !handsFree(items, unit.id)) continue;
                            fetcherId = unit.id;
                            break;
                        }
                        items[picked]->setFetcher(fetcherId);
                        if (fetcherId < 0) picked = -1;
                    }
                    // unit_item: right click on a ground item sends one npc to pick it up; the first selected npc with free hands becomes its fetcher, and with no free npc the click counts as a ground click

                    int planned = -1;
                    for (size_t i = 0; picked < 0 && i < walls.size(); ++i) {
                        if (walls[i].isBuilt() || walls[i].isDestroyed() || !inSight(walls[i].getPos()) || !overlaps(point, objectRect(walls[i]))) continue;
                        planned = static_cast<int>(i);
                        break;
                    }
                    // ai_script: right click on a wall plan gives every selected npc the buildWall script on it; any other right click order replaces it
                    int aimed = -1;
                    for (size_t i = 0; picked < 0 && planned < 0 && i < trees.size(); ++i) {
                        if (trees[i].isFelled() || !inSight(trees[i].getPos()) || !overlaps(point, objectRect(trees[i]))) continue;
                        aimed = static_cast<int>(i);
                        break;
                    }
                    // economy: a felled tree is gone, so a right click where it stood is a ground click; with several trees the first one under the click is the target
                    Slot remembered;
                    for (const Seen& entry : localFog.getSeen()) {
                        if (!showVeil || picked >= 0 || planned >= 0 || aimed >= 0) break;
                        if (localFog.state(entry.pos) != FogState::Discovered || (entry.kind == Seen::Kind::Wall && entry.built)) continue;
                        const SDL_FRect shown = entry.kind == Seen::Kind::Wall ? objectRect(walls[entry.index]) : SDL_FRect{entry.pos.x - treeSize.x / 2.0f, entry.pos.y - treeSize.y, treeSize.x, treeSize.y};
                        if (!overlaps(point, shown)) continue;
                        remembered.kind = Slot::Kind::Remembered;
                        remembered.seen = entry.kind;
                        remembered.point = entry.pos;
                        break;
                    }
                    // fog: clicks hit what the screen shows; hidden things under the cursor count as ground, and a tree or plan shown by the snapshot becomes a remembered target
                    for (npc& unit : npcs) {
                        if (!unit.isSelected()) continue;
                        agent& brain = agents[unit.id];
                        if (planned >= 0) {
                            Slot site;
                            site.kind = Slot::Kind::Wall;
                            site.index = planned;
                            brain.start("buildWall", {{"target", site}, {"plan", site}}, world);
                            continue;
                        }
                        if (unit.id == fetcherId) {
                            Slot thing;
                            thing.kind = Slot::Kind::Item;
                            thing.thing = items[picked].get();
                            const std::string& image = items[picked]->getImage();
                            const std::string kind = image.substr(0, image.rfind(".png"));
                            brain.start("pickup", {{"target", thing}, {kind, thing}}, world);
                            continue;
                        }
                        if (aimed >= 0) {
                            Slot stand;
                            stand.kind = Slot::Kind::Tree;
                            stand.index = aimed;
                            brain.start("gatherWood", {{"target", stand}, {"tree", stand}}, world);
                            continue;
                        }
                        if (remembered.kind == Slot::Kind::Remembered) {
                            const bool plan = remembered.seen == Seen::Kind::Wall;
                            if (plan) brain.start("buildWall", {{"target", remembered}, {"plan", remembered}}, world);
                            else brain.start("gatherWood", {{"target", remembered}, {"tree", remembered}}, world);
                            continue;
                        }
                        const Vector2D<float> spot = selected > 1 ? fuzzyArrive(rng, clicked, bgSize, groundArriveRad) : clicked;
                        if (shift) {
                            brain.stop(world);
                            unit.addDestination(spot, clicked);
                            continue;
                        }
                        Slot ground;
                        ground.kind = Slot::Kind::Point;
                        ground.point = spot;
                        ground.mark = clicked;
                        brain.start("moveTo", {{"target", ground}}, world);
                    }
                    // ai_script: a right click order is a script run by the npc's agent; a plan starts buildWall, the fetcher starts pickup with the item under "target" and its kind (image without .png), a tree starts gatherWood, and a ground click starts moveTo to a fuzzyArrive spot when several are selected (the clicked point when one), while shift stops the agent and queues the spot as a plain destination
                    // ui: an object target shows the arrow on the object instead of a cross; the agent's moveTo uses the object's pos as mark, which the draw step recognises, while a ground click keeps the clicked point as mark
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                mouse = toPixels(event.button.x, event.button.y);
                if (event.button.button == SDL_BUTTON_LEFT && dragging && placing) {
                    std::shared_ptr<WallGroup> group = std::make_shared<WallGroup>();
                    for (const PlanSpot& spot : planLine(dragStart, toMap(mouse), walls)) {
                        const wall candidate = makeWall(spot, look, group);
                        if (!canPlace(candidate, walls, trees)) continue;
                        walls.push_back(candidate);
                        walls.back().join();
                        if (group->posts == wallGroupSize) group = std::make_shared<WallGroup>();
                    }
                    placing = false;
                    dragging = false;
                }
                // economy-wall-group: one drag splits into groups of wallGroupSize posts; releasing plans a row from the drag start toward the cursor, skipping spots that are taken without using up a group slot, starts a new group each time one fills, and ends placement
                if (event.button.button == SDL_BUTTON_LEFT && dragging) {
                    const SDL_FRect box = boxBetween(dragStart, toMap(mouse));
                    for (npc& unit : npcs) unit.setSelected(inSight(unit.getPos()) && overlaps(box, npcRect(unit)));
                    for (tree& stand : trees) {
                        const bool objectHit = !stand.isFelled() && inSight(stand.getPos()) && overlaps(box, objectRect(stand));
                        if (objectHit && !stand.isClicked()) stand.onClick();
                        stand.setClicked(objectHit);
                    }
                    // economy: a felled tree can no longer be selected; every tree is checked against the box
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
        if (editor.takeSaved()) {
            for (const std::string& error : loadScripts(scriptDir, scripts)) SDL_Log("script: %s", error.c_str());
        }
        // ui_editor: npc scripts are edited in game; a save in the editor reloads assets/scripts like F5 once, a broken file keeps its old version

        const float deltaTime = timer.tick();
        world.deltaTime = deltaTime;
        // ai_script: skills that take time (build, use) need this frame's length; world carries it to the agents
        if (mouseSeen) moveCamera(viewport, deltaTime);

        for (Footprint& print : footprints) print.age += deltaTime;
        std::erase_if(footprints, [](const Footprint& print) { return print.age >= footprintLife; });

        const Vector2D<float> viewMin = cameraPx();
        const Vector2D<float> viewSpan = viewSize(viewport);
        std::vector<Vector2D<float>> npcFeet;
        for (const npc& unit : npcs) npcFeet.push_back(unit.getPos());
        const std::vector<int> remap = chunks.update(viewMin, {viewMin.x + viewSpan.x, viewMin.y + viewSpan.y}, npcFeet, viewField, trees, walls);
        if (!remap.empty()) {
            std::vector<bool> keptDropped;
            std::vector<int> keptLastHp;
            std::vector<float> keptHitAge;
            for (size_t t = 0; t < remap.size(); ++t) {
                if (remap[t] < 0) continue;
                keptDropped.push_back(planksDropped[t]);
                keptLastHp.push_back(treeLastHp[t]);
                keptHitAge.push_back(treeHitAge[t]);
            }
            planksDropped = std::move(keptDropped);
            treeLastHp = std::move(keptLastHp);
            treeHitAge = std::move(keptHitAge);
            for (agent& brain : agents) brain.remapTrees(remap);
        }
        for (size_t t = planksDropped.size(); t < trees.size(); ++t) {
            planksDropped.push_back(trees[t].isFelled());
            treeLastHp.push_back(trees[t].getHp());
            treeHitAge.push_back(hpBarShow);
        }
        // map: only chunks near the view or an npc may hold grass and trees, and this runs after the camera moved but before solids, agents or any tree array read tree indexes this frame; the store frees far chunks and loads near ones, an erase shifts planksDropped, treeLastHp, treeHitAge and every agent's tree slots to the new indexes, and each newly loaded tree gets entries with planks already dropped if it comes back felled and its hp bar hidden

        solids.clear();
        for (const tree& stand : trees) {
            if (!stand.isFelled()) solids.push_back(stand.getCollider());
        }
        for (const wall& segment : walls) {
            if (segment.isBuilt()) solids.push_back(segment.getCollider());
        }
        if (solids.size() != solidCount) {
            solidCount = solids.size();
            for (npc& unit : npcs) {
                if (unit.isMoving()) unit.replan();
            }
        }
        std::vector<Vector2D<float>> before;
        for (npc& unit : npcs) {
            if (unit.needsRoute()) {
                const navWindow area = navWindowFor(unit.getPos(), unit.getTargetedPos(), bgSize);
                nav.build(solids, area.origin, area.size);
                unit.setRoute(nav.findPath(unit.getPos(), unit.getTargetedPos()));
            }
            before.push_back(unit.getPos());
        }
        // pathfinding: a grid over the whole 40000 map no longer fits in memory; every moving npc replans when the count of standing trees and built walls changes (felled, built, or streamed in or out), and each npc without a path gets the grid built over just the window of its trip from this frame's solids, then a path around them, while before keeps each npc's position for the stuck check
        for (npc& unit : npcs) unit.update(deltaTime);

        colliders.clear();
        for (const npc& unit : npcs) colliders.push_back(unit.getCollider());
        for (const tree& stand : trees) {
            if (!stand.isFelled()) colliders.push_back(stand.getCollider());
        }
        for (const wall& segment : walls) {
            if (segment.isBuilt()) colliders.push_back(segment.getCollider());
        }
        // economy: every standing tree and every built wall is solid, after the npcs; a plan and a destroyed wall are not
        // core: npcs must not walk through the object; its fixed collider goes after the npcs, so colliders[i] still matches npcs[i] and only npcs are pushed, and a felled tree is no longer solid
        resolveCollisions(colliders);
        for (size_t i = 0; i < npcs.size(); ++i) {
            const Vector2D<float> pushed = {
                std::clamp(colliders[i].pos.x, 0.0f, bgSize.x),
                std::clamp(colliders[i].pos.y, 0.0f, bgSize.y),
            };
            if (pushed != npcs[i].getPos()) npcs[i].pushTo(pushed);
        }
        // movement: npcs walked through each other; after all have moved, one collider per npc goes through resolveCollisions and every npc that was pushed takes its new position, kept inside the map. Other solid objects join by adding their collider to the same list
        for (size_t i = 0; i < npcs.size(); ++i) npcs[i].trackProgress(before[i], deltaTime);
        // pathfinding: an npc pushed back by others or a changed map can stall on its path; trackProgress compares the position after push-back with before, and after 500 ms of barely moving it drops the path so needsRoute gets a new one next frame

        for (size_t i = 0; i < npcs.size(); ++i) {
            const Vector2D<float> pos = npcs[i].getPos();
            if (std::hypot(pos.x - lastFootprint[i].x, pos.y - lastFootprint[i].y) >= footprintGap) {
                footprints.push_back({pos, 0.0f, facingAngle(npcs[i].getFacing())});
                lastFootprint[i] = pos;
            }
        }
        // movement: footprints follow the npc and fade after 1000 ms; each npc drops one at its feet every footprintGap px walked, every footprint ages by deltaTime and is erased at footprintLife

        for (const std::unique_ptr<item>& owned : items) {
            item& thing = *owned;
            // ai_script: picking up a fetched item moved from here to the agent's pickup skill
            if (!thing.isHeld()) continue;
            npc& holder = npcs[thing.getHolder()];
            if (thing.updateUse(deltaTime) && thing.swings()) {
                GameObject* target = nullptr;
                for (tree& stand : trees) {
                    if (stand.isFelled()) continue;
                    Collider hitbox = stand.getChopHitbox();
                    Collider swing = holder.getCollider();
                    if (!separate(hitbox, swing)) continue;
                    target = &stand;
                    break;
                }
                for (size_t w = 0; !target && w < walls.size(); ++w) {
                    if (!walls[w].isBuilt()) continue;
                    Collider hitbox = walls[w].getHitbox();
                    Collider swing = holder.getCollider();
                    if (separate(hitbox, swing)) target = &walls[w];
                }
                // combat: a swing hits the first tree in reach, else the first built wall whose hitbox the holder touches
                thing.use(holder, target);
            }
            // animation: the swing plays over the item's useDuration and use runs once on the frame it ends
            // combat: an item that does not swing was already used at the start, so its end only finishes the cooldown
            // economy: a swing hits the tree when the holder's feet oval touches the tree's chop hitbox, which is chopScale times its solid oval, so an npc pushed against the trunk always hits; otherwise the target is nullptr and the swing misses
            const float side = holder.isFacingLeft() ? -1.0f : 1.0f;
            const float reach = holder.getSize().x * spriteScale / 2.0f + holdGap + thing.getSize().x / 2.0f;
            if (thing.getImage() == "plank.png") {
                thing.moveTo({holder.getPos().x, holder.getPos().y - shoulderHeight + thing.getUseOffset()});
            } else {
                thing.moveTo({holder.getPos().x + side * reach, holder.getPos().y - holdHeight + thing.getUseOffset()});
            }
            // economy: a held plank rests across the holder's shoulder instead of in the hand; it is centered on the npc at shoulderHeight, on both sides whichever way the npc faces
            // animation: a swinging item is shifted up or down by getUseOffset from its held spot, 0 when idle
        }
        // unit_item: a held item stays at its holder's side after the npcs moved; its center is half the npc width + holdGap + half the item width from the npc's center, on the right, or on the left when the npc faces left, and up or down keep the last side because isFacingLeft only changes on a left or right step

        std::vector<std::vector<Vector2D<float>>> teamFeet(playerCount);
        for (const npc& unit : npcs) teamFeet[std::clamp(unit.getPlayer(), 0, playerCount - 1)].push_back(unit.getPos());
        for (int p = 0; p < playerCount; ++p) fogs[p].update(teamFeet[p], trees, walls);
        // fog: scripts and drawing need this frame's sight; every team's map updates after the npcs moved and before the agents run

        consumed.clear();
        for (agent& brain : agents) brain.run(world, scripts);
        for (size_t i = items.size(); i-- > 0;) {
            item* const thing = items[i].get();
            if (std::find(consumed.begin(), consumed.end(), thing) == consumed.end()) continue;
            for (agent& brain : agents) brain.forget(thing);
            items.erase(items.begin() + static_cast<std::ptrdiff_t>(i));
            itemSprites.erase(itemSprites.begin() + static_cast<std::ptrdiff_t>(i));
        }
        // ai_script: npcs haul, build, chop and fetch by their scripts instead of a hard-coded wall job; every agent plays one frame of its script, then each plank its use skill put in consumed is forgotten by every agent and erased with its sprite, from the back so indexes stay valid

        for (arrow& shot : arrows) {
            shot.update(deltaTime);
            if (!shot.isFlying()) continue;
            const SDL_FRect point = {shot.getPos().x, shot.getPos().y, 0.0f, 0.0f};
            for (tree& stand : trees) {
                if (stand.isFelled() || !overlaps(point, objectRect(stand))) continue;
                stand.chop(arrowDamage);
                shot.spend();
                break;
            }
            for (size_t w = 0; shot.isFlying() && w < walls.size(); ++w) {
                if (!walls[w].isBuilt() || !overlaps(point, objectRect(walls[w]))) continue;
                walls[w].damage(arrowDamage);
                shot.spend();
            }
        }
        // combat: arrows damage trees and built walls; a flying arrow whose center is over a standing tree's or a built wall's sprite area takes arrowDamage hp off the first one and is spent, so it is erased below, and a felled tree drops planks like after an axe chop
        std::erase_if(arrows, [](const arrow& shot) { return shot.isGone(); });
        // combat: arrows fly after the bows fired this frame; each moves and ages by deltaTime, and one is erased arrowLife ms after its shot

        for (size_t t = 0; t < trees.size(); ++t) {
            if (!trees[t].isFelled() || planksDropped[t]) continue;
            for (int i = 0; i < plankCount; ++i) {
                items.push_back(std::make_unique<item>(fuzzyArrive(rng, trees[t].getPos(), bgSize, plankScatter), Vector2D<float>{plankW * spriteScale * plankScale, plankH * spriteScale * plankScale}, "plank.png"));
                itemSprites.push_back(plankSprite);
            }
            planksDropped[t] = true;
        }
        // economy: each felled tree drops its planks once, around where it stood
        // economy: the tree drops wood when felled; once, plankCount planks are added as ground items at random spots within plankScatter map px of where it stood, after the item loop so the list is not changed while it is walked

        for (size_t t = 0; t < trees.size(); ++t) {
            treeHitAge[t] = trees[t].getHp() < treeLastHp[t] ? 0.0f : treeHitAge[t] + deltaTime;
            treeLastHp[t] = trees[t].getHp();
        }
        wallLastHp.resize(walls.size(), wallHp);
        wallHitAge.resize(walls.size(), hpBarShow);
        for (size_t w = 0; w < walls.size(); ++w) {
            wallHitAge[w] = walls[w].getHp() < wallLastHp[w] ? 0.0f : wallHitAge[w] + deltaTime;
            wallLastHp[w] = walls[w].getHp();
        }
        // combat: damage from axes and arrows is all done by here; a drop in hp since last frame restarts the bar timer, otherwise it ages by deltaTime, and walls planned this frame start hidden

        if (!SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)) return fail("SDL_SetRenderDrawColor");
        if (!SDL_RenderClear(renderer)) return fail("SDL_RenderClear");
        if (!SDL_SetRenderScale(renderer, zoom, zoom)) return fail("SDL_SetRenderScale");
        // camera: zoom must scale everything drawn; the renderer multiplies all coordinates by zoom, so draw code keeps working in map px
        renderView = viewSize(viewport);
        if (!drawBackground(renderer)) return fail("drawBackground");

        for (const Footprint& print : footprints) {
            if (!inSight(print.pos)) continue;
            const Uint8 alpha = static_cast<Uint8>(255.0f * (1.0f - print.age / footprintLife));
            if (!drawSprite(renderer, sprites.footprint, print.pos, alpha, print.angle)) return fail("drawSprite footprint");
        }
        std::vector<bool> treeTargeted(trees.size(), false);
        std::vector<bool> wallTargeted(walls.size(), false);
        std::vector<bool> itemTargeted(items.size(), false);
        for (const npc& unit : npcs) {
            if (!unit.isMoving()) continue;
            std::vector<Vector2D<float>> marks = unit.getQueuedMark();
            marks.push_back(unit.getTargetedMark());
            for (const Vector2D<float>& mark : marks) {
                bool onObject = false;
                for (size_t t = 0; t < trees.size(); ++t) {
                    if (trees[t].isFelled() || mark != trees[t].getPos()) continue;
                    treeTargeted[t] = true;
                    onObject = true;
                }
                for (size_t w = 0; w < walls.size(); ++w) {
                    if (walls[w].isDestroyed() || mark != walls[w].getPos()) continue;
                    wallTargeted[w] = true;
                    onObject = true;
                }
                if (onObject) continue;
                // economy: a mark on any standing tree or on a wall flags it for the arrow instead of a cross
                bool onItem = false;
                for (size_t i = 0; i < items.size(); ++i) {
                    if (items[i]->isHeld() || mark != items[i]->getPos()) continue;
                    itemTargeted[i] = true;
                    onItem = true;
                }
                if (onItem) continue;
                if (!drawSprite(renderer, sprites.cross, mark, 255)) return fail("drawSprite cross");
            }
        }
        // ui: npcs sent together showed one cross each at their spread-out points; the cross is drawn at each destination's mark, the clicked point they share, so one click shows one cross until the last npc heading there arrives, and a mark on the object or on a ground item draws no cross but flags that object or item for the arrow
        const Vector2D<float> pathCam = cameraPx();
        std::vector<SDL_FPoint> pathPoints;
        for (const npc& unit : npcs) {
            if (!unit.isMoving()) continue;
            pathPoints.clear();
            pathPoints.push_back({unit.getPos().x - pathCam.x, unit.getPos().y - pathCam.y});
            for (const Vector2D<float>& point : unit.getRoute()) pathPoints.push_back({point.x - pathCam.x, point.y - pathCam.y});
            pathPoints.push_back({unit.getTargetedPos().x - pathCam.x, unit.getTargetedPos().y - pathCam.y});
            if (!drawPathLine(renderer, pathPoints)) return fail("drawPathLine");
        }
        // pathfinding: the way an npc takes around trees and walls must be visible; every moving npc draws a white line pathWidth map px wide on the ground from its feet through each waypoint to targetedPos, in map px minus the camera like the sprites, and later rects set their own color

        drawables.clear();
        for (const GrassClump* clump : chunks.clumps()) {
            if (!inRenderArea(clumpRect(*clump))) continue;
            if (!addGrassClump(drawables, grassSprites, *clump)) return fail("addGrassClump");
        }
        // render: grass now lives in the loaded chunks and many of their clumps are still off screen; every clump the chunk store lists is checked, and only those whose bound touches the render area (screen + renderMargin) are queued for sorting and drawing
        for (const npc& unit : npcs) {
            if (inSight(unit.getPos()) && !addDrawable(drawables, spriteFor(sprites, unit), unit.getPos(), 1.0f)) return fail("addDrawable npc");
        }
        for (const tree& stand : trees) {
            if (!stand.isFelled() && inSight(stand.getPos()) && !addDrawable(drawables, treeSprite, stand.getPos(), treeDrawSize)) return fail("addDrawable tree");
        }
        for (const wall& segment : walls) {
            if (segment.isDestroyed() || !inSight(segment.getPos())) continue;
            const Uint8 alpha = segment.isBuilt() ? 255 : static_cast<Uint8>(planAlpha + (255 - planAlpha) * segment.getProgress());
            if (!addWallDrawables(drawables, look, {segment.getPos(), postOf(segment)}, alpha)) return fail("addDrawable wall");
        }
        for (const Seen& entry : localFog.getSeen()) {
            if (!showVeil || localFog.state(entry.pos) != FogState::Discovered) continue;
            if (entry.kind == Seen::Kind::Tree) {
                if (!addDrawable(drawables, treeSprite, entry.pos, treeDrawSize)) return fail("addDrawable seen tree");
                continue;
            }
            const Uint8 alpha = entry.built ? 255 : static_cast<Uint8>(planAlpha + (255 - planAlpha) * entry.progress);
            if (!addWallDrawables(drawables, look, {entry.pos, postOf(walls[entry.index])}, alpha)) return fail("addDrawable seen wall");
        }
        // fog: a discovered spot shows the past, not the present; live npcs, trees, walls, items, arrows and footprints only draw in sight, and the snapshot's trees and walls draw where the team has no sight
        if (placing) {
            for (const PlanSpot& spot : planLine(dragging ? dragStart : toMap(mouse), toMap(mouse), walls)) {
                if (!canPlace(makeWall(spot, look), walls, trees)) continue;
                if (!addWallDrawables(drawables, look, spot, planAlpha)) return fail("addDrawable plan");
            }
        }
        // economy: posts and their ghosts are drawn by addWallDrawables
        // economy: walls stand on the map and sort by feet; a plan is see-through and fills in as it is built, and while placing, the line that a release would plan (one snapped post before the drag) shows as ghosts where it can go
        // economy: a felled tree is gone from the map and is not drawn
        // core: the object stands on the map like npcs and grass; it joins the feet-y sorted list so a lower npc shows in front of it
        for (size_t i = 0; i < items.size(); ++i) {
            if (!inSight(items[i]->isHeld() ? npcs[items[i]->getHolder()].getPos() : items[i]->getPos())) continue;
            const bool mirrored = items[i]->isHeld() && npcs[items[i]->getHolder()].isFacingLeft();
            float textureW = 0.0f;
            float textureH = 0.0f;
            if (!SDL_GetTextureSize(itemSprites[i], &textureW, &textureH)) return fail("SDL_GetTextureSize item");
            const float scale = items[i]->getSize().x / (textureW * spriteScale);
            if (!addDrawable(drawables, itemSprites[i], items[i]->getPos(), scale, mirrored ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE)) return fail("addDrawable item");
            // economy: an item is drawn at its own size, so a bigger plank looks bigger; the draw scale is the item width over the sprite width at spriteScale, 1 for every other item
            if (items[i]->isHeld()) drawables.back().feet = npcs[items[i]->getHolder()].getPos().y + 0.01f;
            if (items[i]->isHeld() && items[i]->getImage() == "plank.png") {
                const Facing facing = npcs[items[i]->getHolder()].getFacing();
                const bool lookingUp = facing == Facing::Up || facing == Facing::UpLeft || facing == Facing::UpRight;
                if (!lookingUp) drawables.back().feet -= 0.02f;
            }
            // economy: a plank on the shoulder sorts just behind its holder so the npc's face shows in front of it, but an npc looking up shows its back, so then the plank stays just in front like other held items
            if (items[i]->isUsing()) {
                const float turn = (mirrored ? -1.0f : 1.0f) * items[i]->getUseAngle();
                const float radians = turn * 3.14159265f / 180.0f;
                const float half = items[i]->getSize().y / 2.0f;
                const Vector2D<float> pivot = items[i]->getPos();
                drawables.back().center = {pivot.x + std::sin(radians) * half, pivot.y - std::cos(radians) * half};
                drawables.back().angle = turn;
            }
            // animation: a swinging item turns outward around its handle, clockwise on the right and counter-clockwise on the left; its center is swung around the bottom pivot by the angle, then the sprite is turned by the same angle about that center
        }
        for (const arrow& shot : arrows) {
            if (!inSight(shot.getPos())) continue;
            drawables.push_back({arrowSprite, shot.getPos(), 1.0f, shot.getFeet(), SDL_FLIP_NONE, shot.getAngle()});
        }
        // combat: an arrow sorts by the ground point under it, so it passes behind things lower on the map; it is drawn centered on its pos, turned along its flight
        // unit_item: a ground item sorts by its own feet like everything else; a held item sorts just below its holder's feet so it always shows in front of that npc, and it is mirrored while held on the left
        std::stable_sort(drawables.begin(), drawables.end(), [](const Drawable& a, const Drawable& b) {
            return a.feet < b.feet;
        });
        for (const Drawable& item : drawables) {
            if (!drawSprite(renderer, item.texture, item.center, item.alpha, item.angle, item.size, item.flip)) return fail("drawSprite drawable");
        }
        // render: grass always drew under the npc even when it stood lower on the map; grass strips and npcs now go in one list sorted by feet y, so whatever is lower on the map is drawn later and shows in front
        if (showSolid) {
            for (const npc& unit : npcs) {
                if (!drawHitbox(renderer, unit.getCollider())) return fail("drawHitbox npc");
            }
            for (const tree& stand : trees) {
                if (!stand.isFelled() && !drawHitbox(renderer, stand.getCollider())) return fail("drawHitbox tree");
            }
            for (const wall& segment : walls) {
                if (segment.isBuilt() && !drawHitbox(renderer, segment.getCollider())) return fail("drawHitbox wall");
            }
        }
        if (showChop) {
            for (const tree& stand : trees) {
                if (!stand.isFelled() && !drawHitbox(renderer, stand.getChopHitbox())) return fail("drawHitbox chop");
            }
            for (const wall& segment : walls) {
                if (segment.isBuilt() && !drawHitbox(renderer, segment.getHitbox())) return fail("drawHitbox wall hit");
            }
        }
        // test: the dev views draw over every sprite; U shows what is solid now (npcs, standing trees, built walls) and I what an axe can hit (standing trees, built walls)

        const float arrowBob = std::sin(static_cast<float>(SDL_GetTicks()) / 300.0f) * arrowBobHeight;
        for (const npc& unit : npcs) {
            if (!unit.isSelected() || !inSight(unit.getPos())) continue;

            const SDL_FRect rect = npcRect(unit);
            const Vector2D<float> arrow = {unit.getPos().x, rect.y - arrowGap + arrowBob};
            if (!drawSprite(renderer, sprites.selectedArrow, arrow, 255, 0.0, arrowSize)) return fail("drawSprite selectedArrow");
        }
        // ui: selected npcs showed a green outline; they now show selectedArrow.png centered arrowGap px above the head, bobbing up and down by arrowBobHeight on a sine of the clock
        for (size_t t = 0; t < trees.size(); ++t) {
            const tree& stand = trees[t];
            if (stand.isFelled() || !inSight(stand.getPos())) continue;
            if (treeHitAge[t] < hpBarShow && !drawHpBar(renderer, stand.getPos(), static_cast<float>(stand.getHp()) / static_cast<float>(stand.getMaxHp()))) return fail("drawHpBar");
            if (!stand.isClicked() && !treeTargeted[t]) continue;
            const Vector2D<float> arrow = {stand.getPos().x, objectRect(stand).y - arrowGap + arrowBob};
            if (!drawSprite(renderer, sprites.selectedArrow, arrow, 255, 0.0, arrowSize)) return fail("drawSprite selectedArrow");
        }
        // combat: the tree shows its hp only for hpBarShow ms after it was last hit; the white bar is drawn over the sprites, under the arrows
        for (size_t w = 0; w < walls.size(); ++w) {
            const wall& segment = walls[w];
            if (segment.isDestroyed() || !inSight(segment.getPos())) continue;
            const float left = segment.isBuilt() ? static_cast<float>(segment.getHp()) / static_cast<float>(segment.getMaxHp()) : segment.getProgress();
            const bool showBar = segment.isGroupFirst() && (segment.isBuilt() ? wallHitAge[w] < hpBarShow : segment.hasAllPlanks());
            if (showBar && !drawHpBar(renderer, segment.getGroupMiddle(), left)) return fail("drawHpBar wall");
            // economy-wall-group: a group has one hp, so one bar at the middle of its posts; only its first post draws it, for hpBarShow ms after a hit once built, and with all planks in as a plan to show build progress
            if (segment.isGroupFirst() && !segment.isBuilt() && !drawWallCount(renderer, segment.getGroupMiddle(), std::max(look.sizes[0].y, look.sizes[1].y), segment.getDelivered())) return fail("drawWallCount");
            // economy-wall-group: a group being made shows its plank counter from 0 until it is built, drawn once by its first post above its middle
            if (!wallTargeted[w]) continue;
            const Vector2D<float> arrow = {segment.getPos().x, objectRect(segment).y - arrowGap + arrowBob};
            // economy: a post's arrow sits above the post
            if (!drawSprite(renderer, sprites.selectedArrow, arrow, 255, 0.0, arrowSize)) return fail("drawSprite selectedArrow");
        }
        // economy: a wall's bar shows build progress once all planks are in, then hp once built; a wall an npc is walking to shows the arrow
        // ui: an object shows the arrow while it is clicked and while any npc is still walking to it (in place of the cross); the same bobbing arrow is drawn arrowGap px above the top of its sprite area
        for (size_t i = 0; i < items.size(); ++i) {
            if (!itemTargeted[i] || !inSight(items[i]->getPos())) continue;
            const Vector2D<float> arrow = {items[i]->getPos().x, objectRect(*items[i]).y - arrowGap + arrowBob};
            if (!drawSprite(renderer, sprites.selectedArrow, arrow, 255, 0.0, arrowSize)) return fail("drawSprite selectedArrow");
        }
        // unit_item: a ground item an npc is walking to shows the bobbing arrow above it in place of the cross
        if (showVeil && !drawVeil(renderer, veil, localFog)) return fail("drawVeil");
        // fog: the veil covers the whole map and everything on it, including crosses, path lines and plans; the drag box and screen UI draw after it
        if (dragging && !placing && !drawOutline(renderer, boxBetween(dragStart, toMap(mouse)))) return fail("drawOutline");
        // economy: a drag while placing draws ghost walls instead of the selection box
        // render: draw order decides what covers what; background, then ground marks (footprints fading with age, cross at each moving npc's destination, white path line of each moving npc), then grass and npcs sorted by feet y, then selection arrows, drag box on top

        if (!SDL_SetRenderScale(renderer, 1.0f, 1.0f)) return fail("SDL_SetRenderScale");
        const SDL_FRect terminalButton = terminalButtonRect(viewport);
        if (!SDL_RenderTexture(renderer, terminalIcon, nullptr, &terminalButton)) return fail("SDL_RenderTexture terminal icon");
        // ui-terminal: the button stays on screen while the map scrolls and zooms; it is drawn at scale 1 in real pixels, over the map and under the Terminal
        if (!editor.draw(renderer, viewport, density)) return fail("editor draw");
        // ui_editor: npc scripts are edited in game; the editor is drawn last over everything at scale 1 in real pixels, and the next frame sets zoom again before drawing the map
        if (!SDL_RenderPresent(renderer)) return fail("SDL_RenderPresent");
    }

    for (const NpcSprites& set : sprites.players) {
        SDL_DestroyTexture(set.up);
        SDL_DestroyTexture(set.left);
        SDL_DestroyTexture(set.right);
    }
    if (veil) SDL_DestroyTexture(veil);
    SDL_DestroyTexture(terminalIcon);
    SDL_DestroyTexture(sprites.footprint);
    SDL_DestroyTexture(sprites.cross);
    SDL_DestroyTexture(sprites.selectedArrow);
    SDL_DestroyTexture(treeSprite);
    for (SDL_Texture* texture : itemTextures) SDL_DestroyTexture(texture);
    // economy: planks share one texture, so textures are destroyed from the owned list, each once
    for (SDL_Texture* texture : grassSprites) SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
