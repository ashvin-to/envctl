#pragma once
#include <string>
#include <vector>
#include <map>

namespace envctl {

struct EnvVar {
    std::string key;
    std::string value;
    std::string comment;
    bool is_secret = false;
};

struct EnvFile {
    std::string path;
    std::vector<EnvVar> vars;

    static EnvFile parse(const std::string& path);
    static EnvFile parse_string(const std::string& content);
    std::string to_env_string() const;
    std::string to_json() const;
};

std::map<std::string, std::string> parse_env_file(const std::string& path);
std::string strip_quotes(const std::string& s);

} // namespace envctl
