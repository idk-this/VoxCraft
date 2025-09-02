//
// Created by IDKTHIS on 05.07.2025.
//

#include <iostream>
#include <filesystem>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

int main() {
    namespace fs = std::filesystem;

#ifdef _WIN32
    char exePathBuffer[MAX_PATH];
    if (!GetModuleFileNameA(nullptr, exePathBuffer, MAX_PATH)) {
        std::cerr << "Failed to get executable path\n";
        return -1;
    }

    fs::path exePath = fs::path(exePathBuffer).parent_path();
    fs::path engineDir = exePath / "Engine" / "bin";
    fs::path dllPath = exePath / "Game" / "bin" / "VoxCraft.dll";

    // Добавляем путь к DLL в поисковые пути
    if (!SetDllDirectoryA(engineDir.string().c_str())) {
        std::cerr << "Failed to set DLL directory\n";
        return -1;
    }

    HMODULE lib = LoadLibraryA(dllPath.string().c_str());
    if (!lib) {
        std::cerr << "Failed to load " << dllPath << "\n";
        return -1;
    }

    using GameEntryFn = int(*)();
    auto entry = reinterpret_cast<GameEntryFn>(GetProcAddress(lib, "GameEntry"));
    if (!entry) {
        std::cerr << "No GameEntry in " << dllPath << "\n";
        return -1;
    }

    return entry();

#else
    fs::path libPath = fs::path("Game") / "bin" / "libVoxCraft.so";
    void* lib = dlopen(libPath.c_str(), RTLD_NOW);
    if (!lib) {
        std::cerr << "Failed to load " << libPath << ": " << dlerror() << "\n";
        return -1;
    }

    using GameEntryFn = int(*)();
    dlerror(); // сброс предыдущих ошибок
    auto entry = reinterpret_cast<GameEntryFn>(dlsym(lib, "GameEntry"));
    if (!entry) {
        std::cerr << "No GameEntry in " << libPath << ": " << dlerror() << "\n";
        dlclose(lib);
        return -1;
    }

    int result = entry();
    dlclose(lib);
    return result;
#endif
}
