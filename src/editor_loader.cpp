// editor_loader.cpp —— 加载/保存：读目录 → 解码 PNG+pos.txt；保存 PNG / 重打包 spk
#include "editor_loader.h"
#include "editor_common.h"    // gFrames, uploadTex, setupTexParams
#include "editor_palette.h"   // buildPalette, paletteReset
#include "editor_history.h"   // historyReset
#include "editor_camera.h"    // camCenterOnFrame
#include "file_manager.h"     // 你写的库：listDirectory / combinePath / createDirectory
#include "lodepng.h"
#include "spk_packer.h"       // save_png / SpriteEntry
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>

// gFrames 的定义（extern 声明在 editor_common.h）
std::vector<Frame> gFrames;
// 输出 spk 文件名（定义在 loader，UI 里用输入框改）
std::string gSpkName = "sprites";
// 手动指定的 pos.txt（空=用目录内 pos.txt）
std::string gPosPath;

// file_manager 没有提供文件内容读写接口，这里用 fopen 读原始字节
static bool readFileBytes(const std::string& p, std::vector<uint8_t>& out) {
    FILE* f = fopen(p.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    out.resize(sz);
    if (sz > 0 && fread(out.data(), 1, sz, f) != (size_t)sz) { fclose(f); return false; }
    fclose(f);
    return true;
}

bool loadDir(const std::string& dir) {
    gFrames.clear();
    historyReset();
    paletteReset();

    file_manager fm;
    std::vector<std::string> pngs;
    for (auto& e : fm.listDirectory(dir)) {
        if (!e.isDirectory && endsWithPng(e.name)) pngs.push_back(e.name);
    }
    if (pngs.empty()) return false;
    std::sort(pngs.begin(), pngs.end(), comparePngName);

    std::vector<std::pair<int, int>> pos;
    {
        // 手动指定了 pos.txt 优先用它的；没有或打不开就回退到目录内 pos.txt
        FILE* f = nullptr;
        if (!gPosPath.empty()) f = fopen(gPosPath.c_str(), "r");
        if (!f) f = fopen(combinePath(dir, "pos.txt").c_str(), "r");
        int a, b;
        while (f && fscanf(f, "%d %d", &a, &b) == 2) pos.push_back({a, b});
        if (f) fclose(f);
    }

    for (size_t i = 0; i < pngs.size(); ++i) {
        Frame fr;
        std::vector<uint8_t> raw;
        if (!readFileBytes(combinePath(dir, pngs[i]), raw)) continue;
        unsigned w = 0, h = 0;
        if (lodepng::decode(fr.rgba, w, h, raw.data(), raw.size()) != 0) continue;
        fr.w = (int)w; fr.h = (int)h;
        fr.off_x = (i < pos.size()) ? pos[i].first  : 0;
        fr.off_y = (i < pos.size()) ? pos[i].second : 0;
        glGenTextures(1, &fr.tex);
        setupTexParams(fr.tex);
        uploadTex(fr.tex, fr.rgba.data(), fr.w, fr.h);
        gFrames.push_back(std::move(fr));
    }
    if (gFrames.empty()) return false;

    buildPalette();
    camCenterOnFrame(0);
    gCur = 0;
    return true;
}

void saveAllToDir(const std::string& dir) {
    file_manager fm;
    if (!dir.empty()) fm.createDirectory(dir);
    for (size_t i = 0; i < gFrames.size(); ++i) {
        auto& fr = gFrames[i];
        save_png(combinePath(dir, std::to_string(i) + ".png"), fr.rgba, (uint32_t)fr.w, (uint32_t)fr.h);
    }
    FILE* f = fopen(combinePath(dir, "pos.txt").c_str(), "w");
    if (f) {
        for (auto& fr : gFrames) fprintf(f, "%d\t%d\n", fr.off_x, fr.off_y);
        fclose(f);
    }
}

bool writeSpkToDir(const std::string& dir) {
    size_t n = gFrames.size();

    // 文件名：空则默认 sprites；用户没带 .spk 后缀就补上，带了就按用户的来
    std::string name = gSpkName;
    if (name.empty()) name = "sprites";
    bool hasExt = name.size() >= 4 && (name.compare(name.size() - 4, 4, ".spk") == 0 ||
                                       name.compare(name.size() - 4, 4, ".SPK") == 0);
    if (!hasExt) name += ".spk";

    uint64_t header = 4 + (uint64_t)n * sizeof(SpriteEntry);
    uint64_t cur = header;
    std::vector<SpriteEntry> ent(n);
    for (size_t i = 0; i < n; ++i) {
        auto& fr = gFrames[i];
        ent[i].width = (uint32_t)fr.w;
        ent[i].height = (uint32_t)fr.h;
        ent[i].off_x = fr.off_x;
        ent[i].off_y = fr.off_y;
        ent[i].format = 1;
        ent[i].data_size = (uint32_t)fr.rgba.size();
        ent[i].offset = cur;
        cur += fr.rgba.size();
    }
    FILE* f = fopen(combinePath(dir, name).c_str(), "wb");
    if (!f) return false;
    uint32_t total = (uint32_t)n;
    fwrite(&total, 4, 1, f);
    fwrite(ent.data(), sizeof(SpriteEntry), n, f);
    for (size_t i = 0; i < n; ++i)
        fwrite(gFrames[i].rgba.data(), 1, gFrames[i].rgba.size(), f);
    fclose(f);
    return true;
}
