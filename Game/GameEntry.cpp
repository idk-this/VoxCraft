//
// Created by IDKTHIS on 02.09.2025.
//
#include "Application/Application.h"

extern "C"
#ifdef _WIN32
__declspec(dllexport)
#endif
int GameEntry() {
    Engine::Application app("VoxCraft Beta", 1920, 1080);
    app.Run();
    return 0;
}