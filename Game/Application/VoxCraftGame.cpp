//
// Created by IDKTHIS on 02.09.2025.
//

#include "VoxCraftGame.h"

#include <unordered_set>
#include <set>
#include <utility>

#include "imgui.h"
#include "Core/CVar/Console.h"
#include "ECS/Player/AVoxCraftPlayerController.h"
#include "Core/CVar/CVar.h"
#include "Core/ECS/Base/UWorld.h"
#include "Core/ECS/Components/UMeshComponent.h"

#include "Core/ECS/Components/UTransformComponent.h"
#include "Core/ECS/Player/ULocalPlayer.h"
#include "Core/Log/Logger.h"
#include "Core/Physics/Components/UCollisionComponent.h"
#include "Core/Physics/Components/UPhysicComponent.h"
#include "Core/UI/ConsoleUI.h"
#include "Core/UI/Core/XMLParser.h"
#include "Core/Utils/CameraUtils.h"
#include "Core/Utils/FileSystem.h"
#include "Core/Utils/FileLoaders/ImageLoader.h"
#include "ECS/Player/AVoxCraftPlayer.h"
#include "Platform/Window/SDL3/SDL3Window.h"
#include "Platform/Window/Components/WindowInputComponent.h"
#include "ECS/World/AChunk.h"
#include "ECS/World/AChunkManager.h"
#include "ECS/World/Generators/UWorldGenerator.h"
#include "UI/Debug/DebugOverlay.h"

REGISTER_COMMAND_CALLBACK("say", "Print text to chat", [](const CommandArgs& args){
                          if (args.empty()) return;
                          std::string msg;
                          for (auto& a : args) msg += a + " ";
                          LOG_INFO("Say", "{}", msg);
                          });
DECLARE_CONVAR("t_test", 1, "test value", CVAR_RUNTIME_ONLY | CVAR_CONSOLE_EDIT);

VoxCraftGame::VoxCraftGame()
: Engine::Application()
{
    SET_CVAR("w_title", "VoxCraft Beta");
    SET_CVAR("sv_allow_modding", true);
    SUBSCRIBE_COMMAND("asd", TestSay2);
    DECLARE_CVAR_CALLBACK("t_test", [this](const CVarValue& oldValue, const CVarValue& newValue) {

        this->TestUpdated(oldValue, newValue);
     });
}

VoxCraftGame::~VoxCraftGame()
{
}


void VoxCraftGame::TestSay2(const CommandArgs& args)
{
    LOG_INFO("TestSay2", "Got tp command with {} args", args.size());
}

void VoxCraftGame::TestUpdated(const CVarValue& oldValue, const CVarValue& newValue)
{
    auto toStr = [](const CVarValue& v) {
        return std::visit([](auto&& val) -> std::string {
            if constexpr (std::is_same_v<std::decay_t<decltype(val)>, bool>)
                return val ? "true" : "false";
            else if constexpr (std::is_same_v<std::decay_t<decltype(val)>, std::string>)
                return val;
            else
                return std::to_string(val);
        }, v);
    };

    LOG_INFO("CVar update", "Updated t_test old: {} new: {}", toStr(oldValue), toStr(newValue));
}


void VoxCraftGame::Init()
{
    Application::Init();
    ConsoleSystem::Instance().Execute("say Hello World!!!");
    bool mainPakLoaded = voxCraftPak.Open(Engine::FileSystem::GetWorkingDirectory() + "Content/Paks/VoxCraftRes.voxpak");
    if (!mainPakLoaded)
    {
        LOG_FATAL("Application", "Failed to load VoxCraftRes.voxpak");
        return;
    }

    m_debugOverlay = std::make_shared<DebugOverlay>();
    m_debugOverlay->Init();
    m_localPlayer = std::make_shared<ULocalPlayer>();

    auto playerContoller = m_world->SpawnActor<AVoxCraftPlayerController>();
    playerContoller->SetPlayer(m_localPlayer);
    auto playerPawn = m_world->SpawnActor<AVoxCraftPlayer>();
    playerPawn->GetComponent<UTransformComponent>()->position = glm::vec3(0, 25, 0);
    m_localPlayer->GetController()->Possess(playerPawn);
    m_chunkManager_id = m_world->SpawnActor<AChunkManager>()->GetObjectID();
    m_blockSize = 1.0f;
}


