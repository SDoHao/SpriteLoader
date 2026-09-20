# SpriteLoader
PNG文件打包工具，将序列帧PNG打包为自定义SPK精灵包，提升加载性能。

## 项目目录结构

```text
SpriteLoader/
├── images/                # 资源目录，存放 0.png ~ N.png、pos.txt
├── include/
│   ├── file_manager.h
│   ├── lodepng.h            # lodepng PNG 解码库
│   ├── spk_packer.h
│   ├── editor_common.h      # 精灵编辑器：共享数据结构/工具
│   ├── editor_config.h      # 精灵编辑器：记录路径/目录选择
│   ├── editor_camera.h      # 精灵编辑器：相机/缩放
│   ├── editor_loader.h      # 精灵编辑器：加载/保存
│   ├── editor_palette.h     # 精灵编辑器：色盘/换色
│   ├── editor_history.h     # 精灵编辑器：操作历史
│   └── editor_canvas.h      # 精灵编辑器：画布
├── src/
│   ├── lodepng.cpp
│   ├── spk_packer.cpp
│   ├── sprite_loader.cpp    # Lua C 模块，编译为 sprite_loader.dll
│   ├── sprite_editor.cpp    # 精灵编辑器：入口 + UI 组装
│   ├── editor_config.cpp    # 精灵编辑器：各功能模块实现
│   ├── editor_camera.cpp / editor_loader.cpp
│   └── editor_palette.cpp / editor_history.cpp / editor_canvas.cpp
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
4. **sprite_editor.exe**：可视化序列帧编辑器，见下文「精灵编辑器 sprite_editor.exe」。

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

---

# 精灵编辑器 sprite_editor.exe

可视化序列帧编辑器，用"点点点"完成两个核心功能：**手动调整每帧坐标**（洋葱皮辅助）与**全帧色盘换色**（容差匹配），并支持保存输出、重新打包 `sprites.spk`、操作历史撤销/重做。技术栈：GLFW + OpenGL3 + ImGui + lodepng。

## 依赖（公共目录，不进项目）

| 依赖 | 默认路径 | 说明 |
|---|---|---|
| GLFW 3.4 | `E:/Code/utils/glfw-3.4.bin.WIN64` | 预编译库，含 `lib-mingw-w64/libglfw3dll.a`、`glfw3.dll` |
| Dear ImGui | `E:/Code/utils/imgui` | 源码 + `backends` |

两个路径都是 CMake 缓存变量，可覆盖：
```bash
cmake -S . -B build -DIMGUI_DIR=你的imgui路径 -DGLFW_DIR=你的glfw路径
```

## 编译

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --target sprite_editor -j
# 产物：build/sprite_editor.exe（POST_BUILD 自动把 glfw3.dll 拷到 exe 旁）
```

## 界面与用法

- **画布**：整个窗口就是画布，PNG 铺满全屏；原点 `(0,0)` = **角色脚下**（锚点位置），精灵左上角 = `(off_x-ref_x, off_y-ref_y)`。坐标轴/原点/精灵随缩放（10%~400%）整体缩放。
- **左边小窗**：选择并加载目录、选择输出目录、保存 PNG+pos.txt、重新打包 sprites.spk、操作历史/撤销/重做。
- **右边小窗**：坐标（off_x/off_y、±1 微调、参考点 ref_x/ref_y 默认 232/328）+ 色盘换色。
- **底部小窗**：帧控制（滑块/上一帧/下一帧/输入）+ 缩放 + 洋葱皮。

| 操作 | 说明 |
|---|---|
| 选择并加载目录 | 选含 `0.png~N.png` + `pos.txt` 的目录；目录记入 `sprite_editor.cfg`，下次启动自动加载 |
| 调整坐标 | 左键在画布上拖动当前帧，或右侧输入/±1；改的是裸 `pos.txt` 偏移，参考点不变 |
| 洋葱皮 | 上一帧 30% 透明垫底，可勾选关闭 |
| 平移画布 | 鼠标**中键**按住拖动，整体移动画布/原点 |
| 色盘换色 | 点色卡选源色 → 设目标色 → 调容差（RGB 欧氏距离 0~441）→ 预览当前帧 → 应用到全部帧（alpha 保留） |
| 保存输出 | 选输出目录后点「保存 PNG + pos.txt」（PNG 命名 `i.png`，pos.txt 用 `\t` 分隔） |
| 重新打包 | 独立按钮「重新打包 sprites.spk」，从内存直接写 SPK，绕开 `spk_packer.cpp` 内写死的通道替换 |
| 撤销/重做 | `Ctrl+Z` / `Ctrl+Shift+Z` / `Ctrl+Y`，操作历史面板实时可见 |

> 坐标系口径：锚点 `(ref_x-off_x, ref_y-off_y)` 落在角色脚下（原点），故精灵左上角相对原点 = `(off_x-ref_x, off_y-ref_y)`。