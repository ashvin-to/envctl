#include "commands/commands.h"
#include <iostream>
#include <regex>
#include <fstream>
#include <sstream>
#include "file/envfile.h"

namespace envctl {

struct ValidationRule {
    std::string key;
    std::string type;
    bool required;
    std::string pattern;
};

static bool validate_type(const std::string& value, const std::string& type) {
    if (type == "url") {
        return value.find("://") != std::string::npos;
    } else if (type == "email") {
        return value.find("@") != std::string::npos && value.find(".") != std::string::npos;
    } else if (type == "int" || type == "integer") {
        for (char c : value) if (!isdigit(c) && c != '-' && c != '+') return false;
        return !value.empty();
    } else if (type == "bool" || type == "boolean") {
        return value == "true" || value == "false" || value == "1" || value == "0";
    } else if (type == "port") {
        try { int p = std::stoi(value); return p >= 1 && p <= 65535; }
        catch (...) { return false; }
    } else if (type == "path") {
        return value.size() > 0 && value[0] == '/';
    }
    return true;
}

int cmd_validate(Context& ctx, const std::vector<std::string>& args) {
    std::string template_file;
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--template" && i + 1 < args.size()) template_file = args[++i];
        else if (args[i] == "--help") {
            std::cout << "envctl validate — Validate variables against a template\n\n";
            std::cout << "Usage: envctl validate [--template <.env.example>]\n\n";
            std::cout << "Validates types: url, email, int, bool, port, path\n";
            std::cout << "Required vars must be non-empty.\n";
            return 0;
        }
    }

    int64_t pid = ctx.require_profile();
    auto vars_list = ctx.db.list_variables(pid);
    std::map<std::string, Variable> vars;
    for (auto& v : vars_list) vars[v.key] = v;

    std::vector<ValidationRule> rules;

    if (!template_file.empty()) {
        auto ef = EnvFile::parse(template_file);
        for (auto& v : ef.vars) {
            ValidationRule rule;
            rule.key = v.key;
            rule.required = !v.value.empty();

            // Check for type hints in comments: # @type url
            if (!v.comment.empty()) {
                auto type_pos = v.comment.find("@type");
                if (type_pos != std::string::npos) {
                    rule.type = v.comment.substr(type_pos + 6);
                    while (!rule.type.empty() && rule.type.back() == ' ')
                        rule.type.pop_back();
                }
            }
            rules.push_back(rule);
        }
    } else {
        for (auto& [k, v] : vars) {
            ValidationRule rule;
            rule.key = k;
            rule.required = false;
            rules.push_back(rule);
        }
    }

    int errors = 0;
    for (auto& rule : rules) {
        auto it = vars.find(rule.key);
        bool exists = it != vars.end();
        std::string value = exists ? it->second.value : "";

        if (rule.required && (!exists || value.empty())) {
            std::cerr << "  MISSING: " << rule.key << " (required)\n";
            errors++;
            continue;
        }
        if (exists && !rule.type.empty() && !validate_type(value, rule.type)) {
            std::cerr << "  INVALID: " << rule.key << " (expected " << rule.type << ")\n";
            errors++;
            continue;
        }
        std::cout << "  OK: " << rule.key << "\n";
    }

    if (errors == 0) std::cout << "\nAll validations passed.\n";
    else std::cout << "\n" << errors << " validation error(s).\n";
    return errors == 0 ? 0 : 1;
}

} // namespace envctl
