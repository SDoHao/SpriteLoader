// editor_common.h —— 跨模块共享：数据结构 + 工具函数 + 核心全局
#pragma once
#include "imgui.h"
#include <cstdint>
#include <cmath>
#include <vector>
#include <array>
#include <string>
#include <memory>

#ifdef _WIN32
#include <GL/gl.h>
#include <GL/glext.h>
#endif

// ---------------- 数据结构 ----------------
struct Frame {
    int w = 0, h = 0;
    std::vector<uint8_t> rgba;   // 权威像素 RGBA8888
    int off_x = 0, off_y = 0;    // 裸 pos.txt 偏移
    GLuint tex = 0;
};

struct ReplPixel {
    int frame;
    size_t idx;
    std::array<uint8_t, 4> old;
    std::array<uint8_t, 4> neu;
};

struct Op {
    enum Type { POS, COLOR };
    Type type = POS;
    int frame = 0;
    int oldX = 0, oldY = 0, newX = 0, newY = 0;
    std::vector<ReplPixel> diffs;
    std::string desc;
};

// ---------------- 共享工具（inline，避免跨 cpp ODR 问题） ----------------
inline void uploadTex(GLuint tex, const uint8_t* px, int w, int h) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
}

inline void setupTexParams(GLuint tex) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

inline int rgbDist(const uint8_t* a, const uint8_t* b) {
    int dr = (int)a[0] - (int)b[0];
    int dg = (int)a[1] - (int)b[1];
    int db = (int)a[2] - (int)b[2];
    return (int)std::sqrt((float)(dr * dr + dg * dg + db * db));
}

// ---------------- 跨模块共享全局（定义见各自模块 .cpp） ----------------
extern std::vector<Frame> gFrames;   // 定义在 editor_loader.cpp
extern int gCur;                     // 定义在 sprite_editor.cpp
extern bool gOnion;                  // 定义在 sprite_editor.cpp
