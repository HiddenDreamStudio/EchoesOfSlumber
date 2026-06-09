#include "EnemyStitchling.h"
#include "Engine.h"
#include "Textures.h"
#include "Physics.h"
#include "Render.h"
#include "Player.h"
#include "Scene.h"
#include "Input.h"
#include "Log.h"

EnemyStitchling::EnemyStitchling() : Entity(EntityType::ENEMY_STITCHLING) {
    name = "EnemyStitchling";
    health = 3;
    maxHealth = 3;
}

EnemyStitchling::~EnemyStitchling() {}

bool EnemyStitchling::Awake() {
    return true;
}

bool EnemyStitchling::Start() {
    // Textures — new yoyo spritesheets
    idleTexture_   = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/SP_Yoyo_Idle.png");
    alertTexture_  = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/SP_Yoyo_Alerta.png");
    dieTexture_    = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/SP_Yoyo_Desaparecer.png");

    // Create main body at spawn position
    int spawnX = (int)position.getX() + 16;
    int spawnY = (int)position.getY() + 16;

    pbody = Engine::GetInstance().physics->CreateCircleSensor(spawnX, spawnY, 16, bodyType::STATIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::ENEMY;

    // Create trap sensor (wide rectangle on the ground, slightly below the body)
    trapSensor_ = Engine::GetInstance().physics->CreateRectangleSensor(spawnX, spawnY + 16, 120, 24, bodyType::STATIC);
    trapSensor_->listener = this;
    trapSensor_->ctype = ColliderType::UNKNOWN;

    playerRef_ = Engine::GetInstance().scene->player.get();

    // Load AnimationSets — new yoyo XMLs
    idleAnims_.LoadFromTSX("assets/textures/animations/yoyoIdle.xml", { {0, "idle"} });
    alertAnims_.LoadFromTSX("assets/textures/animations/yoyoAlerta.xml", { {0, "alert"} });
    dieAnims_.LoadFromTSX("assets/textures/animations/yoyoDesaparecer.xml", { {0, "die"} });

    dieAnims_.SetLoop("die", false);

    idleAnims_.SetCurrent("idle");
    alertAnims_.SetCurrent("alert");
    dieAnims_.SetCurrent("die");

    EnterState(State::IDLE);
    return true;
}

bool EnemyStitchling::Update(float dt) {
    if (!playerRef_ && Engine::GetInstance().scene->player) {
        playerRef_ = Engine::GetInstance().scene->player.get();
    }

    if (pbody != nullptr) {
        int x, y;
        pbody->GetPosition(x, y);
        position.setX((float)x);
        position.setY((float)y);
    }

    if (playerRef_ && currentState_ != State::DEATH) {
        int px, py;
        playerRef_->pbody->GetPosition(px, py);
        facingRight_ = (px > position.getX());
    }

    // Update active animations
    switch (currentState_) {
    case State::IDLE:
        idleAnims_.Update(dt);
        break;
    case State::ALERT:
        alertAnims_.Update(dt);
        break;
    case State::TRAP_ACTIVE:
        // Play desaparecer animation until it finishes
        if (!disappearDone_) {
            dieAnims_.Update(dt);
        }
        break;
    default:
        break;
    }

    UpdateFSM(dt);
    Draw(dt);

    return true;
}

bool EnemyStitchling::CleanUp() {
    if (pbody != nullptr) {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }
    if (trapSensor_ != nullptr) {
        Engine::GetInstance().physics->DeletePhysBody(trapSensor_);
        trapSensor_ = nullptr;
    }

    RemovePlayerSlowdown();

    if (idleTexture_) Engine::GetInstance().textures->UnLoad(idleTexture_);
    if (alertTexture_) Engine::GetInstance().textures->UnLoad(alertTexture_);
    if (dieTexture_) Engine::GetInstance().textures->UnLoad(dieTexture_);

    idleTexture_ = nullptr;
    alertTexture_ = nullptr;
    dieTexture_ = nullptr;

    return true;
}

void EnemyStitchling::UpdateFSM(float dt) {
    stateTimer_ += dt;

    switch (currentState_) {
    case State::IDLE:
        if (GetDistanceToPlayer() <= 200.0f) {
            EnterState(State::ALERT);
        }
        break;

    case State::ALERT:
        if (GetDistanceToPlayer() > 250.0f) {
            EnterState(State::IDLE);
        }
        break;

    case State::TRAP_ACTIVE:
        if (playerRef_) {
            // Phase 1: desaparecer animation is playing — yoyo is visually disappearing
            if (!disappearDone_) {
                if (dieAnims_.HasFinishedOnce("die")) {
                    // Animation finished — yoyo has disappeared, now throw ropes and trap the player
                    disappearDone_ = true;
                    ApplyPlayerSlowdown();

                    // Destroy physics bodies since the yoyo is now gone
                    if (pbody) {
                        Engine::GetInstance().physics->DeletePhysBody(pbody);
                        pbody = nullptr;
                    }
                    if (trapSensor_) {
                        Engine::GetInstance().physics->DeletePhysBody(trapSensor_);
                        trapSensor_ = nullptr;
                    }
                }
            }
            // Phase 2: yoyo gone, player is trapped by ropes — mash E to escape
            else {
                if (playerWasSlowed_) {
                    if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_E) == KEY_DOWN) {
                        escapeMashes_++;
                        if (escapeMashes_ >= MASHES_TO_ESCAPE) {
                            // Player escaped the ropes!
                            RemovePlayerSlowdown();
                            active = false;
                            pendingToDelete = true;
                        }
                    }
                } else {
                    // Player already freed (e.g. by external means) — just clean up
                    active = false;
                    pendingToDelete = true;
                }
            }
        }
        break;

    default:
        break;
    }
}

