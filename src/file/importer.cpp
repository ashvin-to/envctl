#include "file/importer.h"
#include "file/envfile.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace envctl {

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

ImportResult import_env_file(Database& db, int64_t profile_id, const std::string& path, bool overwrite) {
    ImportResult r;
    auto ef = EnvFile::parse(path);
    if (ef.vars.empty()) {
        r.errors.push_back("No variables found in " + path);
        return r;
    }
    for (auto& v : ef.vars) {
        auto existing = db.get_variable(profile_id, v.key);
        if (existing && !overwrite) {
            r.skipped++;
            continue;
        }
        db.set_variable(profile_id, v.key, v.value);
        r.imported++;
    }
    return r;
}

ImportResult import_docker_compose(Database& db, int64_t profile_id, const std::string& path, bool overwrite) {
    ImportResult r;
    std::ifstream f(path);
    if (!f.is_open()) {
        r.errors.push_back("Cannot open " + path);
        return r;
    }

    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    // Simple YAML parser for .env key: value pairs in docker-compose environment section
    // Handles: KEY: value and KEY: "value"
    std::istringstream iss(content);
    std::string line;
    bool in_env = false;
    while (std::getline(iss, line)) {
        std::string trimmed = trim(line);
        if (trimmed.find("environment:") != std::string::npos ||
            trimmed.find("env_file:") != std::string::npos) {
            in_env = true;
            continue;
        }
        if (in_env && !trimmed.empty() && trimmed[0] != ' ' && trimmed[0] != '\t' && trimmed[0] != '-' &&
            trimmed.find(':') == std::string::npos && trimmed.find('=') == std::string::npos) {
            in_env = false;
        }
        if (in_env) {
            std::string key, value;

            // Handle "- KEY=value" format (docker-compose list style)
            if (trimmed.find("- ") == 0) {
                trimmed = trimmed.substr(2);
            }

            auto eq = trimmed.find('=');
            auto colon = trimmed.find(':');

            if (eq != std::string::npos) {
                key = trim(trimmed.substr(0, eq));
                value = trim(trimmed.substr(eq + 1));
            } else if (colon != std::string::npos) {
                key = trim(trimmed.substr(0, colon));
                value = trim(trimmed.substr(colon + 1));
            }

            if (!key.empty()) {
                value = strip_quotes(value);
                auto existing = db.get_variable(profile_id, key);
                if (existing && !overwrite) { r.skipped++; continue; }
                db.set_variable(profile_id, key, value);
                r.imported++;
            }
        }
    }
    return r;
}

ImportResult import_json(Database& db, int64_t profile_id, const std::string& path, bool overwrite) {
    ImportResult r;
    std::ifstream f(path);
    if (!f.is_open()) {
        r.errors.push_back("Cannot open " + path);
        return r;
    }

    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    // Simple JSON parser: extract "KEY": "VALUE" pairs
    size_t pos = 0;
    while (pos < content.size()) {
        auto key_start = content.find('"', pos);
        if (key_start == std::string::npos) break;
        auto key_end = content.find('"', key_start + 1);
        if (key_end == std::string::npos) break;
        std::string key = content.substr(key_start + 1, key_end - key_start - 1);

        auto val_start = content.find('"', key_end + 1);
        if (val_start == std::string::npos) break;
        auto val_end = content.find('"', val_start + 1);
        if (val_end == std::string::npos) break;
        std::string value = content.substr(val_start + 1, val_end - val_start - 1);

        pos = val_end + 1;

        if (!key.empty() && key[0] != '{') {
            auto existing = db.get_variable(profile_id, key);
            if (existing && !overwrite) { r.skipped++; continue; }
            db.set_variable(profile_id, key, value);
            r.imported++;
        }
    }
    return r;
}

} // namespace envctl
