//
// Created by IDKTHIS on 23.09.2025.
//

#include "AVoxCraftPlayerController.h"

#include "Application/Application.h"
#include "Core/ECS/Base/UWorld.h"
#include "Core/ECS/Components/UCameraComponent.h"
#include "Core/Physics/Components/UPhysicComponent.h"
#include "Platform/Window/IWindow.h"
#include "Platform/Window/Components/WindowInputComponent.h"
#include "Platform/Window/Components/Keys.h"

void AVoxCraftPlayerController::Update(float deltaTime)
{
    APlayerController::Update(deltaTime);
    IWindow* window = Engine::GetCurrentContext().GetWindow();
    if (!window->IsRelativeMouseMode()) return;

    auto* transform = m_pawn->GetComponent<UTransformComponent>();
    auto* physic = m_pawn->GetComponent<UPhysicComponent>();
    if (!transform) return;

    float moveSpeed = (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_LEFT_SHIFT) ? Speed * 2.5f : Speed);

    if (window->GetInputComponent()->IsKeyPressed(KeyCode::KEY_I))
        Speed *= 2.5f;
    if (window->GetInputComponent()->IsKeyPressed(KeyCode::KEY_P))
        Speed = 3.5f;
    if (window->GetInputComponent()->IsKeyPressed(KeyCode::KEY_Z))
        transform->position.y = 23.5f;
    if (window->GetInputComponent()->IsKeyPressed(KeyCode::KEY_L))
        m_pawn->AddComponent(std::make_shared<UPhysicComponent>());
    if (window->GetInputComponent()->IsKeyPressed(KeyCode::KEY_X))
        transform->position.x = 999 * 16;

    auto* camera = m_pawn->GetComponent<UCameraComponent>();

    float sensitivity = 0.52f;
    camera->AddYawPitch(
        -window->GetInputComponent()->GetMouseState().deltaX * sensitivity,
        -window->GetInputComponent()->GetMouseState().deltaY * sensitivity
    );

    camera->RelativeRotation.z = 0.0f;
    transform->rotation = glm::angleAxis(glm::radians(camera->RelativeRotation.y), glm::vec3(0, 1, 0));
    glm::vec3 moveDirection{0.0f};
    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_SPACE))
    {
        glm::vec3 force = camera->GetUpVector();
        force *= 20;
        if (physic)
        {
            m_pawn->GetComponent<UPhysicComponent>()->AddForce(force);
        }
    }
    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_W))
        moveDirection += camera->GetForwardVector();
    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_S))
        moveDirection -= camera->GetForwardVector();
    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_D))
        moveDirection += camera->GetRightVector();
    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_A))
        moveDirection -= camera->GetRightVector();
    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_SPACE))
    {
        //moveDirection += camera->GetUpVector();
    }
    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_LEFT_CONTROL))
    {
        moveDirection -= camera->GetUpVector();
    }
    if (physic)
    {
        physic->SetVelocity(moveDirection * moveSpeed * deltaTime);
    }else
        transform->Move(moveDirection * moveSpeed * deltaTime);

}