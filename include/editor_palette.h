// editor_palette.h —— 色盘/换色：全帧色卡提取 + 容差换色 + 当前帧预览
#pragma once
#include <cstdint>
#include <vector>
#include <array>

extern std::vector<std::array<uint8_t, 3>> gPal;  // 全帧去重色卡(按RGB升序)
extern int gSrcIdx;                               // 选中色卡下标，-1 未选
extern std::array<uint8_t, 3> gSrc;               // 源色
extern std::array<uint8_t, 3> gTargetU8;          // 目标色
extern float gTol;                                // 容差(RGB欧氏距离)
extern bool gPreview;                             // 是否实时预览当前帧
extern int gPreviewCount;                         // 预览命中像素数
extern unsigned int gPreviewTex;                  // 预览用纹理
extern bool gPreviewTexValid;

void paletteReset();   // 清空色盘状态
void buildPalette();   // 重建色卡
int applyAll();        // 应用到全部帧，返回命中像素数（0=无命中）
void updatePreview();  // 重算当前帧预览
