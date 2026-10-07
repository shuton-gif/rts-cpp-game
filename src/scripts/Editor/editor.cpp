#include "Editor/editor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <utility>

#include "AI/script.h"

static const SDL_Color editorWhite = {255, 255, 255, 255};
static const SDL_Color editorGray = {0x85, 0x85, 0x85, 255};
static const SDL_Color editorDark = {0x33, 0x33, 0x33, 255};
static const SDL_Color editorGreen = {0x6a, 0x99, 0x55, 255};
static const SDL_Color editorPink = {0xc5, 0x86, 0xc0, 255};
static const SDL_Color editorBlue = {0x4f, 0xc1, 0xff, 255};
static const SDL_Color editorOrange = {0xce, 0x91, 0x78, 255};
static const SDL_Color editorRed = {0xf4, 0x47, 0x47, 255};
static const SDL_Color editorPopup = {0x25, 0x25, 0x26, 255};
static const SDL_Color editorPicked = {0x3a, 0x3d, 0x41, 255};
static const Uint8 panelAlpha = 204;
// ui_editor: scripts are edited in game so npc behavior can change without leaving it, and a familiar dark theme keeps it readable; the panel uses fixed VS Code like colors for text, tabs, comments, keywords, calls, args, errors, the autocomplete popup and its picked row, and its black background is panelAlpha (0.8 opacity)

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
            pushToken(tokens, std::string(1, c), editorWhite);
            ++at;
            continue;
        }
        if (isWordChar(c)) {
            std::size_t end = at;
            while (end < line.size() && isWordChar(line[end])) ++end;
            const std::string word = line.substr(at, end - at);
            SDL_Color color = editorWhite;
            if (word == "if" || word == "not" || word == "else") color = editorPink;
            else if (isScriptCall(word)) color = editorBlue;
            pushToken(tokens, word, color);
            at = end;
            continue;
        }
        pushToken(tokens, std::string(1, c), editorWhite);
        ++at;
    }
    return tokens;
}
// ui_editor: scripts are edited in game so npc behavior can change without leaving it, and colors show mistakes before saving; '#' to the end is green, if/not/else pink, a known call blue, text inside '(' ')' orange (stopping at ')' or '#'), and everything else white

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

std::vector<std::string>& scriptEditor::lines()
{
    return this->buffers[this->tab];
}

scriptEditor::editorLayout scriptEditor::layout(float scale) const
{
    editorLayout area;
    area.cell = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale;
    area.line = 10.0f * scale;
    area.pad = area.cell;
    area.tabY = this->panel.y + area.pad;
    area.textY = area.tabY + 2.0f * area.line;
    area.statusY = this->panel.y + this->panel.h - area.pad - area.line;
    area.visible = std::max(1, static_cast<int>(std::floor((area.statusY - area.line - area.textY) / area.line)));

    std::size_t count = this->buffers.empty() ? 1 : this->buffers[this->tab].size();
    area.digits = 1;
    while (count >= 10) {
        count /= 10;
        ++area.digits;
    }
    area.gutterX = this->panel.x + area.pad;
    area.textX = area.gutterX + static_cast<float>(area.digits + 1) * area.cell;
    area.textW = std::max(0.0f, this->panel.x + this->panel.w - area.pad - area.textX);
    return area;
}
// ui_editor: draw and mouse hits must use the same positions; inside the panel with one cell of padding come the tab row, a blank line, the text rows down to one line above the status row, and the gutter as wide as the line count digits plus one cell

void scriptEditor::keepCursorVisible(int visible)
{
    if (this->row < this->scroll) this->scroll = this->row;
    if (this->row >= this->scroll + visible) this->scroll = this->row - visible + 1;
}
// ui_editor: the cursor must never be edited off screen; after a key the scroll moves just enough to put the cursor row inside the visible rows

