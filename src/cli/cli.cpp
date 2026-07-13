#include "cli/cli.h"
#include "commands/commands.h"
#include <iostream>
#include <algorithm>

namespace envctl {

static void print_usage(const std::vector<Command>& cmds) {
    std::cout << "envctl — Local Environment Manager\n\n";
    std::cout << "Usage: envctl <command> [args]\n\n";
    std::cout << "Commands:\n";
    for (auto& c : cmds) {
        printf("  %-16s %s\n", c.name.c_str(), c.description.c_str());
    }
    std::cout << "\nRun 'envctl <command> --help' for command-specific help.\n";
}

int run_cli(int argc, char* argv[]) {
    Context ctx;
    ctx.init();

    std::vector<Command> commands = {
        {"init",     "Initialize envctl for this project",     cmd_init},
        {"list",     "List variables in active profile",       cmd_list},
        {"set",      "Set a variable",                         cmd_set},
        {"unset",    "Remove a variable",                      cmd_unset},
        {"use",      "Switch active profile",                  cmd_use},
        {"profiles", "List profiles",                          cmd_profiles},
        {"create",   "Create a new profile",                   cmd_create},
        {"diff",     "Diff two profiles",                      cmd_diff},
        {"shell",    "Shell integration (eval output)",        cmd_shell},
        {"import",   "Import from .env, Docker, JSON",         cmd_import},
        {"export",   "Export to .env, JSON, shell export",     cmd_export},
        {"history",  "View change history",                    cmd_history},
        {"validate", "Validate variables against template",    cmd_validate},
        {"mask",     "List variables with masked secrets",     cmd_mask},
        {"watch",    "Watch .env file for changes",            cmd_watch},
        {"sync",     "Sync profiles via git",                  cmd_sync},
        {"tui",      "Launch terminal UI",                     cmd_tui},
        {"info",     "Show project and profile details",       cmd_info},
    };

    if (argc < 2) {
        print_usage(commands);
        return 0;
    }

    std::string cmd_name = argv[1];
    if (cmd_name == "--help" || cmd_name == "-h") {
        print_usage(commands);
        return 0;
    }

    for (auto& c : commands) {
        if (c.name == cmd_name) {
            std::vector<std::string> args(argv + 2, argv + argc);
            return c.func(ctx, args);
        }
    }

    std::cerr << "Unknown command: " << cmd_name << "\n";
    print_usage(commands);
    return 1;
}

} // namespace envctl
