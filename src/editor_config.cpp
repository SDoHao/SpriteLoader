// editor_config.cpp —— 记录路径：源/输出目录持久化 + 目录选择对话框
#include "editor_config.h"
#include "file_manager.h"
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <objbase.h>
#endif

std::string gDir;
std::string gOutDir;

// UTF-8/ANSI 字符串 → 宽字符（给 Windows 原生对话框用）
static std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
    if (n > 0) w.resize(n - 1);
    return w;
}

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

bool pickFolder(std::string& out, const std::string& initial) {
    // 用现代 IFileDialog，支持默认定位到 initial 目录；失败则回退旧版 SHBrowseForFolder
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool ok = false;
    std::wstring wi = utf8ToWide(initial);

    IFileOpenDialog* dlg = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dlg));
    if (SUCCEEDED(hr) && dlg) {
        DWORD opts = 0;
        dlg->GetOptions(&opts);
        dlg->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        if (!initial.empty()) {
            PIDLIST_ABSOLUTE pidl = nullptr;
            if (SUCCEEDED(SHParseDisplayName(wi.c_str(), nullptr, &pidl, 0, nullptr))) {
                IShellItem* item = nullptr;
                if (SUCCEEDED(SHCreateItemFromIDList(pidl, IID_PPV_ARGS(&item)))) {
                    dlg->SetFolder(item);
                    item->Release();
                }
                CoTaskMemFree(pidl);
            }
        }
        if (SUCCEEDED(dlg->Show(nullptr))) {
            IShellItem* item = nullptr;
            if (SUCCEEDED(dlg->GetResult(&item))) {
                PWSTR path = nullptr;
                if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                    int n = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
                    if (n > 1) {
                        std::string s(n - 1, 0);
                        WideCharToMultiByte(CP_UTF8, 0, path, -1, &s[0], n - 1, nullptr, nullptr);
                        out = s;
                        ok = true;
                    }
                    CoTaskMemFree(path);
                }
                item->Release();
            }
        }
        dlg->Release();
    }
    CoUninitialize();
    if (ok) return true;

    // 回退：旧版 SHBrowseForFolder（也尽量定位到 initial）
    BROWSEINFOA bi{};
    char path[MAX_PATH] = {0};
    bi.lpszTitle = "选择目录";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST root = nullptr;
    if (!initial.empty()) {
        if (SUCCEEDED(SHParseDisplayName(wi.c_str(), nullptr, &root, 0, nullptr)))
            bi.pidlRoot = root;
    }
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl) {
        if (SHGetPathFromIDListA(pidl, path)) { out = path; ok = true; }
        IMalloc* im = nullptr;
        if (SUCCEEDED(SHGetMalloc(&im))) { im->Free(pidl); im->Release(); }
    }
    if (root) {
        IMalloc* im = nullptr;
        if (SUCCEEDED(SHGetMalloc(&im))) { im->Free(root); im->Release(); }
    }
    return ok;
}

bool pickFile(std::string& out, const std::string& initial) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool ok = false;
    std::wstring wi = utf8ToWide(initial);

    IFileOpenDialog* dlg = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dlg));
    if (SUCCEEDED(hr) && dlg) {
        DWORD opts = 0;
        dlg->GetOptions(&opts);
        dlg->SetOptions(opts | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);
        COMDLG_FILTERSPEC filters[] = {
            { L"文本文件 (*.txt)", L"*.txt" },
            { L"所有文件 (*.*)",   L"*.*" }
        };
        dlg->SetFileTypes(2, filters);
        dlg->SetDefaultExtension(L"txt");
        if (!initial.empty()) {
            PIDLIST_ABSOLUTE pidl = nullptr;
            if (SUCCEEDED(SHParseDisplayName(wi.c_str(), nullptr, &pidl, 0, nullptr))) {
                IShellItem* item = nullptr;
                if (SUCCEEDED(SHCreateItemFromIDList(pidl, IID_PPV_ARGS(&item)))) {
                    dlg->SetFolder(item);
                    item->Release();
                }
                CoTaskMemFree(pidl);
            }
        }
        if (SUCCEEDED(dlg->Show(nullptr))) {
            IShellItem* item = nullptr;
            if (SUCCEEDED(dlg->GetResult(&item))) {
                PWSTR path = nullptr;
                if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                    int n = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
                    if (n > 1) {
                        std::string s(n - 1, 0);
                        WideCharToMultiByte(CP_UTF8, 0, path, -1, &s[0], n - 1, nullptr, nullptr);
                        out = s;
                        ok = true;
                    }
                    CoTaskMemFree(path);
                }
                item->Release();
            }
        }
        dlg->Release();
    }
    CoUninitialize();
    return ok;
}
