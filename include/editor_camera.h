// editor_camera.h —— 摄像头：画布世界坐标(游戏像素)→屏幕的相机与缩放
#pragma once
#include "imgui.h"

extern int gRefX;            // 可编辑锚点基准 x（默认 232）
extern int gRefY;            // 可编辑锚点基准 y（默认 328）
extern float gZoom;          // 画布缩放百分比 10~400
extern float gCamX, gCamY;   // 相机：屏幕中心对应的世界坐标(游戏像素)

// 世界坐标(游戏像素) → 屏幕。围绕 pivot(屏幕中心) 缩放，原点/坐标轴/精灵一起缩放
inline ImVec2 worldToScreen(const ImVec2& pivot, float wx, float wy) {
    const float sc = gZoom / 100.f;
    return ImVec2(pivot.x + (wx - gCamX) * sc, pivot.y + (wy - gCamY) * sc);
}

// 相机对准某帧的左上角
void camCenterOnFrame(int frameIdx);
