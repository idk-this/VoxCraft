//
// Created by IDKTHIS on 02.09.2025.
//

#pragma once
#include "Application/Application.h"


class VoxCraftGame : public Engine::Application {
    public:
        VoxCraftGame();
        ~VoxCraftGame() override;
        void Init() override;
        void Update(float deltaTime) override;
        void Run() override;

    VoxPak voxCraftPak;

};
