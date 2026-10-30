#include "Editor/editor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

#include "AI/script.h"

static const SDL_Color editorWhite = {255, 255, 255, 255};
static const SDL_Color editorBlack = {0, 0, 0, 255};
static const SDL_Color editorGray = {0x85, 0x85, 0x85, 255};
static const SDL_Color editorGreen = {0x6a, 0x99, 0x55, 255};
static const SDL_Color editorPink = {0xc5, 0x86, 0xc0, 255};
static const SDL_Color editorBlue = {0x4f, 0xc1, 0xff, 255};
static const SDL_Color editorOrange = {0xce, 0x91, 0x78, 255};
static const SDL_Color editorRed = {0xf4, 0x47, 0x47, 255};
static const SDL_Color termStrip = {0x32, 0x3c, 0x39, 255};
static const SDL_Color termBar = {0x69, 0x6a, 0x6a, 255};
static const SDL_Color termEdge = {0x59, 0x56, 0x52, 255};
static const SDL_Color termBand = {0xeb, 0xeb, 0xeb, 255};
static const SDL_Color termTabOn = {0x9b, 0xad, 0xb7, 255};
static const SDL_Color termTabOff = {0x84, 0x7e, 0x87, 255};
static const SDL_Color termRed = {0xd9, 0x57, 0x63, 255};
static const SDL_Color termRedDim = {0x9f, 0x2f, 0x2f, 255};
static const SDL_Color termCorner = {0xd9, 0xd9, 0xd9, 255};
static const SDL_Color termListEdge = {0x22, 0x20, 0x34, 255};
static const SDL_Color termRedCore = {0xc8, 0x51, 0x5c, 255};
static const SDL_Color termRedDimCore = {0x93, 0x2c, 0x2c, 255};
static const float panelWide = 128.0f;
static const float panelHigh = 96.0f;
static const float stripHigh = 5.0f;
static const float barHigh = 7.0f;
static const float bandHigh = 2.0f;
static const float closeWide = 5.0f;
static const float minusWide = 8.0f;
static const float plusWide = 7.0f;
static const float tabWide = 24.0f;
static const float rowHigh = 4.0f;
static const float rowsPad = 4.0f;
static const float barThick = 3.0f;
static const float barInset = 2.0f;
static const float barTopInset = 4.0f;
static const float minWide = 64.0f;
static const float minHigh = 40.0f;
static const int edgeLeft = 1;
static const int edgeRight = 2;
static const int edgeTop = 4;
static const int edgeBottom = 8;
static const int edgeAll = edgeLeft | edgeRight | edgeTop | edgeBottom;
// ui-terminal: the Terminal must match terminal-ref.aseprite; these are its sampled colors, its sizes in art pixels, the smallest panel a drag can make, and the bits naming a panel edge, where all four together move the whole panel; the syntax colors and popup colors stay as they were

static bool isWordChar(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

static void pushToken(std::vector<EditorToken>& tokens, const std::string& text, SDL_Color color)
{
    if (text.empty()) return;
    const bool same = !tokens.empty() && tokens.back().color.r == color.r && tokens.back().color.g == color.g && tokens.back().color.b == color.b && tokens.back().color.a == color.a;
    if (same) tokens.back().text += text;
    else tokens.push_back({text, color});
}
// ui_editor: fewer tokens means fewer draw calls; pushToken skips empty text and merges a run into the previous token when both have the same color

std::vector<EditorToken> colorLine(const std::string& line)
{
    std::vector<EditorToken> tokens;
    std::size_t at = 0;
    bool inArg = false;
    while (at < line.size()) {
        const char c = line[at];
        if (c == '#') {
            pushToken(tokens, line.substr(at), editorGreen);
            break;
        }
        if (inArg && c != ')') {
            std::size_t end = line.find_first_of(")#", at);
            if (end == std::string::npos) end = line.size();
            pushToken(tokens, line.substr(at, end - at), editorOrange);
            at = end;
            continue;
        }
        if (c == '(' || c == ')') {
            inArg = c == '(';
            pushToken(tokens, std::string(1, c), editorBlack);
            ++at;
            continue;
        }
        if (isWordChar(c)) {
            std::size_t end = at;
            while (end < line.size() && isWordChar(line[end])) ++end;
            const std::string word = line.substr(at, end - at);
            SDL_Color color = editorBlack;
            if (word == "if" || word == "not" || word == "else") color = editorPink;
            else if (isScriptCall(word)) color = editorBlue;
            pushToken(tokens, word, color);
            at = end;
            continue;
        }
        pushToken(tokens, std::string(1, c), editorBlack);
        ++at;
    }
    return tokens;
}
// ui-terminal: white text cannot be read on the white rows; '#' to the end is green, if/not/else pink, a known call blue, text inside '(' ')' orange (stopping at ')' or '#'), and everything else black

std::vector<Suggestion> suggest(const std::string& line, int col, bool force, int& wordStart)
{
    static const char* const keywords[] = {"if", "not", "else"};
    static const char* const calls[] = {"find", "holding", "moveTo", "pickup", "target", "use", "full", "build", "drop", "stop"};
    static const char* const args[] = {"plank", "axe", "bow", "sword", "sheild", "pick-axe", "hoe", "tree", "plan", "wall", "target"};

    const std::size_t end = static_cast<std::size_t>(std::clamp(col, 0, static_cast<int>(line.size())));
    std::size_t start = end;
    while (start > 0 && (isWordChar(line[start - 1]) || line[start - 1] == '-')) --start;
    wordStart = static_cast<int>(start);

    std::vector<Suggestion> found;
    const std::string before = line.substr(0, start);
    if (before.find('#') != std::string::npos) return found;
    const std::string prefix = line.substr(start, end - start);
    if (prefix.empty() && !force) return found;

    const std::size_t open = before.rfind('(');
    const std::size_t close = before.rfind(')');
    const bool inArg = open != std::string::npos && (close == std::string::npos || close < open);
    const auto add = [&found, &prefix](const char* name, const std::string& insert, int back, SDL_Color color) {
        const std::string text = name;
        if (text.compare(0, prefix.size(), prefix) == 0) found.push_back({text, insert, back, color});
    };
    if (inArg) {
        for (const char* arg : args) add(arg, arg, 0, editorOrange);
        return found;
    }
    for (const char* keyword : keywords) add(keyword, std::string(keyword) + " ", 0, editorPink);
    for (const char* call : calls) add(call, std::string(call) + "()", 1, editorBlue);
    return found;
}
// ui_editor: typing skill names and args by hand was slow and error prone; suggest walks back over [A-Za-z0-9_-] from col, gives nothing after '#' or for an empty word unless force, and lists case-sensitive prefix matches (exact ones kept): args as is inside an unclosed '(', else keywords with a trailing space then calls as name() with the cursor stepped back into the parens

static float textScale(float density)
{
    return std::max(1.0f, std::round(2.0f * density));
}
// ui_editor: debug font glyphs are 8 px and too small on Retina; text is drawn at a whole render scale of 2 x density, at least 1, so glyph pixels stay sharp

static std::vector<std::string> readLines(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    std::stringstream stream;
    stream << file.rdbuf();
    const std::string text = stream.str();

    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos) end = text.size();
        std::string line = text.substr(start, end - start);
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        lines.push_back(std::move(line));
        start = end + 1;
    }
    if (lines.empty()) lines.emplace_back();
    return lines;
}
// ui_editor: a script file becomes editable lines; readLines splits on '\n', strips '\r', drops the empty piece after a final newline, and gives one empty line for an empty or missing file

