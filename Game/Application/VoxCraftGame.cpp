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
#include "Core/Physics/Components/UPhysicComponent.h"
#include "Core/UI/ConsoleUI.h"
#include "Core/UI/Core/XMLParser.h"
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
std::optional<glm::ivec3> TraceBlock(AChunk* chunk,
                                     const glm::vec3& start,
                                     const glm::vec3& dir,
                                     float maxDist,
                                     float blockSize = 1.0f)
{
    auto transform = chunk->GetComponent<UTransformComponent>();
    if (!transform) return std::nullopt;

    glm::vec3 localStart = start - transform->position;

    int x = static_cast<int>(std::floor(localStart.x / blockSize));
    int y = static_cast<int>(std::floor(localStart.y / blockSize));
    int z = static_cast<int>(std::floor(localStart.z / blockSize));

    glm::vec3 deltaDist = glm::abs(glm::vec3(
        blockSize / dir.x,
        blockSize / dir.y,
        blockSize / dir.z
    ));

    glm::ivec3 step(
        dir.x > 0 ? 1 : -1,
        dir.y > 0 ? 1 : -1,
        dir.z > 0 ? 1 : -1
    );

    glm::vec3 sideDist;
    auto nextBoundary = [&](float pos, float d, int step) {
        return step > 0 ? (std::floor(pos / blockSize) + 1) * blockSize - pos
                        : pos - std::floor(pos / blockSize) * blockSize;
    };

    sideDist.x = nextBoundary(localStart.x, dir.x, step.x) / std::abs(dir.x);
    sideDist.y = nextBoundary(localStart.y, dir.y, step.y) / std::abs(dir.y);
    sideDist.z = nextBoundary(localStart.z, dir.z, step.z) / std::abs(dir.z);

    float traveled = 0.0f;

    while (traveled < maxDist) {
        if (x >= 0 && x < 16 &&
            y >= 0 && y < 16 &&
            z >= 0 && z < 16)
        {
            if (chunk->GetBlock(x, y, z) != 0) {
                return glm::ivec3(x, y, z);
            }
        } else {
            break;
        }
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            sideDist.x += deltaDist.x;
            x += step.x;
            traveled = sideDist.x;
        } else if (sideDist.y < sideDist.z) {
            sideDist.y += deltaDist.y;
            y += step.y;
            traveled = sideDist.y;
        } else {
            sideDist.z += deltaDist.z;
            z += step.z;
            traveled = sideDist.z;
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


    if (window->GetInputComponent()->GetMouseState().buttons[1])
    {
        auto* player = static_cast<AVoxCraftPlayer*>(m_localPlayer->GetController()->GetPawn().get());
        UCameraComponent* camera = player->GetComponent<UCameraComponent>();
        auto hit = m_world->LineTrace(camera->GetWorldPosition(), camera->GetForwardVector(), 500.0f, m_localPlayer->GetController()->GetPawn().get() );

        if (hit.bHit) {
            if (auto* chunk = dynamic_cast<AChunk*>(hit.HitActor.get())) {
               auto blockCoords = TraceBlockDDA(chunk, camera->GetWorldPosition(), glm::normalize(camera->GetForwardVector()), 500.0f);
                if (blockCoords) {
                    LOG_INFO("HIT", "Block: {} {} {} ({})", blockCoords->coords.x, blockCoords->coords.y, blockCoords->coords.z, chunk->GetBlock(blockCoords->coords.x, blockCoords->coords.y, blockCoords->coords.z));
                    chunk->SetBlock(blockCoords->coords.x, blockCoords->coords.y, blockCoords->coords.z, 0);
                }
            }
        }
    }
    {
        auto* playerPawn = m_localPlayer->GetController()->GetPawn().get();
        if (playerPawn) {

            glm::vec3 playerPos = playerPawn->GetComponent<UTransformComponent>()->position;
            m_world->GetActor<AChunkManager>(m_chunkManager_id)->Update(deltaTime, playerPos);
            if (playerPawn->HasComponent(typeid(UPhysicComponent)))
            {
                playerPos.y -= 1;
                playerPawn->GetComponent<UPhysicComponent>()->SetGrounded(m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetBlock(playerPos) != 0);

            }
        }
    }
    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_Q)) {
        if (!Engine::GetCurrentContext().GetWorld()->GetActors().empty()) {
            int idx = std::rand() % Engine::GetCurrentContext().GetWorld()->GetActors().size();
            AActor* actor = Engine::GetCurrentContext().GetWorld()->GetActors()[idx].get();

            if (auto transform = actor->GetComponent<UTransformComponent>()) {
                glm::vec3 offset(
                    (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.0f,
                    (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.0f,
                    (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.0f
                );
                transform->SetPosition(transform->position + offset);
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

