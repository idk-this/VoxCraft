//
// Created by IDKTHIS on 12.10.2025.
//

#include "DebugOverlay.h"

#include "Core/UI/Core/XMLParser.h"

void DebugOverlay::Init()
{
    m_root = UISystem::XMLUIParser::ParseUIFile("Content/test_hud.xml", this);
}

void DebugOverlay::Render()
{
    if (m_root)
    {
        m_root->Render();
    }
}
