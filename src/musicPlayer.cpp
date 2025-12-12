#include <ncurses.h>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <locale.h>
#include <dirent.h>
#include <vector>
#include <algorithm>
#include <sys/wait.h>
#include <ctime>
#include <sstream>
#include <iomanip>

using namespace std;

struct Entry {
    string name;
    bool is_dir;
    bool is_mp3;
};

string normalize_path(const string& path) {
    char resolved_path[PATH_MAX];
    if (realpath(path.c_str(), resolved_path) != nullptr) return string(resolved_path);
    return path;
}

vector<Entry> get_directory_contents(const string &folder) {
    vector<Entry> entries;
    DIR *dir = opendir(folder.c_str());
    if (!dir) return entries;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        string name = entry->d_name;
        if (name == ".") continue;
        Entry e;
        e.name = name;
        e.is_dir = (entry->d_type == DT_DIR);
        e.is_mp3 = !e.is_dir && name.size() > 4 && name.substr(name.size() - 4) == ".mp3";
        entries.push_back(e);
    }
    sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b){ return a.name < b.name; });
    return entries;
}

string format_time(double seconds) {
    int mins = seconds / 60;
    int secs = (int)seconds % 60;
    ostringstream ss;
    ss << mins << ":" << setw(2) << setfill('0') << secs;
    return ss.str();
}

void draw_progress_bar(int row, int col, double position, double duration) {
    string elapsed_str = format_time(position);
    string total_str = format_time(duration);
    int bar_width = col - 4 - elapsed_str.size() - total_str.size() - 2;
    if (bar_width < 10) bar_width = 10;
    double fraction = (duration > 0) ? position / duration : 0;
    if (fraction > 1.0) fraction = 1.0;
    int pos = fraction * bar_width;

    move(row, 2);
    printw("%s ", elapsed_str.c_str());

    wstring bar;
    bar.append(pos, L'█');
    bar.append(bar_width - pos, L'░');
    addwstr(bar.c_str());

    printw(" %s", total_str.c_str());
}

void draw_box(int rows, int cols) {
    mvaddwstr(0, 0, L"┌"); 
    mvaddwstr(0, cols-1, L"┐");
    mvaddwstr(rows-1, 0, L"└"); 
    mvaddwstr(rows-1, cols-1, L"┘");

    for(int i = 1; i < cols-1; i++){
        mvaddwstr(0, i, L"─"); 
        mvaddwstr(rows-1, i, L"─");
    }

    for(int i = 1; i < rows-1; i++){
        mvaddwstr(i, 0, L"│"); 
        mvaddwstr(i, cols-1, L"│");
    }
}

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
    string current_folder = home_env ? string(home_env) + "/Music" : "/tmp";
    current_folder = normalize_path(current_folder);
    vector<Entry> entries = get_directory_contents(current_folder);

    while(true){
        getmaxyx(stdscr, rows, cols);

        if(redraw_static){
            clear();
            draw_box(rows, cols);
            mvprintw(1,2,"'q'=quit, up/down=move, 'a'=action, 'p'=pause/resume");
            string folder_display = current_folder;
            if ((int)folder_display.size() > cols - 4) folder_display = "..." + folder_display.substr(folder_display.size() - (cols - 7));
            mvprintw(3,2,"Folder: %s", folder_display.c_str());
            redraw_static = false;
        }

        int start_row = 5;
        int max_entries = rows - start_row - 2;
        int end_index = min((int)entries.size(), selected_entry + max_entries);
        int first_index = max(0, end_index - max_entries);

        for(int i = first_index; i < end_index; i++){
            string display_name = entries[i].name;
            if(entries[i].is_dir) display_name += "/";
            if ((int)display_name.size() > cols - 8) display_name = display_name.substr(0, cols - 11) + "...";
            mvprintw(start_row + i - first_index, 4, "%s", display_name.c_str());
            mvchgat(start_row + i - first_index, 4, display_name.size(), (i == selected_entry ? A_REVERSE : A_NORMAL), 1, NULL);
        }

        if(mpv_pid != 0){
            int status;
            pid_t result = waitpid(mpv_pid, &status, WNOHANG);
            if(result > 0) mpv_pid = 0;
            else {
                double elapsed = paused ? paused_time : difftime(time(nullptr), song_start_time);
                draw_progress_bar(rows-2, cols, elapsed, 0); 
            }
        }

        refresh();
        int ch = getch();
        if(ch != ERR){
            if(ch == 'q') break;
            if(ch == KEY_UP){ selected_entry--; if(selected_entry<0) selected_entry=0; }
            if(ch == KEY_DOWN){ selected_entry++; if(selected_entry >= (int)entries.size()) selected_entry=entries.size()-1; }

            if(ch == 'p' && mpv_pid != 0){
                if(paused){ kill(mpv_pid,SIGCONT); song_start_time=time(nullptr)-paused_time; }
                else{ kill(mpv_pid,SIGSTOP); paused_time=difftime(time(nullptr),song_start_time); }
                paused = !paused;
            }

            if(ch == 'a'){
                if(entries.empty()) continue;
                Entry chosen = entries[selected_entry];
                string full_path = current_folder + "/" + chosen.name;
                if(chosen.is_dir){
                    current_folder = normalize_path(full_path);
                    entries = get_directory_contents(current_folder);
                    selected_entry = 0; 
                    redraw_static = true;
                } else if(chosen.is_mp3){
                    if(mpv_pid != 0) kill(mpv_pid,SIGKILL);
                    mpv_pid = fork();
                    if(mpv_pid == 0){ execlp("mpv","mpv","--no-terminal",full_path.c_str(),nullptr); _exit(1); }
                    current_song_index = selected_entry;
                    song_start_time = time(nullptr); 
                    paused = false; 
                    paused_time = 0;
                }
            }
        }
    }

    if(mpv_pid != 0) kill(mpv_pid,SIGKILL);
    endwin();
    return 0;
}

// g++ src/musicPlayer.cpp -o musicPlayer -lncurses