void scriptEditor::selectTab(int index)
{
    this->tab = this->names.empty() ? 0 : std::clamp(index, 0, static_cast<int>(this->names.size()) - 1);
    this->row = 0;
    this->col = 0;
    this->scroll = 0;
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

bool scriptEditor::handleKey(const SDL_KeyboardEvent& key, float density)
{
    const SDL_Keycode code = key.key;
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
        this->keepCursorVisible(this->layout(textScale(density)).visible);
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
    this->keepCursorVisible(visible);
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
    if (!this->contains(pixel)) return false;

    const editorLayout area = this->layout(textScale(density));
    const int count = this->buffers.empty() ? 1 : static_cast<int>(this->lines().size());
    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        this->wheel += event.wheel.y;
        const int notches = static_cast<int>(this->wheel);
        this->wheel -= static_cast<float>(notches);
        this->scroll = std::clamp(this->scroll - 3 * notches, 0, std::max(0, count - area.visible));
        return true;
    }
    if (event.type != SDL_EVENT_MOUSE_BUTTON_DOWN || event.button.button != SDL_BUTTON_LEFT) return true;
    this->completing = false;
    // ui_editor: typing skill names and args by hand was slow and error prone; a left click inside the panel moves away from the typed word, so it closes the autocomplete popup

    if (pixel.y >= area.tabY && pixel.y < area.tabY + area.line) {
        float left = this->panel.x + area.pad;
        const float right = this->panel.x + this->panel.w - area.pad;
        for (std::size_t i = 0; i < this->names.size(); ++i) {
            const float width = static_cast<float>(this->names[i].size() + 2) * area.cell;
            if (left + width > right) break;
            if (pixel.x >= left && pixel.x < left + width) {
                this->selectTab(static_cast<int>(i));
                return true;
            }
            left += width + area.cell;
        }
        return true;
    }

    const float bottom = area.textY + static_cast<float>(area.visible) * area.line;
    if (this->buffers.empty() || pixel.y < area.textY || pixel.y >= bottom || pixel.x < area.gutterX) return true;
    this->row = std::clamp(this->scroll + static_cast<int>(std::floor((pixel.y - area.textY) / area.line)), 0, count - 1);
    const int length = static_cast<int>(this->lines()[this->row].size());
    this->col = std::clamp(static_cast<int>(std::round((pixel.x - area.textX) / area.cell)), 0, length);
    return true;
}
// ui_editor: clicks on the panel must not reach the map, but the map stays usable around it; mouse events inside the panel are consumed, the wheel scrolls 3 rows per notch, a left click on a tab switches to it, and one in the text puts the cursor on the nearest row and column

