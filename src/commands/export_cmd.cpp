#include "commands/commands.h"
#include "file/envfile.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace envctl {

int cmd_export(Context& ctx, const std::vector<std::string>& args) {
    std::string format = "env";
    std::string output;
    bool resolved = false;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--format" && i + 1 < args.size()) format = args[++i];
        else if (args[i] == "--output" && i + 1 < args.size()) output = args[++i];
        else if (args[i] == "--resolved" || args[i] == "-r") resolved = true;
        else if (args[i] == "--help") {
            std::cout << "envctl export — Export environment variables\n\n";
            std::cout << "Usage: envctl export [--format <fmt>] [--output <file>] [--resolved]\n\n";
            std::cout << "Formats: env (default), json, shell, docker\n";
            return 0;
        }
    }

    int64_t pid = ctx.require_profile();
    auto vars = resolved ? ctx.db.resolve_variables(pid) : ctx.db.list_variables(pid);

    std::ostringstream out;

    if (format == "json") {
        out << "{\n";
        for (size_t i = 0; i < vars.size(); ++i) {
            out << "  \"" << vars[i].key << "\": \"" << vars[i].value << "\"";
            if (i + 1 < vars.size()) out << ",";
            out << "\n";
        }
        out << "}\n";
    } else if (format == "shell") {
        for (auto& v : vars)
            out << "export " << v.key << "=\"" << v.value << "\"\n";
    } else if (format == "docker") {
        out << "version: '3'\n\nservices:\n  app:\n    environment:\n";
        for (auto& v : vars)
            out << "      - " << v.key << "=" << v.value << "\n";
    } else {
        for (auto& v : vars)
            out << v.key << "=\"" << v.value << "\"\n";
    }

    if (output.empty()) {
        std::cout << out.str();
    } else {
        std::ofstream f(output);
        f << out.str();
        std::cout << "Exported to " << output << "\n";
    }
    return 0;
}

} // namespace envctl