static bool drawText(SDL_Renderer* renderer, float x, float y, float scale, const std::string& text, SDL_Color color)
{
    if (!SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a)) return false;
    return SDL_RenderDebugText(renderer, x / scale, y / scale, text.c_str());
}
// ui_editor: positions are worked out in real pixels but text is drawn under render scale; drawText sets the color and divides the position by scale because SDL scales coordinates too

static float snapTo(float value, float scale)
{
    return std::floor(value / scale) * scale;
}

static bool fillArt(SDL_Renderer* renderer, float x, float y, float art, float pixelX, float pixelY, float wide, float high, SDL_Color color)
{
    if (!SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a)) return false;
    const SDL_FRect box = {x + pixelX * art, y + pixelY * art, wide * art, high * art};
    return SDL_RenderFillRect(renderer, &box);
}

static bool inRect(Vector2D<float> pixel, const SDL_FRect& box)
{
    return pixel.x >= box.x && pixel.x < box.x + box.w && pixel.y >= box.y && pixel.y < box.y + box.h;
}
// ui-terminal: art scales separately from text; snapTo keeps text on whole text pixels so it stays sharp, fillArt fills a rect given in art pixels from a corner at render scale 1, and inRect is the mouse hit test

static float charCells(const std::string& line, std::size_t at)
{
    if (line[at] != ' ') return 1.0f;
    return line.find_first_not_of(' ') < at ? 0.5f : 1.0f;
}

static float lineCells(const std::string& line, int col)
{
    float cells = 0.0f;
    const std::size_t end = std::min(static_cast<std::size_t>(std::max(col, 0)), line.size());
    for (std::size_t i = 0; i < end; ++i) cells += charCells(line, i);
    return cells;
}

static int colAtCells(const std::string& line, float target)
{
    float cells = 0.0f;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const float wide = charCells(line, i);
        if (target < cells + wide / 2.0f) return static_cast<int>(i);
        cells += wide;
    }
    return static_cast<int>(line.size());
}
// ui-terminal: the user found the space between words too big; a space after the first word of a line is half a cell, while indent spaces stay a full cell so blocks line up, and lineCells and colAtCells turn a column into a width in cells and back

std::vector<std::string>& scriptEditor::lines()
{
    return this->buffers[this->tab];
}

const std::string& scriptEditor::tabLabel(std::size_t index) const
{
    return index < this->names.size() ? this->names[index] : this->typedName;
}

scriptEditor::editorLayout scriptEditor::layout(float scale) const
{
    editorLayout area;
    area.art = this->artScale;
    area.cell = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale;
    area.line = std::max((rowHigh * area.art + area.cell) / 2.0f, 10.0f * scale);
    area.popupLine = 10.0f * scale;
    area.pad = area.art;
    area.statusY = snapTo(this->panel.y + (stripHigh * area.art - area.cell) / 2.0f, scale);
    area.barY = this->panel.y + stripHigh * area.art;
    const float trackTop = area.barY + (barHigh + bandHigh) * area.art;
    area.textY = trackTop + rowsPad * area.art;
    area.textOffset = (area.line - area.cell) / 2.0f;

    std::size_t count = this->buffers.empty() ? 1 : this->buffers[this->tab].size();
    area.digits = 1;
    while (count > 10) {
        count = (count + 9) / 10;
        ++area.digits;
    }
    area.digits = std::max(2, area.digits);
    area.gutterX = snapTo(this->panel.x + area.art, scale);
    area.textX = area.gutterX + static_cast<float>(area.digits + 1) * area.cell + area.cell / 2.0f;

    if (!this->buffers.empty()) {
        area.lineCount = static_cast<int>(this->buffers[this->tab].size());
        for (const std::string& line : this->buffers[this->tab]) area.widest = std::max(area.widest, static_cast<int>(std::ceil(lineCells(line, static_cast<int>(line.size())))));
    }
    const float right = this->panel.x + this->panel.w;
    const float bottom = this->panel.y + this->panel.h;
    const float thick = barThick * area.art;
    for (int pass = 0; pass < 2; ++pass) {
        area.visible = std::max(1, static_cast<int>(std::floor((bottom - (area.showHorizontal ? thick : 0.0f) - area.textY) / area.line)));
        area.textW = std::max(0.0f, right - area.art - (area.showVertical ? thick : 0.0f) - area.textX);
        area.columns = std::max(1, static_cast<int>(std::floor(area.textW / area.cell)));
        area.showVertical = area.lineCount > area.visible;
        area.showHorizontal = area.widest + 1 > area.columns;
    }
    area.visible = std::max(1, static_cast<int>(std::floor((bottom - (area.showHorizontal ? thick : 0.0f) - area.textY) / area.line)));
    area.textW = std::max(0.0f, right - area.art - (area.showVertical ? thick : 0.0f) - area.textX);
    area.columns = std::max(1, static_cast<int>(std::floor(area.textW / area.cell)));
    // ui-terminal: a bar takes room from the text, which can make the other bar needed; each bar shows only when its content overflows, settled over two passes

    const float inset = barInset * area.art;
    if (area.showVertical) {
        area.verticalTrack = {right - thick, trackTop, thick, bottom - (area.showHorizontal ? thick : 0.0f) - trackTop};
        const float topInset = barTopInset * area.art;
        const float length = std::max(0.0f, area.verticalTrack.h - topInset - inset);
        const float thumb = std::min(length, std::max(2.0f * area.art, length * static_cast<float>(area.visible) / static_cast<float>(area.lineCount)));
        const int last = area.lineCount - area.visible;
        const float at = (length - thumb) * static_cast<float>(std::clamp(this->scroll, 0, last)) / static_cast<float>(last);
        area.verticalThumb = {area.verticalTrack.x + area.art, area.verticalTrack.y + topInset + at, area.art, thumb};
    }
    if (area.showHorizontal) {
        area.horizontalTrack = {this->panel.x, bottom - thick, this->panel.w - (area.showVertical ? thick : 0.0f), thick};
        const float length = std::max(0.0f, area.horizontalTrack.w - 2.0f * inset);
        const float thumb = std::min(length, std::max(2.0f * area.art, length * static_cast<float>(area.columns) / static_cast<float>(area.widest + 1)));
        const int last = area.widest + 1 - area.columns;
        const float at = (length - thumb) * static_cast<float>(std::clamp(this->scrollX, 0, last)) / static_cast<float>(last);
        area.horizontalThumb = {area.horizontalTrack.x + inset + at, area.horizontalTrack.y + area.art, thumb, area.art};
    }
    // ui-terminal: a thumb's length must follow the file size; each thumb is its track's length times the share of lines or columns on screen, at least 2 art pixels, placed by the scroll, 1 art pixel thick and inset as in the reference, 4 art pixels at the vertical bar's top and 2 at the other ends

    const std::size_t shown = this->names.size() + (this->naming ? 1 : 0);
    const bool hasMinus = !this->naming && !this->names.empty();
    const float room = this->panel.w - (plusWide + (hasMinus ? minusWide : 0.0f)) * area.art;
    std::vector<float> widths;
    float total = 0.0f;
    for (std::size_t i = 0; i < shown; ++i) {
        widths.push_back(std::max(tabWide * area.art, static_cast<float>(this->tabLabel(i).size() + 2) * area.cell));
        total += widths.back();
    }
    const float squeeze = total > room ? std::max(0.0f, room) / total : 1.0f;

    float x = this->panel.x;
    area.closeButton = {x, this->panel.y, closeWide * area.art, stripHigh * area.art};
    for (std::size_t i = 0; i < shown; ++i) {
        const float wide = std::max(3.0f, std::floor(widths[i] * squeeze / area.art)) * area.art;
        area.tabs.push_back({x, area.barY, wide, barHigh * area.art});
        x += wide;
        if (hasMinus && static_cast<int>(i) == this->tab) {
            area.minusButton = {x, area.barY, minusWide * area.art, barHigh * area.art};
            x += area.minusButton.w;
        }
    }
    area.plusButton = {x, area.barY, plusWide * area.art, barHigh * area.art};
    return area;
}
// ui-terminal: draw and mouse hits must use the same positions, in the reference's layout; from the panel top come the strip with the red button in its corner, the tab bar, a light band and as many rows as fit, each one glyph plus half the gap a 4 art pixel row leaves because the user found that gap too big (never under one glyph plus 2 text pixels), the gutter fits the highest 0-based row number (2 digits at least) plus ':', and the bar runs tabs (at least 24 art pixels, wide enough for their names, squeezed together when the bar fills up) with - right after the selected one, then +

