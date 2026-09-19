#include "spk_packer.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <algorithm>
#include <utility>
#include "lodepng.h"
#include "file_manager.h"

static uint8_t* read_file(const char* path, size_t* out_sz)
{
    FILE* f = fopen(path, "rb");
    if (!f) return nullptr;
    fseek(f, 0, SEEK_END);
    *out_sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* buf = (uint8_t*)malloc(*out_sz);
    fread(buf, 1, *out_sz, f);
    fclose(f);
    return buf;
}

// 内存守卫：自动释放分配的数组，离开作用域自动清理
struct PackResourceGuard
{
    SpriteEntry* entries = nullptr;
    uint8_t** pixel_bufs = nullptr;
    size_t* pixel_sizes = nullptr;
    int count = 0;

    explicit PackResourceGuard(int cnt) : count(cnt)
    {
        if (cnt <= 0) return;
        entries = new SpriteEntry[cnt];
        pixel_bufs = new uint8_t*[cnt];
        pixel_sizes = new size_t[cnt];
        memset(pixel_bufs, 0, sizeof(uint8_t*) * cnt);
    }

    ~PackResourceGuard()
    {
        for (int i = 0; i < count; ++i)
        {
            delete[] pixel_bufs[i];
        }
        delete[] pixel_bufs;
        delete[] pixel_sizes;
        delete[] entries;
    }

    // 禁止拷贝
    PackResourceGuard(const PackResourceGuard&) = delete;
    PackResourceGuard& operator=(const PackResourceGuard&) = delete;
};

// 读取pos.txt，按顺序返回偏移对，行数不足填充(0,0)
static std::vector<std::pair<int32_t, int32_t>> load_pos_list(const std::string& input_dir, int expect_count)
{
    std::vector<std::pair<int32_t, int32_t>> pos_list(expect_count, {0,0});
    std::string pos_path = combinePath(input_dir, "pos.txt");
    FILE* fp = fopen(pos_path.c_str(), "r");
    if (!fp)
    {
        return pos_list;
    }
    int idx = 0;
    int tx, ty;
    while (idx < expect_count && fscanf(fp, "%d %d", &tx, &ty) == 2)
    {
        pos_list[idx].first  = static_cast<int32_t>(tx);
        pos_list[idx].second = static_cast<int32_t>(ty);
        idx++;
    }
    fclose(fp);
    return pos_list;
}

int pack_sprites_to_spk(const std::string& input_dir, const std::string& output_spk_path)
{
    file_manager fm;
    std::vector<std::string> png_files;
    std::vector<file_manager::file_entry> entries = fm.listDirectory(input_dir);

    // 扫描目录收集png
    for (const auto& entry : entries)
    {
        if (entry.isDirectory)
            continue;
        if (endsWithPng(entry.name))
        {
            png_files.push_back(entry.name);
        }
    }

    // 文件名升序排序
    // 字典排序 std::sort(png_files.begin(), png_files.end());
    // 自然数字排序
    std::sort(png_files.begin(), png_files.end(), comparePngName);

    int img_count = static_cast<int>(png_files.size());
    if (img_count <= 0)
    {
        printf("Warning: no png files found in %s\n", input_dir.c_str());
        return 0;
    }

    auto pos_list = load_pos_list(input_dir, img_count);
    PackResourceGuard guard(img_count);
    SpriteEntry* sprite_entries = guard.entries;
    uint8_t** pixel_bufs = guard.pixel_bufs;
    size_t* pixel_sizes = guard.pixel_sizes;

    // 逐个解码PNG
    for (int i = 0; i < img_count; ++i)
    {
        size_t png_len = 0;
        std::string full_path = combinePath(input_dir, png_files[i]);
        const char* file_name = full_path.c_str();
        uint8_t* png_raw = read_file(file_name, &png_len);
        if (!png_raw)
        {
            printf("Error open file: %s\n", file_name);
            return 1;
        }

        uint32_t w, h;
        std::vector<uint8_t> rgba;
        unsigned err = lodepng::decode(rgba, w, h, png_raw, png_len);
        free(png_raw);

        if (err != 0)
        {
            printf("Decode error %s: %s\n", file_name, lodepng_error_text(err));
            return 1;
        }

        // ========== Love2D 垂直翻转，需要就取消注释 ==========
        // size_t row_bytes = (size_t)w * 4;
        // uint8_t* tmp_row = new uint8_t[row_bytes];
        // uint8_t* data_ptr = rgba.data();
        // for (size_t y = 0; y < h / 2; ++y)
        // {
        //     size_t y2 = h - 1 - y;
        //     uint8_t* r1 = data_ptr + y * row_bytes;
        //     uint8_t* r2 = data_ptr + y2 * row_bytes;
        //     memcpy(tmp_row, r1, row_bytes);
        //     memcpy(r1, r2, row_bytes);
        //     memcpy(r2, tmp_row, row_bytes);
        // }
        // delete[] tmp_row;

        sprite_entries[i].width = w;
        sprite_entries[i].height = h;
        sprite_entries[i].off_x = pos_list[i].first;
        sprite_entries[i].off_y = pos_list[i].second;
        sprite_entries[i].format = 1;
        sprite_entries[i].data_size = static_cast<uint32_t>((size_t)w * h * 4);
        sprite_entries[i].offset = 0;

        pixel_bufs[i] = new uint8_t[rgba.size()];
        memcpy(pixel_bufs[i], rgba.data(), rgba.size());
        pixel_sizes[i] = sprite_entries[i].data_size;

        printf("Processed %s | %ux%u | off=(%d,%d)\n", file_name, w, h, sprite_entries[i].off_x, sprite_entries[i].off_y);
    }

    // 计算每个贴图在spk内的偏移
    uint64_t header_size = 4 + (uint64_t)img_count * sizeof(SpriteEntry);
    uint64_t cur_off = header_size;
    for (int i = 0; i < img_count; ++i)
    {
        sprite_entries[i].offset = cur_off;
        cur_off += pixel_sizes[i];
    }

    // 写出spk文件
    FILE* out = fopen(output_spk_path.c_str(), "wb");
    if (!out)
    {
        printf("Failed open output spk: %s\n", output_spk_path.c_str());
        return 1;
    }
    uint32_t total = static_cast<uint32_t>(img_count);
    fwrite(&total, 4, 1, out);
    fwrite(sprite_entries, sizeof(SpriteEntry), img_count, out);
    for (int i = 0; i < img_count; ++i)
    {
        fwrite(pixel_bufs[i], 1, pixel_sizes[i], out);
    }
    fclose(out);
    printf("Pack finished, output %s\n", output_spk_path.c_str());

    return 0;
}
