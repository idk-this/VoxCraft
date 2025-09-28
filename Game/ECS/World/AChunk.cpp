//
// Created by IDKTHIS on 28.09.2025.
//

#include "AChunk.h"

#include "Application/VoxCraftGame.h"
#include "Core/ECS/Components/UMeshComponent.h"
#include "Core/Log/Logger.h"
#include "Core/Utils/FileLoaders/ImageLoader.h"

struct Face { int idx[4]; glm::ivec3 normal; };
struct BlockFaceUV {
    glm::vec2 uv0;
    glm::vec2 uv1;
};
struct BlockAtlasInfo {
    // Для каждого блока — UV для 6 граней (в нормализованных 0..1 координатах большого атласа)
    BlockFaceUV faces[6];
};


static std::vector<std::string> g_blockAtlases = {
    "Textures/Blocks/Bedrock/Block.png",   // blockId = 2
    "Textures/Blocks/Dirt/Block.png",   // blockId = 2
    "Textures/Blocks/Grass/Block.png",   // blockId = 1
    "Textures/Blocks/Stone/Block.png"   // blockId = 2
    // Добавляй здесь остальные блоки
};
static std::vector<std::string> MakeRepeatedAtlases(size_t repeatCount) {
    static const std::vector<std::string> baseAtlases = {
        "Textures/Blocks/Bedrock/Block.png", // blockId = 0
        "Textures/Blocks/Dirt/Block.png",    // blockId = 1
        "Textures/Blocks/Grass/Block.png",   // blockId = 2
        "Textures/Blocks/Stone/Block.png"    // blockId = 3
    };

    std::vector<std::string> result;
    result.reserve(baseAtlases.size() * repeatCount);

    for (size_t i = 0; i < repeatCount; i++) {
        result.insert(result.end(), baseAtlases.begin(), baseAtlases.end());
    }

    return result;
}
static bool BuildCombinedAtlas(
    VoxCraftGame* gameCtx,
    std::vector<uint8_t>& outPixels,
    int& outWidth,
    int& outHeight,
    int& outChannels,
    std::vector<BlockAtlasInfo>& outBlockInfos)
{

    outPixels.clear();
    outBlockInfos.clear();
    struct Img {
        int w=0,h=0,c=0;
        std::vector<uint8_t> pixels;
    };

    std::vector<Img> images;
    images.reserve(g_blockAtlases.size());
    for (auto &path : g_blockAtlases) {
        std::vector<uint8_t> fileData = gameCtx->voxCraftPak.ReadFileWithOverride(path);
        if (fileData.empty()) {
            LOG_WARN("AChunk", "Failed to read atlas file {}", path.c_str());
            return false;
        }
        std::shared_ptr<UTexture> tmpTex = nullptr;
        if (!Engine::FileLoaders::ImageLoader::Load(fileData, tmpTex)) {
            LOG_WARN("AChunk", "ImageLoader::Load failed for {}", path.c_str());
            return false;
        }

        // --- Допущение: UTexture имеет GetWidth/GetHeight/GetChannels/GetData() ---
        int w = tmpTex->GetWidth();
        int h = tmpTex->GetHeight();
        int c = tmpTex->GetChannels();
        const uint8_t* dataPtr = tmpTex->GetData(); // pointer to pixel bytes (RGBA or RGB)

        if (!dataPtr || w <= 0 || h <= 0) {
            LOG_WARN("AChunk", "Invalid image loaded %s", path.c_str());
            return false;
        }

        Img img;
        img.w = w; img.h = h; img.c = c;
        img.pixels.resize(static_cast<size_t>(w) * h * c);
        std::copy(dataPtr, dataPtr + (size_t)w * h * c, img.pixels.data());
        images.push_back(std::move(img));
    }

    // 2) проверка каналов — приведём к RGBA (4 канала)
    outChannels = 4;
    for (auto &img : images) {
        if (img.c != 4) {
            // конвертируем RGB->RGBA (добавляем alpha=255)
            std::vector<uint8_t> conv;
            conv.reserve((size_t)img.w * img.h * 4);
            for (int i = 0; i < img.w * img.h; ++i) {
                uint8_t r = img.pixels[i * img.c + 0];
                uint8_t g = (img.c > 1) ? img.pixels[i * img.c + 1] : r;
                uint8_t b = (img.c > 2) ? img.pixels[i * img.c + 2] : r;
                conv.push_back(r); conv.push_back(g); conv.push_back(b); conv.push_back(255);
            }
            img.pixels.swap(conv);
            img.c = 4;
        }
    }

    // 3) вычислить итоговый размер (склеиваем горизонтально)
    outHeight = 0;
    outWidth = 0;
    for (auto &img : images) {
        outWidth += img.w;
        outHeight = std::max(outHeight, img.h);
    }

    if (outWidth == 0 || outHeight == 0) return false;

    // 4) выделить буфер и заполнить нулями
    outPixels.assign((size_t)outWidth * outHeight * outChannels, 0);

    // 5) копирование каждой текстуры в big atlas, выравнивание по верхнему краю
    int offsetX = 0;
    outBlockInfos.resize(images.size()); // один BlockAtlasInfo на image / blockId
    for (size_t idx = 0; idx < images.size(); ++idx) {
        const Img &img = images[idx];

        for (int y = 0; y < img.h; ++y) {
            for (int x = 0; x < img.w; ++x) {
                size_t srcIndex = (size_t)(y * img.w + x) * img.c;
                size_t dstIndex = (size_t)((y * outWidth) + (x + offsetX)) * outChannels;
                // копируем RGBA
                outPixels[dstIndex + 0] = img.pixels[srcIndex + 0];
                outPixels[dstIndex + 1] = img.pixels[srcIndex + 1];
                outPixels[dstIndex + 2] = img.pixels[srcIndex + 2];
                outPixels[dstIndex + 3] = img.pixels[srcIndex + 3];
            }
            // если img.h < outHeight — оставшиеся строки остаются 0 (transparent)
        }

        // 6) заполним BlockAtlasInfo — предполагаем, что в маленьком атласе
        // 6 частей по горизонтали (как у тебя: atlasParts = 6)
        const int atlasParts = 6;
        int facePixelW = img.w / atlasParts; // целая арифметика — лучше чтобы делилось ровно
        float invW = 1.0f / static_cast<float>(outWidth);
        float invH = 1.0f / static_cast<float>(outHeight);

        for (int face = 0; face < atlasParts; ++face) {
            // пиксельные координаты в большом атласе
            int px0 = offsetX + face * facePixelW;
            int px1 = px0 + facePixelW;

            // нормализованные UV (v: 0..1, верх = 0)
            float u0 = px0 * invW;
            float v0 = 0.0f;
            float u1 = px1 * invW;
            float v1 = img.h * invH; // img.h may be <= outHeight, so v1 <= 1.0

            // запомним (в нотации (u0,v0) - левый-низ ???)
            // Важно: в твоем коде texCoords используется (u,v) где v=0..1,
            // я оставил v0 = 0.0, v1 = img.h/outHeight — при твоём рендерере это должно работать.
            outBlockInfos[idx].faces[face].uv0 = glm::vec2(u0, v0);
            outBlockInfos[idx].faces[face].uv1 = glm::vec2(u1, v1);
        }

        offsetX += img.w;
    }

    return true;
}

