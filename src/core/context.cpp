#include "core/context.h"
#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

namespace fs = std::filesystem;

namespace envctl {

std::string get_home_dir() {
    const char* h = std::getenv("HOME");
    return h ? std::string(h) : "/tmp";
}

std::string get_cwd() {
    return fs::current_path().string();
}

std::string get_username() {
    const char* u = std::getenv("USER");
    return u ? std::string(u) : "unknown";
}

bool Context::init(const std::string& root) {
    fs::path p = fs::absolute(root);
    std::error_code ec;
    p = fs::canonical(p, ec);
    if (ec) p = fs::absolute(root);
    project_root = p.string();
    home_dir = get_home_dir();
    std::string db_path = home_dir + "/.config/envctl/envctl.db";
    fs::create_directories(fs::path(db_path).parent_path());
    if (!db.open(db_path)) return false;
    db.init_schema();
    return true;
}

std::optional<Project> Context::current_project() {
    return db.get_project_by_path(project_root);
}

std::optional<Profile> Context::current_profile() {
    auto proj = current_project();
    if (!proj) return std::nullopt;
    auto active = db.get_active_profile(proj->id);
    if (!active) return std::nullopt;
    return db.get_profile(*active);
}

int64_t Context::require_project() {
    auto proj = current_project();
    if (!proj) {
        std::cerr << "No project found. Run 'envctl init' first.\n";
        std::exit(1);
    }
    return proj->id;
}

int64_t Context::require_profile() {
    auto prof = current_profile();
    if (!prof) {
        std::cerr << "No active profile. Run 'envctl use <profile>' first.\n";
        std::exit(1);
    }
    return prof->id;
}

} // namespace envctl
