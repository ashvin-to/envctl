#include "commands/commands.h"
#include <iostream>

namespace envctl {

int cmd_shell(Context& ctx, const std::vector<std::string>& args) {
    if (!args.empty() && args[0] == "--help") {
        std::cout << "envctl shell — Output shell export commands\n\n";
        std::cout << "Usage: eval $(envctl shell [profile])\n";
        std::cout << "\nOutputs export statements for the active (or specified) profile.\n";
        return 0;
    }

    int64_t pid = ctx.require_project();
    int64_t profile_id;

    if (!args.empty()) {
        auto prof = ctx.db.get_profile_by_name(pid, args[0]);
        if (!prof) {
            std::cerr << "Profile '" << args[0] << "' not found\n";
            return 1;
        }
        profile_id = prof->id;
    } else {
        auto active = ctx.db.get_active_profile(pid);
        if (!active) {
            std::cerr << "No active profile. Run 'envctl use <profile>' first.\n";
            return 1;
        }
        profile_id = *active;
    }

    auto vars = ctx.db.resolve_variables(profile_id);
    for (auto& v : vars) {
        std::cout << "export " << v.key << "=\"" << v.value << "\"\n";
    }
    return 0;
}

} // namespace envctl
