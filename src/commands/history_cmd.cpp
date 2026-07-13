#include "commands/commands.h"
#include <iostream>
#include <iomanip>
#include <map>

namespace envctl {

int cmd_history(Context& ctx, const std::vector<std::string>& args) {
    int limit = 50;
    std::string key;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--limit" && i + 1 < args.size()) limit = std::stoi(args[++i]);
        else if (args[i] == "--key" && i + 1 < args.size()) key = args[++i];
        else if (args[i] == "--help") {
            std::cout << "envctl history — View change history\n\n";
            std::cout << "Usage: envctl history [--key <key>] [--limit <n>]\n";
            return 0;
        }
    }

    int64_t pid = ctx.require_profile();
    std::vector<HistoryEntry> entries;

    if (!key.empty()) {
        auto v = ctx.db.get_variable(pid, key);
        if (!v) {
            std::cerr << "Variable '" << key << "' not found\n";
            return 1;
        }
        entries = ctx.db.get_history(v->id);
    } else {
        entries = ctx.db.get_all_history(pid, limit);
    }

    if (entries.empty()) {
        std::cout << "  (no history)\n";
        return 0;
    }

    auto vars = ctx.db.list_variables(pid);
    std::map<int64_t, std::string> var_names;
    for (auto& v : vars) var_names[v.id] = v.key;

    for (auto& h : entries) {
        std::string name = var_names.count(h.variable_id) ? var_names[h.variable_id] : "?";
        printf("  %s  %-12s  %-20s -> %s  (by %s)\n",
               h.changed_at.c_str(), name.c_str(),
               h.old_value.empty() ? "(new)" : h.old_value.c_str(),
               h.new_value.empty() ? "(deleted)" : h.new_value.c_str(),
               h.changed_by.c_str());
    }
    return 0;
}

} // namespace envctl
