#include <cstdio>
#include <string>
#include "spk_packer.h"
#include "file_manager.h"

int main(int argc, char** argv)
{
    std::string input_path = ".";
    if (argc >= 2)
    {
        input_path = argv[1];
    }
    printf("Current path: %s\n", file_manager{}.getCurrentDirectory().c_str());
    printf("Input path: %s\n", input_path.c_str());

    int ret = pack_sprites_to_spk(input_path, "sprites.spk");
    return ret;
}