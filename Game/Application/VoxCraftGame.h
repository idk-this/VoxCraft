//
// Created by IDKTHIS on 02.09.2025.
//

#pragma once
#include "Application/Application.h"
#include "Core/CVar/Console.h"
#include "Core/CVar/CVar.h"
#include "Core/ECS/Base/Structures/FObjectID.h"

struct FObjectID;
class AAChunkManager;
class DebugOverlay;

namespace UISystem
{
    class UIElement;
}

class AChunk;
class UWorldGenerator;


class VoxCraftGame : public Engine::Application {
    public:
        VoxCraftGame();
        ~VoxCraftGame() override;

        void TestSay2(const CommandArgs& args);
        void TestUpdated(const CVarValue& oldValue, const CVarValue& newValue);

        void Init() override;
        void Update(float deltaTime) override;
        void Run() override;

    VoxPak voxCraftPak;
    std::shared_ptr<DebugOverlay> m_debugOverlay;
    FObjectID m_chunkManager_id;
    float m_blockSize = 1.0f;
};
