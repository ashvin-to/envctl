#include "commands/commands.h"
#include <iostream>
#include <iomanip>

namespace envctl {

int cmd_info(Context& ctx, const std::vector<std::string>& args) {
    if (!args.empty() && args[0] == "--help") {
        std::cout << "envctl info — Show project and profile details\n\n";
        std::cout << "Usage: envctl info\n";
        return 0;
    }

    auto proj = ctx.current_project();
    if (!proj) {
        std::cerr << "No project initialized here. Run 'envctl init'.\n";
        return 1;
    }

    std::cout << "Project\n";
    std::cout << "  ID:        " << proj->id << "\n";
    std::cout << "  Name:      " << proj->name << "\n";
    std::cout << "  Path:      " << proj->root_path << "\n";
    std::cout << "  Created:   " << proj->created_at << "\n";

    auto profiles = ctx.db.list_profiles(proj->id);
    std::cout << "  Profiles:  " << profiles.size() << "\n";

    auto active = ctx.db.get_active_profile(proj->id);
    if (active) {
        auto prof = ctx.db.get_profile(*active);
        if (prof) {
            auto vars = ctx.db.list_variables(prof->id);
            std::cout << "\nActive Profile\n";
            std::cout << "  ID:        " << prof->id << "\n";
            std::cout << "  Name:      " << prof->name << "\n";
            std::cout << "  Variables: " << vars.size() << "\n";
            if (prof->parent_id > 0) {
                auto parent = ctx.db.get_profile(prof->parent_id);
                if (parent) std::cout << "  Inherits:  " << parent->name << " (id=" << parent->id << ")\n";
            }
        }
    } else {
        std::cout << "\nActive Profile: (none)\n";
    }

    std::cout << "\nDatabase: " << ctx.db.db_path() << "\n";
    return 0;
}

} // namespace envctl