std::optional<glm::ivec3> GetBlockCoordsFromHit(AChunk* chunk, const glm::vec3& hitLocation, float blockSize = 1.0f) {
    if (!chunk) return std::nullopt;

    auto transform = chunk->GetComponent<UTransformComponent>();
    if (!transform) return std::nullopt;

    glm::vec3 localPos = hitLocation - transform->position;

    int chunkSize = 16;

    int x = glm::clamp(static_cast<int>(std::floor(localPos.x / blockSize)), 0, chunkSize - 1);
    int y = glm::clamp(static_cast<int>(std::floor(localPos.y / blockSize)), 0, chunkSize - 1);
    int z = glm::clamp(static_cast<int>(std::floor(localPos.z / blockSize)), 0, 256 - 1);

    if (x >= 0 && x < chunkSize &&
        y >= 0 && y < chunkSize &&
        z >= 0 && z < 256)
    {
        return glm::ivec3(x, y, z);
    }

    return std::nullopt;
}
// Исправленная версия TraceBlock:
std::optional<glm::ivec3> TraceBlock(AChunk* chunk,
                                     const glm::vec3& start,
                                     const glm::vec3& dir,
                                     float maxDist,
                                     float blockSize = 1.0f)
{
    if (!chunk) return std::nullopt;

    auto transform = chunk->GetComponent<UTransformComponent>();
    if (!transform) return std::nullopt;

    glm::vec3 localStart = start - transform->position;

    // Нормализуем направление
    glm::vec3 rayDir = glm::normalize(dir);

    // Начальная позиция в блоках
    glm::vec3 rayPos = localStart / blockSize;

    // Шаг и направление
    glm::ivec3 step(
        rayDir.x > 0 ? 1 : -1,
        rayDir.y > 0 ? 1 : -1,
        rayDir.z > 0 ? 1 : -1
    );

    glm::vec3 deltaDist = glm::abs(glm::vec3(
        rayDir.x == 0 ? 1e30f : 1.0f / std::abs(rayDir.x),
        rayDir.y == 0 ? 1e30f : 1.0f / std::abs(rayDir.y),
        rayDir.z == 0 ? 1e30f : 1.0f / std::abs(rayDir.z)
    ));

    glm::ivec3 voxel(
        static_cast<int>(std::floor(rayPos.x)),
        static_cast<int>(std::floor(rayPos.y)),
        static_cast<int>(std::floor(rayPos.z))
    );

    glm::vec3 sideDist;
    sideDist.x = (rayDir.x > 0 ? (voxel.x + 1 - rayPos.x) : (rayPos.x - voxel.x)) * deltaDist.x;
    sideDist.y = (rayDir.y > 0 ? (voxel.y + 1 - rayPos.y) : (rayPos.y - voxel.y)) * deltaDist.y;
    sideDist.z = (rayDir.z > 0 ? (voxel.z + 1 - rayPos.z) : (rayPos.z - voxel.z)) * deltaDist.z;

    float traveled = 0.0f;
    int chunkSize = 16; // Должно соответствовать m_chunkSize из AChunk
    int chunkHeight = 32; // Должно соответствовать CHUNK_HEIGHT из AChunk

    while (traveled < maxDist) {
        // Проверяем границы с правильной высотой
        if (voxel.x >= 0 && voxel.x < chunkSize &&
            voxel.y >= 0 && voxel.y < chunkHeight &&
            voxel.z >= 0 && voxel.z < chunkSize)
        {
            if (chunk->GetBlock(voxel.x, voxel.y, voxel.z) != 0) {
                return glm::ivec3(voxel.x, voxel.y, voxel.z);
            }
        }

        // DDA шаг
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            traveled = sideDist.x;
            sideDist.x += deltaDist.x;
            voxel.x += step.x;
        } else if (sideDist.y < sideDist.z) {
            traveled = sideDist.y;
            sideDist.y += deltaDist.y;
            voxel.y += step.y;
        } else {
            traveled = sideDist.z;
            sideDist.z += deltaDist.z;
            voxel.z += step.z;
        }
    }

    return std::nullopt;
}
struct BlockHit {
    glm::ivec3 coords;
    glm::ivec3 normal;
};

