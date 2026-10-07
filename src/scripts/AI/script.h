#ifndef SCRIPT_H
#define SCRIPT_H

#include <map>
#include <string>
#include <vector>

struct Node {
    std::string call;
    std::string arg;
    bool negate = false;
    bool isIf = false;
    std::vector<Node> then;
    std::vector<Node> otherwise;
    int line = 0;
};
// ai_script: one script line is one skill call or one branch; a statement is call(arg), an if is the condition call(arg) (negate for not) with its then block and else block, and line is the 1-based source line for errors

struct Script {
    std::string name;
    std::vector<Node> body;
};
// ai_script: a script is one npc behavior named after its file; body is the top-level block the interpreter re-runs every frame

bool isScriptCall(const std::string& name);
bool parseScript(const std::string& text, Script& out, std::string& error);
std::vector<std::string> loadScripts(const std::string& dir, std::map<std::string, Script>& scripts);
// ai_script: npc behaviors are chained skills written as text so they change without recompiling; isScriptCall checks the fixed call list, parseScript turns indented text into nodes (false with "line N: msg" and out untouched), and loadScripts parses every .txt in dir into scripts by stem, keeping the old version of a broken file and returning one error per failure

#endif
