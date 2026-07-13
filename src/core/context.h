#pragma once
#include "core/database.h"

namespace envctl {

struct Context {
    Database db;
    std::string project_root;
    std::string home_dir;

    bool init(const std::string& root = ".");
    std::optional<Project> current_project();
    std::optional<Profile> current_profile();
    int64_t require_project();
    int64_t require_profile();
};

std::string get_home_dir();
std::string get_cwd();
std::string get_username();

} // namespace envctl
