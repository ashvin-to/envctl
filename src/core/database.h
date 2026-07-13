#pragma once
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <sqlite3.h>

namespace envctl {

struct Project {
    int64_t id = 0;
    std::string name;
    std::string root_path;
    std::string created_at;
};

struct Profile {
    int64_t id = 0;
    int64_t project_id = 0;
    std::string name;
    int64_t parent_id = 0;
};

struct Variable {
    int64_t id = 0;
    int64_t profile_id = 0;
    std::string key;
    std::string value;
    bool secret = false;
    std::string description;
    std::string created_at;
    std::string updated_at;
};

struct HistoryEntry {
    int64_t id = 0;
    int64_t variable_id = 0;
    std::string old_value;
    std::string new_value;
    std::string changed_at;
    std::string changed_by;
};

struct Template {
    int64_t id = 0;
    int64_t project_id = 0;
    std::string name;
    std::string example_json;
};

struct ValidationResult {
    bool valid = true;
    std::string key;
    std::string error;
};

class Database {
public:
    Database();
    ~Database();

    bool open(const std::string& path);
    bool open_in_dir(const std::string& dir);
    void close();
    bool is_open() const { return db_ != nullptr; }
    std::string db_path() const { return db_path_; }

    bool init_schema();

    // Projects
    Project create_project(const std::string& name, const std::string& root_path);
    std::optional<Project> get_project(int64_t id);
    std::optional<Project> get_project_by_path(const std::string& path);
    std::vector<Project> list_projects();

    // Profiles
    Profile create_profile(int64_t project_id, const std::string& name, int64_t parent_id = 0);
    std::optional<Profile> get_profile(int64_t id);
    std::optional<Profile> get_profile_by_name(int64_t project_id, const std::string& name);
    std::vector<Profile> list_profiles(int64_t project_id);
    bool delete_profile(int64_t id);

    // Variables
    Variable set_variable(int64_t profile_id, const std::string& key, const std::string& value,
                          bool secret = false, const std::string& description = "");
    std::optional<Variable> get_variable(int64_t profile_id, const std::string& key);
    std::vector<Variable> list_variables(int64_t profile_id, bool mask_secrets = false);
    bool delete_variable(int64_t profile_id, const std::string& key);

    // History
    void record_history(int64_t variable_id, const std::string& old_value,
                        const std::string& new_value, const std::string& changed_by);
    std::vector<HistoryEntry> get_history(int64_t variable_id);
    std::vector<HistoryEntry> get_all_history(int64_t profile_id, int limit = 50);

    // Templates
    Template create_template(int64_t project_id, const std::string& name, const std::string& example_json);
    std::vector<Template> list_templates(int64_t project_id);

    // Active profile
    bool set_active_profile(int64_t project_id, int64_t profile_id);
    std::optional<int64_t> get_active_profile(int64_t project_id);

    // Resolve inherited variables (walk parent chain)
    std::vector<Variable> resolve_variables(int64_t profile_id, bool mask_secrets = false);

private:
    sqlite3* db_ = nullptr;
    std::string db_path_;
    bool exec(const std::string& sql);
    bool exec_simple(const std::string& sql);
    int64_t last_insert_rowid();
};

} // namespace envctl
