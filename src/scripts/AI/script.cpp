#include "AI/script.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

struct ScriptLine {
    int number = 0;
    int indent = 0;
    std::string text;
};
// ai_script: the parser works on cleaned lines; number is the 1-based line in the original text, indent the leading spaces, text the rest without comment or trailing whitespace

bool isScriptCall(const std::string& name)
{
    static const char* const calls[] = {
        "find", "holding", "moveTo", "pickup", "target", "use", "full", "build", "drop", "stop"};
    for (const char* call : calls)
        if (name == call) return true;
    return false;
}
// ai_script: only skills the interpreter knows may appear in a script; a name is a call when it matches one of the fixed list exactly

static bool fail(int line, const std::string& message, std::string& error)
{
    error = "line " + std::to_string(line) + ": " + message;
    return false;
}
// ai_script: every parse error points at the line to fix; fail writes "line N: msg" into error and returns false so callers can return it directly

static bool splitLines(const std::string& text, std::vector<ScriptLine>& lines, std::string& error)
{
    std::size_t start = 0;
    int number = 0;
    while (start <= text.size()) {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos) end = text.size();
        std::string raw = text.substr(start, end - start);
        start = end + 1;
        ++number;

        if (!raw.empty() && raw.back() == '\r') raw.pop_back();
        std::size_t hash = raw.find('#');
        if (hash != std::string::npos) raw.erase(hash);
        while (!raw.empty() && (raw.back() == ' ' || raw.back() == '\t')) raw.pop_back();
        if (raw.empty()) continue;

        std::size_t indent = 0;
        while (indent < raw.size() && (raw[indent] == ' ' || raw[indent] == '\t')) {
            if (raw[indent] == '\t') return fail(number, "tabs are not allowed", error);
            ++indent;
        }
        lines.push_back({number, static_cast<int>(indent), raw.substr(indent)});
    }
    return true;
}
// ai_script: comments, blank lines and line endings must not matter to the block structure; splitLines cuts on '\n', drops '\r', '#' comments and trailing whitespace, skips empty lines, and rejects a tab in the indent

static bool isNameChar(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

static bool parseCall(const ScriptLine& line, const std::string& text, Node& node, std::string& error)
{
    std::size_t open = text.find('(');
    if (open == std::string::npos || open == 0) return fail(line.number, "expected name(arg)", error);
    std::string name = text.substr(0, open);
    for (char c : name)
        if (!isNameChar(c)) return fail(line.number, "expected name(arg)", error);
    if (!isScriptCall(name)) return fail(line.number, "unknown call '" + name + "'", error);

    std::size_t close = open + 1;
    while (close < text.size() && (isNameChar(text[close]) || text[close] == '-')) ++close;
    if (close != text.size() - 1 || text[close] != ')') return fail(line.number, "expected name(arg)", error);

    node.call = name;
    node.arg = text.substr(open + 1, close - open - 1);
    node.line = line.number;
    return true;
}
// ai_script: a call is exactly name(arg) with no spaces; parseCall checks the name is a known call, the arg is letters, digits, '_' or '-' (may be empty), and nothing follows ')'

static bool firstWordIs(const std::string& text, const std::string& word)
{
    if (text.compare(0, word.size(), word) != 0) return false;
    return text.size() == word.size() || text[word.size()] == ' ';
}

static std::size_t skipSpaces(const std::string& text, std::size_t at)
{
    while (at < text.size() && text[at] == ' ') ++at;
    return at;
}
// ai_script: keywords are whole words split from the call by spaces; firstWordIs matches if/not/else only when followed by a space or the line end, and skipSpaces steps over one or more spaces

static bool parseBlock(const std::vector<ScriptLine>& lines, std::size_t& pos, int indent, std::vector<Node>& out, std::string& error)
{
    while (pos < lines.size()) {
        const ScriptLine& line = lines[pos];
        if (line.indent < indent) return true;
        if (line.indent > indent) return fail(line.number, "unexpected indent", error);
        if (firstWordIs(line.text, "else")) return fail(line.number, "else without if", error);

        Node node;
        if (!firstWordIs(line.text, "if")) {
            if (!parseCall(line, line.text, node, error)) return false;
            out.push_back(std::move(node));
            ++pos;
            continue;
        }

        std::size_t at = skipSpaces(line.text, 2);
        std::string condition = line.text.substr(at);
        if (firstWordIs(condition, "not") && condition.size() > 3) {
            node.negate = true;
            condition = condition.substr(skipSpaces(condition, 3));
        }
        if (!parseCall(line, condition, node, error)) return false;
        node.isIf = true;
        ++pos;

        if (pos >= lines.size() || lines[pos].indent <= indent) return fail(line.number, "if needs an indented block", error);
        if (!parseBlock(lines, pos, lines[pos].indent, node.then, error)) return false;

        if (pos < lines.size() && lines[pos].indent == indent && firstWordIs(lines[pos].text, "else")) {
            const ScriptLine& elseLine = lines[pos];
            if (elseLine.text != "else") return fail(elseLine.number, "nothing may follow else", error);
            ++pos;
            if (pos >= lines.size() || lines[pos].indent <= indent) return fail(elseLine.number, "else needs an indented block", error);
            if (!parseBlock(lines, pos, lines[pos].indent, node.otherwise, error)) return false;
        }
        out.push_back(std::move(node));
    }
    return true;
}
// ai_script: nesting is shown by indent like python; parseBlock reads lines of exactly indent until a smaller one, an if takes the deeper block after it and an else at the same indent right after that block, and any other deeper line or lone else is an error

bool parseScript(const std::string& text, Script& out, std::string& error)
{
    std::vector<ScriptLine> lines;
    if (!splitLines(text, lines, error)) return false;

    Script script;
    std::size_t pos = 0;
    if (!lines.empty() && lines[0].indent != 0) return fail(lines[0].number, "unexpected indent", error);
    if (!parseBlock(lines, pos, 0, script.body, error)) return false;

    script.name = out.name;
    out = std::move(script);
    return true;
}
// ai_script: a broken script must not replace a working one; parseScript builds into a local Script from indent 0 and only moves the body into out on success, keeping out.name

std::vector<std::string> loadScripts(const std::string& dir, std::map<std::string, Script>& scripts)
{
    std::error_code ec;
    std::filesystem::directory_iterator it(dir, ec);
    if (ec) return {dir + ": cannot open"};

    std::vector<std::string> errors;
    for (const std::filesystem::directory_entry& entry : it) {
        if (!entry.is_regular_file(ec) || entry.path().extension() != ".txt") continue;

        std::ifstream file(entry.path(), std::ios::binary);
        std::stringstream text;
        text << file.rdbuf();

        Script script;
        std::string error;
        std::string stem = entry.path().stem().string();
        if (!parseScript(text.str(), script, error)) {
            errors.push_back(entry.path().filename().string() + ": " + error);
            continue;
        }
        script.name = stem;
        scripts[stem] = std::move(script);
    }
    return errors;
}
// ai_script: scripts are edited while the game runs and reloaded; loadScripts parses every .txt in dir into scripts[stem], and a file that fails leaves its old entry in place and adds "<file>: line N: msg" to the returned list
