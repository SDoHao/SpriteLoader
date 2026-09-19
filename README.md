# SpriteLoader
PNG文件打包工具，将序列帧PNG打包为自定义SPK精灵包，提升加载性能。

## 项目目录结构

```text
SpriteLoader/
├── images/                # 资源目录，存放 0.png ~ N.png、pos.txt
├── include/
│   ├── file_manager.h
│   ├── lodepng.h          # lodepng PNG 解码库
│   └── spk_packer.h
├── src/
│   ├── lodepng.cpp
│   ├── spk_packer.cpp
│   └── sprite_loader.cpp  # Lua C 模块，编译为 sprite_loader.dll
├── main.lua               # LÖVE2D 测试脚本，对比 PNG/SPK 加载
├── conf.lua
├── packer.cpp             # packer 入口
├── .gitignore
└── README.md
```


## 功能说明
1. **spk_packer.exe**：打包工具，读取 `images/` 下按数字命名的序列PNG（`0.png,1.png,2.png...241.png`）和pos.txt，生成 `sprites.spk`。
   > ⚠️ 文件排序使用**数字自然排序**，不再使用字符串字典序，保证帧顺序正确。
2. **sprite_loader.dll**：LÖVE2D Lua C扩展，负责在Lua端读取SPK包，解析帧头与RGBA原始像素。
3. `main.lua`：LÖVE2D测试程序，左右分屏对比原始PNG与SPK包渲染，输出加载耗时、帧偏移校验。

---

# 编译方式
## 方式1：手动 g++ 编译
> 需要 MinGW-w64 环境，提前准备好 lua5.1 头文件与库文件。

### ① 编译打包器 spk_packer.exe
```bash
g++ packer.cpp src/spk_packer.cpp src/lodepng.cpp -o spk_packer.exe -O2 -static-libgcc -std=c++17 -Iinclude
```

### ② 编译 Lua 扩展 sprite_loader.dll

```shell
g++ -shared sprite_loader.cpp -o sprite_loader.dll -I"XXX\lua-5.1.4\src" -L"XXX\love-11.5-win64" -llua51 -static-libgcc -std=c++17 -Wl,--enable-auto-import
```
修改 `-I` / `-L` 路径为你本地 lua5.1 源码与 love 库路径

## 方式 2：CMake 编译

> 推荐使用 MinGW-w64 + CMake，一键构建打包器与 dll，无需手动维护编译命令。
> 要求：CMake >=3.16，MinGW-w64 工具链。
