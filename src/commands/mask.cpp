#include "commands/commands.h"
#include <iostream>
#include <iomanip>

namespace envctl {

int cmd_mask(Context& ctx, const std::vector<std::string>& args) {
    if (!args.empty() && args[0] == "--help") {
        std::cout << "envctl mask — List variables with masked secrets\n\n";
        std::cout << "Usage: envctl mask\n";
        std::cout << "\nHides secret values, showing **** instead.\n";
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
        std::cout << "  " << std::left << std::setw(max_key) << v.key << " = " << v.value;
        if (v.secret) std::cout << "  (secret)";
        std::cout << "\n";
    }
    return 0;
}

} // namespace envctl