std::optional<BlockHit> TraceBlockDDA(AChunk* chunk,
                                      const glm::vec3& rayOrigin,
                                      const glm::vec3& rayDirNormalized,
                                      float maxDistance,
                                      float blockSize = 1.0f)
{
    if (!chunk) return std::nullopt;
    if (glm::length2(rayDirNormalized) < 1e-12f) return std::nullopt;

    const glm::vec3 chunkMin = chunk->GetComponent<UTransformComponent>()->position;
    const int chunkSize = 16;
    const glm::vec3 chunkMax = chunkMin + glm::vec3(chunkSize * blockSize);

    glm::vec3 invDir(
        (rayDirNormalized.x != 0.0f) ? 1.0f / rayDirNormalized.x : std::numeric_limits<float>::infinity(),
        (rayDirNormalized.y != 0.0f) ? 1.0f / rayDirNormalized.y : std::numeric_limits<float>::infinity(),
        (rayDirNormalized.z != 0.0f) ? 1.0f / rayDirNormalized.z : std::numeric_limits<float>::infinity()
    );

    glm::vec3 t1 = (chunkMin - rayOrigin) * invDir;
    glm::vec3 t2 = (chunkMax - rayOrigin) * invDir;

    glm::vec3 tmin3 = glm::min(t1, t2);
    glm::vec3 tmax3 = glm::max(t1, t2);

    float tEntry = std::max(std::max(tmin3.x, tmin3.y), tmin3.z);
    float tExit  = std::min(std::min(tmax3.x, tmax3.y), tmax3.z);

    if (tExit < 0.0f) return std::nullopt;
    if (tEntry > tExit) return std::nullopt;
    if (tEntry > maxDistance) return std::nullopt;

    float tStart = std::max(tEntry, 0.0f);
    float tLimit = std::min(tExit, maxDistance);

    glm::vec3 posAtEntry = rayOrigin + rayDirNormalized * tStart;
    glm::vec3 local = posAtEntry - chunkMin;

    auto floored = [](float v)->int { return static_cast<int>(std::floor(v)); };

    int ix = floored(local.x / blockSize);
    int iy = floored(local.y / blockSize);
    int iz = floored(local.z / blockSize);

    ix = glm::clamp(ix, 0, chunkSize - 1);
    iy = glm::clamp(iy, 0, chunkSize - 1);
    iz = glm::clamp(iz, 0, chunkSize - 1);

    int stepX = (rayDirNormalized.x > 0.0f) ? 1 : -1;
    int stepY = (rayDirNormalized.y > 0.0f) ? 1 : -1;
    int stepZ = (rayDirNormalized.z > 0.0f) ? 1 : -1;

    const float INF = std::numeric_limits<float>::infinity();

    auto makeBoundaryT = [&](int voxelIndex, float rayOriginCoord, float axisChunkMin, float dirComp, int step) -> float {
        if (dirComp == 0.0f) return INF;
        float boundaryLocal = (voxelIndex + (step > 0 ? 1.0f : 0.0f)) * blockSize;
        float boundaryWorld = axisChunkMin + boundaryLocal;
        return (boundaryWorld - rayOriginCoord) / dirComp;
    };

    float tMaxX = makeBoundaryT(ix, rayOrigin.x, chunkMin.x, rayDirNormalized.x, stepX);
    float tMaxY = makeBoundaryT(iy, rayOrigin.y, chunkMin.y, rayDirNormalized.y, stepY);
    float tMaxZ = makeBoundaryT(iz, rayOrigin.z, chunkMin.z, rayDirNormalized.z, stepZ);

    float tDeltaX = (rayDirNormalized.x == 0.0f) ? INF : (blockSize / std::abs(rayDirNormalized.x));
    float tDeltaY = (rayDirNormalized.y == 0.0f) ? INF : (blockSize / std::abs(rayDirNormalized.y));
    float tDeltaZ = (rayDirNormalized.z == 0.0f) ? INF : (blockSize / std::abs(rayDirNormalized.z));

    float currentT = tStart;

    if (ix >= 0 && ix < chunkSize && iy >= 0 && iy < chunkSize && iz >= 0 && iz < chunkSize) {
        if (chunk->GetBlock(ix, iy, iz) != 0) {
            return BlockHit{ glm::ivec3(ix, iy, iz), glm::ivec3(0) };
        }
    }

    while (currentT <= tLimit) {
        glm::ivec3 normal(0);

        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                ix += stepX;
                currentT = tMaxX;
                tMaxX += tDeltaX;
                normal = { -stepX, 0, 0 };
            } else {
                iz += stepZ;
                currentT = tMaxZ;
                tMaxZ += tDeltaZ;
                normal = { 0, 0, -stepZ };
            }
        } else {
            if (tMaxY < tMaxZ) {
                iy += stepY;
                currentT = tMaxY;
                tMaxY += tDeltaY;
                normal = { 0, -stepY, 0 };
            } else {
                iz += stepZ;
                currentT = tMaxZ;
                tMaxZ += tDeltaZ;
                normal = { 0, 0, -stepZ };
            }
        }

        if (ix < 0 || ix >= chunkSize ||
            iy < 0 || iy >= chunkSize ||
            iz < 0 || iz >= chunkSize) {
            break;
        }

        if (currentT > tLimit) break;

        if (chunk->GetBlock(ix, iy, iz) != 0) {
            return BlockHit{ glm::ivec3(ix, iy, iz), normal };
        }
    }

    return std::nullopt;
}
void VoxCraftGame::DrawBlockBounds() {
    auto* player = static_cast<AVoxCraftPlayer*>(m_localPlayer->GetController()->GetPawn().get());
    UCameraComponent* camera = player->GetComponent<UCameraComponent>();

    glm::vec3 rayStart = camera->GetWorldPosition();
    glm::vec3 rayDir = camera->GetForwardVector();

    // Выполняем raycast
    RaycastResult hit = PerformRaycast(rayStart, rayDir, 50.0f);


    if (hit.hit && hit.chunk) {
        float blockSize = 1.0f;
        glm::vec3 chunkPos = hit.chunk->GetComponent<UTransformComponent>()->position;
        glm::vec3 min = chunkPos + glm::vec3(hit.blockCoord) * blockSize;
        glm::vec3 max = min + glm::vec3(blockSize);
        glm::vec3 vertices[8] = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z},
            {max.x, max.y, min.z}, {min.x, max.y, min.z},
            {min.x, min.y, max.z}, {max.x, min.y, max.z},
            {max.x, max.y, max.z}, {min.x, max.y, max.z}
        };
        int edges[12][2] = {
            {0,1},{1,2},{2,3},{3,0},
            {4,5},{5,6},{6,7},{7,4},
            {0,4},{1,5},{2,6},{3,7}
        };
        if (hit.hit && hit.chunk) {
            float blockSize = 1.0f;
            glm::vec3 chunkPos = hit.chunk->GetComponent<UTransformComponent>()->position;
            glm::vec3 blockWorldPos = chunkPos + glm::vec3(hit.blockCoord) * blockSize + glm::vec3(blockSize/2); // центр блока

            glm::vec2 screenPos;
            if (CameraUtils::WorldToScreen(camera->GetViewMatrix(), camera->GetProjectionMatrix(), blockWorldPos, screenPos)) {
                ImGui::GetBackgroundDrawList()->AddCircleFilled(
                    ImVec2(screenPos.x, screenPos.y),
                    5.0f,
                    IM_COL32(0, 255, 0, 255)
                );
            }
        }
        for (int i = 0; i < 12; i++) {
            glm::vec2 p1, p2;
            if (CameraUtils::WorldToScreen(camera->GetViewMatrix(), camera->GetProjectionMatrix(), vertices[edges[i][0]], p1) &&
                CameraUtils::WorldToScreen(camera->GetViewMatrix(), camera->GetProjectionMatrix(), vertices[edges[i][1]], p2)) {

                ImGui::GetBackgroundDrawList()->AddLine(
                    ImVec2(p1.x, p1.y),
                    ImVec2(p2.x, p2.y),
                    IM_COL32(0, 255, 0, 255),
                    2.0f
                );
            }
        }
    }
}
VoxCraftGame::RaycastResult VoxCraftGame::PerformRaycast(const glm::vec3& start, const glm::vec3& direction, float maxDistance) {
    RaycastResult result;

    // Нормализуем направление
    glm::vec3 rayDir = glm::normalize(direction);
    glm::vec3 rayPos = start;

    // Параметры для DDA алгоритма
    glm::ivec3 voxel(
        static_cast<int>(std::floor(rayPos.x)),
        static_cast<int>(std::floor(rayPos.y)),
        static_cast<int>(std::floor(rayPos.z))
    );

    glm::vec3 rayStep(
        (rayDir.x > 0) ? 1 : -1,
        (rayDir.y > 0) ? 1 : -1,
        (rayDir.z > 0) ? 1 : -1
    );

    // Вектор до следующей границы вокселя
    glm::vec3 nextBoundary = glm::vec3(
        (rayStep.x > 0) ? (voxel.x + 1) : voxel.x,
        (rayStep.y > 0) ? (voxel.y + 1) : voxel.y,
        (rayStep.z > 0) ? (voxel.z + 1) : voxel.z
    );

    // Расстояние до следующей границы
    glm::vec3 tMax = glm::vec3(
        (rayDir.x != 0) ? (nextBoundary.x - rayPos.x) / rayDir.x : std::numeric_limits<float>::max(),
        (rayDir.y != 0) ? (nextBoundary.y - rayPos.y) / rayDir.y : std::numeric_limits<float>::max(),
        (rayDir.z != 0) ? (nextBoundary.z - rayPos.z) / rayDir.z : std::numeric_limits<float>::max()
    );

    // Изменение t при переходе между вокселями
    glm::vec3 tDelta = glm::vec3(
        (rayDir.x != 0) ? rayStep.x / rayDir.x : std::numeric_limits<float>::max(),
        (rayDir.y != 0) ? rayStep.y / rayDir.y : std::numeric_limits<float>::max(),
        (rayDir.z != 0) ? rayStep.z / rayDir.z : std::numeric_limits<float>::max()
    );

    glm::ivec3 lastVoxel = voxel;
    float traveled = 0.0f;

    while (traveled < maxDistance) {
        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            traveled = tMax.x;
            tMax.x += tDelta.x;
            voxel.x += static_cast<int>(rayStep.x);
        } else if (tMax.y < tMax.z) {
            traveled = tMax.y;
            tMax.y += tDelta.y;
            voxel.y += static_cast<int>(rayStep.y);
        } else {
            traveled = tMax.z;
            tMax.z += tDelta.z;
            voxel.z += static_cast<int>(rayStep.z);
        }

        if (traveled > maxDistance) break;

        rayPos = start + rayDir * traveled;
        glm::ivec3 chunkCoord = m_world->GetActor<AChunkManager>(m_chunkManager_id)->WorldToChunkCoord(glm::vec3(voxel.x, voxel.y, voxel.z));

        // Получаем чанк
        auto chunk = m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetChunk(chunkCoord);
        if (!chunk) continue;

        // Получаем координаты блока в чанке
        glm::ivec3 blockCoord = m_world->GetActor<AChunkManager>(m_chunkManager_id)->WorldToBlockCoord(glm::vec3(voxel.x, voxel.y, voxel.z));

        // Проверяем границы блока
        if (blockCoord.x >= 0 && blockCoord.x < 16 &&
            blockCoord.y >= 0 && blockCoord.y < 32 &&
            blockCoord.z >= 0 && blockCoord.z < 16) {

            // Проверяем, есть ли блок в этой позиции
            if (chunk->GetBlock(blockCoord.x, blockCoord.y, blockCoord.z) != 0) {
                result.hit = true;
                result.blockCoord = blockCoord;
                result.chunk = chunk.get();
                result.distance = traveled;

                // Определяем нормаль (направление попадания)
                if (voxel.x != lastVoxel.x) {
                    result.normal = glm::ivec3((voxel.x > lastVoxel.x) ? -1 : 1, 0, 0);
                } else if (voxel.y != lastVoxel.y) {
                    result.normal = glm::ivec3(0, (voxel.y > lastVoxel.y) ? -1 : 1, 0);
                } else {
                    result.normal = glm::ivec3(0, 0, (voxel.z > lastVoxel.z) ? -1 : 1);
                }

                return result;
            }
        }

        lastVoxel = voxel;
    }

    return result;
}

