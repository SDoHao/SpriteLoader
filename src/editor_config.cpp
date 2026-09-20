// editor_config.cpp —— 记录路径：源/输出目录持久化 + 目录选择对话框
#include "editor_config.h"
#include "file_manager.h"
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#endif

std::string gDir;
std::string gOutDir;

std::string cfgPath() {
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string p = buf;
    size_t pos = p.find_last_of("\\/");
    if (pos != std::string::npos) p = p.substr(0, pos);
    return combinePath(p, "sprite_editor.cfg");
}

void saveConfig() {
    FILE* f = fopen(cfgPath().c_str(), "w");
    if (!f) return;
    fprintf(f, "sourcedir=%s\n", gDir.c_str());
    fprintf(f, "outdir=%s\n", gOutDir.c_str());
    fclose(f);
}

void loadConfig() {
    FILE* f = fopen(cfgPath().c_str(), "r");
    if (!f) return;
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        char* val = eq + 1;
        size_t n = strlen(val);
        while (n > 0 && (val[n - 1] == '\n' || val[n - 1] == '\r')) val[--n] = 0;
        if (strcmp(line, "sourcedir") == 0) gDir = val;
        else if (strcmp(line, "outdir") == 0) gOutDir = val;
    }
    fclose(f);
}

bool pickFolder(std::string& out) {
    BROWSEINFOA bi{};
    char path[MAX_PATH] = {0};
    bi.lpszTitle = "选择目录";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (!pidl) return false;
    if (SHGetPathFromIDListA(pidl, path)) {
        out = path;
        IMalloc* im = nullptr;
        if (SUCCEEDED(SHGetMalloc(&im))) { im->Free(pidl); im->Release(); }
        return true;
    }
    return false;
}
