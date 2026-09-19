#ifndef SPK_PACKER_H
#define SPK_PACKER_H

#include <cstdint>
#include <string>
#include <algorithm>
#include <vector>
#include <regex>

enum class Channel
{
    R = 0,
    G = 1,
    B = 2,
    A = 3
};

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
int pack_sprites_to_spk(const std::string& input_dir, const std::string& output_spk_path,bool export_modified_png);

/**
 * @brief 将RGBA8888像素数据保存为PNG文件
 * @param out_path 输出png完整路径
 * @param rgba 像素buffer RGBA8888
 * @param w 宽度
 * @param h 高度
 * @return true成功，false失败
 */
bool save_png(const std::string& out_path, const std::vector<uint8_t>& rgba, uint32_t w, uint32_t h);

/**
 * @brief 通道替换：源通道数值 > threshold 的像素，把源通道颜色写入目标通道
 * @param rgba_data RGBA8888像素缓冲区
 * @param pixel_count 像素总数 = w * h
 * @param src_channel 源通道枚举
 * @param dst_channel 目标通道枚举
 * @param threshold 阈值(0~255)：src通道值大于该阈值才执行替换
 */
void channel_replace(uint8_t* rgba_data,size_t pixel_count,Channel src_channel,Channel dst_channel,uint8_t threshold);

#endif
