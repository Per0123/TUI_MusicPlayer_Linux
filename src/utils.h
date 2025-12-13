#pragma once
#include <string>
#include <vector>

struct Entry {
    std::string name;
    bool is_dir;
    bool is_mp3;
};

std::string normalize_path(const std::string& path);
std::vector<Entry> get_dir_contents(const std::string &folder);
std::string format_time(double seconds);
