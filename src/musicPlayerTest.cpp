#include <ncurses.h>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <string>
#include <locale.h>
#include <dirent.h>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctime>
#include <sstream>
#include <iomanip>

struct Entry {
    std::string name;
    bool is_dir;
    bool is_mp3;
};

std::vector<Entry> get_directory_contents(const std::string &folder) {
    std::vector<Entry> entries;
    DIR *dir = opendir(folder.c_str());
    if (!dir) return entries;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == ".") continue;
        Entry e;
        e.name = name;
        e.is_dir = (entry->d_type == DT_DIR);
        if (entry->d_type == DT_UNKNOWN) {
            struct stat st;
            std::string path = folder + "/" + name;
            if (stat(path.c_str(), &st) == 0) e.is_dir = S_ISDIR(st.st_mode);
        }
        e.is_mp3 = name.size() > 4 && name.substr(name.size() - 4) == ".mp3";
        entries.push_back(e);
    }
    std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b){ return a.name < b.name; });
    return entries;
}

bool is_directory(const std::string &path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) return false;
    return (info.st_mode & S_IFDIR);
}

bool is_mp3(const std::string &filename) {
    return filename.size() > 4 && filename.substr(filename.size() - 4) == ".mp3";
}

std::string format_time(double seconds) {
    int mins = seconds / 60;
    int secs = (int)seconds % 60;
    std::ostringstream ss;
    ss << mins << ":" << std::setw(2) << std::setfill('0') << secs;
    return ss.str();
}

void draw_progress_bar(int row, int col, double position, double duration) {
    std::string elapsed_str = format_time(position);
    std::string total_str = format_time(duration);
    int bar_width = col - 4 - elapsed_str.size() - total_str.size() - 2;
    if (bar_width < 10) bar_width = 10;
    double fraction = (duration > 0) ? position / duration : 0;
    if (fraction > 1.0) fraction = 1.0;
    int pos = fraction * bar_width;
    move(row, 2);
    printw("%s ", elapsed_str.c_str());
    std::wstring bar;
    bar.append(pos, L'█');
    bar.append(bar_width - pos, L'░');
    addwstr(bar.c_str());
    printw(" %s", total_str.c_str());
}

double get_mp3_duration(const std::string &path) {
    std::string cmd = "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"" + path + "\" 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return 0;
    char buffer[128];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) result += buffer;
    pclose(pipe);
    try { return std::stod(result); }
    catch (...) { return 0; }
}

void draw_box(int rows, int cols) {
    mvaddwstr(0, 0, L"┌");
    mvaddwstr(0, cols - 1, L"┐");
    mvaddwstr(rows - 1, 0, L"└");
    mvaddwstr(rows - 1, cols - 1, L"┘");
    for (int i = 1; i < cols - 1; ++i) {
        mvaddwstr(0, i, L"─");
        mvaddwstr(rows - 1, i, L"─");
    }
    for (int i = 1; i < rows - 1; ++i) {
        mvaddwstr(i, 0, L"│");
        mvaddwstr(i, cols - 1, L"│");
    }
}

int main() {
    setlocale(LC_ALL, "");
    initscr();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    timeout(100);

    const char* home_env = std::getenv("HOME");
    std::string current_folder = home_env ? std::string(home_env) + "/Music" : "/tmp";
    std::vector<Entry> entries = get_directory_contents(current_folder);

    int rows, cols;
    pid_t mpv_pid = 0;
    int selected_entry = 0;
    int current_song_index = -1;
    bool paused = false;
    double song_duration = 0;
    time_t song_start_time = 0;
    double paused_time = 0;
    bool redraw_static = true;

    while (true) {
        getmaxyx(stdscr, rows, cols);
        if (redraw_static) {
            clear();
            draw_box(rows, cols);
            mvprintw(1, 2, "'q'=quit, up/down=move, 'a'=action, 'p'=pause/resume");
            mvprintw(3, 2, "Folder: %s", current_folder.c_str());
            redraw_static = false;
        }

        int start_row = 5;
        for (size_t i = 0; i < entries.size(); i++) {
            std::string display_name = entries[i].name;
            if (entries[i].is_dir) display_name += "/";

            mvprintw(start_row + i, 4, "%s", display_name.c_str());
            mvchgat(start_row + i, 4, display_name.size(),
                    (i == selected_entry ? A_REVERSE : A_NORMAL),
                    0, NULL);
        }

        if (mpv_pid != 0) {
            int status;
            pid_t result = waitpid(mpv_pid, &status, WNOHANG);
            if (result > 0) {
                mpv_pid = 0;
                paused = false;
                paused_time = 0;
                int next = current_song_index + 1;
                while (next < (int)entries.size() && !entries[next].is_mp3) next++;
                if (next < (int)entries.size()) {
                    current_song_index = next;
                    selected_entry = next;
                    std::string full_path = current_folder + "/" + entries[next].name;
                    mpv_pid = fork();
                    if (mpv_pid == 0) {
                        execlp("mpv", "mpv", "--no-terminal", full_path.c_str(), nullptr);
                        _exit(1);
                    }
                    song_duration = get_mp3_duration(full_path);
                    song_start_time = time(nullptr);
                }
            } else {
                double elapsed = paused ? paused_time : difftime(time(nullptr), song_start_time);
                draw_progress_bar(rows - 2, cols, elapsed, song_duration);
            }
        }

        refresh();
        int ch = getch();
        if (ch != ERR) {
            if (ch == 'q') break;
            if (ch == KEY_UP) { selected_entry--; if (selected_entry < 0) selected_entry = 0; }
            if (ch == KEY_DOWN) { selected_entry++; if (selected_entry >= (int)entries.size()) selected_entry = entries.size() - 1; }

            if (ch == 'p' && mpv_pid != 0) {
                if (paused) {
                    kill(mpv_pid, SIGCONT);
                    song_start_time = time(nullptr) - paused_time;
                } else {
                    kill(mpv_pid, SIGSTOP);
                    paused_time = difftime(time(nullptr), song_start_time);
                }
                paused = !paused;
            }

            if (ch == 'a') {
                if (entries.empty()) continue;
                Entry chosen = entries[selected_entry];
                std::string full_path = current_folder + "/" + chosen.name;
                if (chosen.is_dir) {
                    current_folder = full_path;
                    entries = get_directory_contents(current_folder);
                    selected_entry = 0;
                    redraw_static = true;
                } else if (chosen.is_mp3) {
                    if (mpv_pid != 0) {
                        if (current_song_index == selected_entry) {
                            kill(mpv_pid, SIGKILL);
                            mpv_pid = 0;
                            current_song_index = -1;
                            paused = false;
                            paused_time = 0;
                            continue;
                        } else {
                            kill(mpv_pid, SIGKILL);
                            mpv_pid = 0;
                        }
                    }
                    mpv_pid = fork();
                    if (mpv_pid == 0) {
                        execlp("mpv", "mpv", "--no-terminal", full_path.c_str(), nullptr);
                        _exit(1);
                    }
                    current_song_index = selected_entry;
                    song_duration = get_mp3_duration(full_path);
                    song_start_time = time(nullptr);
                    paused = false;
                    paused_time = 0;
                }
            }
        }
    }

    if (mpv_pid != 0) kill(mpv_pid, SIGKILL);
    endwin();
    return 0;
}

// g++ musicPlayerTest/musicPlayerTest.cpp -o musicPlayerTest/musicPlayerTest -lncurses
