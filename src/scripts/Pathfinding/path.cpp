#include "Pathfinding/path.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <queue>
#include <utility>

#include "NPC/npc.h"

const float ovalRatio = 2.0f;
const float diagonalCost = 1.41421356f;
const float sightStep = navCell / 4.0f;
// pathfinding: blocking uses the same 1 : 2 oval stretch as collision, a diagonal step costs sqrt 2 against 1 straight, and line of sight samples a segment every quarter cell

struct navNode {
    float f;
    float g;
    int cell;

    bool operator>(const navNode& other) const
    {
        if (f != other.f) return f > other.f;
        return g < other.g;
    }
};
// pathfinding: the A* open list is a binary heap of these; f is cost plus heuristic, and on equal f the node with more cost (closer to the goal) is taken first

navWindow navWindowFor(Vector2D<float> from, Vector2D<float> to, Vector2D<float> mapSize)
{
    const auto axis = [](const float a, const float b, const float map, float& origin, float& size) {
        const float mapEnd = std::max(0.0f, std::ceil(map / navCell) * navCell);
        const float lo = std::clamp(std::min(a, b) - navMargin, 0.0f, std::max(0.0f, map));
        const float hi = std::clamp(std::max(a, b) + navMargin, 0.0f, std::max(0.0f, map));
        origin = std::floor(lo / navCell) * navCell;
        size = std::ceil(hi / navCell) * navCell - origin;
        const float cap = static_cast<float>(navMaxCells) * navCell;
        if (size <= cap) return;
        const float start = b >= a ? std::floor((a - navMargin) / navCell) * navCell : std::ceil((a + navMargin) / navCell) * navCell - cap;
        origin = std::clamp(start, 0.0f, std::max(0.0f, mapEnd - cap));
        size = cap;
    };
    navWindow window;
    axis(from.x, to.x, mapSize.x, window.origin.x, window.size.x);
    axis(from.y, to.y, mapSize.y, window.origin.y, window.size.y);
    return window;
}
// pathfinding: a whole map grid of 40000 map px costs ~350 MB; per axis the span of from and to grows by navMargin, is clamped to the map and snapped outward to navCell multiples, and a span over navMaxCells cells becomes navMaxCells cells starting navMargin behind from on the side away from to (from centered then shifted toward to as far as it can go), clamped to the map

void navGrid::build(const std::vector<Collider>& solids, Vector2D<float> origin, Vector2D<float> size)
{
    this->origin = origin;
    this->width = std::max(0, static_cast<int>(std::ceil(size.x / navCell)));
    this->height = std::max(0, static_cast<int>(std::ceil(size.y / navCell)));
    const std::size_t cells = static_cast<std::size_t>(this->width) * static_cast<std::size_t>(this->height);
    this->blocked.assign(cells, 0);
    if (this->stamp.size() < cells) {
        this->cost.resize(cells, 0.0f);
        this->parent.resize(cells, -1);
        this->stamp.resize(cells, 0);
        this->closed.resize(cells, 0);
        this->component.resize(cells, -1);
    }
    this->labeled = false;
    if (cells == 0) return;

    for (const Collider& solid : solids) {
        const float reach = solid.radius + colliderRadius;
        const float sx = solid.pos.x - origin.x;
        const float sy = solid.pos.y - origin.y;
        const int x0 = std::max(0, static_cast<int>(std::floor((sx - reach) / navCell)));
        const int x1 = std::min(this->width - 1, static_cast<int>(std::floor((sx + reach) / navCell)));
        const int y0 = std::max(0, static_cast<int>(std::floor((sy - reach / ovalRatio) / navCell)));
        const int y1 = std::min(this->height - 1, static_cast<int>(std::floor((sy + reach / ovalRatio) / navCell)));
        if (x0 > x1 || y0 > y1) continue;
        for (int y = y0; y <= y1; ++y) {
            const float dy = ((static_cast<float>(y) + 0.5f) * navCell - sy) * ovalRatio;
            for (int x = x0; x <= x1; ++x) {
                const float dx = (static_cast<float>(x) + 0.5f) * navCell - sx;
                if (std::hypot(dx, dy) < reach) this->blocked[static_cast<std::size_t>(y) * this->width + x] = 1;
            }
        }
    }
}
// pathfinding: a cell is blocked when an npc at its center would overlap a solid, and only the window around the trip is gridded; solids are shifted into window space by origin, y is stretched by ovalRatio like collision's separate, the npc collider radius is added to the solid's, only the cells in each solid's inflated box clamped to the window are tested (a box missing the window is skipped), the other buffers only grow by resize so stale stamps stay below generation, and the connected areas are marked stale

int navGrid::cellOf(Vector2D<float> point) const
{
    const int x = std::clamp(static_cast<int>(std::floor((point.x - this->origin.x) / navCell)), 0, this->width - 1);
    const int y = std::clamp(static_cast<int>(std::floor((point.y - this->origin.y) / navCell)), 0, this->height - 1);
    return y * this->width + x;
}
// pathfinding: points outside the window still need a cell; the map point minus the window origin is divided by navCell and clamped to the edge cells

