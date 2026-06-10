#include "Lever.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "Input.h"
#include "EntityManager.h"
#include "Log.h"

Lever::Lever() : Entity(EntityType::LEVER) {
    name = "Lever";
}

Lever::~Lever() {}

bool Lever::Awake() { return true; }

bool Lever::Start() {
    texNormal = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Palanca_Normal.png");
    texActive = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Palanca_Active.png");

    pbody = Engine::GetInstance().physics->CreateRectangleSensor((int)position.getX(), (int)position.getY(), texW, texH, bodyType::KINEMATIC);
    pbody->ctype = ColliderType::UNKNOWN;
    pbody->listener = this;

    return true;
}

bool Lever::Update(float dt) {
    if (!active) return true;

    // 1. NORMAL LEVER (Player + E key)
    if (!isPressurePlate && playerInRange && Engine::GetInstance().input->GetKey(SDL_SCANCODE_E) == KEY_DOWN) {
        if (isLockedByPuzzle) {
            LOG("Lever is stuck. Puzzles not completed yet.");
            return true;
        }
        isActivated = !isActivated;

        if (targetPlatform == nullptr && !targetPlatformName.empty()) {
            for (auto& ent : Engine::GetInstance().entityManager->entities) {
                if (ent->type == EntityType::PLATFORM) {
                    auto plat = std::dynamic_pointer_cast<Platform>(ent);
                    if (plat && plat->platformName == targetPlatformName) { targetPlatform = plat; break; }
                }
            }
        }
        if (targetPlatform != nullptr) {
            targetPlatform->activatedByLever = isActivated;
            LOG("Lever toggled. Platform %s is %s", targetPlatformName.c_str(), isActivated ? "ON" : "OFF");
        }
    }

    // 2. PRESSURE PLATE
    if (isPressurePlate) {
        if (targetPlatform == nullptr && !targetPlatformName.empty()) {
            for (auto& ent : Engine::GetInstance().entityManager->entities) {
                if (ent->type == EntityType::PLATFORM) {
                    auto plat = std::dynamic_pointer_cast<Platform>(ent);
                    if (plat && plat->platformName == targetPlatformName) { targetPlatform = plat; break; }
                }
            }
        }
        if (targetPlatform != nullptr) targetPlatform->activatedByLever = isActivated;
    }

    // Draw appropriate texture
    SDL_Texture* currentTex = isActivated ? texActive : texNormal;
    if (currentTex) {
        int bx, by;
        pbody->GetPosition(bx, by);
        Engine::GetInstance().render->DrawTexture(currentTex, bx - texW / 2, by - texH / 2);
    }

    return true;
}

bool Lever::CleanUp() {
    if (texNormal) Engine::GetInstance().textures->UnLoad(texNormal);
    if (texActive) Engine::GetInstance().textures->UnLoad(texActive);
    if (pbody) Engine::GetInstance().physics->DeletePhysBody(pbody);
    return true;
}

void Lever::OnCollision(PhysBody* physA, PhysBody* physB) {
    if (physB->ctype == ColliderType::PLAYER) {
        playerInRange = true;
    }
    if (physB->ctype == ColliderType::PUSH_ROCK || physB->ctype == ColliderType::BOX) {
        objectsOnTop++;
        if (isPressurePlate && objectsOnTop > 0) isActivated = true;
    }
}

void Lever::OnCollisionEnd(PhysBody* physA, PhysBody* physB) {
    if (physB->ctype == ColliderType::PLAYER) {
        playerInRange = false;
    }
    if (physB->ctype == ColliderType::PUSH_ROCK || physB->ctype == ColliderType::BOX) {
        objectsOnTop--;
        if (isPressurePlate && objectsOnTop <= 0) {
            isActivated = false;
            objectsOnTop = 0;
        }
    }
}