void scriptEditor::keepCursorVisible(const editorLayout& area)
{
    if (this->row < this->scroll) this->scroll = this->row;
    if (this->row >= this->scroll + area.visible) this->scroll = this->row - area.visible + 1;
    if (this->buffers.empty()) return;
    const float cells = lineCells(this->lines()[this->row], this->col);
    if (cells < static_cast<float>(this->scrollX)) this->scrollX = static_cast<int>(std::floor(cells));
    if (cells >= static_cast<float>(this->scrollX + area.columns)) this->scrollX = static_cast<int>(std::ceil(cells)) - area.columns + 1;
}
// ui-terminal: the cursor must never be edited off screen; after a key both scrolls move just enough to put the cursor inside the visible rows and the visible cells of its line, whose spaces can be half a cell

void scriptEditor::selectTab(int index)
{
    this->tab = this->names.empty() ? 0 : std::clamp(index, 0, static_cast<int>(this->names.size()) - 1);
    this->row = 0;
    this->col = 0;
    this->scroll = 0;
    this->scrollX = 0;
    this->errorLine = -1;
    this->status.clear();
    this->completing = false;
}
// ui_editor: another script's error does not belong to this one, and typing skill names and args by hand was slow and error prone; switching tab puts the cursor and scroll at the top, clears the error line and status, and closes the autocomplete popup

void scriptEditor::insertText(const std::string& text)
{
    std::string& line = this->lines()[this->row];
    std::string printable;
    for (const char c : text)
        if (c >= 32 && c < 127) printable += c;
    line.insert(static_cast<std::size_t>(this->col), printable);
    this->col += static_cast<int>(printable.size());
}
// ui_editor: tabs are forbidden in scripts and the debug font is ASCII; only printable ASCII is inserted at the cursor, which moves past it

void scriptEditor::newLine()
{
    std::vector<std::string>& text = this->lines();
    std::string& line = text[this->row];
    const std::size_t indent = line.find_first_not_of(' ') == std::string::npos ? line.size() : line.find_first_not_of(' ');
    const std::string rest = std::string(std::min(indent, static_cast<std::size_t>(this->col)), ' ') + line.substr(static_cast<std::size_t>(this->col));
    const int newCol = static_cast<int>(std::min(indent, static_cast<std::size_t>(this->col)));
    line.erase(static_cast<std::size_t>(this->col));
    text.insert(text.begin() + this->row + 1, rest);
    ++this->row;
    this->col = newCol;
}
// ui_editor: blocks are set by indent so a new line should stay in the same block; Return splits the line at the cursor and the new line starts with the current line's leading spaces (no more than the cursor column)

void scriptEditor::backspace()
{
    std::vector<std::string>& text = this->lines();
    if (this->col > 0) {
        text[this->row].erase(static_cast<std::size_t>(this->col - 1), 1);
        --this->col;
        return;
    }
    if (this->row == 0) return;
    this->col = static_cast<int>(text[this->row - 1].size());
    text[this->row - 1] += text[this->row];
    text.erase(text.begin() + this->row);
    --this->row;
}

void scriptEditor::deleteForward()
{
    std::vector<std::string>& text = this->lines();
    if (static_cast<std::size_t>(this->col) < text[this->row].size()) {
        text[this->row].erase(static_cast<std::size_t>(this->col), 1);
        return;
    }
    if (this->row + 1 >= static_cast<int>(text.size())) return;
    text[this->row] += text[this->row + 1];
    text.erase(text.begin() + this->row + 1);
}
// ui_editor: lines are joined like any text editor; Backspace removes the char before the cursor or at column 0 joins the line onto the previous one, and Delete removes the char under it or at the end pulls up the next line

void scriptEditor::save()
{
    if (this->buffers.empty()) return;
    std::string text;
    for (const std::string& line : this->lines()) text += line + '\n';

    Script parsed;
    std::string error;
    this->errorLine = -1;
    if (parseScript(text, parsed, error)) {
        this->status = "saved";
    } else {
        this->status = error;
        if (error.compare(0, 5, "line ") == 0) {
            int number = 0;
            for (std::size_t i = 5; i < error.size() && error[i] >= '0' && error[i] <= '9'; ++i) number = number * 10 + (error[i] - '0');
            if (number > 0) this->errorLine = number - 1;
        }
    }

    const std::string& name = this->names[this->tab];
    for (const std::string& dir : {this->sourceDir, this->buildDir}) {
        const std::string path = dir + "/" + name + ".txt";
        std::ofstream file(path, std::ios::binary);
        file << text;
        file.close();
        if (!file) this->status = "cannot write " + path;
    }
    this->saved = true;
}
// ui_editor: scripts are edited in game so npc behavior can change without leaving it, and the change must survive a rebuild; save checks the text with parseScript (error row from "line N: msg"), writes it to the source and build dirs even when broken, and sets the saved flag

void scriptEditor::refreshSuggestions(bool force)
{
    this->completing = false;
    if (this->buffers.empty()) return;
    this->suggestions = suggest(this->lines()[this->row], this->col, force, this->wordStart);
    this->picked = 0;
    this->pickedScroll = 0;
    this->completing = !this->suggestions.empty();
}

void scriptEditor::acceptSuggestion()
{
    this->completing = false;
    if (this->buffers.empty() || this->picked < 0 || this->picked >= static_cast<int>(this->suggestions.size())) return;
    const Suggestion& choice = this->suggestions[this->picked];
    std::string& line = this->lines()[this->row];
    const int start = std::clamp(this->wordStart, 0, this->col);
    line.replace(static_cast<std::size_t>(start), static_cast<std::size_t>(this->col - start), choice.insert);
    this->col = start + static_cast<int>(choice.insert.size()) - choice.cursorBack;
}
// ui_editor: typing skill names and args by hand was slow and error prone; refreshSuggestions reruns suggest at the cursor (picked row and list scroll back to the top) and opens the popup only on a match, and acceptSuggestion replaces the typed word with the picked insert text and steps the cursor back by its cursorBack

void scriptEditor::closeTab()
{
    if (this->names.empty()) return;
    this->closedNames.push_back(std::move(this->names[this->tab]));
    this->closedBuffers.push_back(std::move(this->buffers[this->tab]));
    this->names.erase(this->names.begin() + this->tab);
    this->buffers.erase(this->buffers.begin() + this->tab);
    this->selectTab(this->tab);
}
// ui-terminal: the - button must not lose the file or its edits; closeTab moves the current tab's name and buffer to the closed lists, where the next open finds them, and selects the tab now in its place

void scriptEditor::startNaming()
{
    this->completing = false;
    this->status.clear();
    this->typedName.clear();
    this->naming = true;
}

