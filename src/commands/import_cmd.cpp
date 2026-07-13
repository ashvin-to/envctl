#include "commands/commands.h"
#include "file/importer.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace envctl {

int cmd_import(Context& ctx, const std::vector<std::string>& args) {
    bool overwrite = false;
    std::string source;
    std::string format;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--overwrite") overwrite = true;
        else if (args[i] == "--format" && i + 1 < args.size()) format = args[++i];
        else if (args[i] == "--help") {
            std::cout << "envctl import — Import environment variables\n\n";
            std::cout << "Usage: envctl import <file> [--format <fmt>] [--overwrite]\n\n";
            std::cout << "Formats: auto (default), env, docker, json\n";
            std::cout << "  .env files       -> env\n";
            std::cout << "  docker-compose.* -> docker\n";
            std::cout << "  *.json           -> json\n";
            return 0;
        }
        else if (source.empty()) source = args[i];
    }

    if (source.empty()) {
        std::cerr << "Usage: envctl import <file> [--format <fmt>] [--overwrite]\n";
        return 1;
    }

    if (!fs::exists(source)) {
        std::cerr << "File not found: " << source << "\n";
        return 1;
    }

    if (format.empty()) {
        auto ext = fs::path(source).extension().string();
        if (ext == ".json") format = "json";
        else if (fs::path(source).filename().string().find("docker-compose") != std::string::npos) format = "docker";
        else format = "env";
    }

    int64_t pid = ctx.require_profile();

    ImportResult result;
    if (format == "json") result = import_json(ctx.db, pid, source, overwrite);
    else if (format == "docker") result = import_docker_compose(ctx.db, pid, source, overwrite);
    else result = import_env_file(ctx.db, pid, source, overwrite);

    std::cout << "Imported: " << result.imported << ", Skipped: " << result.skipped << "\n";
    for (auto& e : result.errors) std::cerr << "Error: " << e << "\n";
    return result.errors.empty() ? 0 : 1;
}

} // namespace envctl
