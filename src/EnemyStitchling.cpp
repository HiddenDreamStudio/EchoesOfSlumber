#include "EnemyStitchling.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
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
    idleTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/SP_Yoyo_Idle.png");
    alertTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/SP_Yoyo_Alerta.png");
    dieTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/SP_Yoyo_Desaparecer.png");

    int spawnX = (int)position.getX() + 16;
    int spawnY = (int)position.getY() + 16;

    pbody = Engine::GetInstance().physics->CreateCircleSensor(spawnX, spawnY, 16, bodyType::STATIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::ENEMY;

    trapSensor_ = Engine::GetInstance().physics->CreateRectangleSensor(spawnX, spawnY + 16, 120, 24, bodyType::STATIC);
    trapSensor_->listener = this;
    trapSensor_->ctype = ColliderType::UNKNOWN;

    playerRef_ = Engine::GetInstance().scene->player.get();

    idleAnims_.LoadFromTSX("assets/textures/animations/yoyoIdle.xml", { {0, "idle"} });
    alertAnims_.LoadFromTSX("assets/textures/animations/yoyoAlerta.xml", { {0, "alert"} });
    dieAnims_.LoadFromTSX("assets/textures/animations/yoyoDesaparecer.xml", { {0, "die"} });

    dieAnims_.SetLoop("die", false);

    idleAnims_.SetCurrent("idle");
    alertAnims_.SetCurrent("alert");
    dieAnims_.SetCurrent("die");

    attackFxId = Engine::GetInstance().audio->LoadFx("assets/audio/Enemies/Yoyo/attack_yoyo.wav");
    disappearFxId = Engine::GetInstance().audio->LoadFx("assets/audio/Enemies/Yoyo/desaparecer.wav");

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

    // ?? Death: play die animation then delete ????????????????????????????????
    if (currentState_ == State::DEATH) {
        dieAnims_.Update(dt);
        Draw(dt);
        if (dieAnims_.HasFinishedOnce("die")) {
            active = false;
            pendingToDelete = true;
        }
        return true;
    }

    switch (currentState_) {
    case State::IDLE:
        idleAnims_.Update(dt);
        break;
    case State::ALERT:
        alertAnims_.Update(dt);
        break;
    case State::TRAP_ACTIVE:
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

    if (idleTexture_)  Engine::GetInstance().textures->UnLoad(idleTexture_);
    if (alertTexture_) Engine::GetInstance().textures->UnLoad(alertTexture_);
    if (dieTexture_)   Engine::GetInstance().textures->UnLoad(dieTexture_);

    idleTexture_ = nullptr;
    alertTexture_ = nullptr;
    dieTexture_ = nullptr;

    return true;
}

// ?????????????????????????????????????????????????????????????????????????????
//  FSM
// ?????????????????????????????????????????????????????????????????????????????

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
            if (!disappearDone_) {
                // Wait for disappear animation to finish
                if (dieAnims_.HasFinishedOnce("die")) {
                    Engine::GetInstance().audio->PlayFxSpatial(disappearFxId, position);
                    disappearDone_ = true;
                    ApplyPlayerSlowdown();

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
            else {
                if (playerWasSlowed_) {
                    escapeTimer_ += dt;

                    // Player mashes E to escape
                    if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_E) == KEY_DOWN) {
                        escapeMashes_++;
                        if (escapeMashes_ >= MASHES_TO_ESCAPE) {
                            RemovePlayerSlowdown();
                            active = false;
                            pendingToDelete = true;
                            break;
                        }
                    }

                    // Time ran out — deal damage and release
                    if (escapeTimer_ >= ESCAPE_TIME_LIMIT) {
                        if (playerRef_->pbody && playerRef_->pbody->listener) {
                            playerRef_->pbody->listener->TakeDamage(1);
                        }
                        RemovePlayerSlowdown();
                        active = false;
                        pendingToDelete = true;
                    }
                }
                else {
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
        Engine::GetInstance().audio->PlayFxSpatial(attackFxId, position);
        dieAnims_.ResetCurrent();
        disappearDone_ = false;
        escapeMashes_ = 0;
        escapeTimer_ = 0.0f;
        break;
    case State::DEATH:
        dieAnims_.ResetCurrent();
        break;
    default:
        break;
    }
}

// ?????????????????????????????????????????????????????????????????????????????
//  Player slowdown helpers
// ?????????????????????????????????????????????????????????????????????????????

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

// ?????????????????????????????????????????????????????????????????????????????
//  Collision
// ?????????????????????????????????????????????????????????????????????????????

void EnemyStitchling::OnCollision(PhysBody* physA, PhysBody* physB) {
    if (currentState_ == State::DEATH) return;

    LOG("EnemyStitchling::OnCollision triggered: physA ctype = %d, physB ctype = %d", (int)physA->ctype, (int)physB->ctype);

    // Player attack hits the yoyo body directly
    if (physA == pbody || physB == pbody) {
        PhysBody* other = (physA == pbody) ? physB : physA;
        if (other->ctype == ColliderType::ATTACK || other->ctype == ColliderType::SLINGSHOT_PROJ) {
            TakeDamage(1);
            if (other->ctype == ColliderType::SLINGSHOT_PROJ && other->listener)
                other->listener->pendingToDelete = true;
            return;
        }
    }

    // Trap sensor: player steps on rope
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
    // No special logic needed
}

void EnemyStitchling::TakeDamage(int damage) {
    // Only damageable in IDLE or ALERT states
    if (currentState_ == State::TRAP_ACTIVE || currentState_ == State::DEATH) return;

    health -= damage;
    LOG("EnemyStitchling took %d damage -> health: %d/%d", damage, health, maxHealth);

    if (health <= 0) {
        health = 0;
        if (pbody != nullptr) {
            Engine::GetInstance().physics->DeletePhysBody(pbody);
            pbody = nullptr;
        }
        if (trapSensor_ != nullptr) {
            Engine::GetInstance().physics->DeletePhysBody(trapSensor_);
            trapSensor_ = nullptr;
        }
        currentState_ = State::DEATH;
        dieAnims_.ResetCurrent();
        Engine::GetInstance().audio->PlayFxSpatial(disappearFxId, position);
    }
}

// ?????????????????????????????????????????????????????????????????????????????
//  Draw
// ?????????????????????????????????????????????????????????????????????????????

void EnemyStitchling::Draw(float dt) {
    // Don't draw once the yoyo has disappeared via trap
    if (disappearDone_) return;

    SDL_Texture* currentTexture = nullptr;
    SDL_Rect     frameRect = { 0, 0, 0, 0 };

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
    case State::DEATH:
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
        int px = (int)position.getX() - drawW / 2;
        int py = (int)position.getY() + 16 - drawH;

        SDL_FlipMode flip = facingRight_ ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        Engine::GetInstance().render->DrawTexture(
            currentTexture, px, py, &frameRect,
            1.0f, -1.0f, 0, INT_MAX, INT_MAX, flip, scale
        );
    }
}

// ?????????????????????????????????????????????????????????????????????????????
//  Distance util
// ?????????????????????????????????????????????????????????????????????????????

float EnemyStitchling::GetDistanceToPlayer() const {
    if (!playerRef_ || !playerRef_->pbody || !pbody || playerRef_->IsHiding()) return 9999.0f;
    int px, py, sx, sy;
    playerRef_->pbody->GetPosition(px, py);
    pbody->GetPosition(sx, sy);
    float dx = (float)(sx - px);
    float dy = (float)(sy - py);
    return std::sqrt(dx * dx + dy * dy);
}