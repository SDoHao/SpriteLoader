#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <vector>
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

static bool endsWithPng(const std::string & str){
    if(str.size() < 4){
        return false;
    }
    std::string suffix = str.substr(str.size() - 4,4);
    return suffix == ".png";

}

static std::string combinePath(const std::string& dir, const std::string& filename)
{
    return (fs::path(dir) / fs::path(filename)).string();
}

class file_manager{
public:
struct file_entry {
    std::string name;
    bool isDirectory;
};

file_manager() = default;
~file_manager() = default;

std::vector<file_entry> listDirectory(const std::string& path = "") {
    std::vector<file_entry> entries;
    fs::path dirPath = path.empty() ? fs::current_path() : fs::path(path);

    try {
        for (const auto& entry : fs::directory_iterator(dirPath)) {
            entries.emplace_back(file_entry{
                entry.path().filename().string(),
                entry.is_directory()
                });
        }
    }
    catch (const fs::filesystem_error& e) {
        throw std::runtime_error("无法访问目录: " + std::string(e.what()));
    }

    return entries;
}

void deleteFile(const std::string& path) {
    try {
        if (!fs::remove(path)) {
            throw std::runtime_error("无法删除文件");
        }
    }
    catch (const fs::filesystem_error& e) {
        throw std::runtime_error("删除文件失败: " + std::string(e.what()));
    }
}

void deleteDirectory(const std::string& path) {
    try {
        if (!fs::remove_all(path)) {
            throw std::runtime_error("无法删除目录");
        }
    }
    catch (const fs::filesystem_error& e) {
        throw std::runtime_error("删除目录失败: " + std::string(e.what()));
    }
}

std::string getCurrentDirectory() {
    return fs::current_path().string();
}

void changeDirectory(const std::string& path) {
    try {
        fs::current_path(path);
    }
    catch (const fs::filesystem_error& e) {
        throw std::runtime_error("无法切换目录: " + std::string(e.what()));
    }
}

void createDirectory(const std::string& path) {
    try {
        if (!fs::create_directories(path)) {
            throw std::runtime_error("无法创建目录");
        }
    }
    catch (const fs::filesystem_error& e) {
        throw std::runtime_error("创建目录失败: " + std::string(e.what()));
    }
}
};

#endif // FILE_MANAGER_H