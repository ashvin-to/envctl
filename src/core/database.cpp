#include "core/database.h"
#include "schema/schema.h"
#include <filesystem>
#include <stdexcept>
#include <iostream>
#include <map>
#include <algorithm>

namespace fs = std::filesystem;

namespace envctl {

Database::Database() {}
Database::~Database() { close(); }

bool Database::open(const std::string& path) {
    if (db_) close();
    db_path_ = path;
    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db_) << "\n";
        db_ = nullptr;
        return false;
    }
    exec("PRAGMA journal_mode=WAL;");
    exec("PRAGMA foreign_keys=ON;");
    return true;
}

bool Database::open_in_dir(const std::string& dir) {
    fs::path p = fs::path(dir) / ".envctl" / "envctl.db";
    fs::create_directories(p.parent_path());
    return open(p.string());
}

void Database::close() {
    if (db_) { sqlite3_close(db_); db_ = nullptr; }
}

bool Database::exec(const std::string& sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        if (err) { sqlite3_free(err); }
        return false;
    }
    return true;
}

bool Database::exec_simple(const std::string& sql) {
    return exec(sql);
}

int64_t Database::last_insert_rowid() {
    return sqlite3_last_insert_rowid(db_);
}

bool Database::init_schema() {
    return exec(SCHEMA_SQL);
}

// --- Projects ---

Project Database::create_project(const std::string& name, const std::string& root_path) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "INSERT INTO projects (name, root_path) VALUES (?, ?)", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, root_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return get_project(last_insert_rowid()).value();
}

std::optional<Project> Database::get_project(int64_t id) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT id, name, root_path, created_at FROM projects WHERE id=?", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Project p;
        p.id = sqlite3_column_int64(stmt, 0);
        p.name = (const char*)sqlite3_column_text(stmt, 1);
        p.root_path = (const char*)sqlite3_column_text(stmt, 2);
        p.created_at = (const char*)sqlite3_column_text(stmt, 3);
        sqlite3_finalize(stmt);
        return p;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::optional<Project> Database::get_project_by_path(const std::string& path) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT id, name, root_path, created_at FROM projects WHERE root_path=?", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Project p;
        p.id = sqlite3_column_int64(stmt, 0);
        p.name = (const char*)sqlite3_column_text(stmt, 1);
        p.root_path = (const char*)sqlite3_column_text(stmt, 2);
        p.created_at = (const char*)sqlite3_column_text(stmt, 3);
        sqlite3_finalize(stmt);
        return p;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<Project> Database::list_projects() {
    std::vector<Project> out;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT id, name, root_path, created_at FROM projects ORDER BY name", -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Project p;
        p.id = sqlite3_column_int64(stmt, 0);
        p.name = (const char*)sqlite3_column_text(stmt, 1);
        p.root_path = (const char*)sqlite3_column_text(stmt, 2);
        p.created_at = (const char*)sqlite3_column_text(stmt, 3);
        out.push_back(p);
    }
    sqlite3_finalize(stmt);
    return out;
}

// --- Profiles ---

Profile Database::create_profile(int64_t project_id, const std::string& name, int64_t parent_id) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "INSERT INTO profiles (project_id, name, parent_id) VALUES (?, ?, ?)", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, project_id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, parent_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return get_profile(last_insert_rowid()).value();
}

std::optional<Profile> Database::get_profile(int64_t id) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT id, project_id, name, parent_id FROM profiles WHERE id=?", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Profile p;
        p.id = sqlite3_column_int64(stmt, 0);
        p.project_id = sqlite3_column_int64(stmt, 1);
        p.name = (const char*)sqlite3_column_text(stmt, 2);
        p.parent_id = sqlite3_column_int64(stmt, 3);
        sqlite3_finalize(stmt);
        return p;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::optional<Profile> Database::get_profile_by_name(int64_t project_id, const std::string& name) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT id, project_id, name, parent_id FROM profiles WHERE project_id=? AND name=?", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, project_id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Profile p;
        p.id = sqlite3_column_int64(stmt, 0);
        p.project_id = sqlite3_column_int64(stmt, 1);
        p.name = (const char*)sqlite3_column_text(stmt, 2);
        p.parent_id = sqlite3_column_int64(stmt, 3);
        sqlite3_finalize(stmt);
        return p;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<Profile> Database::list_profiles(int64_t project_id) {
    std::vector<Profile> out;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT id, project_id, name, parent_id FROM profiles WHERE project_id=? ORDER BY name", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, project_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Profile p;
        p.id = sqlite3_column_int64(stmt, 0);
        p.project_id = sqlite3_column_int64(stmt, 1);
        p.name = (const char*)sqlite3_column_text(stmt, 2);
        p.parent_id = sqlite3_column_int64(stmt, 3);
        out.push_back(p);
    }
    sqlite3_finalize(stmt);
    return out;
}

