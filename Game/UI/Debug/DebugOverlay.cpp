//
// Created by IDKTHIS on 12.10.2025.
//

#include "DebugOverlay.h"

#include "Application/Application.h"
#include "Core/UI/Core/XMLParser.h"
#include "Core/GameInfo.h"

void DebugOverlay::Init()
{
    m_root = UISystem::XMLUIParser::ParseUI(Engine::GetCurrentContext().GetPak("VoxCraftRes")->ReadFileWithOverrideString("UI/debug_overlay.xml"), this);

    SetProperty("GameBuildDate", "Build Time: " VOXCRAFT_BUILD_DATE);
    SetProperty("GameBuildType", "Build: " VOXCRAFT_BUILD_TYPE " (build: " VOXCRAFT_VERSION_FULL_STR ")");
}

void DebugOverlay::Render()
{
    if (m_root)
    {
        m_root->Render();
    }
}
