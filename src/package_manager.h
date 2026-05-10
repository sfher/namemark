// package_manager.h
#pragma once
#include <string>
#include <vector>

struct PackageInfo {
    std::string name;
    std::string path;
    std::string display;
};

class PackageManager {
public:
    std::vector<PackageInfo> scan();
    const std::string& current() const { return current_; }
    void set_current(const std::string& name) { current_ = name; }

private:
    std::string current_;
};