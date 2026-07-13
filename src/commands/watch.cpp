#include "commands/commands.h"
#include "file/envfile.h"
#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <map>
#include <sys/inotify.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace envctl {

int cmd_watch(Context& ctx, const std::vector<std::string>& args) {
    std::string file_path;
    bool reload = true;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--no-reload") reload = false;
        else if (args[i] == "--help") {
            std::cout << "envctl watch — Watch a .env file for changes\n\n";
            std::cout << "Usage: envctl watch <file.env> [--no-reload]\n";
            std::cout << "\nWatches the file and imports changes automatically.\n";
            return 0;
        }
        else if (file_path.empty()) file_path = args[i];
    }

    if (file_path.empty()) {
        std::cerr << "Usage: envctl watch <file.env>\n";
        return 1;
    }

    if (!fs::exists(file_path)) {
        std::cerr << "File not found: " << file_path << "\n";
        return 1;
    }

    int64_t pid = ctx.require_profile();

    // Initial load
    auto prev = parse_env_file(file_path);
    for (auto& [k, v] : prev) {
        ctx.db.set_variable(pid, k, v);
    }
    std::cout << "Watching " << file_path << " for changes... (Ctrl+C to stop)\n";

    int fd = inotify_init();
    if (fd < 0) {
        std::cerr << "inotify_init failed\n";
        return 1;
    }

    int wd = inotify_add_watch(fd, file_path.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
    if (wd < 0) {
        std::cerr << "inotify_add_watch failed\n";
        close(fd);
        return 1;
    }

    char buf[4096];
    while (true) {
        int len = read(fd, buf, sizeof(buf));
        if (len > 0) {
            auto current = parse_env_file(file_path);

            // Detect changes
            for (auto& [k, v] : current) {
                auto it = prev.find(k);
                if (it == prev.end()) {
                    std::cout << "  + " << k << " = " << v << "\n";
                    ctx.db.set_variable(pid, k, v);
                } else if (it->second != v) {
                    std::cout << "  ~ " << k << ": " << it->second << " -> " << v << "\n";
                    ctx.db.set_variable(pid, k, v);
                }
            }
            for (auto& [k, v] : prev) {
                if (current.find(k) == current.end()) {
                    std::cout << "  - " << k << "\n";
                    ctx.db.delete_variable(pid, k);
                }
            }
            prev = current;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    inotify_rm_watch(fd, wd);
    close(fd);
    return 0;
}

} // namespace envctl