bool navGrid::isFree(int cell, int start) const
{
    return !this->blocked[cell] || cell == start;
}
// pathfinding: an npc pressed against a trunk stands in a blocked cell and must still be able to leave it; the start cell always counts as free

bool navGrid::lineOfSight(Vector2D<float> a, Vector2D<float> b, int skipCell) const
{
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const int steps = std::max(1, static_cast<int>(std::ceil(std::hypot(dx, dy) / sightStep)));
    int last = -1;
    for (int s = 0; s <= steps; ++s) {
        const float t = static_cast<float>(s) / static_cast<float>(steps);
        const int cell = this->cellOf({a.x + dx * t, a.y + dy * t});
        if (cell == last) continue;
        last = cell;
        if (cell != skipCell && this->blocked[cell]) return false;
    }
    return true;
}
// pathfinding: a straight walk is only safe when no blocked cell lies on it; the segment is sampled every sightStep map px and fails on any blocked cell except skipCell, the cell the walk starts in

int navGrid::nearestFree(int goal, int start) const
{
    const int goalX = goal % this->width;
    const int goalY = goal / this->width;
    int best = -1;
    int bestDistance = 0;
    for (int r = 1; r <= goalSearchCells; ++r) {
        if (best >= 0 && r * r > bestDistance) break;
        for (int y = goalY - r; y <= goalY + r; ++y) {
            if (y < 0 || y >= this->height) continue;
            const int stepX = (y == goalY - r || y == goalY + r) ? 1 : 2 * r;
            for (int x = goalX - r; x <= goalX + r; x += stepX) {
                if (x < 0 || x >= this->width) continue;
                const int cell = y * this->width + x;
                if (!this->isFree(cell, start)) continue;
                const int distance = (x - goalX) * (x - goalX) + (y - goalY) * (y - goalY);
                if (best < 0 || distance < bestDistance) {
                    best = cell;
                    bestDistance = distance;
                }
            }
        }
    }
    return best;
}
// pathfinding: npcs are sent to a tree's center, a cell A* can never reach; rings of growing radius around the goal are scanned up to goalSearchCells, and the free cell nearest in straight distance wins once no outer ring can be closer (-1 when none)

void navGrid::label() const
{
    int* const area = this->component.data();
    const unsigned char* const wall = this->blocked.data();
    const int cols = this->width;
    const int rows = this->height;
    const auto root = [area](int cell) {
        while (area[cell] != cell) {
            area[cell] = area[area[cell]];
            cell = area[cell];
        }
        return cell;
    };
    const auto join = [area, &root](const int a, const int b) {
        const int ra = root(a);
        const int rb = root(b);
        if (ra < rb) area[rb] = ra;
        else if (rb < ra) area[ra] = rb;
    };

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const int cell = y * cols + x;
            if (wall[cell]) {
                area[cell] = -1;
                continue;
            }
            area[cell] = cell;
            const bool left = x > 0 && !wall[cell - 1];
            if (left) join(cell, cell - 1);
            if (y == 0) continue;
            const int up = cell - cols;
            if (wall[up]) continue;
            join(cell, up);
            if (left && !wall[up - 1]) join(cell, up - 1);
            if (x + 1 < cols && !wall[cell + 1] && !wall[up + 1]) join(cell, up + 1);
        }
    }
    for (int cell = 0; cell < cols * rows; ++cell) {
        if (!wall[cell]) area[cell] = root(cell);
    }
    this->labeled = true;
}
// pathfinding: an A* toward a walled-in goal would search the whole map (over 20 ms); free cells are joined into connected areas by one union find pass over the grid with the same steps A* takes (a diagonal only when both cut cells are free), once per build on the first search that needs it, so an unreachable goal is known before searching

