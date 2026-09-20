// editor_history.h —— 操作历史：撤销/重做/坐标提交
#pragma once
#include <vector>
#include <memory>
#include "editor_common.h"   // Op

extern std::vector<std::unique_ptr<Op>> gHistory;
extern size_t gHistPos;

void historyReset();                                     // 清空历史
void pushOp(std::unique_ptr<Op> op);                     // 入栈（截断 redo）
void doUndo();
void doRedo();
void commitPos(int fi, int ox, int oy, int nx, int ny);  // 记录一次坐标改动
