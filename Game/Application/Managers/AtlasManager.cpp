//
// Created by IDKTHIS on 28.09.2025.
//

#include "AtlasManager.h"

#include <cstring>

#include "Core/ECS/Resources/UTexture.h"

AtlasManager::AtlasManager(int maxAtlasSize, int blockWidth, int blockHeight)
    : m_maxAtlasSize(maxAtlasSize), m_blockWidth(blockWidth), m_blockHeight(blockHeight) {}

BlockUV AtlasManager::RegisterBlockAtlas(const std::string& path) {
    Engine::FileLoaders::ImageData img;
    if (!Engine::FileLoaders::ImageLoader::Load(path, img)) {
        throw std::runtime_error("Failed to load block texture: " + path);
    }

    if (m_atlases.empty()) {
        m_atlases.push_back(CreateEmptyAtlas());
    }

    Atlas& atlas = m_atlases.back();

    if (atlas.nextX + img.width > atlas.width) {
        atlas.nextX = 0;
        atlas.nextY += atlas.rowHeight;
        atlas.rowHeight = 0;
    }

    if (atlas.nextY + img.height > atlas.height) {
        m_atlases.push_back(CreateEmptyAtlas());
        return RegisterBlockAtlas(path);
    }

    if (img.height > atlas.rowHeight) {
        atlas.rowHeight = img.height;
    }

    CopySubImage(atlas.texture, atlas.nextX, atlas.nextY, img);

    glm::vec4 uv;
    uv.x = (float)atlas.nextX / atlas.width;
    uv.y = (float)atlas.nextY / atlas.height;
    uv.z = (float)(atlas.nextX + img.width) / atlas.width;
    uv.w = (float)(atlas.nextY + img.height) / atlas.height;

    BlockUV result{ (int)m_atlases.size() - 1, uv };

    atlas.nextX += img.width;

    return result;
}

std::shared_ptr<UTexture> AtlasManager::GetAtlas(int atlasId) const {
    if (atlasId < 0 || atlasId >= (int)m_atlases.size()) {
        throw std::out_of_range("Invalid atlasId");
    }
    return m_atlases[atlasId].texture;
}

AtlasManager::Atlas AtlasManager::CreateEmptyAtlas() {
    Atlas atlas;
    atlas.width  = m_maxAtlasSize;
    atlas.height = m_maxAtlasSize;
    atlas.nextX = 0;
    atlas.nextY = 0;
    atlas.rowHeight = 0;

    std::vector<uint8_t> empty(atlas.width * atlas.height * 4, 0);
    atlas.texture = std::make_shared<UTexture>(
        "atlas",
        atlas.width,
        atlas.height,
        4,
        std::move(empty)
    );

    return atlas;
}

void AtlasManager::CopySubImage(std::shared_ptr<UTexture>& dst, int dstX, int dstY,
                                const Engine::FileLoaders::ImageData& src) {
    if (!dst) return;

    int dstW = dst->GetWidth();
    int channels = dst->GetChannels();

    unsigned char* dstPixels = dst->GetMutableData().data();
    for (int row = 0; row < src.height; ++row) {
        int dstOffset = ((dstY + row) * dstW + dstX) * channels;
        int srcOffset = row * src.width * src.channels;
        std::memcpy(dstPixels + dstOffset, src.pixels.data() + srcOffset,
                    src.width * channels);
    }
}