bool scriptEditor::drawContents(SDL_Renderer* renderer, float density)
{
    const float scale = textScale(density);
    const editorLayout area = this->layout(scale);
    const int count = this->buffers.empty() ? 0 : static_cast<int>(this->lines().size());
    this->scroll = std::clamp(this->scroll, 0, std::max(0, count - area.visible));
    const float right = this->panel.x + this->panel.w - area.pad;

    if (!SDL_SetRenderScale(renderer, 1.0f, 1.0f)) return false;
    if (!SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND)) return false;
    if (!SDL_SetRenderDrawColor(renderer, 0, 0, 0, panelAlpha)) return false;
    if (!SDL_RenderFillRect(renderer, &this->panel)) return false;

    float left = this->panel.x + area.pad;
    for (std::size_t i = 0; i < this->names.size(); ++i) {
        const float width = static_cast<float>(this->names[i].size() + 2) * area.cell;
        if (left + width > right) break;
        if (static_cast<int>(i) == this->tab) {
            const SDL_FRect box = {left, area.tabY, width, area.line};
            if (!SDL_SetRenderDrawColor(renderer, editorDark.r, editorDark.g, editorDark.b, editorDark.a)) return false;
            if (!SDL_RenderFillRect(renderer, &box)) return false;
        }
        left += width + area.cell;
    }
    // ui_editor: the game should show faintly behind the editor and the current tab must stand out; the black panel is blended at panelAlpha (0.8), then a #333333 box behind the current tab

    if (!SDL_SetRenderScale(renderer, scale, scale)) return false;
    const SDL_Rect inner = {static_cast<int>(this->panel.x / scale), static_cast<int>(this->panel.y / scale), static_cast<int>(this->panel.w / scale), static_cast<int>(this->panel.h / scale)};
    if (!SDL_SetRenderClipRect(renderer, &inner)) return false;

    left = this->panel.x + area.pad;
    for (std::size_t i = 0; i < this->names.size(); ++i) {
        const float width = static_cast<float>(this->names[i].size() + 2) * area.cell;
        if (left + width > right) break;
        const SDL_Color color = static_cast<int>(i) == this->tab ? editorWhite : editorGray;
        if (!drawText(renderer, left + area.cell, area.tabY + scale, scale, this->names[i], color)) return false;
        left += width + area.cell;
    }

    const int end = std::min(count, this->scroll + area.visible);
    for (int i = this->scroll; i < end; ++i) {
        const std::string number = std::to_string(i + 1);
        const float x = area.gutterX + static_cast<float>(area.digits - static_cast<int>(number.size())) * area.cell;
        const float y = area.textY + static_cast<float>(i - this->scroll) * area.line + scale;
        if (!drawText(renderer, x, y, scale, number, i == this->errorLine ? editorRed : editorGray)) return false;
    }

    if (!this->status.empty()) {
        const SDL_Color color = this->status == "saved" ? editorGreen : editorRed;
        if (!drawText(renderer, this->panel.x + area.pad, area.statusY + scale, scale, this->status, color)) return false;
    }
    // ui_editor: names, line numbers and the save result are drawn at the text scale clipped to the panel; tabs white when current else #858585, right-aligned numbers #858585 with the error row in #f44747, and status green when saved else red

    const SDL_Rect textArea = {static_cast<int>(area.textX / scale), static_cast<int>(area.textY / scale), static_cast<int>(area.textW / scale), area.visible * 10};
    if (!SDL_SetRenderClipRect(renderer, &textArea)) return false;
    for (int i = this->scroll; i < end; ++i) {
        float x = area.textX;
        const float y = area.textY + static_cast<float>(i - this->scroll) * area.line + scale;
        for (const EditorToken& token : colorLine(this->lines()[i])) {
            if (!drawText(renderer, x, y, scale, token.text, token.color)) return false;
            x += static_cast<float>(token.text.size()) * area.cell;
        }
    }
    if (!SDL_SetRenderClipRect(renderer, nullptr)) return false;
    // ui_editor: long lines must not spill over the panel; each visible row is drawn token by token in its syntax color, clipped to the text area

    const float cursorX = area.textX + static_cast<float>(this->col) * area.cell;
    const bool visible = count > 0 && this->row >= this->scroll && this->row < end && cursorX + scale <= area.textX + area.textW;
    if (!visible) return true;
    const SDL_FRect cursor = {cursorX / scale, (area.textY + static_cast<float>(this->row - this->scroll) * area.line + scale) / scale, 1.0f, SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE};
    if (!SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)) return false;
    if (!SDL_RenderFillRect(renderer, &cursor)) return false;
    return this->drawSuggestions(renderer, scale, area);
}
// ui_editor: the caret shows where typing goes, and typing skill names and args by hand was slow and error prone; drawn at the text scale as a white bar 1 x scale px wide and one glyph tall at the cursor column, hidden when scrolled or clipped away, with the autocomplete popup drawn over everything after it

