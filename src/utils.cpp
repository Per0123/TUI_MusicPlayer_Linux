#include "utils.h"
#include <dirent.h>
#include <algorithm>
#include <limits.h>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>

std::string normalize_path(const std::string& path) {
    char resolved_path[PATH_MAX];
    if (realpath(path.c_str(), resolved_path) != nullptr) return std::string(resolved_path);
    return path;
}

std::vector<Entry> get_dir_contents(const std::string &folder) {
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
        e.is_mp3 = !e.is_dir && name.size() > 4 && name.substr(name.size() - 4) == ".mp3";
        entries.push_back(e);
    }
    closedir(dir);
    std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b){ return a.name < b.name; });
    return entries;
}

std::string format_time(double seconds) {
    int mins = seconds / 60;
    int secs = (int)seconds % 60;
    std::ostringstream ss;
    ss << mins << ":" << std::setw(2) << std::setfill('0') << secs;
    return ss.str();
}

double get_song_duration(const std::string &file) {
    double duration = 0;
    std::string cmd = "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"" + file + "\"";
    FILE* fp = popen(cmd.c_str(), "r");
    if (fp) {
        char buf[64];
        if (fgets(buf, sizeof(buf), fp)) {
            duration = atof(buf);
        }
        pclose(fp);
    }
    return duration;
}