AChunk::AChunk()
{
    AddComponent(std::make_shared<UTransformComponent>());
    m_blocks.resize(m_chunkSize * m_chunkSize * m_chunkSize, 0);
    // создаём UMeshComponent — текстуру заменим на объединённый атлас ниже
    auto meshComp = std::make_shared<UMeshComponent>();
    meshComp->Mesh = std::make_shared<UMesh>();
    AddComponent(meshComp);

    // Соберём все маленькие атласы в один большой
    VoxCraftGame* gameCtx = static_cast<VoxCraftGame*>(Engine::GetCurrentContext().Get());

    std::vector<uint8_t> bigPixels;
    int bigW=0, bigH=0, bigC=0;
    std::vector<BlockAtlasInfo> atlasInfos;

    if (BuildCombinedAtlas(gameCtx, bigPixels, bigW, bigH, bigC, atlasInfos)) {
        // Предполагаем, что UTexture умеет принимать сырой буфер:
        // meshComp->Texture->SetFromPixels(width, height, channels, bigPixels);
        // Если у тебя другой API — замени этот вызов соответствующим.
        if (!meshComp->Texture) meshComp->Texture = nullptr;
        std::shared_ptr<UTexture> atlasTex = std::make_shared<UTexture>(
            "CombinedAtlas",  // путь можешь указать фиктивный
            bigW,
            bigH,
            bigC,
            std::move(bigPixels) // переносим, чтобы не копировать
        );

        meshComp->Texture = atlasTex;
        // Сохраним atlasInfos в самом компоненте/меше — у тебя, видимо, есть Mesh->userData или можно сохранить где-то в чарнке.
        // Для простоты — сохраним их в Mesh (предполагаем что UMesh имеет поле userData map<string, any>).
        // Если такого нет — сохрани глобально или как статическое поле в AChunk (ниже добавлено).
    } else {
        LOG_WARN("AChunk", "Failed to build combined atlas");
    }

    // Сразу вызовем генерации (в ней мы пересоберём вершины с использованием atlasInfos).
    GenerateChunk();
}