void scriptEditor::createScript()
{
    const auto taken = [this](const std::string& name) {
        std::error_code error;
        if (std::find(this->names.begin(), this->names.end(), name) != this->names.end()) return true;
        if (std::find(this->closedNames.begin(), this->closedNames.end(), name) != this->closedNames.end()) return true;
        return std::filesystem::exists(this->sourceDir + "/" + name + ".txt", error) || std::filesystem::exists(this->buildDir + "/" + name + ".txt", error);
    };
    if (this->typedName.empty()) {
        this->status = "name is empty";
        return;
    }
    if (taken(this->typedName)) {
        this->status = this->typedName + " is taken";
        return;
    }
    for (const std::string& dir : {this->sourceDir, this->buildDir}) {
        const std::string path = dir + "/" + this->typedName + ".txt";
        std::ofstream file(path, std::ios::binary);
        file.close();
        if (!file) {
            this->status = "cannot write " + path;
            return;
        }
    }
    this->names.push_back(this->typedName);
    this->buffers.push_back({std::string()});
    this->naming = false;
    this->selectTab(static_cast<int>(this->names.size()) - 1);
    this->status = "created";
    this->saved = true;
}
// ui-terminal: the + button makes a script without overwriting one; startNaming opens the name tab, and createScript refuses an empty name or one already open, parked or on disk (the reason shows in the strip and the name tab stays), then writes the empty .txt to both dirs, selects its tab and sets the saved flag so main.cpp loads it

bool scriptEditor::handleKey(const SDL_KeyboardEvent& key, float density)
{
    const SDL_Keycode code = key.key;
    if (this->naming) {
        if (code == SDLK_ESCAPE) this->naming = false;
        else if (code == SDLK_RETURN || code == SDLK_KP_ENTER) this->createScript();
        else if (code == SDLK_BACKSPACE && !this->typedName.empty()) this->typedName.pop_back();
        return true;
    }
    // ui-terminal: keys belong to the name while the new tab is open; Esc cancels it, Enter creates the script, Backspace edits the name, and every other key is ignored
    if (code == SDLK_ESCAPE && this->completing) {
        this->completing = false;
        return true;
    }
    if (code == SDLK_ESCAPE) {
        this->close();
        return true;
    }
    if ((key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0 && code == SDLK_S) {
        this->save();
        return true;
    }
    if (this->buffers.empty()) return true;
    if ((key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0 && code == SDLK_SPACE) {
        this->refreshSuggestions(true);
        return true;
    }
    if (this->completing && (code == SDLK_UP || code == SDLK_DOWN)) {
        const int count = static_cast<int>(this->suggestions.size());
        this->picked = std::clamp(this->picked + (code == SDLK_UP ? -1 : 1), 0, count - 1);
        this->pickedScroll = std::clamp(this->pickedScroll, std::max(0, this->picked - 7), this->picked);
        return true;
    }
    if (this->completing && code == SDLK_TAB) {
        this->acceptSuggestion();
        this->keepCursorVisible(this->layout(textScale(density)));
        return true;
    }
    // ui_editor: typing skill names and args by hand was slow and error prone; while the popup is open Escape closes only it, Up/Down move the picked row (clamped, list scrolled to keep it in the 8 rows) and Tab accepts it, and Ctrl or Cmd+Space opens it even for an empty word

    std::vector<std::string>& text = this->lines();
    const int last = static_cast<int>(text.size()) - 1;
    const int visible = this->layout(textScale(density)).visible;
    const auto length = [&text](int at) { return static_cast<int>(text[at].size()); };
    switch (code) {
    case SDLK_LEFT:
        if (this->col > 0) --this->col;
        else if (this->row > 0) this->col = length(--this->row);
        this->completing = false;
        break;
    case SDLK_RIGHT:
        if (this->col < length(this->row)) ++this->col;
        else if (this->row < last) {
            ++this->row;
            this->col = 0;
        }
        this->completing = false;
        break;
    case SDLK_UP: this->row = std::max(0, this->row - 1); break;
    case SDLK_DOWN: this->row = std::min(last, this->row + 1); break;
    case SDLK_HOME:
        this->col = 0;
        this->completing = false;
        break;
    case SDLK_END:
        this->col = length(this->row);
        this->completing = false;
        break;
    case SDLK_PAGEUP:
        this->row = std::max(0, this->row - visible);
        this->completing = false;
        break;
    case SDLK_PAGEDOWN:
        this->row = std::min(last, this->row + visible);
        this->completing = false;
        break;
    case SDLK_BACKSPACE:
        this->backspace();
        this->refreshSuggestions(false);
        break;
    case SDLK_DELETE: this->deleteForward(); break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        this->newLine();
        this->completing = false;
        break;
    case SDLK_TAB: this->insertText("  "); break;
    default: break;
    }
    this->col = std::min(this->col, length(this->row));
    this->keepCursorVisible(this->layout(textScale(density)));
    return true;
}
// ui_editor: the game must not react to typing while the editor is open, and typing skill names and args by hand was slow and error prone; Escape closes it (F1 no longer does, so every key but Escape types), Ctrl or Cmd+S saves, arrows, Home/End and PageUp/PageDown (one screen of rows) move the cursor and close the popup, Backspace reruns the suggestions, Delete and Return (closes the popup) edit, Tab with the popup closed inserts 2 spaces, and every key is consumed

bool scriptEditor::handleMouse(const SDL_Event& event, float density)
{
    float x = 0.0f;
    float y = 0.0f;
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        x = event.motion.x;
        y = event.motion.y;
    } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        x = event.wheel.mouse_x;
        y = event.wheel.mouse_y;
    } else {
        x = event.button.x;
        y = event.button.y;
    }
    const Vector2D<float> pixel = {x * density, y * density};
    if (this->dragging != editorDrag::none) {
        if (event.type == SDL_EVENT_MOUSE_MOTION) this->dragTo(pixel, density);
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) this->dragging = editorDrag::none;
        return true;
    }
    // ui-terminal: a drag must not stop when the mouse leaves the panel; while one runs every mouse event belongs to it, and the left button's release ends it

    const int edges = this->edgesAt(pixel);
    this->setEdgeCursor(edges);
    if (edges != 0) {
        this->hover = editorButton::none;
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
            this->completing = false;
            this->naming = false;
            this->dragging = editorDrag::resize;
            this->dragEdges = edges;
            this->dragStart = pixel;
            this->panelStart = this->panel;
        }
        return true;
    }
    // ui-terminal: the panel resizes from any edge or corner; the mouse within 1 art pixel of an edge gets a resize cursor, and a left press there starts the drag

    if (!this->contains(pixel)) {
        this->hover = editorButton::none;
        return false;
    }

    editorLayout area = this->layout(textScale(density));
    this->hover = editorButton::none;
    if (inRect(pixel, area.closeButton)) this->hover = editorButton::close;
    else if (inRect(pixel, area.minusButton)) this->hover = editorButton::minus;
    else if (inRect(pixel, area.plusButton)) this->hover = editorButton::plus;
    // ui-terminal: buttons sit bright and go dim while hovered; every mouse event in the panel records the button under it, and one outside clears it
    const int count = this->buffers.empty() ? 1 : static_cast<int>(this->lines().size());
    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        this->wheel += event.wheel.y;
        const int notches = static_cast<int>(this->wheel);
        this->wheel -= static_cast<float>(notches);
        this->scroll = std::clamp(this->scroll - 3 * notches, 0, std::max(0, count - area.visible));
        this->wheelX += event.wheel.x;
        const int notchesX = static_cast<int>(this->wheelX);
        this->wheelX -= static_cast<float>(notchesX);
        this->scrollX = std::clamp(this->scrollX + 3 * notchesX, 0, std::max(0, area.widest + 1 - area.columns));
        return true;
    }
    // ui-terminal: long lines scroll sideways too; the wheel's up and down moves 3 rows per notch and its left and right 3 columns per notch, each stopping where its content ends
    if (event.type != SDL_EVENT_MOUSE_BUTTON_DOWN || event.button.button != SDL_BUTTON_LEFT) return true;
    this->completing = false;
    this->naming = false;
    area = this->layout(textScale(density));
    // ui_editor: typing skill names and args by hand was slow and error prone; a left click inside the panel moves away from the typed word, so it closes the autocomplete popup and drops a name being typed

    if (inRect(pixel, area.closeButton)) {
        this->close();
        return true;
    }
    if (inRect(pixel, area.verticalTrack)) {
        const bool onThumb = inRect(pixel, area.verticalThumb);
        this->dragging = editorDrag::verticalBar;
        this->dragGrab = onThumb ? pixel.y - area.verticalThumb.y : area.verticalThumb.h / 2.0f;
        if (!onThumb) this->dragTo(pixel, density);
        return true;
    }
    if (inRect(pixel, area.horizontalTrack)) {
        const bool onThumb = inRect(pixel, area.horizontalThumb);
        this->dragging = editorDrag::horizontalBar;
        this->dragGrab = onThumb ? pixel.x - area.horizontalThumb.x : area.horizontalThumb.w / 2.0f;
        if (!onThumb) this->dragTo(pixel, density);
        return true;
    }
    // ui-terminal: a bar moves by its thumb or a click on its track; a press on the thumb grabs it where it was pressed, and one on the track centers the thumb there and keeps dragging
    if (pixel.y < area.barY) {
        this->dragging = editorDrag::resize;
        this->dragEdges = edgeAll;
        this->dragStart = pixel;
        this->panelStart = this->panel;
        return true;
    }
    // ui-terminal: the user wants to move the panel by its top bar; a left press on the dark strip, off the red button, starts a drag of all four edges together
    if (pixel.y >= area.barY && pixel.y < area.barY + area.plusButton.h) {
        if (inRect(pixel, area.minusButton)) {
            this->closeTab();
        } else if (inRect(pixel, area.plusButton)) {
            this->startNaming();
        } else {
            for (std::size_t i = 0; i < area.tabs.size(); ++i) {
                if (inRect(pixel, area.tabs[i])) this->selectTab(static_cast<int>(i));
            }
        }
        return true;
    }

    const float bottom = area.textY + static_cast<float>(area.visible) * area.line;
    if (this->buffers.empty() || pixel.y < area.textY || pixel.y >= bottom || pixel.x < area.gutterX) return true;
    this->row = std::clamp(this->scroll + static_cast<int>(std::floor((pixel.y - area.textY) / area.line)), 0, count - 1);
    const int length = static_cast<int>(this->lines()[this->row].size());
    this->col = std::clamp(colAtCells(this->lines()[this->row], static_cast<float>(this->scrollX) + (pixel.x - area.textX) / area.cell), 0, length);
    return true;
}
// ui-terminal: the tab bar now holds buttons and clicks on the panel must not reach the map; mouse events inside the panel are consumed, a left click on the red button closes the Terminal, on - closes the tab, on + starts a new script, on a tab switches to it, and one in the text puts the cursor on the nearest row and column

