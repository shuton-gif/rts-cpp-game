#ifndef EDITOR_H
#define EDITOR_H

#include <SDL3/SDL.h>

#include <string>
#include <vector>

#include "Vector2D.h"

struct EditorToken {
    std::string text;
    SDL_Color color;
};

std::vector<EditorToken> colorLine(const std::string& line);
// ui_editor: scripts are edited in game so npc behavior can change without leaving it, and colors make the language readable; colorLine cuts one line into runs colored as comment, keyword, known call, call arg or plain text

struct Suggestion {
    std::string text;
    std::string insert;
    int cursorBack;
    SDL_Color color;
};

std::vector<Suggestion> suggest(const std::string& line, int col, bool force, int& wordStart);
// ui_editor: typing skill names and args by hand was slow and error prone; suggest takes the [A-Za-z0-9_-] word before col (wordStart is where it starts) and returns args inside an unclosed '(' else keywords then calls that start with it, nothing after '#' or for an empty word unless force, each with the text that replaces the word and how far the cursor steps back after it

class scriptEditor
{

private:
    struct editorLayout {
        float cell = 0.0f;
        float line = 0.0f;
        float pad = 0.0f;
        float tabY = 0.0f;
        float gutterX = 0.0f;
        float textX = 0.0f;
        float textY = 0.0f;
        float textW = 0.0f;
        float statusY = 0.0f;
        int visible = 1;
        int digits = 1;
    };
    // ui_editor: drawing and mouse hits must agree on where things are; editorLayout holds the cell size, line height and the tab row, gutter, text area and status row positions in real pixels

    bool opened = false;
    std::vector<std::string> names;
    std::vector<std::vector<std::string>> buffers;
    int tab = 0;
    int row = 0;
    int col = 0;
    int scroll = 0;
    float wheel = 0.0f;
    std::string sourceDir;
    std::string buildDir;
    std::string status;
    int errorLine = -1;
    bool saved = false;
    SDL_FRect panel = {0.0f, 0.0f, 0.0f, 0.0f};
    SDL_Window* window = nullptr;
    // ui_editor: unsaved edits must survive closing the pop-up; one line buffer per script name, the current tab, cursor and scroll, both script dirs, the last save result with its error row (-1 none), the saved flag for main.cpp to reload, the panel rect from the last draw and the window that gets text input

    bool completing = false;
    std::vector<Suggestion> suggestions;
    int picked = 0;
    int pickedScroll = 0;
    int wordStart = 0;
    // ui_editor: typing skill names and args by hand was slow and error prone; the autocomplete popup keeps its open flag, the matches, the picked row, the first of its 8 visible rows and the column where the typed word starts

    std::vector<std::string>& lines();
    editorLayout layout(float scale) const;
    void keepCursorVisible(int visible);
    void selectTab(int index);
    void insertText(const std::string& text);
    void newLine();
    void backspace();
    void deleteForward();
    void save();
    void refreshSuggestions(bool force);
    void acceptSuggestion();
    bool drawSuggestions(SDL_Renderer* renderer, float scale, const editorLayout& area);
    // ui_editor: typing skill names and args by hand was slow and error prone; refreshSuggestions reruns suggest at the cursor and opens the popup only on a match, acceptSuggestion puts the picked one in place of the typed word, and drawSuggestions draws the popup at the cursor line
    bool handleKey(const SDL_KeyboardEvent& key, float density);
    bool handleMouse(const SDL_Event& event, float density);
    bool drawContents(SDL_Renderer* renderer, float density);
    // ui_editor: each edit action is one small step on the current buffer; helpers move the cursor, edit text, save, route keys and mouse, and draw the panel inside a render scale the caller restores

public:
    void open(SDL_Window* window, const std::vector<std::string>& names, const std::string& sourceDir, const std::string& buildDir);
    void close();
    bool isOpen() const;
    bool handleEvent(const SDL_Event& event, float density);
    bool takeSaved();
    bool contains(Vector2D<float> pixel) const;
    bool draw(SDL_Renderer* renderer, Vector2D<float> viewport, float density);
};
// ui_editor: scripts are edited in game so npc behavior can change without leaving it; a pop-up IDE with one tab per script, colored text, line numbers and the parse error, which takes all keys while open and only the mouse inside its panel

#endif
