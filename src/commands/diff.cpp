#include "commands/commands.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <map>

namespace envctl {

int cmd_diff(Context& ctx, const std::vector<std::string>& args) {
    if (args.size() < 2 || args[0] == "--help") {
        std::cout << "envctl diff — Compare two profiles\n\n";
        std::cout << "Usage: envctl diff <profile1> <profile2>\n";
        return (args.size() < 2) ? 1 : 0;
    }

    int64_t pid = ctx.require_project();
    auto p1 = ctx.db.get_profile_by_name(pid, args[0]);
    auto p2 = ctx.db.get_profile_by_name(pid, args[1]);

    if (!p1) { std::cerr << "Profile '" << args[0] << "' not found\n"; return 1; }
    if (!p2) { std::cerr << "Profile '" << args[1] << "' not found\n"; return 1; }

    auto v1 = ctx.db.resolve_variables(p1->id);
    auto v2 = ctx.db.resolve_variables(p2->id);

    std::map<std::string, std::string> m1, m2;
    for (auto& v : v1) m1[v.key] = v.value;
    for (auto& v : v2) m2[v.key] = v.value;

    std::cout << "Diff: " << args[0] << " vs " << args[1] << "\n\n";

    bool any = false;

    // Keys only in first
    for (auto& [k, val] : m1) {
        if (m2.find(k) == m2.end()) {
            std::cout << "  - " << k << " = " << val << "\n";
            any = true;
        }
    }

    // Keys only in second
    for (auto& [k, val] : m2) {
        if (m1.find(k) == m1.end()) {
            std::cout << "  + " << k << " = " << val << "\n";
            any = true;
        }
    }

    // Changed values
    for (auto& [k, val] : m1) {
        auto it = m2.find(k);
        if (it != m2.end() && it->second != val) {
            std::cout << "  ~ " << k << ": " << val << " -> " << it->second << "\n";
            any = true;
        }
    }

    if (!any) std::cout << "  (no differences)\n";
    return 0;
}

} // namespace envctl