int scriptEditor::edgesAt(Vector2D<float> pixel) const
{
    const float band = this->artScale;
    const float right = this->panel.x + this->panel.w;
    const float bottom = this->panel.y + this->panel.h;
    if (pixel.x < this->panel.x - band || pixel.x >= right + band || pixel.y < this->panel.y - band || pixel.y >= bottom + band) return 0;
    int edges = 0;
    if (pixel.x < this->panel.x + band) edges |= edgeLeft;
    if (pixel.x >= right - band) edges |= edgeRight;
    if (pixel.y < this->panel.y + band) edges |= edgeTop;
    if (pixel.y >= bottom - band) edges |= edgeBottom;
    return edges;
}

void scriptEditor::setEdgeCursor(int edges)
{
    if (edges == this->cursorEdges) return;
    this->cursorEdges = edges;
    SDL_SystemCursor shape = SDL_SYSTEM_CURSOR_DEFAULT;
    if (edges == (edgeLeft | edgeTop) || edges == (edgeRight | edgeBottom)) shape = SDL_SYSTEM_CURSOR_NWSE_RESIZE;
    else if (edges == (edgeRight | edgeTop) || edges == (edgeLeft | edgeBottom)) shape = SDL_SYSTEM_CURSOR_NESW_RESIZE;
    else if (edges == edgeLeft || edges == edgeRight) shape = SDL_SYSTEM_CURSOR_EW_RESIZE;
    else if (edges == edgeTop || edges == edgeBottom) shape = SDL_SYSTEM_CURSOR_NS_RESIZE;
    static SDL_Cursor* cursors[SDL_SYSTEM_CURSOR_COUNT] = {};
    if (!cursors[shape]) cursors[shape] = SDL_CreateSystemCursor(shape);
    if (cursors[shape]) SDL_SetCursor(cursors[shape]);
}
// ui-terminal: resize handles are invisible, so the cursor shows them; edgesAt gives the edge bits for a point within 1 art pixel of the panel's edge, inside or out, and setEdgeCursor switches to the matching system cursor only when the bits change, keeping one cursor per shape for the whole run

void scriptEditor::dragTo(Vector2D<float> pixel, float density)
{
    const float scale = textScale(density);
    if (this->dragging == editorDrag::resize && this->dragEdges == edgeAll) {
        this->panel.x = snapTo(std::clamp(this->panelStart.x + pixel.x - this->dragStart.x, 0.0f, std::max(0.0f, this->viewportSize.x - this->panel.w)), scale);
        this->panel.y = snapTo(std::clamp(this->panelStart.y + pixel.y - this->dragStart.y, 0.0f, std::max(0.0f, this->viewportSize.y - this->panel.h)), scale);
        this->resized = true;
        return;
    }
    // ui-terminal: moving must keep the whole panel reachable; with all edges dragged the corner follows the mouse, its size stays, and the panel stops at the screen's edges
    if (this->dragging == editorDrag::resize) {
        const float least = this->artScale;
        const float right = this->panelStart.x + this->panelStart.w;
        const float bottom = this->panelStart.y + this->panelStart.h;
        if (this->dragEdges & edgeLeft) {
            this->panel.x = snapTo(std::clamp(this->panelStart.x + pixel.x - this->dragStart.x, 0.0f, std::max(0.0f, right - minWide * least)), scale);
            this->panel.w = right - this->panel.x;
        }
        if (this->dragEdges & edgeRight) this->panel.w = std::clamp(this->panelStart.w + pixel.x - this->dragStart.x, std::min(minWide * least, this->viewportSize.x - this->panel.x), std::max(minWide * least, this->viewportSize.x - this->panel.x));
        if (this->dragEdges & edgeTop) {
            this->panel.y = snapTo(std::clamp(this->panelStart.y + pixel.y - this->dragStart.y, 0.0f, std::max(0.0f, bottom - minHigh * least)), scale);
            this->panel.h = bottom - this->panel.y;
        }
        if (this->dragEdges & edgeBottom) this->panel.h = std::clamp(this->panelStart.h + pixel.y - this->dragStart.y, std::min(minHigh * least, this->viewportSize.y - this->panel.y), std::max(minHigh * least, this->viewportSize.y - this->panel.y));
        this->resized = true;
        return;
    }
    // ui-terminal: a resize must stay usable and on screen; the dragged edges follow the mouse from where the drag began, the opposite edges stay put, and the panel stays between 64x40 art pixels and the screen

    const editorLayout area = this->layout(scale);
    if (this->dragging == editorDrag::verticalBar && area.showVertical) {
        const float range = area.verticalTrack.h - (barTopInset + barInset) * area.art - area.verticalThumb.h;
        const float at = pixel.y - this->dragGrab - area.verticalTrack.y - barTopInset * area.art;
        const int last = area.lineCount - area.visible;
        this->scroll = range > 0.0f ? std::clamp(static_cast<int>(std::round(at / range * static_cast<float>(last))), 0, last) : 0;
    }
    if (this->dragging == editorDrag::horizontalBar && area.showHorizontal) {
        const float range = area.horizontalTrack.w - 2.0f * barInset * area.art - area.horizontalThumb.w;
        const float at = pixel.x - this->dragGrab - area.horizontalTrack.x - barInset * area.art;
        const int last = area.widest + 1 - area.columns;
        this->scrollX = range > 0.0f ? std::clamp(static_cast<int>(std::round(at / range * static_cast<float>(last))), 0, last) : 0;
    }
}
// ui-terminal: dragging a thumb must move the text; dragTo turns the thumb's place along its track into the first row or column shown, from the first at the track's start to the last that still fills the panel at its end

