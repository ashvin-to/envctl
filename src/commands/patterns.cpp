#include "commands/commands.h"
#include <iostream>

namespace envctl {

int cmd_patterns(Context& ctx, const std::vector<std::string>& args) {
    if (args.empty() || args[0] == "--help" || args[0] == "-h") {
        std::cout << "envctl patterns — Manage custom secret-masking patterns\n\n";
        std::cout << "Usage:\n";
        std::cout << "  envctl patterns add <PATTERN>     Add a custom pattern (substring, case-insensitive)\n";
        std::cout << "  envctl patterns remove <PATTERN>  Remove a custom pattern\n";
        std::cout << "  envctl patterns list              List custom patterns for this project\n";
        std::cout << "\nCustom patterns are additive to the built-in secret/password/token/key/cred\n";
        std::cout << "patterns and apply only to the current project.\n";
        return 0;
    }

    int64_t pid = ctx.require_project();
    const std::string& action = args[0];

    if (action == "list") {
        auto pats = ctx.db.list_secret_patterns(pid);
        if (pats.empty()) {
            std::cout << "  (no custom patterns)\n";
        } else {
            for (auto& p : pats) std::cout << "  " << p << "\n";
        }
        return 0;
    }

    if (action == "add" || action == "remove") {
        if (args.size() < 2) {
            std::cerr << "Usage: envctl patterns " << action << " <PATTERN>\n";
            return 1;
        }
        const std::string& pattern = args[1];
        bool ok;
        if (action == "add") {
            ok = ctx.db.add_secret_pattern(pid, pattern);
            if (ok) std::cout << "Added pattern: " << pattern << "\n";
            else std::cout << "Pattern already exists: " << pattern << "\n";
        } else {
            ok = ctx.db.remove_secret_pattern(pid, pattern);
            if (ok) std::cout << "Removed pattern: " << pattern << "\n";
            else std::cout << "Pattern not found: " << pattern << "\n";
        }
        return 0;
    }

    std::cerr << "Unknown patterns action: " << action << "\n";
    std::cerr << "Run 'envctl patterns --help' for usage.\n";
    return 1;
}

} // namespace envctl
