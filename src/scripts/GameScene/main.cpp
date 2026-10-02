#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

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
const int tileSize = 100;
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

float zoom = 1.0f;
const float zoomStep = 1.1f;
const float zoomMin = 0.5f;
const float zoomMax = 4.0f;

static Vector2D<float> viewSize(const Vector2D<float> viewport)
{
    return {viewport.x / zoom, viewport.y / zoom};
}
// camera: zoom changes how much map fits on screen; zoom 1 is the default size, and the visible map area is the window size divided by zoom

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
    const float step = cameraSpeed * deltaTime / zoom;

    if (mouse.x <= edgeMargin) camera.x -= step;
    if (mouse.x >= viewport.x - 1.0f - edgeMargin) camera.x += step;
    if (mouse.y <= edgeMargin) camera.y -= step;
    if (mouse.y >= viewport.y - 1.0f - edgeMargin) camera.y += step;

    clampCamera(viewport);
}
// camera: edge scroll like League of Legends; cursor within edgeMargin of a window edge moves that axis by cameraSpeed * deltaTime (divided by zoom so screen speed is the same at any zoom), then camera is locked inside the map

static void zoomCamera(const Vector2D<float> viewport, const Vector2D<float> cursor, const float wheel)
{
    const float lowest = std::max({zoomMin, viewport.x / bgSize.x, viewport.y / bgSize.y});
    const float before = zoom;
    zoom = std::clamp(zoom * std::pow(zoomStep, wheel), lowest, zoomMax);

    camera.x += cursor.x / before - cursor.x / zoom;
    camera.y += cursor.y / before - cursor.y / zoom;
    clampCamera(viewport);
}
// camera: mouse scroll zooms in and out; each wheel notch multiplies zoom by zoomStep within zoomMin..zoomMax (never so far out that the view is bigger than the map), and the camera shifts so the map point under the cursor stays under it

static bool drawBackground(SDL_Renderer* renderer, const Vector2D<float> viewport)
{
    const Vector2D<float> view = viewSize(viewport);
    const int camX = static_cast<int>(std::floor(camera.x));
    const int camY = static_cast<int>(std::floor(camera.y));
    const int endX = std::min(camX + static_cast<int>(std::ceil(view.x)) + 1, static_cast<int>(bgSize.x));
    const int endY = std::min(camY + static_cast<int>(std::ceil(view.y)) + 1, static_cast<int>(bgSize.y));

    for (int ty = camY / tileSize; ty * tileSize < endY; ++ty) {
        for (int tx = camX / tileSize; tx * tileSize < endX; ++tx) {
            const Uint8 shade = (tx + ty) % 2 == 0 ? 255 : 200;
            const SDL_FRect tile = {
                static_cast<float>(tx * tileSize - camX),
                static_cast<float>(ty * tileSize - camY),
                static_cast<float>(tileSize),
                static_cast<float>(tileSize),
            };
            if (!SDL_SetRenderDrawColor(renderer, shade, shade, shade, 255)) return false;
            if (!SDL_RenderFillRect(renderer, &tile)) return false;
        }
    }
    return true;
}
// render: map is bigger than the screen; only tiles overlapping the view are drawn, offset by the whole-px camera so tiles have no seams

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

static bool drawSprite(SDL_Renderer* renderer, SDL_Texture* texture, const Vector2D<float> center, const Uint8 alpha, const double angle = 0.0)
{
    float w = 0.0f;
    float h = 0.0f;
    if (!SDL_GetTextureSize(texture, &w, &h)) return false;

    const Vector2D<float> cam = cameraPx();
    const SDL_FRect dst = {
        std::floor(center.x - w * spriteScale / 2.0f) - cam.x,
        std::floor(center.y - h * spriteScale / 2.0f) - cam.y,
        w * spriteScale,
        h * spriteScale,
    };
    if (!SDL_SetTextureAlphaMod(texture, alpha)) return false;
    return SDL_RenderTextureRotated(renderer, texture, nullptr, &dst, angle, nullptr, SDL_FLIP_NONE);
}
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
        pos.y - size.y * spriteScale / 2.0f,
        size.x * spriteScale,
        size.y * spriteScale,
    };
}

