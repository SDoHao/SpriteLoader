// editor_history.cpp —— 操作历史：撤销/重做/坐标提交
#include "editor_history.h"
#include "editor_common.h"   // gFrames, uploadTex
#include "editor_palette.h"  // gPreviewTexValid（换色撤销/重做后要失效预览）
#include <cstdio>
#include <string>

std::vector<std::unique_ptr<Op>> gHistory;
size_t gHistPos = 0;

void historyReset() {
    gHistory.clear();
    gHistPos = 0;
}

void pushOp(std::unique_ptr<Op> op) {
    gHistory.resize(gHistPos);
    gHistory.push_back(std::move(op));
    ++gHistPos;
}

void doUndo() {
    if (gHistPos == 0) return;
    Op* op = gHistory[gHistPos - 1].get();
    if (op->type == Op::POS) {
        gFrames[op->frame].off_x = op->oldX;
        gFrames[op->frame].off_y = op->oldY;
    } else {
        for (auto& rp : op->diffs) {
            uint8_t* px = &gFrames[rp.frame].rgba[rp.idx * 4];
            px[0] = rp.old[0]; px[1] = rp.old[1]; px[2] = rp.old[2]; px[3] = rp.old[3];
        }
        for (auto& fr : gFrames) uploadTex(fr.tex, fr.rgba.data(), fr.w, fr.h);
        gPreviewTexValid = false;
    }
    --gHistPos;
}

void doRedo() {
    if (gHistPos >= gHistory.size()) return;
    Op* op = gHistory[gHistPos].get();
    if (op->type == Op::POS) {
        gFrames[op->frame].off_x = op->newX;
        gFrames[op->frame].off_y = op->newY;
    } else {
        for (auto& rp : op->diffs) {
            uint8_t* px = &gFrames[rp.frame].rgba[rp.idx * 4];
            px[0] = rp.neu[0]; px[1] = rp.neu[1]; px[2] = rp.neu[2]; px[3] = rp.neu[3];
        }
        for (auto& fr : gFrames) uploadTex(fr.tex, fr.rgba.data(), fr.w, fr.h);
        gPreviewTexValid = false;
    }
    ++gHistPos;
}

void commitPos(int fi, int ox, int oy, int nx, int ny) {
    if (ox == nx && oy == ny) return;
    auto op = std::make_unique<Op>();
    op->type = Op::POS;
    op->frame = fi;
    op->oldX = ox; op->oldY = oy; op->newX = nx; op->newY = ny;
    op->desc = "帧" + std::to_string(fi) + " 坐标→(" + std::to_string(nx) + "," + std::to_string(ny) + ")";
    pushOp(std::move(op));
}
