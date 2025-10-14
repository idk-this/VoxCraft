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
    AChunk(glm::ivec3 chunkCoord, UWorldGenerator* worldGenerator);
    void GenerateChunk();
    void UpdateMesh();
    void SetBlock(int x, int y, int z, uint8_t blockId);
    uint8_t GetBlock(int x, int y, int z) const;
    glm::ivec3 GetChunkCoord() const { return m_chunkCoord; }
    FAABB GetBoundingBox() override {
        auto transform = GetComponent<UTransformComponent>();
        if (!transform) {
            return { glm::vec3(-0.5f), glm::vec3(0.5f) };
        }

        glm::vec3 pos = transform->position;
        float blockSize = 1.0f;
        glm::vec3 min = pos;
        glm::vec3 max = pos + glm::vec3(m_chunkSize * blockSize);

        return { min, max };
    }

private:
    void UpdateCollision();
    UWorldGenerator* m_worldGenerator;
    glm::ivec3 m_chunkCoord;
    uint8_t m_chunkSize = 16;
    std::vector<uint8_t> m_blocks;
};