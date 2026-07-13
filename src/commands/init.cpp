#include "commands/commands.h"
#include "file/importer.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace envctl {

int cmd_init(Context& ctx, const std::vector<std::string>& args) {
    std::string name;
    std::string root = ctx.project_root;
    bool no_import = false;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--name" && i + 1 < args.size()) name = args[++i];
        else if (args[i] == "--no-import") no_import = true;
        else if (args[i] == "--help") {
            std::cout << "envctl init — Initialize envctl for this project\n\n";
            std::cout << "Usage: envctl init [--name <name>] [--no-import]\n\n";
            std::cout << "Creates a .envctl directory and registers this project.\n";
            std::cout << "Auto-imports existing .env files into matching profiles:\n";
            std::cout << "  .env              -> development\n";
            std::cout << "  .env.development  -> development\n";
            std::cout << "  .env.production   -> production\n";
            std::cout << "  .env.staging      -> staging\n";
            std::cout << "\n  Skips .env.local (import manually: envctl import .env.local)\n";
            std::cout << "\n  --no-import  Skip auto-import\n";
            return 0;
        }
    }

    if (name.empty()) name = fs::path(root).filename().string();

    auto existing = ctx.db.get_project_by_path(root);
    if (existing) {
        std::cout << "Project already initialized: " << existing->name << "\n";
        return 0;
    }

    auto proj = ctx.db.create_project(name, root);

    // Create default profiles
    auto dev = ctx.db.create_profile(proj.id, "development");
    ctx.db.set_variable(dev.id, "ENV", "development");
    ctx.db.set_active_profile(proj.id, dev.id);

    auto prod = ctx.db.create_profile(proj.id, "production");
    ctx.db.set_variable(prod.id, "ENV", "production");

    auto staging = ctx.db.create_profile(proj.id, "staging", dev.id);
    ctx.db.set_variable(staging.id, "ENV", "staging");

    fs::create_directories(fs::path(root) / ".envctl");

    std::cout << "Initialized envctl project: " << name << "\n";
    std::cout << "Created profiles: development (active), production, staging\n";

    // Auto-import existing .env files
    if (!no_import) {
        struct { std::string file; std::string profile; bool overwrite; } imports[] = {
            {".env",            "development", false},
            {".env.development","development", false},
            {".env.production", "production",  false},
            {".env.staging",    "staging",     false},
        };

        bool any_imported = false;
        for (auto& imp : imports) {
            fs::path p = fs::path(root) / imp.file;
            if (!fs::exists(p)) continue;

            auto prof = ctx.db.get_profile_by_name(proj.id, imp.profile);
            if (!prof) continue;

            auto result = import_env_file(ctx.db, prof->id, p.string(), imp.overwrite);
            if (result.imported > 0) {
                if (!any_imported) std::cout << "\nAuto-imported:\n";
                std::cout << "  " << imp.file << " -> " << imp.profile
                          << " (" << result.imported << " vars";
                if (result.skipped > 0) std::cout << ", " << result.skipped << " skipped";
                std::cout << ")\n";
                any_imported = true;
            }
        }
        if (!any_imported) {
            std::cout << "\nNo .env files found to import.\n";
            std::cout << "Run 'envctl import <file>' to import manually.\n";
        }
    }

    std::cout << "\nDB: " << ctx.db.db_path() << "\n";
    return 0;
}

} // namespace envctl
