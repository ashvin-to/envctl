#include "tui/tui.h"
#include "core/context.h"
#include <ncurses.h>
#include <cstring>
#include <vector>
#include <string>

namespace envctl {

enum class Panel { PROFILES, VARIABLES, HISTORY };

struct TuiState {
    Context& ctx;
    Panel panel = Panel::PROFILES;
    int selected = 0;
    int scroll = 0;
    bool running = true;
    bool show_help = false;

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
        }
    }
};

static void draw_title(WINDOW* win, int w, Context& ctx) {
    auto proj = ctx.current_project();
    std::string title = "=== envctl";
    if (proj) {
        title += " — " + proj->name + " (id=" + std::to_string(proj->id) + ")";
    } else {
        title += " — Environment Manager";
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
        mvwprintw(win, 2, x, " %s ", tabs[i]);
        if (panels[i] == current) attroff(A_BOLD | A_REVERSE);
        x += strlen(tabs[i]) + 3;
    }
}

static void draw_profiles(TuiState& st, WINDOW* win, int w, int h) {
    int y = 4;
    auto active = st.ctx.db.get_active_profile(st.ctx.require_project());

    mvwprintw(win, y++, 2, "Profiles:");
    y++;
    for (size_t i = 0; i < st.profiles.size(); ++i) {
        auto& p = st.profiles[i];
        if (y >= h - 2) break;

        if ((int)i == st.selected) attron(A_REVERSE);
        if (active && *active == p.id) attron(A_BOLD);

        std::string marker = (active && *active == p.id) ? " *" : "  ";
        mvwprintw(win, y, 4, "%c %s%s", (int)i == st.selected ? '>' : ' ',
                  p.name.c_str(), marker.c_str());

        if (active && *active == p.id) attroff(A_BOLD);
        if ((int)i == st.selected) attroff(A_REVERSE);
        y++;
    }
}

static void draw_variables(TuiState& st, WINDOW* win, int w, int h) {
    int y = 4;
    mvwprintw(win, y++, 2, "Variables:");
    y++;
    for (size_t i = 0; i < st.variables.size(); ++i) {
        auto& v = st.variables[i];
        if (y >= h - 2) break;

        if ((int)i == st.selected) attron(A_REVERSE);

        std::string val = v.secret ? "****" : v.value;
        if (val.size() > (size_t)(w - 30)) val = val.substr(0, w - 33) + "...";

        mvwprintw(win, y, 4, "%c %-20s = %s",
                  (int)i == st.selected ? '>' : ' ',
                  v.key.c_str(), val.c_str());

        if ((int)i == st.selected) attroff(A_REVERSE);
        y++;
    }
}

static void draw_history(TuiState& st, WINDOW* win, int w, int h) {
    int y = 4;
    mvwprintw(win, y++, 2, "History:");
    y++;
    for (size_t i = 0; i < st.history.size(); ++i) {
        auto& he = st.history[i];
        if (y >= h - 2) break;
        mvwprintw(win, y, 4, "%s  var=%ld  -> %s",
                  he.changed_at.c_str(), (long)he.variable_id,
                  he.new_value.empty() ? "(deleted)" : he.new_value.c_str());
        y++;
    }
    if (st.history.empty()) {
        mvwprintw(win, y, 4, "  (no history)");
    }
}

static void draw_help(WINDOW* win, int w, int h) {
    int y = 4;
    const char* help[] = {
        "Keyboard shortcuts:",
        "",
        "  Tab/Shift-Tab  Switch panels",
        "  Up/Down        Navigate items",
        "  Enter          Use profile (in Profiles)",
        "  n              New profile",
        "  d              Delete selected",
        "  e              Edit selected variable",
        "  q              Quit",
        "  ?              Toggle help",
        "",
    };
    for (auto& line : help) {
        if (y >= h - 2) break;
        mvwprintw(win, y, 4, "%s", line);
        y++;
    }
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
        mvprintw(h - 1, 2, " [?] help  [q] quit  [Tab] switch panel");
        attroff(COLOR_PAIR(3));

        refresh();

        int ch = getch();
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
                    ctx.db.set_active_profile(ctx.require_project(), sel.id);
                    st.load_data();
                    st.selected = 0;
                }
                break;
            case 'p': st.panel = Panel::PROFILES; st.selected = 0; st.show_help = false; break;
            case 'v': st.panel = Panel::VARIABLES; st.selected = 0; st.show_help = false; st.load_data(); break;
            case 'h': st.panel = Panel::HISTORY; st.selected = 0; st.show_help = false; st.load_data(); break;
        }

        int max_sel = 0;
        switch (st.panel) {
            case Panel::PROFILES: max_sel = st.profiles.size(); break;
            case Panel::VARIABLES: max_sel = st.variables.size(); break;
            case Panel::HISTORY: max_sel = st.history.size(); break;
        }
        if (max_sel > 0 && st.selected >= max_sel) st.selected = max_sel - 1;
    }

    endwin();
    return 0;
}

} // namespace envctl
