#include "commands/commands.h"
#include <iostream>
#include <iomanip>

namespace envctl {

int cmd_list(Context& ctx, const std::vector<std::string>& args) {
    bool json_out = false;
    bool resolved = false;
    for (auto& a : args) {
        if (a == "--json" || a == "-j") json_out = true;
        if (a == "--resolved" || a == "-r") resolved = true;
        if (a == "--help") {
            std::cout << "envctl list — List variables in active profile\n\n";
            std::cout << "Usage: envctl list [--json] [--resolved]\n";
            std::cout << "  --json      Output as JSON\n";
            std::cout << "  --resolved  Show inherited variables from parent profiles\n";
            return 0;
        }
    }

    int64_t pid = ctx.require_profile();
    auto vars = resolved ? ctx.db.resolve_variables(pid) : ctx.db.list_variables(pid);

    if (json_out) {
        std::cout << "[\n";
        for (size_t i = 0; i < vars.size(); ++i) {
            auto& v = vars[i];
            std::cout << "  {\"key\": \"" << v.key << "\", \"value\": \"" << v.value
                      << "\", \"secret\": " << (v.secret ? "true" : "false");
            if (!v.description.empty()) std::cout << ", \"description\": \"" << v.description << "\"";
            std::cout << "}";
            if (i + 1 < vars.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "]\n";
    } else {
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
            std::cout << "  " << std::left << std::setw(max_key) << v.key;
            if (v.secret) {
                std::cout << " = ****";
            } else {
                std::cout << " = " << v.value;
            }
            if (!v.description.empty()) std::cout << "  # " << v.description;
            std::cout << "\n";
        }
    }
    return 0;
}

} // namespace envctl