bool scriptEditor::drawScrollbars(SDL_Renderer* renderer, const editorLayout& area) const
{
    if (area.showVertical) {
        if (!fillArt(renderer, area.verticalTrack.x, area.verticalTrack.y, 1.0f, 0.0f, 0.0f, area.verticalTrack.w, area.verticalTrack.h, termBand)) return false;
        if (!fillArt(renderer, area.verticalThumb.x, area.verticalThumb.y, 1.0f, 0.0f, 0.0f, area.verticalThumb.w, area.verticalThumb.h, termBar)) return false;
    }
    if (area.showHorizontal) {
        if (!fillArt(renderer, area.horizontalTrack.x, area.horizontalTrack.y, 1.0f, 0.0f, 0.0f, area.horizontalTrack.w, area.horizontalTrack.h, termBand)) return false;
        if (!fillArt(renderer, area.horizontalThumb.x, area.horizontalThumb.y, 1.0f, 0.0f, 0.0f, area.horizontalThumb.w, area.horizontalThumb.h, termBar)) return false;
    }
    if (area.showVertical && area.showHorizontal) {
        const float thick = barThick * area.art;
        return fillArt(renderer, this->panel.x + this->panel.w - thick, this->panel.y + this->panel.h - thick, 1.0f, 0.0f, 0.0f, thick, thick, termCorner);
    }
    return true;
}
// ui-terminal: the bars must match the reference and stay away by default; each needed bar is a light track with a grey thumb, and the corner is darker when both show

bool scriptEditor::drawTabBar(SDL_Renderer* renderer, const editorLayout& area) const
{
    const float art = area.art;
    if (!fillArt(renderer, this->panel.x, area.barY, 1.0f, 0.0f, 0.0f, this->panel.w, barHigh * art, termBar)) return false;

    const SDL_FRect& close = area.closeButton;
    const bool closeHovered = this->hover == editorButton::close;
    if (!fillArt(renderer, close.x, close.y, art, 1.0f, 1.0f, 3.0f, 3.0f, closeHovered ? termRedDim : termRed)) return false;
    if (!fillArt(renderer, close.x, close.y, art, 2.0f, 2.0f, 1.0f, 1.0f, closeHovered ? termRedDimCore : termRedCore)) return false;

    const int active = this->naming ? static_cast<int>(this->names.size()) : this->tab;
    for (std::size_t i = 0; i < area.tabs.size(); ++i) {
        const SDL_FRect& box = area.tabs[i];
        const float wide = box.w / art;
        if (static_cast<int>(i) == active) {
            if (!fillArt(renderer, box.x, box.y, art, 0.0f, 0.0f, wide, barHigh, termEdge)) return false;
            if (!fillArt(renderer, box.x, box.y, art, 2.0f, 0.0f, wide - 4.0f, 1.0f, termTabOn)) return false;
            if (!fillArt(renderer, box.x, box.y, art, 1.0f, 1.0f, wide - 2.0f, 1.0f, termTabOn)) return false;
            if (!fillArt(renderer, box.x, box.y, art, 1.0f, 2.0f, wide - 2.0f, 3.0f, editorWhite)) return false;
            if (!fillArt(renderer, box.x, box.y, art, 1.0f, 5.0f, wide - 2.0f, 1.0f, termTabOn)) return false;
            if (!fillArt(renderer, box.x, box.y, art, 2.0f, 6.0f, wide - 4.0f, 1.0f, termTabOn)) return false;
            continue;
        }
        if (!fillArt(renderer, box.x, box.y, art, 0.0f, 0.0f, wide - 1.0f, barHigh, termTabOff)) return false;
        if (!fillArt(renderer, box.x, box.y, art, 0.0f, 2.0f, wide - 1.0f, 3.0f, editorWhite)) return false;
        if (!fillArt(renderer, box.x, box.y, art, wide - 1.0f, 0.0f, 1.0f, barHigh, termEdge)) return false;
    }

    if (area.minusButton.w > 0.0f) {
        const SDL_FRect& minus = area.minusButton;
        if (!fillArt(renderer, minus.x, minus.y, art, 0.0f, 0.0f, minusWide, barHigh, termEdge)) return false;
        if (!fillArt(renderer, minus.x, minus.y, art, 2.0f, 3.0f, 3.0f, 1.0f, this->hover == editorButton::minus ? termCorner : editorWhite)) return false;
    }

    const SDL_FRect& plus = area.plusButton;
    const SDL_Color glyph = this->hover == editorButton::plus ? termCorner : editorWhite;
    if (!fillArt(renderer, plus.x, plus.y, art, 2.0f, 3.0f, 3.0f, 1.0f, glyph)) return false;
    return fillArt(renderer, plus.x, plus.y, art, 3.0f, 2.0f, 1.0f, 3.0f, glyph);
}
// ui-terminal: the tab bar must match the updated reference; drawTabBar fills the 7 pixel grey bar, the red square with its darker center in the strip's corner (dim while hovered), each tab (selected: blue edge with cut corners between dark sides, others: grey edge with a dark right side, all with 3 white rows), the - tile after the selected tab and the + glyph, both glyphs 3 pixels wide, white at rest and #d9d9d9 while hovered

