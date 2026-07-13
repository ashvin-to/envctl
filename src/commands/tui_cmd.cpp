#include "commands/commands.h"
#include "tui/tui.h"

namespace envctl {

int cmd_tui(Context& ctx, const std::vector<std::string>& args) {
    if (!args.empty() && args[0] == "--help") {
        return 0;
    }
    return run_tui(ctx);
}

} // namespace envctl
