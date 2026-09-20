// editor_config.h —— 记录路径：源/输出目录的持久化 + 目录选择对话框
#pragma once
#include <string>

extern std::string gDir;      // 源目录
extern std::string gOutDir;   // 输出目录

std::string cfgPath();                    // 配置文件完整路径（exe 同目录 sprite_editor.cfg）
void saveConfig();                        // 把当前目录写进 cfg
void loadConfig();                        // 启动时读 cfg
bool pickFolder(std::string& out);        // Windows 目录选择对话框
