#include "commands/commands.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace envctl {

static std::string git_root() {
    std::array<char, 256> buf;
    std::string result;
    FILE* pipe = popen("git rev-parse --show-toplevel 2>/dev/null", "r");
    if (pipe) {
        while (fgets(buf.data(), buf.size(), pipe) != nullptr)
            result += buf.data();
        pclose(pipe);
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
            result.pop_back();
    }
    return result;
}

int cmd_sync(Context& ctx, const std::vector<std::string>& args) {
    bool push = false;
    bool pull = false;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--push") push = true;
        else if (args[i] == "--pull") pull = true;
        else if (args[i] == "--help") {
            std::cout << "envctl sync — Sync profiles via git\n\n";
            std::cout << "Usage: envctl sync [--push | --pull]\n\n";
            std::cout << "Exports all profiles to .envctl/profiles/ for git tracking.\n";
            std::cout << "  --push  Commit and push changes\n";
            std::cout << "  --pull  Pull latest from git\n";
            return 0;
        }
    }

    int64_t pid = ctx.require_project();
    std::string root = git_root();
    if (root.empty()) root = ctx.project_root;

    fs::path sync_dir = fs::path(root) / ".envctl" / "profiles";
    fs::create_directories(sync_dir);

    auto profiles = ctx.db.list_profiles(pid);

    if (pull) {
        std::string cmd = "cd \"" + root + "\" && git pull";
        system(cmd.c_str());
    }

    for (auto& prof : profiles) {
        auto vars = ctx.db.list_variables(prof.id);
        fs::path out = sync_dir / (prof.name + ".env");
        std::ofstream f(out);
        for (auto& v : vars) {
            if (v.secret) f << "# SECRET: " << v.key << "=****\n";
            else f << v.key << "=\"" << v.value << "\"\n";
        }
    }

    std::cout << "Synced " << profiles.size() << " profile(s) to " << sync_dir << "\n";

    if (push) {
        std::string cmd = "cd \"" + root + "\" && git add .envctl/ && git commit -m 'envctl: sync profiles' && git push";
        system(cmd.c_str());
    }

    return 0;
}

} // namespace envctl