bool scriptEditor::drawSuggestions(SDL_Renderer* renderer, float scale, const editorLayout& area)
{
    if (!this->completing || this->suggestions.empty()) return true;
    const int count = static_cast<int>(this->suggestions.size());
    const int rows = std::min(8, count);
    this->picked = std::clamp(this->picked, 0, count - 1);
    this->pickedScroll = std::clamp(this->pickedScroll, std::max(0, this->picked - rows + 1), std::min(this->picked, count - rows));

    std::size_t widest = 0;
    for (const Suggestion& choice : this->suggestions) widest = std::max(widest, choice.text.size());
    const float w = static_cast<float>(widest + 2) * area.cell;
    const float h = static_cast<float>(rows) * area.line + 2.0f * area.cell;
    const float lineY = area.textY + static_cast<float>(this->row - this->scroll) * area.line;
    const float bottom = area.textY + static_cast<float>(area.visible) * area.line;
    float y = lineY + area.line;
    if (y + h > bottom) y = lineY - h;
    y = std::max(y, this->panel.y);
    const float minX = this->panel.x + area.pad;
    const float maxX = std::floor((this->panel.x + this->panel.w - area.pad - w) / scale) * scale;
    const float x = std::max(minX, std::min(area.textX + static_cast<float>(this->wordStart) * area.cell, maxX));

    const SDL_Rect inner = {static_cast<int>(this->panel.x / scale), static_cast<int>(this->panel.y / scale), static_cast<int>(this->panel.w / scale), static_cast<int>(this->panel.h / scale)};
    if (!SDL_SetRenderClipRect(renderer, &inner)) return false;
    const SDL_FRect box = {x / scale, y / scale, w / scale, h / scale};
    if (!SDL_SetRenderDrawColor(renderer, editorPopup.r, editorPopup.g, editorPopup.b, editorPopup.a)) return false;
    if (!SDL_RenderFillRect(renderer, &box)) return false;
    for (int i = 0; i < rows; ++i) {
        const int index = this->pickedScroll + i;
        const float rowY = y + area.cell + static_cast<float>(i) * area.line;
        if (index == this->picked) {
            const SDL_FRect highlight = {x / scale, rowY / scale, w / scale, area.line / scale};
            if (!SDL_SetRenderDrawColor(renderer, editorPicked.r, editorPicked.g, editorPicked.b, editorPicked.a)) return false;
            if (!SDL_RenderFillRect(renderer, &highlight)) return false;
        }
        const Suggestion& choice = this->suggestions[index];
        if (!drawText(renderer, x + area.cell, rowY + scale, scale, choice.text, choice.color)) return false;
    }
    return SDL_SetRenderClipRect(renderer, nullptr);
}
// ui_editor: typing skill names and args by hand was slow and error prone; the popup is a #252526 box with one cell of padding under the cursor line (above it when it would pass the text area bottom), left at the typed word start and clamped inside the panel, showing up to 8 suggestions in their syntax colors from the list scroll with the picked row on #3a3d41

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
        if (old != this->names.end()) kept.push_back(std::move(this->buffers[old - this->names.begin()]));
        else kept.push_back(readLines(buildDir + "/" + name + ".txt"));
    }
    this->names = std::move(sorted);
    this->buffers = std::move(kept);

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
}

bool scriptEditor::isOpen() const { return this->opened; }
// ui_editor: typing must go back to being game keys after closing, and typing skill names and args by hand was slow and error prone; close stops text input and hides the panel and the autocomplete popup, keeping every buffer for the next open

bool scriptEditor::handleEvent(const SDL_Event& event, float density)
{
    if (!this->opened) return false;
    switch (event.type) {
    case SDL_EVENT_TEXT_INPUT:
        if (!this->buffers.empty()) {
            this->insertText(event.text.text);
            this->refreshSuggestions(false);
            this->keepCursorVisible(this->layout(textScale(density)).visible);
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
    this->panel.x = std::floor(viewport.x * 0.1f / scale) * scale;
    this->panel.y = std::floor(viewport.y * 0.1f / scale) * scale;
    this->panel.w = std::round(viewport.x * 0.8f);
    this->panel.h = std::round(viewport.y * 0.8f);

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
// ui_editor: the game keeps drawing at its zoom after the editor; draw centers a panel 80% of the viewport (corner on a whole text pixel), draws it, then clears the clip and restores the caller's render scale and blend mode, false on any SDL failure
