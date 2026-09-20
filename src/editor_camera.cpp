// editor_camera.cpp —— 摄像头：相机/缩放状态 + 对准逻辑
#include "editor_camera.h"
#include "editor_common.h"

int gRefX = 232;
int gRefY = 328;
float gZoom = 100.f;
float gCamX = 0, gCamY = 0;

void camCenterOnFrame(int frameIdx) {
    if (frameIdx < 0 || frameIdx >= (int)gFrames.size()) return;
    auto& fr = gFrames[frameIdx];
    gCamX = (float)(gRefX - fr.off_x);
    gCamY = (float)(gRefY - fr.off_y);
}
