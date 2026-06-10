#include "PuzzleManager2.h"
#include "Platform.h"
#include "Lever.h"
#include "PushRock.h"
#include "Engine.h"
#include "Textures.h"
#include "Physics.h" 
#include "Log.h"

PuzzleManager2::PuzzleManager2() {}

PuzzleManager2::~PuzzleManager2() {}

void PuzzleManager2::Init(SDL_Renderer* renderer) {
    renderer_ = renderer;
    Reset();
}

void PuzzleManager2::Reset() {
    puzzleActive_ = true;
    portalOpen_ = false;

    phase1Completed = false;
    phase2Completed = false;
    phase3LeftCompleted = false;
    phase3RightCompleted = false;

    btnPhase1 = nullptr;
    btnPhase2 = nullptr;
    platElevator = nullptr;
    platButton = nullptr;
    btnLeftPhase3 = nullptr;
    btnRightPhase3 = nullptr;
    dropBoxLeft = nullptr;
    dropBoxRight = nullptr;
    btnStaticLeft = nullptr;
    btnStaticRight = nullptr;
    finalBlocker = nullptr;
}

std::shared_ptr<Entity> PuzzleManager2::FindEntityByName(const std::list<std::shared_ptr<Entity>>& entities, const std::string& name) {
    for (const auto& e : entities) {
        if (e->name == name) {
            return e;
        }
    }
    return nullptr;
}

void PuzzleManager2::ReplaceLeverTexture(std::shared_ptr<Lever> lever) {
    if (!lever) return;

    if (lever->texNormal) Engine::GetInstance().textures->UnLoad(lever->texNormal);
    if (lever->texActive) Engine::GetInstance().textures->UnLoad(lever->texActive);

    lever->texNormal = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Boton_Normal.png");
    lever->texActive = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Boton_Pressed.png");
}

void PuzzleManager2::Update(float dt, const std::list<std::shared_ptr<Entity>>& entities) {
    if (!puzzleActive_) return;

    // --- FIND AND INIT ENTITIES ---
    if (!btnPhase1) {
        btnPhase1 = std::static_pointer_cast<Lever>(FindEntityByName(entities, "btn_phase1"));
        if (btnPhase1) {
            ReplaceLeverTexture(btnPhase1);
            if (btnPhase1->pbody) {
                b2Body_SetType(btnPhase1->pbody->body, b2_kinematicBody);
            }
            btnPhase1->isPressurePlate = true;
            LOG("PuzzleManager2: btn_phase1 loaded.");
        }
    }

    if (!btnPhase2) {
        btnPhase2 = std::static_pointer_cast<Lever>(FindEntityByName(entities, "btn_phase2"));
        if (btnPhase2) {
            ReplaceLeverTexture(btnPhase2);
            btnPhase2->isPressurePlate = true;
            LOG("PuzzleManager2: btn_phase2 loaded.");
        }
    }

    // El ascensor (Lejano)
    if (!platElevator) {
        platElevator = std::static_pointer_cast<Platform>(FindEntityByName(entities, "plat_elevator"));
        if (platElevator && !phase2Completed) {
            platElevator->requireLever = true;
            platElevator->activatedByLever = false;
            LOG("PuzzleManager2: plat_elevator loaded and locked.");
        }
    }

    // La plataforma independiente bajo el botón
    if (!platButton) {
        platButton = std::static_pointer_cast<Platform>(FindEntityByName(entities, "plat_button"));
        if (platButton) {
            LOG("PuzzleManager2: plat_button loaded.");
        }
    }

    if (!btnLeftPhase3) {
        btnLeftPhase3 = std::static_pointer_cast<Lever>(FindEntityByName(entities, "btn_left_phase3"));
        if (btnLeftPhase3) {
            ReplaceLeverTexture(btnLeftPhase3);
            btnLeftPhase3->isPressurePlate = true;
        }
    }

    if (!btnRightPhase3) {
        btnRightPhase3 = std::static_pointer_cast<Lever>(FindEntityByName(entities, "btn_right_phase3"));
        if (btnRightPhase3) {
            ReplaceLeverTexture(btnRightPhase3);
            btnRightPhase3->isPressurePlate = true;
        }
    }

    if (!dropBoxLeft) {
        auto e = FindEntityByName(entities, "drop_box_left");
        if (e) {
            dropBoxLeft = std::static_pointer_cast<PushRock>(e);
            dropBoxLeft->Disable();
        }
    }

    if (!dropBoxRight) {
        auto e = FindEntityByName(entities, "drop_box_right");
        if (e) {
            dropBoxRight = std::static_pointer_cast<PushRock>(e);
            dropBoxRight->Disable();
        }
    }

    if (!btnStaticLeft) {
        btnStaticLeft = std::static_pointer_cast<Lever>(FindEntityByName(entities, "btn_static_left"));
        if (btnStaticLeft) {
            ReplaceLeverTexture(btnStaticLeft);
            btnStaticLeft->isPressurePlate = true;
        }
    }

    if (!btnStaticRight) {
        btnStaticRight = std::static_pointer_cast<Lever>(FindEntityByName(entities, "btn_static_right"));
        if (btnStaticRight) {
            ReplaceLeverTexture(btnStaticRight);
            btnStaticRight->isPressurePlate = true;
        }
    }

    if (!finalBlocker) finalBlocker = FindEntityByName(entities, "final_blocker");


    // --- LOGIC ---

    // Movemos btnPhase1 junto a su plataforma específica (plat_button)
    if (btnPhase1 && platButton && platButton->pbody) {
        int platX, platY;
        platButton->pbody->GetPosition(platX, platY);

        int newX = platX + 64;
        int newY = platY - 64;

        if (btnPhase1->pbody) {
            btnPhase1->pbody->SetPosition(newX, newY);
        }
    }

    // Phase 1 & 2: Ambos botones deben estar presionados a la vez para activar el ascensor
    if (!phase2Completed && btnPhase1 && btnPhase2 && platElevator) {
        if (btnPhase1->isActivated && btnPhase2->isActivated) {
            platElevator->requireLever = false;
            platElevator->activatedByLever = true;
            phase1Completed = true;
            phase2Completed = true;
            LOG("PuzzleManager2: Phase 2 completed! Elevator activated.");
        }
        else {
            platElevator->requireLever = true;
            platElevator->activatedByLever = false;
        }
    }

    if (phase2Completed) {
        if (!phase3LeftCompleted && btnLeftPhase3 && btnLeftPhase3->isActivated) {
            if (dropBoxLeft && !dropBoxLeft->active) {
                dropBoxLeft->Enable();
            }
            if (btnStaticLeft && btnStaticLeft->isActivated) {
                phase3LeftCompleted = true;
            }
        }

        if (!phase3RightCompleted && btnRightPhase3 && btnRightPhase3->isActivated) {
            if (dropBoxRight && !dropBoxRight->active) {
                dropBoxRight->Enable();
            }
            if (btnStaticRight && btnStaticRight->isActivated) {
                phase3RightCompleted = true;
            }
        }
    }

    if (!portalOpen_ && phase3LeftCompleted && phase3RightCompleted) {
        portalOpen_ = true;
        puzzleActive_ = false;

        if (finalBlocker) {
            finalBlocker->Disable();
        }
    }
}

void PuzzleManager2::Render(SDL_Renderer* renderer, float cameraX, float cameraY) {
}