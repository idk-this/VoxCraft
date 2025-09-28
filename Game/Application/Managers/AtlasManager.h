//
// Created by IDKTHIS on 28.09.2025.
//

#pragma once
#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "Core/Utils/FileLoaders/ImageLoader.h"

class UTexture;

struct BlockUV {
    int atlasId;
    glm::vec4 uvRect;
};

class AtlasManager {
public:
    AtlasManager(int maxAtlasSize = 4096, int blockWidth = 32, int blockHeight = 32);

    BlockUV RegisterBlockAtlas(const std::string& path);

    std::shared_ptr<UTexture> GetAtlas(int atlasId) const;

private:
    struct Atlas {
        std::shared_ptr<UTexture> texture;
        int width, height;
        int nextX = 0;
        int nextY = 0;
        int rowHeight = 0;
    };

    std::vector<Atlas> m_atlases;
    int m_maxAtlasSize;
    int m_blockWidth;
    int m_blockHeight;

    Atlas CreateEmptyAtlas();
    void CopySubImage(std::shared_ptr<UTexture>& dst, int dstX, int dstY,
                      const Engine::FileLoaders::ImageData& src);
};
