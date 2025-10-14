//
// Created by IDKTHIS on 28.09.2025.
//

#pragma once
#include <cstdint>
#include "Core/ECS/Base/AActor.h"
#include "Core/Log/Logger.h"


class UWorldGenerator;

class AChunk : public AActor {
    UCLASS(AChunk);
public:
    AChunk(glm::ivec3 chunkCoord, int chunkWidth, int chunkHeight, int chunkDepth, UWorldGenerator* worldGenerator);
    void GenerateChunk();
    void UpdateMesh();
    void SetBlock(int x, int y, int z, uint8_t blockId);
    uint8_t GetBlock(int x, int y, int z) const;
    glm::ivec3 GetChunkCoord() const { return m_chunkCoord; }

private:
    void UpdateCollision();
    UWorldGenerator* m_worldGenerator;
    glm::ivec3 m_chunkCoord;
    int m_chunkSize_w = 16;
    int m_chunkSize_h = 32;
    int m_chunkSize_d = 16;
    std::vector<uint8_t> m_blocks;
};