// Нужно где-то хранить atlas infos, чтобы GenerateChunk мог их использовать.
// Для простоты сделаем static внутри cpp (или перемести в класс).
static std::vector<BlockAtlasInfo> s_blockAtlasInfos;
static int s_bigAtlasWidth = 0;
static int s_bigAtlasHeight = 0;
void AChunk::SetBlock(int x, int y, int z, uint8_t blockId) {
    if (x < 0 || y < 0 || z < 0 ||
           x >= m_chunkSize || y >= m_chunkSize || z >= m_chunkSize) {
        return;
           }

    int idx = x + y * m_chunkSize + z * m_chunkSize * m_chunkSize;
    m_blocks[idx] = blockId;
    UpdateMesh();

}

uint8_t AChunk::GetBlock(int x, int y, int z) const {
    if (x < 0 || y < 0 || z < 0 ||
        x >= m_chunkSize || y >= m_chunkSize || z >= m_chunkSize) {
        return 0;
        }
    return m_blocks[x + y * m_chunkSize + z * m_chunkSize * m_chunkSize];
}
void AChunk::GenerateChunk()
{
    // Если s_blockAtlasInfos пуст — попробуем загрузить/восстановить их повторно.
    // Здесь мы повторно строим большой атлас (можно кэшировать выше, но для простоты — повторно вычислим)
    // В реальной игре какую-то часть логики стоит вынести в отдельный AssetManager.
    if (s_blockAtlasInfos.empty()) {
        VoxCraftGame* gameCtx = static_cast<VoxCraftGame*>(Engine::GetCurrentContext().Get());
        std::vector<uint8_t> bigPixels;
        int bigW=0, bigH=0, bigC=0;
        std::vector<BlockAtlasInfo> atlasInfos;
        if (BuildCombinedAtlas(gameCtx, bigPixels, bigW, bigH, bigC, atlasInfos)) {
            s_blockAtlasInfos = std::move(atlasInfos);
            s_bigAtlasWidth = bigW;
            s_bigAtlasHeight = bigH;
            // Установим текстуру для первого найденного UMeshComponent, если есть
            if (GetComponent<UMeshComponent>()) {
                auto meshComp = GetComponent<UMeshComponent>();

                std::shared_ptr<UTexture> atlasTex = std::make_shared<UTexture>(
     "CombinedAtlas",  // путь можешь указать фиктивный
     bigW,
     bigH,
     bigC,
     std::move(bigPixels) // переносим, чтобы не копировать
 );

                meshComp->Texture = atlasTex;
            }
        } else {
            LOG_WARN("AChunk", "GenerateChunk: could not build atlas");
            return;
        }
    }





    // Тест: заполняем все блоки типом 1 (dirt). Для примера можно чередовать:
    for (int x = 0; x < m_chunkSize; x++) {
        for (int z = 0; z < m_chunkSize; z++) {
            float height = (std::sin(x * 0.5f) + std::cos(z * 0.5f)) * 2.0f + (m_chunkSize / 2);
            for (int y = 0; y < m_chunkSize; y++) {
                if (y < 2) {
                    SetBlock(x, y, z, 1); // bedrock
                } else if (y == static_cast<int>(height)) {
                    SetBlock(x, y, z, 3); // grass
                } else if (y < height) {
                    SetBlock(x, y, z, 2); // dirt
                } else {
                    SetBlock(x, y, z, 0); // air
                }
            }
        }
    }
    UpdateMesh();
}

