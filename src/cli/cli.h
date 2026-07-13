#pragma once
#include "core/context.h"
#include <string>
#include <vector>
#include <functional>

namespace envctl {

using CmdFunc = std::function<int(Context&, const std::vector<std::string>&)>;

struct Command {
    std::string name;
    std::string description;
    CmdFunc func;
};

int run_cli(int argc, char* argv[]);

} // namespace envctl