bool Database::delete_profile(int64_t id) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "DELETE FROM variables WHERE profile_id=?", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db_, "DELETE FROM profiles WHERE id=?", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, id);
    sqlite3_step(stmt);
    bool ok = sqlite3_changes(db_) > 0;
    sqlite3_finalize(stmt);
    return ok;
}

// --- Variables ---

Variable Database::set_variable(int64_t profile_id, const std::string& key, const std::string& value,
                                bool secret, const std::string& description) {
    auto existing = get_variable(profile_id, key);
    if (existing) {
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_,
            "UPDATE variables SET value_enc=?, secret=?, description=?, updated_at=datetime('now') WHERE id=?",
            -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, value.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, secret ? 1 : 0);
        sqlite3_bind_text(stmt, 3, description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 4, existing->id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return get_variable(profile_id, key).value();
    }

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "INSERT INTO variables (profile_id, key, value_enc, secret, description) VALUES (?, ?, ?, ?, ?)",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, profile_id);
    sqlite3_bind_text(stmt, 2, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, secret ? 1 : 0);
    sqlite3_bind_text(stmt, 5, description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return get_variable(profile_id, key).value();
}

std::optional<Variable> Database::get_variable(int64_t profile_id, const std::string& key) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "SELECT id, profile_id, key, value_enc, secret, description, created_at, updated_at "
        "FROM variables WHERE profile_id=? AND key=?", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, profile_id);
    sqlite3_bind_text(stmt, 2, key.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Variable v;
        v.id = sqlite3_column_int64(stmt, 0);
        v.profile_id = sqlite3_column_int64(stmt, 1);
        v.key = (const char*)sqlite3_column_text(stmt, 2);
        v.value = (const char*)sqlite3_column_text(stmt, 3);
        v.secret = sqlite3_column_int(stmt, 4) != 0;
        auto desc = sqlite3_column_text(stmt, 5);
        v.description = desc ? (const char*)desc : "";
        auto ca = sqlite3_column_text(stmt, 6);
        v.created_at = ca ? (const char*)ca : "";
        auto ua = sqlite3_column_text(stmt, 7);
        v.updated_at = ua ? (const char*)ua : "";
        sqlite3_finalize(stmt);
        return v;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<Variable> Database::list_variables(int64_t profile_id, bool mask_secrets) {
    std::vector<Variable> out;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "SELECT id, profile_id, key, value_enc, secret, description, created_at, updated_at "
        "FROM variables WHERE profile_id=? ORDER BY key", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, profile_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Variable v;
        v.id = sqlite3_column_int64(stmt, 0);
        v.profile_id = sqlite3_column_int64(stmt, 1);
        v.key = (const char*)sqlite3_column_text(stmt, 2);
        v.value = (const char*)sqlite3_column_text(stmt, 3);
        v.secret = sqlite3_column_int(stmt, 4) != 0;
        auto desc = sqlite3_column_text(stmt, 5);
        v.description = desc ? (const char*)desc : "";
        auto ca = sqlite3_column_text(stmt, 6);
        v.created_at = ca ? (const char*)ca : "";
        auto ua = sqlite3_column_text(stmt, 7);
        v.updated_at = ua ? (const char*)ua : "";
        if (mask_secrets && v.secret) {
            v.value = "****";
        }
        out.push_back(v);
    }
    sqlite3_finalize(stmt);
    return out;
}

bool Database::delete_variable(int64_t profile_id, const std::string& key) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "DELETE FROM variables WHERE profile_id=? AND key=?", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, profile_id);
    sqlite3_bind_text(stmt, 2, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    bool ok = sqlite3_changes(db_) > 0;
    sqlite3_finalize(stmt);
    return ok;
}

// --- History ---

