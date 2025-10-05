//
// Created by IDKTHIS on 02.09.2025.
//

#pragma once
#include "Application/Application.h"
#include "Core/CVar/Console.h"
#include "Core/CVar/CVar.h"

struct PairHash {
    size_t operator()(const std::pair<int,int>& p) const noexcept {
        // простой хеш — достаточно для координат чанков
        return std::hash<long long>()((static_cast<long long>(p.first) << 32) ^ static_cast<unsigned long long>(p.second));
    }
};
class AChunk;
class UWorldGenerator;
struct WorldShift {
    glm::ivec2 chunkOffset = {0, 0};
    glm::ivec2 blockOffset = {0, 0};
};

class VoxCraftGame : public Engine::Application {
    public:
        VoxCraftGame();
        ~VoxCraftGame() override;
        std::shared_ptr<AChunk> LoadChunkAt(int cx, int cz);

        void TestSay2(const CommandArgs& args);
        void TestUpdated(const CVarValue& oldValue, const CVarValue& newValue);

        void UnloadChunkAt(int cx, int cz);
        void LoadChunksAround(int centerCx, int centerCz);
        void Init() override;
        void Update(float deltaTime) override;
        void Run() override;

    VoxPak voxCraftPak;
    WorldShift m_worldShift;
    int m_chunkSize = 16;
    float m_blockSize = 1.0f;
    int m_renderRadius = 3;

    std::unordered_map<std::pair<int,int>, std::shared_ptr<AChunk>, PairHash> m_loadedChunks;

    std::shared_ptr<UWorldGenerator> m_worldGenerator;
    std::pair<int,int> m_currentCenterChunk = {INT_MIN, INT_MIN};
};
