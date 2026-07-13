#include "commands/commands.h"
#include <iostream>

namespace envctl {

int cmd_set(Context& ctx, const std::vector<std::string>& args) {
    bool is_secret = false;
    std::string desc;
    std::string key, value;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--secret" || args[i] == "-s") is_secret = true;
        else if (args[i] == "--description" && i + 1 < args.size()) desc = args[++i];
        else if (args[i] == "--help") {
            std::cout << "envctl set — Set a variable\n\n";
            std::cout << "Usage: envctl set <KEY>=<VALUE> [--secret] [--description <text>]\n";
            std::cout << "  or:  envctl set <KEY> <VALUE> [--secret]\n";
            return 0;
        }
        else if (key.empty()) {
            auto eq = args[i].find('=');
            if (eq != std::string::npos) {
                key = args[i].substr(0, eq);
                value = args[i].substr(eq + 1);
            } else {
                key = args[i];
            }
        }
        else if (value.empty()) value = args[i];
    }

    if (key.empty()) {
        std::cerr << "Usage: envctl set <KEY>=<VALUE> [--secret]\n";
        return 1;
    }

    int64_t pid = ctx.require_profile();

    auto existing = ctx.db.get_variable(pid, key);
    std::string old_val = existing ? existing->value : "";

    auto v = ctx.db.set_variable(pid, key, value, is_secret, desc);

    if (!old_val.empty()) {
        ctx.db.record_history(v.id, old_val, value, get_username());
    }

    std::cout << "Set " << key << " = " << (is_secret ? "****" : value) << "\n";
    return 0;
}

} // namespace envctl