std::vector<Vector2D<float>> navGrid::findPath(Vector2D<float> from, Vector2D<float> to) const
{
    if (this->blocked.empty()) return {to};
    const int start = this->cellOf(from);
    if (this->lineOfSight(from, to, start)) return {to};

    int goal = this->cellOf(to);
    bool exact = true;
    if (!this->isFree(goal, start)) {
        exact = false;
        const int nearest = this->nearestFree(goal, start);
        if (nearest >= 0) goal = nearest;
    }

    ++this->generation;
    if (this->generation == 0) {
        std::fill(this->stamp.begin(), this->stamp.end(), 0);
        std::fill(this->closed.begin(), this->closed.end(), 0);
        this->generation = 1;
    }
    // pathfinding: after the generation wraps, stale stamps beyond the current window would match a new generation once a bigger window reaches them; the whole grown buffer is zeroed, once per 2^32 searches
    const std::uint32_t now = this->generation;
    const int cols = this->width;
    const int cells = cols * this->height;
    int goalX = goal % cols;
    int goalY = goal / cols;
    const auto heuristic = [&](const int x, const int y) {
        const float dx = static_cast<float>(std::abs(x - goalX));
        const float dy = static_cast<float>(std::abs(y - goalY));
        return dx + dy + (diagonalCost - 2.0f) * std::min(dx, dy);
    };

    if (!this->labeled) this->label();
    int areas[9];
    int areaCount = 0;
    const int startX = start % cols;
    const int startY = start / cols;
    for (int dy = -1; dy <= 1; ++dy) {
        const int ny = startY + dy;
        if (ny < 0 || ny >= this->height) continue;
        for (int dx = -1; dx <= 1; ++dx) {
            const int nx = startX + dx;
            if (nx < 0 || nx >= cols) continue;
            const int next = ny * cols + nx;
            if (this->blocked[next]) continue;
            if (dx != 0 && dy != 0 && (!this->isFree(startY * cols + nx, start) || !this->isFree(ny * cols + startX, start))) continue;
            if (std::find(areas, areas + areaCount, this->component[next]) == areas + areaCount) areas[areaCount++] = this->component[next];
        }
    }
    const auto reachable = [&](const int cell) {
        return cell == start || std::find(areas, areas + areaCount, this->component[cell]) != areas + areaCount;
    };
    if (!reachable(goal)) {
        exact = false;
        int best = start;
        float bestDistance = heuristic(startX, startY);
        for (int cell = 0; cell < cells; ++cell) {
            if (this->blocked[cell] || !reachable(cell)) continue;
            const float distance = heuristic(cell % cols, cell / cols);
            if (distance < bestDistance) {
                best = cell;
                bestDistance = distance;
            }
        }
        goal = best;
        goalX = goal % cols;
        goalY = goal / cols;
    }

    std::priority_queue<navNode, std::vector<navNode>, std::greater<navNode>> open;
    this->stamp[start] = now;
    this->cost[start] = 0.0f;
    this->parent[start] = -1;
    open.push({heuristic(startX, startY), 0.0f, start});
    int closest = start;
    float closestDistance = heuristic(startX, startY);
    bool reached = false;

    while (!open.empty()) {
        const navNode node = open.top();
        open.pop();
        if (this->closed[node.cell] == now) continue;
        this->closed[node.cell] = now;
        if (node.cell == goal) {
            closest = goal;
            reached = true;
            break;
        }
        const int x = node.cell % cols;
        const int y = node.cell / cols;
        const float distance = heuristic(x, y);
        if (distance < closestDistance) {
            closest = node.cell;
            closestDistance = distance;
        }

        for (int dy = -1; dy <= 1; ++dy) {
            const int ny = y + dy;
            if (ny < 0 || ny >= this->height) continue;
            for (int dx = -1; dx <= 1; ++dx) {
                const int nx = x + dx;
                if ((dx == 0 && dy == 0) || nx < 0 || nx >= cols) continue;
                const int next = ny * cols + nx;
                if (this->closed[next] == now || !this->isFree(next, start)) continue;
                const bool diagonal = dx != 0 && dy != 0;
                if (diagonal && (!this->isFree(y * cols + nx, start) || !this->isFree(ny * cols + x, start))) continue;
                const float g = node.g + (diagonal ? diagonalCost : 1.0f);
                if (this->stamp[next] == now && this->cost[next] <= g) continue;
                this->stamp[next] = now;
                this->cost[next] = g;
                this->parent[next] = node.cell;
                open.push({g + heuristic(nx, ny), g, next});
            }
        }
    }

    std::vector<Vector2D<float>> points;
    for (int cell = closest; cell != start && cell >= 0; cell = this->parent[cell]) {
        points.push_back({this->origin.x + (static_cast<float>(cell % cols) + 0.5f) * navCell, this->origin.y + (static_cast<float>(cell / cols) + 0.5f) * navCell});
    }
    std::reverse(points.begin(), points.end());
    std::vector<Vector2D<float>> corners;
    Vector2D<float> previous = {this->origin.x + (static_cast<float>(startX) + 0.5f) * navCell, this->origin.y + (static_cast<float>(startY) + 0.5f) * navCell};
    for (std::size_t k = 0; k < points.size(); ++k) {
        const bool last = k + 1 == points.size();
        if (last || points[k].x - previous.x != points[k + 1].x - points[k].x || points[k].y - previous.y != points[k + 1].y - points[k].y) corners.push_back(points[k]);
        previous = points[k];
    }
    points = std::move(corners);
    if (reached && exact && !points.empty()) points.back() = to;
    else points.push_back(to);

    std::vector<Vector2D<float>> path;
    Vector2D<float> anchor = from;
    int anchorCell = start;
    std::size_t i = 0;
    while (i < points.size()) {
        std::size_t j = i;
        while (j + 1 < points.size() && this->lineOfSight(anchor, points[j + 1], anchorCell)) ++j;
        path.push_back(points[j]);
        anchor = points[j];
        anchorCell = this->cellOf(anchor);
        i = j + 1;
    }
    return path;
}
// pathfinding: npcs need a way around solids inside the built window, not just a straight line; cell centers are turned back into map px by adding the window origin, a clear line gives {to}, a blocked goal is swapped for its nearest free cell, an unreachable one for the reachable cell closest to it, A* (8 neighbors, octile heuristic, no corner cutting) runs to the goal or to the closest cell reached, and the cell centers where the path turns are string pulled from from into straight segments that end with to
