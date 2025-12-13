#include "ui.h"
#include "utils.h"
#include <ncurses.h>
#include <string>

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
