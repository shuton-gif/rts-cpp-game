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

    std::vector<std::string>& lines();
    editorLayout layout(float scale) const;
    void keepCursorVisible(int visible);
    void selectTab(int index);
    void insertText(const std::string& text);
    void newLine();
    void backspace();
    void deleteForward();
    void save();
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
