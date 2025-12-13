#include <ncurses.h>
#include <cstdlib>
#include <unistd.h>
#include <ctime>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <locale.h>
#include "utils.h"
#include "ui.h"

int main() {
    setlocale(LC_ALL,"");
    initscr();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    timeout(100);

    int rows, cols;
    pid_t mpv_pid = 0;
    int selected_entry = 0;
    int current_song_index = -1;
    bool paused = false;
    time_t song_start_time = 0;
    double paused_time = 0;
    bool redraw_static = true;

    const char* home_env = getenv("HOME");
    std::string current_folder = home_env ? std::string(home_env) + "/Music" : "/tmp";
    current_folder = normalize_path(current_folder);
    std::vector<Entry> entries = get_dir_contents(current_folder);

    const int margin = 2; 

    while(true) {
        getmaxyx(stdscr, rows, cols);

        int start_row = 5;
        int max_entries = rows - start_row - 3;  
        int end_index = std::min((int)entries.size(), selected_entry + max_entries);
        int first_index = std::max(0, end_index - max_entries);

        if(redraw_static) {
            clear();
            draw_box(rows, cols);

            mvprintw(1, margin, "'q'=quit, up/down=move, 'a'=action, 'p'=pause/resume");

            std::string folder_display = current_folder;
            int max_folder_len = cols - 2*margin;
            if((int)folder_display.size() > max_folder_len)
                folder_display = "..." + folder_display.substr(folder_display.size() - max_folder_len + 3);

            mvprintw(3, margin, "Folder: %s", folder_display.c_str());
            redraw_static = false;
        }

        for(int i = first_index; i < end_index; ++i) {
            int line = start_row + i - first_index;
            if(line >= rows - 1) break; 

            std::string display_name = entries[i].name;
            if(entries[i].is_dir) display_name += "/";

            int max_name_len = cols - 2 * margin - 1; 
            if((int)display_name.size() > max_name_len)
                display_name = display_name.substr(0, max_name_len - 3) + "...";

            for(int col = margin; col < cols - 1; ++col)
                mvaddch(line, col, ' ');

            mvprintw(line, margin, "%s", display_name.c_str());

            int highlight_len = std::min((int)display_name.size(), cols - margin - 1);
            mvchgat(line, margin, highlight_len, (i == selected_entry ? A_REVERSE : A_NORMAL), 1, NULL);
        }

        if(mpv_pid != 0) {
            int status;
            pid_t result = waitpid(mpv_pid, &status, WNOHANG);
            if(result > 0) {
                mpv_pid = 0;
            } else {
                double elapsed = paused ? paused_time : difftime(time(nullptr), song_start_time);
                draw_progress_bar(rows - 2, cols, elapsed, 0);
            }
        }

        refresh();

        int ch = getch();
        if(ch != ERR) {
            if(ch == 'q') break;
            if(ch == KEY_UP) selected_entry = std::max(0, selected_entry - 1);
            if(ch == KEY_DOWN) selected_entry = std::min((int)entries.size() - 1, selected_entry + 1);

            if(ch == 'p' && mpv_pid != 0) {
                if(paused) {
                    kill(mpv_pid, SIGCONT);
                    song_start_time = time(nullptr) - paused_time;
                } else {
                    kill(mpv_pid, SIGSTOP);
                    paused_time = difftime(time(nullptr), song_start_time);
                }
                paused = !paused;
            }

            if(ch == 'a') {
                if(entries.empty()) continue;
                Entry chosen = entries[selected_entry];
                std::string full_path = current_folder + "/" + chosen.name;
                if(chosen.is_dir) {
                    current_folder = normalize_path(full_path);
                    entries = get_dir_contents(current_folder);
                    selected_entry = 0;
                    redraw_static = true;
                } else if(chosen.is_mp3) {
                    if(mpv_pid != 0) kill(mpv_pid, SIGKILL);
                    mpv_pid = fork();
                    if(mpv_pid == 0) {
                        execlp("mpv", "mpv", "--no-terminal", full_path.c_str(), nullptr);
                        _exit(1);
                    }
                    current_song_index = selected_entry;
                    song_start_time = time(nullptr);
                    paused = false;
                    paused_time = 0;
                }
            }
        }
    }

    if(mpv_pid != 0) kill(mpv_pid, SIGKILL);
    endwin();
    return 0;
}

// g++ -std=c++17 -lncurses src/utils.cpp src/ui.cpp src/musicPlayer.cpp -o musicPlayer
