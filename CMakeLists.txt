cmake_minimum_required(VERSION 3.16)
project(envctl VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(NCURSES REQUIRED ncursesw)

find_package(OpenSSL REQUIRED)

add_executable(envctl
    src/main.cpp
    src/core/database.cpp
    src/core/context.cpp
    src/file/envfile.cpp
    src/file/importer.cpp
    src/crypto/encrypt.cpp
    src/schema/schema.cpp
    src/cli/cli.cpp
    src/commands/init.cpp
    src/commands/list.cpp
    src/commands/set.cpp
    src/commands/unset.cpp
    src/commands/use.cpp
    src/commands/profiles.cpp
    src/commands/diff.cpp
    src/commands/shell.cpp
    src/commands/import_cmd.cpp
    src/commands/export_cmd.cpp
    src/commands/history_cmd.cpp
    src/commands/validate.cpp
    src/commands/mask.cpp
    src/commands/patterns.cpp
    src/commands/watch.cpp
    src/commands/sync.cpp
    src/commands/tui_cmd.cpp
    src/commands/info.cpp
    src/tui/tui.cpp
)

target_include_directories(envctl PRIVATE
    src
    ${SQLITE3_INCLUDE_DIRS}
    ${NCURSES_INCLUDE_DIRS}
)

target_link_libraries(envctl PRIVATE
    ${SQLITE3_LIBRARIES}
    ${NCURSES_LIBRARIES}
    OpenSSL::Crypto
    stdc++fs
)

target_compile_options(envctl PRIVATE
    ${SQLITE3_CFLAGS_OTHER}
    ${NCURSES_CFLAGS_OTHER}
    -Wall -Wextra -O2
)

install(TARGETS envctl DESTINATION bin)
