// editor_palette.cpp —— 色盘/换色：全帧色卡提取 + 容差换色 + 当前帧预览
#include "editor_palette.h"
#include "editor_common.h"   // gFrames, uploadTex, rgbDist
#include "editor_history.h"  // pushOp
#include <algorithm>
#include <unordered_map>

std::vector<std::array<uint8_t, 3>> gPal;
int gSrcIdx = -1;
std::array<uint8_t, 3> gSrc{255, 0, 0};
std::array<uint8_t, 3> gTargetU8{0, 255, 0};
float gTol = 0.f;
bool gPreview = false;
int gPreviewCount = 0;
unsigned int gPreviewTex = 0;
bool gPreviewTexValid = false;

void paletteReset() {
    gPal.clear();
    gSrcIdx = -1;
    gPreview = false;
    gPreviewTexValid = false;
}

void buildPalette() {
    gPal.clear();
    std::unordered_map<uint32_t, int> seen;
    for (auto& fr : gFrames) {
        for (size_t i = 0; i + 3 < fr.rgba.size(); i += 4) {
            uint32_t key = ((uint32_t)fr.rgba[i] << 16) | ((uint32_t)fr.rgba[i + 1] << 8) | fr.rgba[i + 2];
            if (seen.find(key) == seen.end()) {
                seen[key] = (int)gPal.size();
                gPal.push_back({fr.rgba[i], fr.rgba[i + 1], fr.rgba[i + 2]});
            }
        }
    }
    // 色卡按 RGB 升序排序，方便查找
    std::sort(gPal.begin(), gPal.end(), [](const auto& a, const auto& b) {
        if (a[0] != b[0]) return a[0] < b[0];
        if (a[1] != b[1]) return a[1] < b[1];
        return a[2] < b[2];
    });
}

int applyAll() {
    int changed = 0;
    auto op = std::make_unique<Op>();
    op->type = Op::COLOR;
    for (size_t fi = 0; fi < gFrames.size(); ++fi) {
        auto& fr = gFrames[fi];
        for (size_t i = 0; i < fr.rgba.size() / 4; ++i) {
            uint8_t* px = &fr.rgba[i * 4];
            if (rgbDist(px, gSrc.data()) <= gTol) {
                ReplPixel rp;
                rp.frame = (int)fi; rp.idx = i;
                rp.old = {px[0], px[1], px[2], px[3]};
                px[0] = gTargetU8[0]; px[1] = gTargetU8[1]; px[2] = gTargetU8[2]; // alpha 保留
                rp.neu = {px[0], px[1], px[2], px[3]};
                op->diffs.push_back(rp);
                ++changed;
            }
        }
    }
    if (changed == 0) return 0;
    op->desc = "换色 命中 " + std::to_string(changed) + " 像素";
    pushOp(std::move(op));
    for (auto& fr : gFrames) uploadTex(fr.tex, fr.rgba.data(), fr.w, fr.h);
    gPreviewTexValid = false;
    return changed;
}

void updatePreview() {
    if (!gPreview || gFrames.empty()) return;
    auto& fr = gFrames[gCur];
    std::vector<uint8_t> buf = fr.rgba;
    int count = 0;
    for (size_t i = 0; i < buf.size() / 4; ++i) {
        uint8_t* px = &buf[i * 4];
        if (rgbDist(px, gSrc.data()) <= gTol) {
            px[0] = gTargetU8[0]; px[1] = gTargetU8[1]; px[2] = gTargetU8[2];
            ++count;
        }
    }
    uploadTex(gPreviewTex, buf.data(), fr.w, fr.h);
    gPreviewTexValid = true;
    gPreviewCount = count;
}
