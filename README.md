### 编译spk_packer.cpp
g++ spk_packer.cpp lodepng.cpp -o spk_packer.exe -O2 -static-libgcc -std=c++1

### 编译sprite_loader.cpp
g++ -shared sprite_loader.cpp -o sprite_loader.dll -I"C:\Users\19951\File\Code\utitls\lua-5.1.4\src" -L"C:\Users\19951\File\Code\utitls\love-11.5-win64" -llua51 -static-libgcc -std=c++17 -Wl,--enable-auto-import