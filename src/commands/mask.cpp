#include "commands/commands.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <cctype>

namespace envctl {

static bool looks_secret(const std::string& key) {
    std::string k;
    k.reserve(key.size());
    for (char c : key) k += (char)std::tolower((unsigned char)c);
    const char* patterns[] = { "secret", "password", "passwd", "token",
                               "api_key", "apikey", "private_key", "cred" };
    for (auto p : patterns) {
        if (k.find(p) != std::string::npos) return true;
    }
    return false;
}

int cmd_mask(Context& ctx, const std::vector<std::string>& args) {
    if (!args.empty() && args[0] == "--help") {
        std::cout << "envctl mask — List variables with masked secrets\n\n";
        std::cout << "Usage: envctl mask\n";
        std::cout << "\nHides secret values (flagged secret or key names matching\n";
        std::cout << "secret/password/token/key/cred patterns), showing **** instead.\n";
        return 0;
    }

    int64_t pid = ctx.require_profile();
    auto vars = ctx.db.list_variables(pid, true);

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
        if (v.secret || looks_secret(v.key)) {
            std::cout << "  " << std::left << std::setw(max_key) << v.key << " = ****";
        } else {
            std::cout << "  " << std::left << std::setw(max_key) << v.key << " = " << v.value;
        }
        std::cout << "\n";
    }
    return 0;
}

} // namespace envctl