bool scriptEditor::drawContents(SDL_Renderer* renderer, float density)
{
    const float scale = textScale(density);
    const editorLayout area = this->layout(scale);
    const int count = this->buffers.empty() ? 0 : static_cast<int>(this->lines().size());
    this->scroll = std::clamp(this->scroll, 0, std::max(0, count - area.visible));
    this->scrollX = std::clamp(this->scrollX, 0, std::max(0, area.widest + 1 - area.columns));

    if (!SDL_SetRenderScale(renderer, 1.0f, 1.0f)) return false;
    if (!SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND)) return false;
    const SDL_Rect whole = {static_cast<int>(this->panel.x), static_cast<int>(this->panel.y), static_cast<int>(this->panel.w), static_cast<int>(this->panel.h)};
    if (!SDL_SetRenderClipRect(renderer, &whole)) return false;
    if (!fillArt(renderer, this->panel.x, this->panel.y, 1.0f, 0.0f, 0.0f, this->panel.w, this->panel.h, editorWhite)) return false;
    if (!fillArt(renderer, this->panel.x, this->panel.y, 1.0f, 0.0f, 0.0f, this->panel.w, stripHigh * area.art, termStrip)) return false;
    if (!fillArt(renderer, this->panel.x, area.barY + barHigh * area.art, 1.0f, 0.0f, 0.0f, this->panel.w, bandHigh * area.art, termBand)) return false;
    if (!this->drawTabBar(renderer, area)) return false;
    if (!this->drawScrollbars(renderer, area)) return false;
    // ui-terminal: the panel can be any size and the user dropped the row dividers; an opaque white panel, the dark strip on top, the tab bar, the light band and the needed scrollbars fill the panel's width and height, clipped to it, with nothing between rows

    if (!SDL_SetRenderScale(renderer, scale, scale)) return false;
    const SDL_Rect inner = {static_cast<int>(this->panel.x / scale), static_cast<int>(this->panel.y / scale), static_cast<int>(this->panel.w / scale), static_cast<int>(this->panel.h / scale)};
    const float tabTextY = snapTo(area.barY + (barHigh * area.art - area.cell) / 2.0f, scale);
    for (std::size_t i = 0; i < area.tabs.size(); ++i) {
        const SDL_FRect& box = area.tabs[i];
        const std::string& label = this->tabLabel(i);
        const float clipRight = std::min(box.x + box.w - area.art, this->panel.x + this->panel.w);
        if (clipRight <= box.x + area.art) continue;
        const SDL_Rect clip = {static_cast<int>((box.x + area.art) / scale), static_cast<int>(box.y / scale), static_cast<int>((clipRight - box.x - area.art) / scale), static_cast<int>(box.h / scale)};
        if (!SDL_SetRenderClipRect(renderer, &clip)) return false;
        const float labelX = snapTo(box.x + area.cell, scale);
        const bool selected = this->naming ? i == this->names.size() : static_cast<int>(i) == this->tab;
        if (!drawText(renderer, labelX, tabTextY, scale, label, selected ? editorBlack : editorGray)) return false;
        if (this->naming && i == this->names.size()) {
            const SDL_FRect caret = {labelX / scale + static_cast<float>(label.size()) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE, tabTextY / scale, 1.0f, SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE};
            if (!SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)) return false;
            if (!SDL_RenderFillRect(renderer, &caret)) return false;
        }
    }
    if (!SDL_SetRenderClipRect(renderer, &inner)) return false;

    const int end = std::min(count, this->scroll + area.visible);
    for (int i = this->scroll; i < end; ++i) {
        std::string number = std::to_string(i);
        number.insert(0, static_cast<std::size_t>(std::max(0, area.digits - static_cast<int>(number.size()))), '0');
        const float y = snapTo(area.textY + static_cast<float>(i - this->scroll) * area.line + area.textOffset, scale);
        if (!drawText(renderer, area.gutterX, y, scale, number + ":", i == this->errorLine ? editorRed : editorBlack)) return false;
    }

    if (!this->status.empty() && !drawText(renderer, snapTo(this->panel.x + closeWide * area.art, scale), area.statusY, scale, this->status, editorWhite)) return false;
    // ui-terminal: names, row numbers and the save result are drawn at the text scale; each tab's label is cut at its white edges (a name being typed shows a caret), selected black else #858585, numbers are 0-based and zero-padded as 00: in black with the error row in #f44747, and the status is white in the dark strip

    const SDL_Rect textArea = {static_cast<int>(area.textX / scale), static_cast<int>(area.textY / scale), static_cast<int>(area.textW / scale), static_cast<int>(static_cast<float>(area.visible) * area.line / scale)};
    if (!SDL_SetRenderClipRect(renderer, &textArea)) return false;
    for (int i = this->scroll; i < end; ++i) {
        const std::string& line = this->lines()[i];
        const float left = area.textX - static_cast<float>(this->scrollX) * area.cell;
        const float y = snapTo(area.textY + static_cast<float>(i - this->scroll) * area.line + area.textOffset, scale);
        std::size_t at = 0;
        for (const EditorToken& token : colorLine(line)) {
            std::size_t start = token.text.find_first_not_of(' ');
            while (start != std::string::npos) {
                std::size_t stop = token.text.find(' ', start);
                if (stop == std::string::npos) stop = token.text.size();
                const float x = left + lineCells(line, static_cast<int>(at + start)) * area.cell;
                if (!drawText(renderer, x, y, scale, token.text.substr(start, stop - start), token.color)) return false;
                start = token.text.find_first_not_of(' ', stop);
            }
            at += token.text.size();
        }
    }
    if (!SDL_SetRenderClipRect(renderer, nullptr)) return false;
    // ui-terminal: long lines must scroll sideways without covering the row numbers, and spaces can be half a cell; each visible row is drawn word by word in its token's syntax color, placed by lineCells from the first shown cell, clipped to the text area

    const float cursorCells = count > 0 ? lineCells(this->lines()[this->row], this->col) : 0.0f;
    const float cursorX = area.textX + (cursorCells - static_cast<float>(this->scrollX)) * area.cell;
    const bool visible = count > 0 && this->row >= this->scroll && this->row < end && cursorCells >= static_cast<float>(this->scrollX) && cursorX + scale <= area.textX + area.textW;
    if (!visible) return true;
    const SDL_FRect cursor = {cursorX / scale, snapTo(area.textY + static_cast<float>(this->row - this->scroll) * area.line + area.textOffset, scale) / scale, 1.0f, SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE};
    if (!SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)) return false;
    if (!SDL_RenderFillRect(renderer, &cursor)) return false;
    return this->drawSuggestions(renderer, scale, area);
}
// ui-terminal: a white caret cannot be seen on the white rows; drawn at the text scale as a black bar 1 x scale px wide and one glyph tall at the cursor column, centered in its row, hidden when scrolled or clipped away, with the autocomplete popup drawn over everything after it

bool scriptEditor::drawSuggestions(SDL_Renderer* renderer, float scale, const editorLayout& area)
{
    if (!this->completing || this->suggestions.empty()) return true;
    const int count = static_cast<int>(this->suggestions.size());
    const int rows = std::min(8, count);
    this->picked = std::clamp(this->picked, 0, count - 1);
    this->pickedScroll = std::clamp(this->pickedScroll, std::max(0, this->picked - rows + 1), std::min(this->picked, count - rows));

    std::size_t widest = 0;
    for (const Suggestion& choice : this->suggestions) widest = std::max(widest, choice.text.size());
    const float w = static_cast<float>(widest + 2) * area.cell + 2.0f * area.art;
    const float h = static_cast<float>(rows) * area.popupLine + 2.0f * area.art;
    const float lineY = area.textY + static_cast<float>(this->row - this->scroll) * area.line;
    const float bottom = area.textY + static_cast<float>(area.visible) * area.line;
    float y = lineY + area.line;
    if (y + h > bottom) y = lineY - h;
    y = snapTo(std::max(y, this->panel.y), scale);
    // ui-terminal: the popup's own rows differ from the script's; its rows use popupLine, and it opens under or over the cursor row on a whole text pixel
    const float minX = this->panel.x + area.pad;
    const float maxX = std::floor((this->panel.x + this->panel.w - area.pad - w) / scale) * scale;
    const float wordCells = lineCells(this->lines()[this->row], this->wordStart);
    const float x = snapTo(std::max(minX, std::min(area.textX + (wordCells - static_cast<float>(this->scrollX)) * area.cell, maxX)), scale);

    const SDL_Rect inner = {static_cast<int>(this->panel.x / scale), static_cast<int>(this->panel.y / scale), static_cast<int>(this->panel.w / scale), static_cast<int>(this->panel.h / scale)};
    if (!SDL_SetRenderClipRect(renderer, &inner)) return false;
    const SDL_FRect box = {x / scale, y / scale, w / scale, h / scale};
    if (!SDL_SetRenderDrawColor(renderer, termListEdge.r, termListEdge.g, termListEdge.b, termListEdge.a)) return false;
    if (!SDL_RenderFillRect(renderer, &box)) return false;
    const SDL_FRect body = {(x + area.art) / scale, (y + area.art) / scale, (w - 2.0f * area.art) / scale, (h - 2.0f * area.art) / scale};
    if (!SDL_SetRenderDrawColor(renderer, editorWhite.r, editorWhite.g, editorWhite.b, editorWhite.a)) return false;
    if (!SDL_RenderFillRect(renderer, &body)) return false;
    for (int i = 0; i < rows; ++i) {
        const int index = this->pickedScroll + i;
        const float rowY = y + area.art + static_cast<float>(i) * area.popupLine;
        if (index == this->picked) {
            const SDL_FRect highlight = {(x + area.art) / scale, rowY / scale, (w - 2.0f * area.art) / scale, area.popupLine / scale};
            if (!SDL_SetRenderDrawColor(renderer, termCorner.r, termCorner.g, termCorner.b, termCorner.a)) return false;
            if (!SDL_RenderFillRect(renderer, &highlight)) return false;
        }
        const Suggestion& choice = this->suggestions[index];
        if (!drawText(renderer, snapTo(x + area.art + area.cell, scale), snapTo(rowY + scale, scale), scale, choice.text, choice.color)) return false;
    }
    return SDL_SetRenderClipRect(renderer, nullptr);
}
// ui-terminal: the autocomplete list must match the reference; it is a box with a 1 art pixel #222034 edge and a white body, under the cursor line (above it when it would pass the text area bottom), left at the typed word start and clamped inside the panel, showing up to 8 suggestions in their syntax colors from the list scroll with the picked row on #d9d9d9