ConsoleUI console;
void VoxCraftGame::Update(float deltaTime)
{
    console.Draw();

    Application::Update(deltaTime);
    if (window->GetInputComponent()->IsKeyPressed(KeyCode::KEY_GRAVE))
    {
        console.Toggle();
    }
    glm::vec3 playerPos = m_localPlayer->GetController()->GetPawn()->GetComponent<UTransformComponent>()->position;
    m_debugOverlay->SetProperty(
    "CurrentPlayerPos",
     std::format("Local Position: X: {:.2f}  Y: {:.2f}  Z: {:.2f}",
                 playerPos.x, playerPos.y, playerPos.z)
     );
    m_debugOverlay->SetProperty(
        "CurrentPlayerBlockPos",
        std::format(
            "Local Block Position: X: {}  Y: {}  Z: {}",
            static_cast<int>(playerPos.x),
            static_cast<int>(playerPos.y),
            static_cast<int>(playerPos.z)
        )
    );
    m_debugOverlay->SetProperty(
        "CurrentPlayerChunkPos",
        std::format(
            "Local Chunk Position: X: {}  Y: {} Z: {}",
                m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetLastCenterChunk().x,
                m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetLastCenterChunk().y,
                m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetLastCenterChunk().z
        )
    );
    auto playerRot = m_localPlayer->GetController()->GetPawn()->GetComponent<UCameraComponent>()->RelativeRotation;
    m_debugOverlay->SetProperty(
        "PlayerViewAngle",
        std::format(
            "View Angle (PYR): P: {:.1f}  Y: {:.1f}  R: {:.1f}",
            playerRot.x,  // Pitch
            playerRot.y,  // Yaw
            playerRot.z   // Roll
        )
    );
    float fps = 1.0f / deltaTime;
    float dtMs = deltaTime * 1000.0f;

    m_debugOverlay->SetProperty(
        "CurrentFps",
        std::format("FPS: {:.1f}  (dt: {:.2f} ms)", fps, dtMs)
    );
    m_debugOverlay->Render();


    static bool lastRightButton = false;
    bool rightButton = window->GetInputComponent()->GetMouseState().buttons[3];
    if (rightButton && !lastRightButton) {
        window->ToggleRelativeMouseMode();
    }
    lastRightButton = rightButton;
    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_LEFT_ALT))
    {
        auto* player = static_cast<AVoxCraftPlayer*>(m_localPlayer->GetController()->GetPawn().get());
        UCameraComponent* camera = player->GetComponent<UCameraComponent>();

        glm::vec3 rayStart = camera->GetWorldPosition();
        glm::vec3 rayDir = camera->GetForwardVector();

        RaycastResult hit = PerformRaycast(rayStart, rayDir, 50.0f);

        if (hit.hit && hit.chunk) {
            // Вычисляем позицию для нового блока (текущий блок + нормаль)
            glm::ivec3 newBlockPos = hit.blockCoord + hit.normal;

            // Проверяем, что новая позиция в пределах чанка
            if (newBlockPos.x >= 0 && newBlockPos.x < 16 &&
                newBlockPos.y >= 0 && newBlockPos.y < 32 &&
                newBlockPos.z >= 0 && newBlockPos.z < 16) {

                // Проверяем, что на этой позиции нет блока
                if (hit.chunk->GetBlock(newBlockPos.x, newBlockPos.y, newBlockPos.z) == 0) {
                    // Ставим новый блок (например, камень с ID = 1)
                    hit.chunk->SetBlock(newBlockPos.x, newBlockPos.y, newBlockPos.z, 1);
                    LOG_INFO("PLACE", "Placed block at: {} {} {} (Chunk: {},{},{})",
                            newBlockPos.x, newBlockPos.y, newBlockPos.z,
                            hit.chunk->GetChunkCoord().x, hit.chunk->GetChunkCoord().y, hit.chunk->GetChunkCoord().z);
                }
            } else {
                // Если блок выходит за пределы текущего чанка, нужно найти соседний чанк
                glm::ivec3 chunkCoord = hit.chunk->GetChunkCoord();
                glm::ivec3 worldVoxel = chunkCoord * 16 + newBlockPos;
                glm::ivec3 newChunkCoord = m_world->GetActor<AChunkManager>(m_chunkManager_id)->WorldToChunkCoord(glm::vec3(worldVoxel));
                glm::ivec3 newBlockCoord = m_world->GetActor<AChunkManager>(m_chunkManager_id)->WorldToBlockCoord(glm::vec3(worldVoxel));

                auto newChunk = m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetChunk(newChunkCoord);
                if (newChunk) {
                    if (newChunk->GetBlock(newBlockCoord.x, newBlockCoord.y, newBlockCoord.z) == 0) {
                        newChunk->SetBlock(newBlockCoord.x, newBlockCoord.y, newBlockCoord.z, 1);
                        LOG_INFO("PLACE", "Placed block in neighbor chunk at: {} {} {} (Chunk: {},{},{})",
                                newBlockCoord.x, newBlockCoord.y, newBlockCoord.z,
                                newChunkCoord.x, newChunkCoord.y, newChunkCoord.z);
                    }
                }
            }
        }
    }

    if (window->GetInputComponent()->GetMouseState().buttons[1]) {
        auto* player = static_cast<AVoxCraftPlayer*>(m_localPlayer->GetController()->GetPawn().get());
        UCameraComponent* camera = player->GetComponent<UCameraComponent>();

        glm::vec3 rayStart = camera->GetWorldPosition();
        glm::vec3 rayDir = camera->GetForwardVector();

        RaycastResult hit = PerformRaycast(rayStart, rayDir, 50.0f);

        if (hit.hit && hit.chunk) {
            LOG_INFO("HIT", "Block: {} {} {} (Type: {})",
                    hit.blockCoord.x, hit.blockCoord.y, hit.blockCoord.z,
                    hit.chunk->GetBlock(hit.blockCoord.x, hit.blockCoord.y, hit.blockCoord.z));

            hit.chunk->SetBlock(hit.blockCoord.x, hit.blockCoord.y, hit.blockCoord.z, 0);
        }
    }
    {
        auto* playerPawn = m_localPlayer->GetController()->GetPawn().get();
        if (playerPawn) {
            glm::vec2 screenPos;
            glm::vec3 cameraPos = playerPawn->GetComponent<UTransformComponent>()->position; // позиция камеры

            m_world->GetActor<AChunkManager>(m_chunkManager_id)->Update(deltaTime, playerPos);
            DrawBlockBounds(
                );
            auto* camera = playerPawn->GetComponent<UCameraComponent>();

            if (playerPawn->HasComponent(typeid(UPhysicComponent)))
            {
                playerPos.y -= 1;
                playerPawn->GetComponent<UPhysicComponent>()->SetGrounded(m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetBlock(playerPos) != 0);
                auto* chunk = m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetChunk(m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetLastCenterChunk()).get();


            }
        }
    }


}