static SDL_FRect boxBetween(const Vector2D<float> a, const Vector2D<float> b)
{
    return {std::min(a.x, b.x), std::min(a.y, b.y), std::abs(a.x - b.x), std::abs(a.y - b.y)};
}

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
    if (!SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255)) return false;
    return SDL_RenderRect(renderer, &rect);
}
// ui: selected npcs and the drag box need to be visible; a green 1 px rectangle is drawn around a map-px rect

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
    if (!SDL_CreateWindowAndRenderer("rts", 0, 0, SDL_WINDOW_FULLSCREEN, &window, &renderer)) {
        return fail("SDL_CreateWindowAndRenderer");
    }
    SDL_SetRenderVSync(renderer, 1);
    SDL_SetWindowMouseGrab(window, true);
    // render: game opens full screen; borderless desktop full screen with vsync, cursor kept inside the window so it can reach the edges

    int windowW = 0;
    int windowH = 0;
    if (!SDL_GetWindowSize(window, &windowW, &windowH)) return fail("SDL_GetWindowSize");
    Vector2D<float> viewport = {static_cast<float>(windowW), static_cast<float>(windowH)};

    camera = {(bgSize.x - viewport.x) / 2.0f, (bgSize.y - viewport.y) / 2.0f};
    // camera: starts at the middle of the map; view is centered by offsetting half of (map - viewport)

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

    std::vector<npc> npcs;
    npcs.emplace_back(0, Vector2D<float>{bgSize.x / 2.0f, bgSize.y / 2.0f});

    std::vector<Vector2D<float>> lastFootprint;
    for (const npc& unit : npcs) lastFootprint.push_back(unit.getPos());
    std::vector<Footprint> footprints;

    bool dragging = false;
    Vector2D<float> dragStart = {0.0f, 0.0f};
    // unit: one npc starts at the middle of the map, which is the middle of the screen because the camera starts centered; npcs sit in a vector so more can be added

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
                break;
            case SDL_EVENT_MOUSE_MOTION:
                mouse = {event.motion.x, event.motion.y};
                mouseSeen = true;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                mouse = {event.button.x, event.button.y};
                mouseSeen = true;
                if (event.button.button == SDL_BUTTON_LEFT) {
                    dragStart = toMap(mouse);
                    dragging = true;
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    for (npc& unit : npcs) {
                        if (unit.isSelected()) unit.setDestinaion(toMap(mouse));
                    }
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                mouse = {event.button.x, event.button.y};
                if (event.button.button == SDL_BUTTON_LEFT && dragging) {
                    const SDL_FRect box = boxBetween(dragStart, toMap(mouse));
                    for (npc& unit : npcs) unit.setSelected(overlaps(box, npcRect(unit)));
                    dragging = false;
                }
                break;
            // input: left click or left drag selects, right click moves the selection; drag start is kept in map px so the box stays put while the camera scrolls, and release selects every npc the box touches
            case SDL_EVENT_MOUSE_WHEEL:
                mouse = {event.wheel.mouse_x, event.wheel.mouse_y};
                zoomCamera(viewport, mouse, event.wheel.y);
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                viewport = {static_cast<float>(event.window.data1), static_cast<float>(event.window.data2)};
                break;
            default:
                break;
            }
        }
        // input: full screen has no close button and the cursor is 0,0 until it moves; Esc quits and edge scroll waits for the first mouse motion

        const float deltaTime = timer.tick();
        if (mouseSeen) moveCamera(viewport, deltaTime);

        for (Footprint& print : footprints) print.age += deltaTime;
        std::erase_if(footprints, [](const Footprint& print) { return print.age >= footprintLife; });

        for (size_t i = 0; i < npcs.size(); ++i) {
            npcs[i].update(deltaTime);

            const Vector2D<float> pos = npcs[i].getPos();
            if (std::hypot(pos.x - lastFootprint[i].x, pos.y - lastFootprint[i].y) >= footprintGap) {
                const float feet = pos.y + npcs[i].getSize().y * spriteScale / 2.0f;
                footprints.push_back({{pos.x, feet}, 0.0f, facingAngle(npcs[i].getFacing())});
                lastFootprint[i] = pos;
            }
        }
        // movement: footprints follow the npc and fade after 1000 ms; each npc drops one at its feet every footprintGap px walked, every footprint ages by deltaTime and is erased at footprintLife

        if (!SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)) return fail("SDL_SetRenderDrawColor");
        if (!SDL_RenderClear(renderer)) return fail("SDL_RenderClear");
        if (!SDL_SetRenderScale(renderer, zoom, zoom)) return fail("SDL_SetRenderScale");
        // camera: zoom must scale everything drawn; the renderer multiplies all coordinates by zoom, so draw code keeps working in map px
        if (!drawBackground(renderer, viewport)) return fail("drawBackground");

        for (const Footprint& print : footprints) {
            const Uint8 alpha = static_cast<Uint8>(255.0f * (1.0f - print.age / footprintLife));
            if (!drawSprite(renderer, sprites.footprint, print.pos, alpha, print.angle)) return fail("drawSprite footprint");
        }
        for (const npc& unit : npcs) {
            if (!unit.isMoving()) continue;
            if (!drawSprite(renderer, sprites.cross, unit.getTargetedPos(), 255)) return fail("drawSprite cross");
        }
        const float arrowBob = std::sin(static_cast<float>(SDL_GetTicks()) / 300.0f) * arrowBobHeight;
        for (const npc& unit : npcs) {
            if (!drawSprite(renderer, spriteFor(sprites, unit), unit.getPos(), 255)) return fail("drawSprite npc");
            if (!unit.isSelected()) continue;

            const SDL_FRect rect = npcRect(unit);
            const Vector2D<float> arrow = {unit.getPos().x, rect.y - arrowGap + arrowBob};
            if (!drawSprite(renderer, sprites.selectedArrow, arrow, 255)) return fail("drawSprite selectedArrow");
        }
        // ui: selected npcs showed a green outline; they now show selectedArrow.png centered arrowGap px above the head, bobbing up and down by arrowBobHeight on a sine of the clock
        if (dragging && !drawOutline(renderer, boxBetween(dragStart, toMap(mouse)))) return fail("drawOutline");
        // render: draw order decides what covers what; background, then footprints (alpha falls linearly with age), cross at each moving npc's destination, npcs with an arrow above when selected, drag box on top

        if (!SDL_RenderPresent(renderer)) return fail("SDL_RenderPresent");
    }

    SDL_DestroyTexture(sprites.npcUp);
    SDL_DestroyTexture(sprites.npcLeft);
    SDL_DestroyTexture(sprites.npcRight);
    SDL_DestroyTexture(sprites.footprint);
    SDL_DestroyTexture(sprites.cross);
    SDL_DestroyTexture(sprites.selectedArrow);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
