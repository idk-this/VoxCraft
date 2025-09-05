//
// Created by IDKTHIS on 02.09.2025.
//

#include "Application/VoxCraftGame.h"

extern "C"
#ifdef _WIN32
__declspec(dllexport)
#endif
int GameEntry() {
    VoxCraftGame app{};
    app.Run();
    return 0;
}
