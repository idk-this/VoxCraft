// Created by IDKTHIS on 05.07.2025.
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

int main() {
    namespace fs = std::filesystem;
    std::map<std::string, std::vector<fs::path>> paths = {
        {"engine", {"Engine/"}},
        {"game", {"Game/VoxCraft.Game"}}
    };

#ifdef _WIN32
    char exePathBuffer[MAX_PATH];
    if (!GetModuleFileNameA(nullptr, exePathBuffer, MAX_PATH)) {
        std::cerr << "Failed to get executable path\n";
        return -1;
    }
    fs::path exePath = fs::path(exePathBuffer).parent_path();

    for (const auto& enginePath : paths["engine"]) {
        fs::path fullEnginePath = exePath / enginePath;
        if (!SetDllDirectoryA(fullEnginePath.string().c_str())) {
            std::cerr << "Failed to set DLL directory: " << fullEnginePath << "\n";
        }
    }

    fs::path gameDllPath = exePath / (paths["game"][0].string() + ".dll");
    HMODULE lib = LoadLibraryA(gameDllPath.string().c_str());
    if (!lib) {
        std::cerr << "Failed to load " << gameDllPath << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return -1;
    }

    using GameEntryFn = int(*)();
    auto entry = reinterpret_cast<GameEntryFn>(GetProcAddress(lib, "GameEntry"));
    if (!entry) {
        std::cerr << "No GameEntry in " << gameDllPath << "\n";
        return -1;
    }
    return entry();

#else
    // Для Linux получаем путь к исполняемому файлу
    char exePathBuffer[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", exePathBuffer, PATH_MAX);
    if (count == -1) {
        std::cerr << "Failed to get executable path\n";
        return -1;
    }
    fs::path exePath = fs::path(exePathBuffer).parent_path();

    // Загружаем библиотеку игры
    fs::path gameLibPath = exePath / paths["game"][0].replace_extension(".so");
    void* lib = dlopen(gameLibPath.c_str(), RTLD_NOW);
    if (!lib) {
        std::cerr << "Failed to load " << gameLibPath << ": " << dlerror() << "\n";
        return -1;
    }

    using GameEntryFn = int(*)();
    dlerror(); // сброс предыдущих ошибок
    auto entry = reinterpret_cast<GameEntryFn>(dlsym(lib, "GameEntry"));
    if (!entry) {
        std::cerr << "No GameEntry in " << gameLibPath << ": " << dlerror() << "\n";
        dlclose(lib);
        return -1;
    }
    int result = entry();
    dlclose(lib);
    return result;
#endif
}