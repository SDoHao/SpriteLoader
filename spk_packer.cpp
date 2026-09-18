#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>
#include "lodepng.h"

#include "file_manager.h"
// 1字节对齐，和读取端完全一致
#pragma pack(push, 1)
struct SpriteEntry
{
    uint32_t width;
    uint32_t height;
    uint32_t off_x;
    uint32_t off_y;
    uint64_t offset;
    uint32_t data_size;
    uint32_t format;
};
#pragma pack(pop)

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

int main(int argc, char** argv)
{
    std::string path;
    if (argc < 2){
        path = ".";
    }
    else{
        path = argv[1];
    }
    std::vector<std::string> png_files;
    file_manager fm;
    printf("Current path: %s\n", fm.getCurrentDirectory().c_str());
    printf("Input path: %s\n", path.c_str());
    std::vector<file_manager::file_entry> Entrys = fm.listDirectory(path);

    for (const auto& entry : Entrys) {
        // printf("scan entry name: %s\n", entry.name.c_str());

        if (entry.isDirectory) {
            // printf("  -> DIR path: %s\n", entry.name.c_str());
            continue;
        }
        if (endsWithPng(entry.name)) {
            png_files.push_back(entry.name);
            //printf("  -> PNG file: %s\n", entry.name.c_str());
        } else {
            // printf("  -> Other file (skip): %s\n", entry.name.c_str());
        }
    }
    
    int img_count = png_files.size();
    SpriteEntry* entries = new SpriteEntry[img_count];
    uint8_t** pixel_bufs = new uint8_t*[img_count];
    size_t* pixel_sizes = new size_t[img_count];

    for (int i = 0; i < img_count; ++i)
    {
        size_t png_len;
        std::string full_path = combinePath(path,png_files[i]);
        const char * file_neme = full_path.c_str();
        uint8_t* png_raw = read_file(file_neme, &png_len);
        if (!png_raw)
        {
            printf("Error open file: %s\n", file_neme);
            return 1;
        }

        uint32_t w, h;
        std::vector<uint8_t> rgba;
        unsigned err = lodepng::decode(rgba, w, h, png_raw, png_len);
        free(png_raw);

        if (err != 0)
        {
            printf("Decode error %s: %s\n", file_neme, lodepng_error_text(err));
            return 1;
        }

        // 垂直翻转像素行，适配Love2D坐标系
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

        entries[i].width = w;
        entries[i].height = h;
        entries[i].off_x = 0;
        entries[i].off_y = 0;
        entries[i].format = 1;
        entries[i].data_size = (uint32_t)((size_t)w * h * 4);
        entries[i].offset = 0;
        pixel_bufs[i] = new uint8_t[rgba.size()];
        memcpy(pixel_bufs[i], rgba.data(), rgba.size());
        pixel_sizes[i] = entries[i].data_size;

        printf("Processed %s | %ux%u\n", file_neme, w, h);
    }

    // 计算偏移
    uint64_t header_size = 4 + (uint64_t)img_count * sizeof(SpriteEntry);
    uint64_t cur_off = header_size;
    for (int i = 0; i < img_count; ++i)
    {
        entries[i].offset = cur_off;
        cur_off += pixel_sizes[i];
    }

    // 写出spk
    FILE* out = fopen("sprites.spk", "wb");
    uint32_t total = img_count;
    fwrite(&total, 4, 1, out);
    fwrite(entries, sizeof(SpriteEntry), img_count, out);
    for (int i = 0; i < img_count; ++i)
    {
        fwrite(pixel_bufs[i], 1, pixel_sizes[i], out);
        delete[] pixel_bufs[i];
    }
    fclose(out);

    printf("Pack finished, output sprites.spk\n");

    delete[] pixel_bufs;
    delete[] pixel_sizes;
    delete[] entries;
    return 0;
}