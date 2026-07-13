#include "commands/commands.h"
#include <iostream>

namespace envctl {

int cmd_use(Context& ctx, const std::vector<std::string>& args) {
    if (args.empty() || args[0] == "--help") {
        std::cout << "envctl use — Switch active profile\n\n";
        std::cout << "Usage: envctl use <profile-name>\n";
        return args.empty() ? 1 : 0;
    }

    int64_t pid = ctx.require_project();
    std::string name = args[0];

    auto prof = ctx.db.get_profile_by_name(pid, name);
    if (!prof) {
        std::cerr << "Profile '" << name << "' not found\n";
        auto profiles = ctx.db.list_profiles(pid);
        std::cout << "Available: ";
        for (auto& p : profiles) std::cout << p.name << " ";
        std::cout << "\n";
        return 1;
    }

    ctx.db.set_active_profile(pid, prof->id);
    std::cout << "Active profile: " << name << "\n";
    return 0;
}

} // namespace envctl
