#pragma once
#include "cli/cli.h"

namespace envctl {

int cmd_init(Context& ctx, const std::vector<std::string>& args);
int cmd_list(Context& ctx, const std::vector<std::string>& args);
int cmd_set(Context& ctx, const std::vector<std::string>& args);
int cmd_unset(Context& ctx, const std::vector<std::string>& args);
int cmd_use(Context& ctx, const std::vector<std::string>& args);
int cmd_profiles(Context& ctx, const std::vector<std::string>& args);
int cmd_create(Context& ctx, const std::vector<std::string>& args);
int cmd_diff(Context& ctx, const std::vector<std::string>& args);
int cmd_shell(Context& ctx, const std::vector<std::string>& args);
int cmd_import(Context& ctx, const std::vector<std::string>& args);
int cmd_export(Context& ctx, const std::vector<std::string>& args);
int cmd_history(Context& ctx, const std::vector<std::string>& args);
int cmd_validate(Context& ctx, const std::vector<std::string>& args);
int cmd_mask(Context& ctx, const std::vector<std::string>& args);
int cmd_patterns(Context& ctx, const std::vector<std::string>& args);
int cmd_watch(Context& ctx, const std::vector<std::string>& args);
int cmd_sync(Context& ctx, const std::vector<std::string>& args);
int cmd_tui(Context& ctx, const std::vector<std::string>& args);
int cmd_info(Context& ctx, const std::vector<std::string>& args);

} // namespace envctl