void EnemyStitchling::EnterState(State newState) {
    LOG("EnemyStitchling::EnterState: transitioning from %d to %d", (int)currentState_, (int)newState);
    currentState_ = newState;
    stateTimer_ = 0.0f;

    switch (currentState_) {
    case State::IDLE:
        idleAnims_.ResetCurrent();
        break;
    case State::ALERT:
        alertAnims_.ResetCurrent();
        break;
    case State::TRAP_ACTIVE:
        dieAnims_.ResetCurrent();
        disappearDone_ = false;
        escapeMashes_ = 0;
        break;
    default:
        break;
    }
}

void EnemyStitchling::ApplyPlayerSlowdown() {
    if (playerRef_ && !playerWasSlowed_) {
        playerRef_->speed = 1.0f;
        playerRef_->isYoyoTrapped_ = true;
        Engine::GetInstance().physics->SetLinearVelocity(playerRef_->pbody, 0.0f, 0.0f);
        playerRef_->yoyoTrapAnims_.ResetCurrent();
        playerWasSlowed_ = true;
    }
}

void EnemyStitchling::RemovePlayerSlowdown() {
    if (playerRef_ && playerWasSlowed_) {
        playerRef_->speed = 4.0f;
        playerRef_->isYoyoTrapped_ = false;
        playerWasSlowed_ = false;
    }
}

void EnemyStitchling::OnCollision(PhysBody* physA, PhysBody* physB) {
    LOG("EnemyStitchling::OnCollision triggered: physA ctype = %d, physB ctype = %d", (int)physA->ctype, (int)physB->ctype);
    if (physA == trapSensor_ || physB == trapSensor_) {
        PhysBody* other = (physA == trapSensor_) ? physB : physA;
        LOG("Collision with trapSensor_! other ctype = %d, currentState_ = %d", (int)other->ctype, (int)currentState_);
        if (other->ctype == ColliderType::PLAYER && (currentState_ == State::IDLE || currentState_ == State::ALERT)) {
            LOG("Player stepped on rope! Yoyo starts disappearing.");
            EnterState(State::TRAP_ACTIVE);
        }
    }
}

void EnemyStitchling::OnCollisionEnd(PhysBody* physA, PhysBody* physB) {
    // No longer release on collision end — the trap is permanent until the player escapes by mashing E
}

void EnemyStitchling::TakeDamage(int damage) {
    // Yoyo cannot be damaged — it only disappears when the player steps on the rope
}

void EnemyStitchling::Draw(float dt) {
    // Don't draw anything once the yoyo has disappeared
    if (disappearDone_) return;

    SDL_Texture* currentTexture = nullptr;
    SDL_Rect frameRect = { 0, 0, 0, 0 };

    switch (currentState_) {
    case State::IDLE:
        currentTexture = idleTexture_;
        frameRect = idleAnims_.GetCurrentFrame();
        break;
    case State::ALERT:
        currentTexture = alertTexture_;
        frameRect = alertAnims_.GetCurrentFrame();
        break;
    case State::TRAP_ACTIVE:
        currentTexture = dieTexture_;
        frameRect = dieAnims_.GetCurrentFrame();
        break;
    default:
        break;
    }

    if (currentTexture && frameRect.w > 0 && frameRect.h > 0) {
        float scale = 1.10f;

        int drawW = (int)(frameRect.w * scale);
        int drawH = (int)(frameRect.h * scale);

        // Center horizontally on the body, anchor bottom of drawn sprite to body bottom (body center + 16px radius)
        int px = (int)position.getX() - drawW / 2;
        int py = (int)position.getY() + 16 - drawH;

        SDL_FlipMode flip = facingRight_ ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

        Engine::GetInstance().render->DrawTexture(currentTexture, px, py, &frameRect, 1.0f, 0, INT_MAX, INT_MAX, flip, scale);
    }
}

float EnemyStitchling::GetDistanceToPlayer() const {
    if (!playerRef_ || !playerRef_->pbody || !pbody) return 9999.0f;
    int px, py;
    playerRef_->pbody->GetPosition(px, py);
    int sx, sy;
    pbody->GetPosition(sx, sy);
    float dx = (float)(sx - px);
    float dy = (float)(sy - py);
    return std::sqrt(dx * dx + dy * dy);
}
