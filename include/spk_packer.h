#ifndef SPK_PACKER_H
#define SPK_PACKER_H

#include <cstdint>
#include <string>
#include <algorithm>
#include <vector>
#include <regex>


inline bool comparePngName(const std::string& a, const std::string& b)
{
    static const std::regex reg(R"(^(\d+)\.png$)"); // 只编译一次
    std::smatch mA, mB;
    int numA = -1, numB = -1;
    if (std::regex_match(a, mA, reg)) numA = std::stoi(mA[1]);
    if (std::regex_match(b, mB, reg)) numB = std::stoi(mB[1]);
    return numA < numB;
}

// 1字节对齐，和读取端完全一致
#pragma pack(push, 1)
struct SpriteEntry
{
    uint32_t width;
    uint32_t height;
    int32_t  off_x;
    int32_t  off_y;
    uint64_t offset;
    uint32_t data_size;
    uint32_t format;
};
#pragma pack(pop)

/**
 * @brief 扫描目录下所有PNG，打包生成spk精灵包
 * @param input_dir 输入PNG所在目录
 * @param output_spk_path 输出spk文件路径
 * @return 0成功，非0失败
 */
int pack_sprites_to_spk(const std::string& input_dir, const std::string& output_spk_path);

#endif
