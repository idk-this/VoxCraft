//
// Created by IDKTHIS on 28.09.2025.
//

#pragma once
#include <cstdint>
#include "Core/ECS/Base/AActor.h"
#include "Core/Log/Logger.h"


class AChunk : public AActor {
    UCLASS(AChunk);
public:
    AChunk();
    void GenerateChunk();
    void UpdateMesh();
    // Получение и установка блока
    void SetBlock(int x, int y, int z, uint8_t blockId);
    uint8_t GetBlock(int x, int y, int z) const;
    FAABB GetBoundingBox() override {
        auto transform = GetComponent<UTransformComponent>();
        if (!transform) {
            LOG_INFO("AABB", "Actor ID: {} GetBoundingBox: no transform, returning default [-0.5,0.5]",
                     GetObjectID().index);
            return { glm::vec3(-0.5f), glm::vec3(0.5f) };
        }

        glm::vec3 pos = transform->position;
        float blockSize = 1.0f;
        glm::vec3 min = pos;
        glm::vec3 max = pos + glm::vec3(m_chunkSize * blockSize);

        LOG_INFO("AABB", "Actor ID: {} GetBoundingBox: pos=({}, {}, {})  Min=({}, {}, {})  Max=({}, {}, {})",
                 GetObjectID().index,
                 pos.x, pos.y, pos.z,
                 min.x, min.y, min.z,
                 max.x, max.y, max.z);

        return { min, max };
    }

private:

    uint8_t m_chunkSize = 16;
    std::vector<uint8_t> m_blocks; // храним все блоки чанка
};
