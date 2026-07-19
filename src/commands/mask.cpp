#include "commands/commands.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <cctype>
#include <vector>
#include <algorithm>

namespace envctl {

static const char* kBuiltinPatterns[] = { "secret", "password", "passwd", "token",
                                          "api_key", "apikey", "private_key", "cred" };

static bool matches_any_pattern(const std::string& key, const std::vector<std::string>& patterns) {
    std::string k;
    k.reserve(key.size());
    for (char c : key) k += (char)std::tolower((unsigned char)c);
    for (auto& p : patterns) {
        std::string pl = p;
        std::transform(pl.begin(), pl.end(), pl.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        if (k.find(pl) != std::string::npos) return true;
    }
    return false;
}

static std::vector<std::string> load_patterns(Context& ctx, int64_t pid) {
    std::vector<std::string> patterns;
    for (auto p : kBuiltinPatterns) patterns.push_back(p);
    auto custom = ctx.db.list_secret_patterns(pid);
    patterns.insert(patterns.end(), custom.begin(), custom.end());
    return patterns;
}

int cmd_mask(Context& ctx, const std::vector<std::string>& args) {
    bool json_out = false;
    for (auto& a : args) {
        if (a == "--help" || a == "-h") {
            std::cout << "envctl mask — List variables with masked secrets\n\n";
            std::cout << "Usage: envctl mask [--json]\n";
            std::cout << "\nHides secret values (flagged secret or key names matching\n";
            std::cout << "built-in or custom patterns), showing **** instead.\n";
            std::cout << "  --json   Output as JSON (values of masked secrets stay ****)\n";
            return 0;
        }
        if (a == "--json" || a == "-j") json_out = true;
    }

    int64_t pid = ctx.require_profile();
    int64_t proj_id = ctx.require_project();
    auto vars = ctx.db.list_variables(pid, true);
    auto patterns = load_patterns(ctx, proj_id);

    if (json_out) {
        std::cout << "[\n";
        for (size_t i = 0; i < vars.size(); ++i) {
            auto& v = vars[i];
            bool masked = v.secret || matches_any_pattern(v.key, patterns);
            std::cout << "  {\"key\": \"" << v.key << "\", \"value\": \""
                      << (masked ? "****" : v.value) << "\", \"secret\": "
                      << (masked ? "true" : "false") << "}";
            if (i + 1 < vars.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "]\n";
        return 0;
    }

    auto prof = ctx.current_profile();
    std::cout << "Profile: " << (prof ? prof->name : "none") << "\n\n";

    if (vars.empty()) {
        std::cout << "  (no variables)\n";
        return 0;
    }

    size_t max_key = 0;
    for (auto& v : vars) max_key = std::max(max_key, v.key.size());
    max_key += 2;

    for (auto& v : vars) {
        if (v.secret || matches_any_pattern(v.key, patterns)) {
            std::cout << "  " << std::left << std::setw(max_key) << v.key << " = ****";
        } else {
            std::cout << "  " << std::left << std::setw(max_key) << v.key << " = " << v.value;
        }
        std::cout << "\n";
    }
    return 0;
}

} // namespace envctl
