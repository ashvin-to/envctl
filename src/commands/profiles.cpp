#include "commands/commands.h"
#include <iostream>

namespace envctl {

int cmd_profiles(Context& ctx, const std::vector<std::string>& args) {
    if (args.size() == 1 && args[0] == "--help") {
        std::cout << "envctl profiles — List all profiles\n\n";
        std::cout << "Usage: envctl profiles\n";
        return 0;
    }

    int64_t pid = ctx.require_project();
    auto profiles = ctx.db.list_profiles(pid);
    auto active = ctx.db.get_active_profile(pid);

    for (auto& p : profiles) {
        std::string marker = (active && *active == p.id) ? " *" : "";
        std::string parent;
        if (p.parent_id > 0) {
            auto pp = ctx.db.get_profile(p.parent_id);
            if (pp) parent = " (inherits: " + pp->name + ")";
        }
        auto vars = ctx.db.list_variables(p.id);
        std::cout << "  " << p.name << marker << parent
                  << " [" << vars.size() << " vars]\n";
    }
    return 0;
}

int cmd_create(Context& ctx, const std::vector<std::string>& args) {
    std::string name, parent;
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--help") {
            std::cout << "envctl create — Create a new profile\n\n";
            std::cout << "Usage: envctl create <name> [--parent <parent-name>]\n";
            return 0;
        }
        if (args[i] == "--parent" && i + 1 < args.size()) parent = args[++i];
        else if (name.empty()) name = args[i];
    }

    if (name.empty()) {
        std::cerr << "Usage: envctl create <name> [--parent <parent>]\n";
        return 1;
    }

    int64_t pid = ctx.require_project();

    if (ctx.db.get_profile_by_name(pid, name)) {
        std::cerr << "Profile '" << name << "' already exists\n";
        return 1;
    }

    int64_t parent_id = 0;
    if (!parent.empty()) {
        auto pp = ctx.db.get_profile_by_name(pid, parent);
        if (!pp) {
            std::cerr << "Parent profile '" << parent << "' not found\n";
            return 1;
        }
        parent_id = pp->id;
    }

    ctx.db.create_profile(pid, name, parent_id);
    std::cout << "Created profile: " << name << "\n";
    return 0;
}

} // namespace envctl