void scriptEditor::open(SDL_Window* window, const std::vector<std::string>& names, const std::string& sourceDir, const std::string& buildDir)
{
    std::vector<std::string> sorted = names;
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    const std::string current = this->tab < static_cast<int>(this->names.size()) ? this->names[this->tab] : "";

    std::vector<std::vector<std::string>> kept;
    int index = -1;
    for (const std::string& name : sorted) {
        if (name == current) index = static_cast<int>(kept.size());
        const auto old = std::find(this->names.begin(), this->names.end(), name);
        const auto parked = std::find(this->closedNames.begin(), this->closedNames.end(), name);
        if (old != this->names.end()) kept.push_back(std::move(this->buffers[old - this->names.begin()]));
        else if (parked != this->closedNames.end()) kept.push_back(std::move(this->closedBuffers[parked - this->closedNames.begin()]));
        else kept.push_back(readLines(buildDir + "/" + name + ".txt"));
    }
    this->names = std::move(sorted);
    this->buffers = std::move(kept);
    this->closedNames.clear();
    this->closedBuffers.clear();
    // ui-terminal: a tab closed with - comes back with its edits; open takes its buffer from the closed lists, then empties them

    if (index < 0) {
        this->selectTab(0);
    } else {
        this->tab = index;
        this->row = std::clamp(this->row, 0, static_cast<int>(this->lines().size()) - 1);
        this->col = std::clamp(this->col, 0, static_cast<int>(this->lines()[this->row].size()));
    }
    this->window = window;
    this->sourceDir = sourceDir;
    this->buildDir = buildDir;
    this->opened = true;
    if (window) SDL_StartTextInput(window);
}
// ui_editor: scripts are edited in game so npc behavior can change without leaving it, and unsaved edits must survive closing the pop-up; open keeps the buffer of every name it already had, reads <buildDir>/<name>.txt for new ones, sorts the tabs, stays on the current tab if it still exists and starts text input

void scriptEditor::close()
{
    if (this->window) SDL_StopTextInput(this->window);
    this->opened = false;
    this->completing = false;
    this->naming = false;
    this->hover = editorButton::none;
    this->dragging = editorDrag::none;
    this->setEdgeCursor(0);
}

bool scriptEditor::isOpen() const { return this->opened; }
// ui-terminal: typing must go back to being game keys after closing; close stops text input and hides the panel, the autocomplete popup and a name being typed, clears the hovered button, any drag and the resize cursor, and keeps every buffer and the panel's size for the next open

bool scriptEditor::handleEvent(const SDL_Event& event, float density)
{
    if (!this->opened) return false;
    switch (event.type) {
    case SDL_EVENT_TEXT_INPUT:
        if (this->naming) {
            for (const char* c = event.text.text; *c != '\0'; ++c) {
                if ((isWordChar(*c) || *c == '-') && this->typedName.size() < 32) this->typedName += *c;
            }
            return true;
        }
        // ui-terminal: a script name becomes a file name; while the new tab is open typed letters, digits, '_' and '-' go to the name, up to 32 of them
        if (!this->buffers.empty()) {
            this->insertText(event.text.text);
            this->refreshSuggestions(false);
            this->keepCursorVisible(this->layout(textScale(density)));
        }
        // ui_editor: typing skill names and args by hand was slow and error prone; every text input reruns the suggestions for the word before the cursor, opening the popup on a match and closing it otherwise
        return true;
    case SDL_EVENT_KEY_DOWN: return this->handleKey(event.key, density);
    case SDL_EVENT_KEY_UP: return true;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_MOTION:
    case SDL_EVENT_MOUSE_WHEEL: return this->handleMouse(event, density);
    default: return false;
    }
}
// ui_editor: main.cpp skips its own handling for an event the editor used; while open, text and every key are consumed, mouse events only inside the panel, and nothing when closed

bool scriptEditor::takeSaved()
{
    const bool was = this->saved;
    this->saved = false;
    return was;
}
// ui_editor: main.cpp reloads scripts once per save; takeSaved returns the saved flag and clears it

bool scriptEditor::contains(Vector2D<float> pixel) const
{
    if (!this->opened) return false;
    return pixel.x >= this->panel.x && pixel.x < this->panel.x + this->panel.w && pixel.y >= this->panel.y && pixel.y < this->panel.y + this->panel.h;
}
// ui_editor: main.cpp must know when the cursor is over the panel; contains tests a real-pixel point against the panel from the last draw, false when closed

bool scriptEditor::draw(SDL_Renderer* renderer, Vector2D<float> viewport, float density)
{
    if (!this->opened) return true;
    const float scale = textScale(density);
    this->artScale = std::max(1.0f, std::floor(std::min(viewport.x * 0.8f / panelWide, viewport.y * 0.8f / panelHigh)));
    this->viewportSize = viewport;
    if (this->resized) {
        this->panel.w = std::min(this->panel.w, viewport.x);
        this->panel.h = std::min(this->panel.h, viewport.y);
        this->panel.x = snapTo(std::clamp(this->panel.x, 0.0f, viewport.x - this->panel.w), scale);
        this->panel.y = snapTo(std::clamp(this->panel.y, 0.0f, viewport.y - this->panel.h), scale);
    } else {
        this->panel.w = panelWide * this->artScale;
        this->panel.h = panelHigh * this->artScale;
        this->panel.x = snapTo((viewport.x - this->panel.w) / 2.0f, scale);
        this->panel.y = snapTo((viewport.y - this->panel.h) / 2.0f, scale);
    }
    // ui-terminal: art pixels must stay even and sharp at any window size, and a size the user dragged must stay; the art scale is the largest whole one that fits 128x96 inside 80% of the viewport, the panel starts at that size centered with its corner on a whole text pixel, and after a resize it keeps its rect, pulled back on screen

    float scaleX = 1.0f;
    float scaleY = 1.0f;
    SDL_BlendMode blend = SDL_BLENDMODE_NONE;
    if (!SDL_GetRenderScale(renderer, &scaleX, &scaleY)) return false;
    if (!SDL_GetRenderDrawBlendMode(renderer, &blend)) return false;
    const bool drawn = this->drawContents(renderer, density);
    const bool unclipped = SDL_SetRenderClipRect(renderer, nullptr);
    const bool rescaled = SDL_SetRenderScale(renderer, scaleX, scaleY);
    const bool reblended = SDL_SetRenderDrawBlendMode(renderer, blend);
    return drawn && unclipped && rescaled && reblended;
}
// ui_editor: the game keeps drawing at its zoom after the editor; draw sizes and centers the panel, draws it, then clears the clip and restores the caller's render scale and blend mode, false on any SDL failure