void VoxCraftGame::Run()
{
    Application::Run();
    m_logSystem->add_output("*", std::cout);
    console.InitializeLoggerHook(m_logSystem.get());
    //uim_logSystem->add_output("*", std::cout);
    LOG_INFO("Application", "Creating window.");
    window = std::make_unique<SDL3Window>();
    if (!window->Create(GET_CVAR(int, "w_size_width"),
                        GET_CVAR(int, "w_size_height"),
                        GET_CVAR(std::string, "w_title"))) {
        LOG_FATAL("Application", "Window creation failed. Parameters: width({}) height({}) title({})",
                  GET_CVAR(int, "w_size_width"),
                  GET_CVAR(int, "w_size_height"),
                  GET_CVAR(std::string, "w_title"));
        return;
                        }
    m_world = std::make_shared<UWorld>();
    LOG_INFO("Application", "Creating Vulkan renderer.");
    renderer = std::make_unique<VulkanRenderer>();
    if (!renderer->Init(window.get(), m_world.get())) {
        LOG_FATAL("Application", "Failed to initialize Vulkan renderer.");
        return;
    }

    auto* imguiContext = Engine::GetCurrentContext().GetImGui()->GetContext();
    ImGui::SetCurrentContext(imguiContext);

    LOG_INFO("Application", "Initializing game world.");
    Init();

    LOG_INFO("Application", "Initialization successful.");
    LOG_INFO("Application", "Starting main loop.");
    MainLoop();

    LOG_INFO("Application", "Main loop terminated. Shutting down renderer.");
    renderer->Cleanup();
    LOG_INFO("Application", "Renderer cleaned up. Executing shutdown procedures.");
    Shutdown();
    LOG_INFO("Application", "Shutdown complete.");
}

