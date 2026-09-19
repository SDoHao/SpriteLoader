cmake_minimum_required(VERSION 3.16)
project(SpriteLoader CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 头文件目录
include_directories(include)

# 静态打包器 spk_packer.exe
add_executable(spk_packer
    packer.cpp
    src/spk_packer.cpp
    src/lodepng.cpp
)
target_compile_options(spk_packer PRIVATE -O2)
target_link_options(spk_packer PRIVATE -static-libgcc)

# # Lua C 模块 sprite_loader.dll
# add_library(sprite_loader SHARED src/sprite_loader.cpp)
# # ========= 【需要修改这两行，改成本地lua路径】 =========
# set(LUA_INC "XXX/lua-5.1.4/src")
# set(LUA_LIB "XXX/love-11.5-win64")
# # =======================================================
# target_include_directories(sprite_loader PRIVATE ${LUA_INC})
# target_link_directories(sprite_loader PRIVATE ${LUA_LIB})
# target_link_libraries(sprite_loader lua51)
# target_link_options(sprite_loader PRIVATE -static-libgcc -Wl,--enable-auto-import)
