//
// Created by IDKTHIS on 23.09.2025.
//

#include "AVoxCraftPlayerController.h"

#include "Application/Application.h"
#include "Core/ECS/Base/UWorld.h"
#include "Core/ECS/Components/UCameraComponent.h"
#include "Platform/Window/IWindow.h"
#include "Platform/Window/Components/WindowInputComponent.h"
#include "Platform/Window/Components/Keys.h"

void AVoxCraftPlayerController::Update(float deltaTime)
{
    APlayerController::Update(deltaTime);
    IWindow* window = Engine::GetCurrentContext().GetWindow();
    if (!window->IsRelativeMouseMode()) return;
    float moveSpeed = (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_LEFT_SHIFT) ? 8.0f : 3.0f) * deltaTime;
    auto* transform = m_pawn->GetComponent<UTransformComponent>();

    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_W))
        transform->Move(transform->GetForwardVector() * moveSpeed);

    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_S))
        transform->Move(-transform->GetForwardVector() * moveSpeed);

    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_D))
        transform->Move(transform->GetRightVector() * moveSpeed);

    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_A))
        transform->Move(-transform->GetRightVector() * moveSpeed);

    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_SPACE))
        transform->Move(transform->GetUpVector() * moveSpeed);

    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_LEFT_CONTROL))
        transform->Move(-transform->GetUpVector() * moveSpeed);

    float sensitivity = 0.52f;
    auto* camera = m_pawn->GetComponent<UCameraComponent>();

    camera->yaw += window->GetInputComponent()->GetMouseState().deltaX * sensitivity;
    camera->pitch -= window->GetInputComponent()->GetMouseState().deltaY * sensitivity;

    // Обновляем rotation трансформа
    m_pawn->GetComponent<UTransformComponent>()->SetRotationYawPitch(camera->yaw, camera->pitch);

}