void Database::record_history(int64_t variable_id, const std::string& old_value,
                              const std::string& new_value, const std::string& changed_by) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "INSERT INTO history (variable_id, old_value_enc, new_value_enc, changed_by) VALUES (?, ?, ?, ?)",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, variable_id);
    sqlite3_bind_text(stmt, 2, old_value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, new_value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, changed_by.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<HistoryEntry> Database::get_history(int64_t variable_id) {
    std::vector<HistoryEntry> out;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "SELECT id, variable_id, old_value_enc, new_value_enc, changed_at, changed_by "
        "FROM history WHERE variable_id=? ORDER BY changed_at DESC", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, variable_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        HistoryEntry h;
        h.id = sqlite3_column_int64(stmt, 0);
        h.variable_id = sqlite3_column_int64(stmt, 1);
        auto ov = sqlite3_column_text(stmt, 2);
        h.old_value = ov ? (const char*)ov : "";
        auto nv = sqlite3_column_text(stmt, 3);
        h.new_value = nv ? (const char*)nv : "";
        auto ca = sqlite3_column_text(stmt, 4);
        h.changed_at = ca ? (const char*)ca : "";
        auto cb = sqlite3_column_text(stmt, 5);
        h.changed_by = cb ? (const char*)cb : "";
        out.push_back(h);
    }
    sqlite3_finalize(stmt);
    return out;
}

std::vector<HistoryEntry> Database::get_all_history(int64_t profile_id, int limit) {
    std::vector<HistoryEntry> out;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "SELECT h.id, h.variable_id, h.old_value_enc, h.new_value_enc, h.changed_at, h.changed_by "
        "FROM history h JOIN variables v ON h.variable_id = v.id "
        "WHERE v.profile_id=? ORDER BY h.changed_at DESC LIMIT ?", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, profile_id);
    sqlite3_bind_int(stmt, 2, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        HistoryEntry h;
        h.id = sqlite3_column_int64(stmt, 0);
        h.variable_id = sqlite3_column_int64(stmt, 1);
        auto ov = sqlite3_column_text(stmt, 2);
        h.old_value = ov ? (const char*)ov : "";
        auto nv = sqlite3_column_text(stmt, 3);
        h.new_value = nv ? (const char*)nv : "";
        auto ca = sqlite3_column_text(stmt, 4);
        h.changed_at = ca ? (const char*)ca : "";
        auto cb = sqlite3_column_text(stmt, 5);
        h.changed_by = cb ? (const char*)cb : "";
        out.push_back(h);
    }
    sqlite3_finalize(stmt);
    return out;
}

// --- Templates ---

Template Database::create_template(int64_t project_id, const std::string& name, const std::string& example_json) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "INSERT INTO templates (project_id, name, example_json) VALUES (?, ?, ?)",
                       -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, project_id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, example_json.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    Template t;
    t.id = last_insert_rowid();
    t.project_id = project_id;
    t.name = name;
    t.example_json = example_json;
    return t;
}

std::vector<Template> Database::list_templates(int64_t project_id) {
    std::vector<Template> out;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT id, project_id, name, example_json FROM templates WHERE project_id=?",
                       -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, project_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Template t;
        t.id = sqlite3_column_int64(stmt, 0);
        t.project_id = sqlite3_column_int64(stmt, 1);
        t.name = (const char*)sqlite3_column_text(stmt, 2);
        auto ej = sqlite3_column_text(stmt, 3);
        t.example_json = ej ? (const char*)ej : "";
        out.push_back(t);
    }
    sqlite3_finalize(stmt);
    return out;
}

// --- Active Profile ---

bool Database::set_active_profile(int64_t project_id, int64_t profile_id) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "INSERT OR REPLACE INTO active_profile (project_id, profile_id, activated_at) VALUES (?, ?, datetime('now'))",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, project_id);
    sqlite3_bind_int64(stmt, 2, profile_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return true;
}

std::optional<int64_t> Database::get_active_profile(int64_t project_id) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT profile_id FROM active_profile WHERE project_id=?", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, project_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t pid = sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
        return pid;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

// --- Resolve with inheritance ---

std::vector<Variable> Database::resolve_variables(int64_t profile_id, bool mask_secrets) {
    std::vector<Variable> out;
    std::vector<int64_t> chain;
    int64_t cur = profile_id;
    while (cur > 0) {
        chain.push_back(cur);
        auto prof = get_profile(cur);
        if (!prof || prof->parent_id == 0) break;
        cur = prof->parent_id;
    }

    std::map<std::string, Variable> merged;
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        auto vars = list_variables(*it, false);
        for (auto& v : vars) {
            merged[v.key] = v;
        }
    }
    for (auto& [k, v] : merged) {
        if (mask_secrets && v.secret) v.value = "****";
        out.push_back(v);
    }
    std::sort(out.begin(), out.end(), [](const Variable& a, const Variable& b) { return a.key < b.key; });
    return out;
}

} // namespace envctl
