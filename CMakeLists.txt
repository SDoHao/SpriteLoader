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

# ================= 精灵编辑器 sprite_editor.exe (GLFW + OpenGL3 + ImGui) =================
# 公共依赖目录，可用 -DIMGUI_DIR=... -DGLFW_DIR=... 覆盖
set(IMGUI_DIR "E:/Code/utils/imgui" CACHE PATH "ImGui 源码目录")
set(GLFW_DIR  "E:/Code/utils/glfw-3.4.bin.WIN64" CACHE PATH "GLFW 预编译目录")

add_executable(sprite_editor
    src/sprite_editor.cpp
    src/editor_config.cpp
    src/editor_camera.cpp
    src/editor_loader.cpp
    src/editor_palette.cpp
    src/editor_history.cpp
    src/editor_canvas.cpp
    src/spk_packer.cpp
    src/lodepng.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(sprite_editor PRIVATE
    include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLFW_DIR}/include
)
target_compile_options(sprite_editor PRIVATE -O2)
target_link_libraries(sprite_editor PRIVATE
    ${GLFW_DIR}/lib-mingw-w64/libglfw3dll.a
    opengl32 shell32 ole32 user32 gdi32 comdlg32
)
# 构建后把 glfw3.dll 拷到 exe 旁边
add_custom_command(TARGET sprite_editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${GLFW_DIR}/lib-mingw-w64/glfw3.dll
        $<TARGET_FILE_DIR:sprite_editor>)
