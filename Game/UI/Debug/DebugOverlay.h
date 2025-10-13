//
// Created by IDKTHIS on 12.10.2025.
//

#pragma once
#include <memory>

#include "Core/UI/Core/DataBinding.h"
#include "Core/UI/Core/UIElement.h"


class DebugOverlay : public UISystem::SimpleDataContext {

public:
    DebugOverlay() = default;
    void Init();
    void Render();

private:
    std::shared_ptr<UISystem::UIElement> m_root;
};
