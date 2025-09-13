//
// Created by IDKTHIS on 02.09.2025.
//

#include <filesystem>
#include <iostream>
#include <fstream>
#include "Core/Common/VoxPak.h"
#include "Application/VoxCraftGame.h"
#include "Core/Utils/FileSystem.h"
using namespace std;
namespace fs = std::filesystem;



struct EntryTmp {
    string path;
    uint64_t offset;
    uint64_t size;
};

void Pack(const fs::path& folder, const string& outFile) {
    vector<fs::path> files;
    for(auto& p: fs::recursive_directory_iterator(folder)) {
        if(p.is_regular_file()) files.push_back(p.path());
    }
    fs::path outPath(outFile);
    if(outPath.has_parent_path()) {
        fs::create_directories(outPath.parent_path());
    }

    ofstream ofs(outFile, ios::binary);
    FileHeader hdr{};
    memcpy(hdr.magic, "VOXPAK", 6);
    hdr.magic[6] = '\0';
    hdr.fileCount = files.size();
    ofs.write(reinterpret_cast<char*>(&hdr), sizeof(hdr));

    vector<EntryTmp> table;
    for(auto& f: files) {
        string rel = fs::relative(f, folder).generic_string();
        uint16_t len = rel.size();
        ofs.write(reinterpret_cast<char*>(&len), sizeof(len));
        ofs.write(rel.data(), len);
        uint64_t dummy=0;
        ofs.write(reinterpret_cast<char*>(&dummy), 8);
        ofs.write(reinterpret_cast<char*>(&dummy), 8);
        table.push_back({rel,0,0});
    }
    for(size_t i=0;i<files.size();i++) {
        auto& f = files[i];
        auto& e = table[i];
        e.offset = ofs.tellp();
        ifstream ifs(f, ios::binary);
        vector<char> buf((istreambuf_iterator<char>(ifs)), {});
        e.size = buf.size();
        ofs.write(buf.data(), buf.size());
    }

    ofs.seekp(sizeof(FileHeader), ios::beg);
    for(auto& e : table) {
        uint16_t len = e.path.size();
        ofs.seekp(sizeof(len)+len, ios::cur);
        ofs.write(reinterpret_cast<char*>(&e.offset), 8);
        ofs.write(reinterpret_cast<char*>(&e.size), 8);
    }
}
extern "C"
#ifdef _WIN32
__declspec(dllexport)
#endif
int GameEntry() {

    /*Pack(fs::path("Content/Mods/TestMod/VulkanShaders"), "Content/Paks/VulkanShaders.voxpak");
    //std::cout << "Platform: " << PLATFORM << "\n";
    std::cout << "Working Directory: " << FileSystem::GetWorkingDirectory() << "\n";
    VoxPak pak;
    pak.Open("Content.voxpak");
    for(auto& f: pak.ListFiles()) cout << f << "\n";
    auto shaderSrc = pak.ReadFileWithOverride("Shaders/SimpleRectangle.shader");*/
    VoxCraftGame app{};
    app.Run();
    return 0;
}
