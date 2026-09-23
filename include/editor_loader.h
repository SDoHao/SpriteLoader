// editor_loader.h —— 加载/保存：读目录 → 解码 PNG+pos.txt；保存 PNG / 重打包 spk
#pragma once
#include <string>

// 输出 spk 文件名（用户输入，未带 .spk 时打包会补后缀）
extern std::string gSpkName;
// 手动指定的 pos.txt 路径；为空则用目录内的 pos.txt
extern std::string gPosPath;

// 加载目录下的 0.png~N.png + pos.txt。成功返回 true，失败返回 false。
bool loadDir(const std::string& dir);
// 把内存里所有帧写为 i.png + pos.txt 到 dir（pos.txt 用 \t 分隔）
void saveAllToDir(const std::string& dir);
// 从内存直接打包 gSpkName 指定的 spk（绕开 spk_packer 内部写死的通道替换）
bool writeSpkToDir(const std::string& dir);
