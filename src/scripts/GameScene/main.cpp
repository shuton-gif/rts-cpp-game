#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>

template <typename T>
struct Vector2D {
    T x;
    T y;
};

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

static void moveCamera(const Vector2D<float> viewport, const float deltaTime)
{
    const float step = cameraSpeed * deltaTime;

    if (mouse.x <= edgeMargin) camera.x -= step;
    if (mouse.x >= viewport.x - 1.0f - edgeMargin) camera.x += step;
    if (mouse.y <= edgeMargin) camera.y -= step;
    if (mouse.y >= viewport.y - 1.0f - edgeMargin) camera.y += step;

    camera.x = std::clamp(camera.x, 0.0f, std::max(0.0f, bgSize.x - viewport.x));
    camera.y = std::clamp(camera.y, 0.0f, std::max(0.0f, bgSize.y - viewport.y));
}
// camera: edge scroll like League of Legends; cursor within edgeMargin of an edge moves that axis by cameraSpeed * deltaTime, then camera is locked inside the map

static bool drawBackground(SDL_Renderer* renderer, const Vector2D<float> viewport)
{
    const int camX = static_cast<int>(std::floor(camera.x));
    const int camY = static_cast<int>(std::floor(camera.y));
    const int endX = std::min(camX + static_cast<int>(viewport.x), static_cast<int>(bgSize.x));
    const int endY = std::min(camY + static_cast<int>(viewport.y), static_cast<int>(bgSize.y));

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

        if (!SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)) return fail("SDL_SetRenderDrawColor");
        if (!SDL_RenderClear(renderer)) return fail("SDL_RenderClear");
        if (!drawBackground(renderer, viewport)) return fail("drawBackground");
        if (!SDL_RenderPresent(renderer)) return fail("SDL_RenderPresent");
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
