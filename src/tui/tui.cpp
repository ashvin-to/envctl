#include "tui/tui.h"
#include "core/context.h"
#include <ncurses.h>
#include <cstring>
#include <vector>
#include <string>

namespace envctl {

enum class Panel { PROFILES, VARIABLES, HISTORY };

enum class InputMode { NONE, EDIT_KEY, EDIT_VALUE, NEW_KEY, NEW_VALUE, CONFIRM_DELETE };

struct TuiState {
    Context& ctx;
    Panel panel = Panel::PROFILES;
    int selected = 0;
    bool running = true;
    bool show_help = false;
    InputMode input_mode = InputMode::NONE;
    std::string input_buf;
    std::string pending_key;
    std::string pending_value;
    bool pending_secret = false;

    std::vector<Profile> profiles;
    std::vector<Variable> variables;
    std::vector<HistoryEntry> history;

    void load_data() {
        int64_t pid = ctx.require_project();
        profiles = ctx.db.list_profiles(pid);
        auto active = ctx.db.get_active_profile(pid);
        if (active) {
            variables = ctx.db.list_variables(*active);
            history = ctx.db.get_all_history(*active);
        } else {
            variables.clear();
            history.clear();
        }
    }

    int64_t active_profile_id() {
        auto active = ctx.db.get_active_profile(ctx.require_project());
        return active ? *active : 0;
    }
};

static const char* input_mode_prompt(InputMode m) {
    switch (m) {
        case InputMode::EDIT_KEY:       return "New key (Enter to keep): ";
        case InputMode::EDIT_VALUE:     return "New value: ";
        case InputMode::NEW_KEY:        return "Variable name: ";
        case InputMode::NEW_VALUE:      return "Value: ";
        case InputMode::CONFIRM_DELETE: return "Delete this variable? (y/N): ";
        default: return "";
    }
}

static void draw_title(WINDOW* win, int w, Context& ctx) {
    auto proj = ctx.current_project();
    std::string title = "=== envctl";
    if (proj) {
        title += "  " + proj->name + " (id=" + std::to_string(proj->id) + ")";
    } else {
        title += "  Environment Manager";
    }
    title += " ===";

    attron(A_BOLD | COLOR_PAIR(1));
    mvwprintw(win, 0, (w - title.size()) / 2, "%s", title.c_str());
    attroff(A_BOLD | COLOR_PAIR(1));

    auto active = ctx.current_profile();
    if (active) {
        attron(COLOR_PAIR(2));
        mvwprintw(win, 1, 2, "Active: %s (id=%ld)", active->name.c_str(), (long)active->id);
        attroff(COLOR_PAIR(2));
    }
}

static void draw_tabs(WINDOW* win, int w, Panel current) {
    int x = 2;
    const char* tabs[] = { "[P]rofiles", "[V]ariables", "[H]istory" };
    Panel panels[] = { Panel::PROFILES, Panel::VARIABLES, Panel::HISTORY };
    for (int i = 0; i < 3; ++i) {
        if (panels[i] == current) attron(A_BOLD | A_REVERSE);
        mvwprintw(win, 3, x, " %s ", tabs[i]);
        if (panels[i] == current) attroff(A_BOLD | A_REVERSE);
        x += strlen(tabs[i]) + 3;
    }
}

static void draw_profiles(TuiState& st, WINDOW* win, int w, int h) {
    int y = 5;
    auto active = st.ctx.db.get_active_profile(st.ctx.require_project());

    attron(A_BOLD);
    mvwprintw(win, y++, 2, "Profiles:");
    attroff(A_BOLD);
    y++;
    for (size_t i = 0; i < st.profiles.size(); ++i) {
        auto& p = st.profiles[i];
        if (y >= h - 2) break;

        if ((int)i == st.selected) attron(A_REVERSE);
        if (active && *active == p.id) attron(A_BOLD | COLOR_PAIR(2));

        std::string marker = (active && *active == p.id) ? " *" : "";
        mvwprintw(win, y, 4, "%c %-20s%s", (int)i == st.selected ? '>' : ' ',
                  p.name.c_str(), marker.c_str());

        if (active && *active == p.id) attroff(A_BOLD | COLOR_PAIR(2));
        if ((int)i == st.selected) attroff(A_REVERSE);
        y++;
    }
}

static void draw_variables(TuiState& st, WINDOW* win, int w, int h) {
    int y = 5;
    attron(A_BOLD);
    mvwprintw(win, y++, 2, "Variables:");
    attroff(A_BOLD);
    y++;

    int col_key = 4;
    int col_eq = 26;
    int col_val = 29;

    attron(COLOR_PAIR(3));
    mvwprintw(win, y, col_key, "%-20s = %s", "KEY", "VALUE");
    attroff(COLOR_PAIR(3));
    y++;

    mvwprintw(win, y, col_key, "%s", std::string(w - 8, '-').c_str());
    y++;

    for (size_t i = 0; i < st.variables.size(); ++i) {
        auto& v = st.variables[i];
        if (y >= h - 2) break;

        if ((int)i == st.selected) attron(A_REVERSE);

        std::string val = v.secret ? "****" : v.value;
        int max_val = w - col_val - 4;
        if ((int)val.size() > max_val) val = val.substr(0, max_val - 3) + "...";

        mvwprintw(win, y, col_key, "%c %-20s", (int)i == st.selected ? '>' : ' ', v.key.c_str());
        mvwprintw(win, y, col_eq, "=");
        if (v.secret) attron(COLOR_PAIR(1));
        mvwprintw(win, y, col_val, "%s", val.c_str());
        if (v.secret) attroff(COLOR_PAIR(1));

        if ((int)i == st.selected) attroff(A_REVERSE);
        y++;
    }

    if (st.variables.empty()) {
        mvwprintw(win, y, 4, "(no variables — press 'n' to add)");
    }
}

static void draw_history(TuiState& st, WINDOW* win, int w, int h) {
    int y = 5;
    attron(A_BOLD);
    mvwprintw(win, y++, 2, "History:");
    attroff(A_BOLD);
    y++;

    attron(COLOR_PAIR(3));
    mvwprintw(win, y, 2, "%-20s %-12s %s", "WHEN", "VARIABLE", "CHANGE");
    attroff(COLOR_PAIR(3));
    y++;
    mvwprintw(win, y, 2, "%s", std::string(w - 4, '-').c_str());
    y++;

    for (size_t i = 0; i < st.history.size(); ++i) {
        auto& he = st.history[i];
        if (y >= h - 2) break;

        std::string when = he.changed_at.size() > 16 ? he.changed_at.substr(5, 11) : he.changed_at;
        std::string old_val = he.old_value.empty() ? "(new)" : he.old_value;
        std::string new_val = he.new_value.empty() ? "(del)" : he.new_value;
        int max_change = w - 40;
        std::string change = old_val + " -> " + new_val;
        if ((int)change.size() > max_change) change = change.substr(0, max_change - 3) + "...";

        mvwprintw(win, y, 2, "%-20s %-12s %s", when.c_str(), "?", change.c_str());
        y++;
    }
    if (st.history.empty()) {
        mvwprintw(win, y, 4, "(no history)");
    }
}

static void draw_help(WINDOW* win, int w, int h) {
    int y = 5;
    const char* help[] = {
        "Keyboard shortcuts:",
        "",
        "  General",
        "    Tab          Switch panels",
        "    Up/Down      Navigate items",
        "    ?            Toggle help",
        "    q            Quit",
        "",
        "  Profiles",
        "    Enter        Switch to selected profile",
        "",
        "  Variables",
        "    n            New variable",
        "    e            Edit selected variable",
        "    d            Delete selected variable",
        "",
    };
    for (auto& line : help) {
        if (y >= h - 2) break;
        if (line[0] == ' ' && line[1] == ' ' && line[2] == ' ' && line[3] != ' ')
            attron(A_BOLD);
        mvwprintw(win, y, 4, "%s", line);
        attroff(A_BOLD);
        y++;
    }
}

static void draw_input_bar(WINDOW* win, int w, TuiState& st) {
    int y = LINES - 2;
    werase(win);
    attron(A_BOLD | COLOR_PAIR(1));
    mvwprintw(win, y, 2, "%s", input_mode_prompt(st.input_mode));
    attroff(A_BOLD | COLOR_PAIR(1));
    attroff(A_REVERSE);
    mvwprintw(win, y, 2 + strlen(input_mode_prompt(st.input_mode)), "%s", st.input_buf.c_str());
    curs_set(1);
    move(y, 2 + strlen(input_mode_prompt(st.input_mode)) + st.input_buf.size());
}

int run_tui(Context& ctx) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);

    TuiState st{ctx};
    st.load_data();

    WINDOW* input_win = newwin(1, COLS, LINES - 2, 0);

    while (st.running) {
        clear();
        int h = LINES;
        int w = COLS;

        draw_title(stdscr, w, st.ctx);
        draw_tabs(stdscr, w, st.panel);

        if (st.show_help) {
            draw_help(stdscr, w, h);
        } else {
            switch (st.panel) {
                case Panel::PROFILES: draw_profiles(st, stdscr, w, h); break;
                case Panel::VARIABLES: draw_variables(st, stdscr, w, h); break;
                case Panel::HISTORY: draw_history(st, stdscr, w, h); break;
            }
        }

        attron(COLOR_PAIR(3));
        if (st.input_mode != InputMode::NONE) {
            mvprintw(h - 1, 2, " Enter=confirm  Esc=cancel");
        } else {
            mvprintw(h - 1, 2, " [?] help  [q] quit  [Tab] switch  [n]ew  [e]dit  [d]el");
        }
        attroff(COLOR_PAIR(3));

        if (st.input_mode != InputMode::NONE) {
            draw_input_bar(input_win, w, st);
        } else {
            curs_set(0);
            werase(input_win);
        }

        wrefresh(input_win);
        refresh();

        int ch = getch();

        // Handle input mode
        if (st.input_mode != InputMode::NONE) {
            if (ch == 27) { // Esc
                st.input_mode = InputMode::NONE;
                st.input_buf.clear();
                continue;
            }
            if (ch == '\n') {
                if (st.input_mode == InputMode::CONFIRM_DELETE) {
                    if (!st.input_buf.empty() && (st.input_buf[0] == 'y' || st.input_buf[0] == 'Y')) {
                        int64_t pid = st.active_profile_id();
                        if (pid && st.selected < (int)st.variables.size()) {
                            auto& v = st.variables[st.selected];
                            st.ctx.db.record_history(v.id, v.value, "", get_username());
                            st.ctx.db.delete_variable(pid, v.key);
                        }
                        st.selected = 0;
                        st.load_data();
                    }
                } else if (st.input_mode == InputMode::NEW_KEY) {
                    st.pending_key = st.input_buf;
                    st.input_mode = InputMode::NEW_VALUE;
                    st.input_buf.clear();
                    continue;
                } else if (st.input_mode == InputMode::NEW_VALUE) {
                    int64_t pid = st.active_profile_id();
                    if (pid && !st.pending_key.empty()) {
                        st.ctx.db.set_variable(pid, st.pending_key, st.input_buf);
                    }
                    st.input_mode = InputMode::NONE;
                    st.input_buf.clear();
                    st.load_data();
                } else if (st.input_mode == InputMode::EDIT_KEY) {
                    st.pending_key = st.input_buf.empty() ?
                        st.variables[st.selected].key : st.input_buf;
                    st.input_mode = InputMode::EDIT_VALUE;
                    st.input_buf.clear();
                    continue;
                } else if (st.input_mode == InputMode::EDIT_VALUE) {
                    int64_t pid = st.active_profile_id();
                    if (pid && st.selected < (int)st.variables.size()) {
                        auto& old = st.variables[st.selected];
                        if (st.pending_key != old.key) {
                            st.ctx.db.delete_variable(pid, old.key);
                        }
                        st.ctx.db.set_variable(pid, st.pending_key, st.input_buf,
                                               old.secret, old.description);
                    }
                    st.input_mode = InputMode::NONE;
                    st.input_buf.clear();
                    st.load_data();
                }
                continue;
            }
            if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                if (!st.input_buf.empty()) st.input_buf.pop_back();
                continue;
            }
            if (ch >= 32 && ch < 127) {
                st.input_buf += (char)ch;
            }
            continue;
        }

        // Normal mode
        switch (ch) {
            case 'q': st.running = false; break;
            case '?': st.show_help = !st.show_help; break;
            case '\t':
                st.show_help = false;
                st.panel = (Panel)(((int)st.panel + 1) % 3);
                st.selected = 0;
                st.load_data();
                break;
            case KEY_UP:
                if (st.selected > 0) st.selected--;
                break;
            case KEY_DOWN:
                st.selected++;
                break;
            case '\n':
                if (st.panel == Panel::PROFILES && !st.profiles.empty() &&
                    st.selected < (int)st.profiles.size()) {
                    auto& sel = st.profiles[st.selected];
                    st.ctx.db.set_active_profile(st.ctx.require_project(), sel.id);
                    st.load_data();
                    st.selected = 0;
                }
                break;
            case 'p': st.panel = Panel::PROFILES; st.selected = 0; st.show_help = false; break;
            case 'v': st.panel = Panel::VARIABLES; st.selected = 0; st.show_help = false; st.load_data(); break;
            case 'h': st.panel = Panel::HISTORY; st.selected = 0; st.show_help = false; st.load_data(); break;
            case 'n':
                if (st.panel == Panel::VARIABLES) {
                    st.input_mode = InputMode::NEW_KEY;
                    st.input_buf.clear();
                }
                break;
            case 'e':
                if (st.panel == Panel::VARIABLES && !st.variables.empty() &&
                    st.selected < (int)st.variables.size()) {
                    st.input_mode = InputMode::EDIT_KEY;
                    st.input_buf.clear();
                }
                break;
            case 'd':
                if (st.panel == Panel::VARIABLES && !st.variables.empty() &&
                    st.selected < (int)st.variables.size()) {
                    st.input_mode = InputMode::CONFIRM_DELETE;
                    st.input_buf.clear();
                }
                break;
        }

        int max_sel = 0;
        switch (st.panel) {
            case Panel::PROFILES: max_sel = st.profiles.size(); break;
            case Panel::VARIABLES: max_sel = st.variables.size(); break;
            case Panel::HISTORY: max_sel = st.history.size(); break;
        }
        if (max_sel > 0 && st.selected >= max_sel) st.selected = max_sel - 1;
    }

    delwin(input_win);
    endwin();
    return 0;
}

} // namespace envctl
