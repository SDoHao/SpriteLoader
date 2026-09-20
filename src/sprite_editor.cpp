// sprite_editor.cpp —— 精灵序列帧编辑器：入口 + UI 组装
// 技术栈: GLFW + OpenGL3 + ImGui + lodepng
// 模块拆分：
//   editor_config.h/.cpp  记录路径(源/输出目录持久化) + 目录选择
//   editor_camera.h/.cpp  摄像头(世界坐标→屏幕 相机/缩放)
//   editor_loader.h/.cpp  加载/保存(读PNG+pos.txt，写PNG/spk)
//   editor_palette.h/.cpp 色盘/换色(色卡+容差+预览)
//   editor_history.h/.cpp 操作历史(撤销/重做)
//   editor_canvas.h/.cpp  画布(精灵+坐标轴/原点+拖动改坐标)
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cstdio>

#include "editor_common.h"
#include "editor_config.h"
#include "editor_camera.h"
#include "editor_loader.h"
#include "editor_palette.h"
#include "editor_history.h"
#include "editor_canvas.h"

int gCur = 0;          // 当前帧(0基)
bool gOnion = true;    // 洋葱皮开关

static void buildUI() {
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 disp = io.DisplaySize;

    // ===== 全屏背景画布窗口（先创建，垫在所有小窗底下，PNG 直接铺满大窗） =====
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(disp);
    ImGui::Begin("##canvas_bg", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoCollapse);
    drawCanvas();
    ImGui::End();

    // ===== 左边小窗：文件加载/输出 + 操作历史 =====
    ImGui::SetNextWindowPos(ImVec2(8, 8), ImGuiCond_FirstUseEver);
    ImGui::Begin("文件加载 / 输出", nullptr, ImGuiWindowFlags_NoCollapse);
    {
        if (ImGui::Button("选择并加载目录")) {
            std::string d;
            if (pickFolder(d)) { if (loadDir(d)) { gDir = d; saveConfig(); } }
        }
        ImGui::TextWrapped("源目录: %s", gDir.c_str());
        ImGui::Separator();
        if (ImGui::Button("选择输出目录")) { if (pickFolder(gOutDir)) saveConfig(); }
        ImGui::TextWrapped("输出: %s", gOutDir.c_str());
        ImGui::BeginDisabled(gFrames.empty() || gOutDir.empty());
        if (ImGui::Button("保存 PNG + pos.txt")) saveAllToDir(gOutDir);
        if (ImGui::Button("重新打包 sprites.spk")) writeSpkToDir(gOutDir);
        ImGui::EndDisabled();

        if (gFrames.empty()) {
            ImGui::Separator();
            ImGui::TextWrapped("未加载目录。先选一个有 0.png~N.png 和 pos.txt 的目录。");
        } else {
            ImGui::Separator();
            if (ImGui::CollapsingHeader("操作历史 / 撤销", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::Button("撤销")) doUndo();
                ImGui::SameLine();
                if (ImGui::Button("重做")) doRedo();
                ImGui::SameLine();
                ImGui::Text("%d / %d", (int)gHistPos, (int)gHistory.size());
                ImGui::BeginChild("hist", ImVec2(0, 150), true);
                for (size_t i = 0; i < gHistory.size(); ++i) {
                    std::string s = std::to_string(i + 1) + "  " + gHistory[i]->desc;
                    if (i >= gHistPos) s += "  [redo]";
                    else if (i == gHistPos - 1) s += "  <--";
                    ImGui::TextUnformatted(s.c_str());
                }
                ImGui::EndChild();
                ImGui::TextWrapped("快捷键: Ctrl+Z 撤销 / Ctrl+Shift+Z 或 Ctrl+Y 重做");
            }
        }
    }
    ImGui::End();

    if (gFrames.empty()) return;   // 没数据不再开右边/底部小窗

    // ===== 右边小窗：坐标 + 色盘 =====
    ImGui::SetNextWindowPos(ImVec2(disp.x - 8 - 380, 8), ImGuiCond_FirstUseEver);
    ImGui::Begin("坐标 / 色盘", nullptr, ImGuiWindowFlags_NoCollapse);
    {
        if (ImGui::CollapsingHeader("坐标", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& fr = gFrames[gCur];
            int oldx = fr.off_x, oldy = fr.off_y;
            int nx = oldx, ny = oldy;
            ImGui::InputInt("off_x", &nx);
            ImGui::InputInt("off_y", &ny);
            if (nx != oldx || ny != oldy) {
                commitPos(gCur, oldx, oldy, nx, ny);
                fr.off_x = nx; fr.off_y = ny;
            }
            if (ImGui::Button("左移1")) { commitPos(gCur, fr.off_x, fr.off_y, fr.off_x - 1, fr.off_y); fr.off_x--; }
            ImGui::SameLine();
            if (ImGui::Button("右移1")) { commitPos(gCur, fr.off_x, fr.off_y, fr.off_x + 1, fr.off_y); fr.off_x++; }
            ImGui::SameLine();
            if (ImGui::Button("上移1")) { commitPos(gCur, fr.off_x, fr.off_y, fr.off_x, fr.off_y - 1); fr.off_y--; }
            ImGui::SameLine();
            if (ImGui::Button("下移1")) { commitPos(gCur, fr.off_x, fr.off_y, fr.off_x, fr.off_y + 1); fr.off_y++; }
            ImGui::Separator();
            ImGui::InputInt("参考点 ref_x", &gRefX);
            ImGui::InputInt("参考点 ref_y", &gRefY);
            ImGui::Text("锚点(角色脚下): (%d, %d)", gRefX - fr.off_x, gRefY - fr.off_y);
            ImGui::Text("左上角(相对原点): (%d, %d)", fr.off_x - gRefX, fr.off_y - gRefY);
            ImGui::TextWrapped("提示：画布上直接拖动当前帧即可改坐标，改的是裸 pos.txt 偏移，参考点不变。原点在角色脚下。");
        }

        if (ImGui::CollapsingHeader("色盘", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextWrapped("色卡(全帧去重 %d 色，按RGB排序)，点击色块选源色:", (int)gPal.size());
            ImGui::BeginChild("pal", ImVec2(0, 130), true);
            for (int i = 0; i < (int)gPal.size(); ++i) {
                auto& c = gPal[i];
                ImVec4 col(c[0] / 255.f, c[1] / 255.f, c[2] / 255.f, 1.f);
                ImGui::PushID(i);
                if (ImGui::ColorButton("##sw", col, ImGuiColorEditFlags_NoTooltip, ImVec2(22, 22))) {
                    gSrcIdx = i; gSrc = c;
                }
                ImGui::PopID();
                if ((i % 10) != 9) ImGui::SameLine();
            }
            ImGui::EndChild();

            if (gSrcIdx >= 0)
                ImGui::TextWrapped("源色: RGB(%d,%d,%d)", gSrc[0], gSrc[1], gSrc[2]);
            else
                ImGui::TextWrapped("源色: 未选择");

            static float targetF[3] = {0.f, 1.f, 0.f};
            if (ImGui::ColorEdit3("目标色", targetF))
                gTargetU8 = {(uint8_t)(targetF[0] * 255.f), (uint8_t)(targetF[1] * 255.f), (uint8_t)(targetF[2] * 255.f)};

            ImGui::SliderFloat("容差(0~441, RGB距离)", &gTol, 0.f, 441.f, "%.1f");
            ImGui::Checkbox("预览(当前帧)", &gPreview);
            if (gPreview) ImGui::Text("命中: %d 像素", gPreviewCount);
            if (ImGui::Button("应用到全部帧")) {
                if (gSrcIdx < 0) {
                    ImGui::OpenPopup("need_src");
                } else if (applyAll() == 0) {
                    ImGui::OpenPopup("no_match");
                }
            }
            if (ImGui::BeginPopup("need_src")) { ImGui::Text("请先选一个源色"); ImGui::EndPopup(); }
            if (ImGui::BeginPopup("no_match")) { ImGui::Text("容差内没有匹配到像素"); ImGui::EndPopup(); }
        }
    }
    ImGui::End();

    // ===== 底部小窗：帧控制（两行）+ 缩放 =====
    ImGui::SetNextWindowPos(ImVec2((disp.x - 720.f) * 0.5f, disp.y - 8 - 70), ImGuiCond_FirstUseEver);
    ImGui::Begin("帧控制", nullptr, ImGuiWindowFlags_NoCollapse);
    {
        // 第一行：帧滑块 / 上一帧 / 下一帧 / 输入(紧凑)
        ImGui::SetNextItemWidth(150.f);
        if (ImGui::SliderInt("##frameslider", &gCur, 0, (int)gFrames.size() - 1, "帧 %d")) gPreviewTexValid = false;
        ImGui::SameLine();
        if (ImGui::Button("< 上一帧")) { gCur = (gCur - 1 + (int)gFrames.size()) % (int)gFrames.size(); gPreviewTexValid = false; }
        ImGui::SameLine();
        if (ImGui::Button("下一帧 >")) { gCur = (gCur + 1) % (int)gFrames.size(); gPreviewTexValid = false; }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(52.f);
        int cur1 = gCur + 1;
        if (ImGui::InputInt("##curinput", &cur1, 0, 0)) {
            if (cur1 < 1) cur1 = 1;
            if (cur1 > (int)gFrames.size()) cur1 = (int)gFrames.size();
            gCur = cur1 - 1;
            gPreviewTexValid = false;
        }

        // 第二行：缩放滑块 / 洋葱皮
        ImGui::SetNextItemWidth(220.f);
        ImGui::SliderFloat("##zoom", &gZoom, 10.f, 400.f, "缩放 %.0f%%");
        ImGui::SameLine(0, 24);
        ImGui::Checkbox("洋葱皮(上一帧 30%)", &gOnion);
    }
    ImGui::End();
}

int main() {
    if (!glfwInit()) { fprintf(stderr, "glfwInit failed\n"); return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* win = glfwCreateWindow(1360, 820, "精灵编辑器", nullptr, nullptr);
    if (!win) { fprintf(stderr, "create window failed\n"); glfwTerminate(); return 1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 中文字体
    ImFontConfig fcfg;
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc", 16.0f, &fcfg,
                                                io.Fonts->GetGlyphRangesChineseFull());
    if (!font) io.Fonts->AddFontDefault();

    glGenTextures(1, &gPreviewTex);
    setupTexParams(gPreviewTex);

    // 记住上次目录：启动时自动加载
    loadConfig();
    if (!gDir.empty() && !loadDir(gDir)) { gDir.clear(); gOutDir.clear(); }

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        updatePreview();

        // 撤销 / 重做快捷键
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) { if (io.KeyShift) doRedo(); else doUndo(); }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) doRedo();

        buildUI();

        ImGui::Render();
        int fbw, fbh;
        glfwGetFramebufferSize(win, &fbw, &fbh);
        glViewport(0, 0, fbw, fbh);
        glClearColor(0.10f, 0.10f, 0.12f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
