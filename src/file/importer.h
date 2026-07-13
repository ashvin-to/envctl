#pragma once
#include "core/database.h"
#include <string>

namespace envctl {

struct ImportResult {
    int imported = 0;
    int skipped = 0;
    std::vector<std::string> errors;
};

struct DockerComposeVar {
    std::string key;
    std::string value;
};

ImportResult import_env_file(Database& db, int64_t profile_id, const std::string& path, bool overwrite = false);
ImportResult import_docker_compose(Database& db, int64_t profile_id, const std::string& path, bool overwrite = false);
ImportResult import_json(Database& db, int64_t profile_id, const std::string& path, bool overwrite = false);

} // namespace envctl
