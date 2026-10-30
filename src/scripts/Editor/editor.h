#ifndef EDITOR_H
#define EDITOR_H

#include <SDL3/SDL.h>

#include <cstddef>
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
    enum class editorButton { none, close, minus, plus };
    enum class editorDrag { none, verticalBar, horizontalBar, resize };

    struct editorLayout {
        float art = 1.0f;
        float cell = 0.0f;
        float line = 0.0f;
        float popupLine = 0.0f;
        float pad = 0.0f;
        float barY = 0.0f;
        float gutterX = 0.0f;
        float textX = 0.0f;
        float textY = 0.0f;
        float textW = 0.0f;
        float textOffset = 0.0f;
        float statusY = 0.0f;
        int visible = 1;
        int digits = 2;
        SDL_FRect closeButton = {0.0f, 0.0f, 0.0f, 0.0f};
        SDL_FRect minusButton = {0.0f, 0.0f, 0.0f, 0.0f};
        SDL_FRect plusButton = {0.0f, 0.0f, 0.0f, 0.0f};
        std::vector<SDL_FRect> tabs;
        int lineCount = 0;
        int widest = 0;
        int columns = 1;
        bool showVertical = false;
        bool showHorizontal = false;
        SDL_FRect verticalTrack = {0.0f, 0.0f, 0.0f, 0.0f};
        SDL_FRect verticalThumb = {0.0f, 0.0f, 0.0f, 0.0f};
        SDL_FRect horizontalTrack = {0.0f, 0.0f, 0.0f, 0.0f};
        SDL_FRect horizontalThumb = {0.0f, 0.0f, 0.0f, 0.0f};
    };
    // ui-terminal: drawing and mouse hits must agree on where the new art sits; editorLayout holds the art scale, text cell, row and popup row heights, the status, gutter and text positions, the tab bar's button and tab rects, and the script's size in lines and columns with each scrollbar's rects, all in real pixels

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

    std::vector<std::string> closedNames;
    std::vector<std::vector<std::string>> closedBuffers;
    bool naming = false;
    std::string typedName;
    editorButton hover = editorButton::none;
    // ui-terminal: the - button keeps edits and the + button asks for a name; closed tabs wait here with their buffers until the next open, naming and typedName hold the name being typed in the new tab, and hover is the button under the mouse

    int scrollX = 0;
    float wheelX = 0.0f;
    float artScale = 1.0f;
    Vector2D<float> viewportSize = {0.0f, 0.0f};
    bool resized = false;
    editorDrag dragging = editorDrag::none;
    float dragGrab = 0.0f;
    int dragEdges = 0;
    int cursorEdges = 0;
    Vector2D<float> dragStart = {0.0f, 0.0f};
    SDL_FRect panelStart = {0.0f, 0.0f, 0.0f, 0.0f};
    // ui-terminal: long lines scroll sideways and the panel can be resized; scrollX is the first column shown, artScale and viewportSize come from the last draw, resized stops draw recentering the panel, and the drag fields hold what a thumb or edge drag started from

    bool completing = false;
    std::vector<Suggestion> suggestions;
    int picked = 0;
    int pickedScroll = 0;
    int wordStart = 0;
    // ui_editor: typing skill names and args by hand was slow and error prone; the autocomplete popup keeps its open flag, the matches, the picked row, the first of its 8 visible rows and the column where the typed word starts

    std::vector<std::string>& lines();
    editorLayout layout(float scale) const;
    void keepCursorVisible(const editorLayout& area);
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
    const std::string& tabLabel(std::size_t index) const;
    void closeTab();
    void startNaming();
    void createScript();
    bool drawTabBar(SDL_Renderer* renderer, const editorLayout& area) const;
    // ui-terminal: the tab bar gets its own actions; tabLabel is a tab's name or the name being typed, closeTab parks the current tab, startNaming and createScript make a new script, and drawTabBar draws the bar's art at scale 1
    int edgesAt(Vector2D<float> pixel) const;
    void setEdgeCursor(int edges);
    void dragTo(Vector2D<float> pixel, float density);
    bool drawScrollbars(SDL_Renderer* renderer, const editorLayout& area) const;
    // ui-terminal: the panel resizes and scrolls by dragging; edgesAt tells which panel edges a point is on, setEdgeCursor shows the matching resize cursor, dragTo applies a thumb or edge drag, and drawScrollbars draws the bars that are needed

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
