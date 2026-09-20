// editor_loader.h —— 加载/保存：读目录 → 解码 PNG+pos.txt；保存 PNG / 重打包 spk
#pragma once
#include <string>

// 加载目录下的 0.png~N.png + pos.txt。成功返回 true，失败返回 false。
bool loadDir(const std::string& dir);
// 把内存里所有帧写为 i.png + pos.txt 到 dir（pos.txt 用 \t 分隔）
void saveAllToDir(const std::string& dir);
// 从内存直接打包 sprites.spk（绕开 spk_packer 内部写死的通道替换）
bool writeSpkToDir(const std::string& dir);