void AChunk::UpdateMesh()
{
    const float blockSize = 1.0f;
    if (!GetComponent<UMeshComponent>()) return;
    UMesh* mesh = GetComponent<UMeshComponent>()->Mesh.get();
    if (!mesh)
    {
        GetComponent<UMeshComponent>()->Mesh = std::make_shared<UMesh>();
        mesh = GetComponent<UMeshComponent>()->Mesh.get();
    }
    mesh->Clear();
     auto hasBlock = [&](int x, int y, int z) -> bool {
        return GetBlock(x,y,z) != 0;
    };

    const glm::vec3 cubeVertices[8] = {
        {-0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f},
        {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f},
        { 0.5f,  0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f},
    };

    const Face faces[6] = {
        {{0,3,2,1}, { 0, 0,-1}}, // back  (-Z)
        {{4,5,6,7}, { 0, 0, 1}}, // front (+Z)
        {{0,1,5,4}, { 0,-1, 0}}, // bottom(-Y)
        {{3,7,6,2}, { 0, 1, 0}}, // top   (+Y)
        {{0,4,7,3}, {-1, 0, 0}}, // left  (-X)
        {{1,2,6,5}, { 1, 0, 0}}, // right (+X)
    };

    for (int x = 0; x < m_chunkSize; x++) {
        for (int y = 0; y < m_chunkSize; y++) {
            for (int z = 0; z < m_chunkSize; z++) {
                if (!hasBlock(x,y,z)) continue;

                glm::vec3 offset(x * blockSize, y * blockSize, z * blockSize);
                uint8_t blockId = GetBlock(x,y,z); // blocks[x + y * m_chunkSize + z * m_chunkSize * m_chunkSize];
                if (blockId == 0) continue;
                // blockId -> index в s_blockAtlasInfos = blockId - 1
                size_t atlasIndex = static_cast<size_t>(blockId - 1);
                if (atlasIndex >= s_blockAtlasInfos.size()) continue;
                const BlockAtlasInfo &binfo = s_blockAtlasInfos[atlasIndex];

                for (int f = 0; f < 6; f++) {
                    glm::ivec3 n = faces[f].normal;
                    if (hasBlock(x + n.x, y + n.y, z + n.z)) continue;

                    // берем UV из binfo для грани f
                    glm::vec2 b0 = binfo.faces[f].uv0;
                    glm::vec2 b1 = glm::vec2(binfo.faces[f].uv1.x, binfo.faces[f].uv0.y);
                    glm::vec2 b2 = binfo.faces[f].uv1;
                    glm::vec2 b3 = glm::vec2(binfo.faces[f].uv0.x, binfo.faces[f].uv1.y);

                    glm::vec2 a,b,c,d;
                    switch (f) {
                        case 0: // back  -> rotate 180
                            a = b2; b = b1; c = b0; d = b3;
                            break;
                        case 1: // front -> rotate 180 + flipH
                            a = b3; b = b2; c = b1; d = b0;
                            break;
                        case 2: // bottom -> no transform
                            a = b0; b = b1; c = b2; d = b3;
                            break;
                        case 3: // top -> no transform
                            a = b0; b = b1; c = b2; d = b3;
                            break;
                        case 4: // left -> rotate 90cw
                            a = b3; b = b2; c = b1; d = b0;
                            break;
                        case 5: // right -> rotate 90cw + flipV
                            a = b2; b = b1; c = b0; d = b3;
                            break;
                        default:
                            a = b0; b = b1; c = b2; d = b3;
                            break;
                    }

                    uint32_t baseIndex = static_cast<uint32_t>(mesh->vertices.size());

                    mesh->vertices.push_back(cubeVertices[faces[f].idx[0]] * blockSize + offset); mesh->texCoords.push_back(a); mesh->colors.push_back(glm::vec3(1.0f));
                    mesh->vertices.push_back(cubeVertices[faces[f].idx[1]] * blockSize + offset); mesh->texCoords.push_back(b); mesh->colors.push_back(glm::vec3(1.0f));
                    mesh->vertices.push_back(cubeVertices[faces[f].idx[2]] * blockSize + offset); mesh->texCoords.push_back(c); mesh->colors.push_back(glm::vec3(1.0f));
                    mesh->vertices.push_back(cubeVertices[faces[f].idx[3]] * blockSize + offset); mesh->texCoords.push_back(d); mesh->colors.push_back(glm::vec3(1.0f));

                    mesh->indices.push_back(baseIndex + 0);
                    mesh->indices.push_back(baseIndex + 1);
                    mesh->indices.push_back(baseIndex + 2);
                    mesh->indices.push_back(baseIndex + 0);
                    mesh->indices.push_back(baseIndex + 2);
                    mesh->indices.push_back(baseIndex + 3);
                }
            }
        }
    }
    mesh->meshDirty = true;
}
