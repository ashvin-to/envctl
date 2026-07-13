#include "file/envfile.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace envctl {

std::string strip_quotes(const std::string& s) {
    if (s.size() >= 2) {
        if ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))
            return s.substr(1, s.size() - 2);
    }
    return s;
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

EnvFile EnvFile::parse_string(const std::string& content) {
    EnvFile ef;
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line.substr(0, 7) == "export ") line = trim(line.substr(7));

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));

        EnvVar ev;
        ev.key = key;
        ev.value = strip_quotes(value);
        ef.vars.push_back(ev);
    }
    return ef;
}

EnvFile EnvFile::parse(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return EnvFile{};
    std::ostringstream ss;
    ss << f.rdbuf();
    EnvFile ef = parse_string(ss.str());
    ef.path = path;
    return ef;
}

std::string EnvFile::to_env_string() const {
    std::ostringstream out;
    for (auto& v : vars) {
        if (!v.comment.empty()) out << "# " << v.comment << "\n";
        out << v.key << "=\"" << v.value << "\"\n";
    }
    return out.str();
}

std::string EnvFile::to_json() const {
    std::ostringstream out;
    out << "{\n";
    for (size_t i = 0; i < vars.size(); ++i) {
        out << "  \"" << vars[i].key << "\": \"" << vars[i].value << "\"";
        if (i + 1 < vars.size()) out << ",";
        out << "\n";
    }
    out << "}\n";
    return out.str();
}

std::map<std::string, std::string> parse_env_file(const std::string& path) {
    std::map<std::string, std::string> m;
    auto ef = EnvFile::parse(path);
    for (auto& v : ef.vars) m[v.key] = v.value;
    return m;
}

} // namespace envctl
