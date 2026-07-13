#include "commands/commands.h"
#include <iostream>

namespace envctl {

int cmd_unset(Context& ctx, const std::vector<std::string>& args) {
    if (args.empty() || args[0] == "--help") {
        std::cout << "envctl unset — Remove a variable\n\n";
        std::cout << "Usage: envctl unset <KEY>\n";
        return args.empty() ? 1 : 0;
    }

    int64_t pid = ctx.require_profile();
    std::string key = args[0];

    auto existing = ctx.db.get_variable(pid, key);
    if (!existing) {
        std::cerr << "Variable '" << key << "' not found\n";
        return 1;
    }

    ctx.db.record_history(existing->id, existing->value, "", get_username());
    ctx.db.delete_variable(pid, key);

    std::cout << "Unset " << key << "\n";
    return 0;
}

} // namespace envctl
