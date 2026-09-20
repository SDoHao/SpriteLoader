// editor_canvas.cpp —— 画布：绘制精灵 + 坐标轴/原点/位置 + 拖动改坐标
#include "editor_canvas.h"
#include "editor_common.h"    // gFrames, gCur, gOnion
#include "editor_camera.h"    // gZoom, worldToScreen
#include "editor_palette.h"   // gPreview, gPreviewTexValid, gPreviewTex
#include "editor_history.h"   // commitPos
#include <cstdio>

void drawCanvas() {
    ImVec2 minc = ImGui::GetCursorScreenPos();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 maxc(minc.x + avail.x, minc.y + avail.y);
    ImVec2 pivot((minc.x + maxc.x) * 0.5f, (minc.y + maxc.y) * 0.5f); // 屏幕中心
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // 棋盘格透明背景
    const int cs = 16;
    for (int y = (int)minc.y; y < (int)maxc.y; y += cs)
        for (int x = (int)minc.x; x < (int)maxc.x; x += cs) {
            int cc = ((x / cs) + (y / cs)) & 1;
            dl->AddRectFilled(ImVec2((float)x, (float)y), ImVec2((float)x + cs, (float)y + cs),
                              cc ? 0xFF2E2E2E : 0xFF3B3B3B);
        }

    if (gFrames.empty()) return;

    auto& fr = gFrames[gCur];
    const float sc = gZoom / 100.f;   // 整张画布的缩放

    // 坐标轴：x=0 竖轴、y=0 横轴，整条铺满画布（随缩放一起）
    float axisX = worldToScreen(pivot, 0, 0).x;   // 竖轴所在屏幕x
    float axisY = worldToScreen(pivot, 0, 0).y;   // 横轴所在屏幕y
    if (axisX >= minc.x && axisX <= maxc.x)
        dl->AddLine(ImVec2(axisX, minc.y), ImVec2(axisX, maxc.y), IM_COL32(120, 140, 255, 180), 1.f);
    if (axisY >= minc.y && axisY <= maxc.y)
        dl->AddLine(ImVec2(minc.x, axisY), ImVec2(maxc.x, axisY), IM_COL32(255, 150, 120, 180), 1.f);

    // 洋葱皮：上一帧 30% 透明（左上角 = off - 参考点，原点即角色脚下/锚点）
    if (gOnion && gCur > 0) {
        auto& pf = gFrames[gCur - 1];
        ImVec2 ptl = worldToScreen(pivot, (float)(pf.off_x - gRefX), (float)(pf.off_y - gRefY));
        dl->AddImage((ImTextureID)(intptr_t)pf.tex, ptl, ImVec2(ptl.x + pf.w * sc, ptl.y + pf.h * sc),
                     ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 77));
    }

    GLuint tex = fr.tex;
    if (gPreview && gPreviewTexValid) tex = gPreviewTex;
    // 锚点 (gRefX-off_x, gRefY-off_y) 落在原点(角色脚下)，故左上角 = (off_x-gRefX, off_y-gRefY)
    ImVec2 topleft = worldToScreen(pivot, (float)(fr.off_x - gRefX), (float)(fr.off_y - gRefY));
    ImVec2 br(topleft.x + fr.w * sc, topleft.y + fr.h * sc);
    dl->AddImage((ImTextureID)(intptr_t)tex, topleft, br);
    dl->AddRect(topleft, br, IM_COL32(0, 255, 0, 255));

    // 原点 (0,0) 标记 + 坐标轴端点标签
    ImVec2 o = worldToScreen(pivot, 0, 0);
    if (o.x >= minc.x && o.x <= maxc.x && o.y >= minc.y && o.y <= maxc.y) {
        dl->AddCircleFilled(o, 4.f, IM_COL32(255, 255, 0, 255));
        dl->AddCircle(o, 7.f, IM_COL32(255, 255, 0, 200), 16, 1.f);
        dl->AddText(ImVec2(o.x + 9, o.y + 4), IM_COL32(255, 255, 0, 255), "0,0");
    }
    if (axisY >= minc.y && axisY <= maxc.y)   // x=0 轴的 "x=0" 标签
        dl->AddText(ImVec2(axisX + 3, maxc.y - 16), IM_COL32(120, 140, 255, 255), "x=0");
    if (axisX >= minc.x && axisX <= maxc.x)   // y=0 轴的 "y=0" 标签
        dl->AddText(ImVec2(maxc.x - 34, axisY + 3), IM_COL32(255, 150, 120, 255), "y=0");

    // 精灵左上角相对原点(角色脚下)的实际位置显示
    char posb[96];
    snprintf(posb, sizeof posb, "左上角(相对原点) x=%d y=%d", fr.off_x - gRefX, fr.off_y - gRefY);
    dl->AddText(ImVec2(topleft.x + 3, topleft.y + 3), IM_COL32(0, 255, 255, 255), posb);

    // 拖动调整坐标（按画布缩放反算，跟手）
    ImGui::InvisibleButton("canvas_io", avail);
    static ImVec2 dragStartMouse;
    static int dragStartX = 0, dragStartY = 0;
    static bool dragging = false;
    if (ImGui::IsItemActive() && ImGui::IsMouseClicked(0) && !dragging) {
        dragStartMouse = ImGui::GetIO().MousePos;
        dragStartX = fr.off_x; dragStartY = fr.off_y;
        dragging = true;
    }
    if (dragging && ImGui::IsMouseDragging(0)) {
        auto& io = ImGui::GetIO();
        const float s = gZoom / 100.f;
        // 左上角 = off - 参考点，故 off 增大 → 精灵向右/向下，用正号让精灵跟着鼠标同向移动
        fr.off_x = dragStartX + (int)((io.MousePos.x - dragStartMouse.x) / s);
        fr.off_y = dragStartY + (int)((io.MousePos.y - dragStartMouse.y) / s);
    }
    if (ImGui::IsMouseReleased(0) && dragging) {
        if (fr.off_x != dragStartX || fr.off_y != dragStartY)
            commitPos(gCur, dragStartX, dragStartY, fr.off_x, fr.off_y);
        dragging = false;
    }

    // 中键按住拖动：平移画布（移动原点/相机 gCamX,gCamY），只在画布窗口上生效
    {
        ImGuiIO& io = ImGui::GetIO();
        static bool panning = false;
        static ImVec2 panStartMouse;
        static float panStartCamX = 0, panStartCamY = 0;
        if (!panning && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
            panning = true;
            panStartMouse = io.MousePos;
            panStartCamX = gCamX; panStartCamY = gCamY;
        }
        if (panning && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            const float s = gZoom / 100.f;
            // 屏幕往右移 = 相机往左移：cam -= delta / 缩放
            gCamX = panStartCamX - (io.MousePos.x - panStartMouse.x) / s;
            gCamY = panStartCamY - (io.MousePos.y - panStartMouse.y) / s;
        }
        if (panning && ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
            panning = false;
        }
    }
}
