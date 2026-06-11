#include "EnemyWindUpScurry.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Physics.h"
#include "Render.h"
#include "Player.h"
#include "Scene.h"
#include "Log.h"
#include <cstdlib>
#include <cmath>

EnemyWindUpScurry::EnemyWindUpScurry() : Entity(EntityType::ENEMY_WINDUP_SCURRY) {
    name = "EnemyWindUpScurry";
    health = 3;
    maxHealth = 3;
}

EnemyWindUpScurry::~EnemyWindUpScurry() {}

bool EnemyWindUpScurry::Awake() {
    return true;
}

bool EnemyWindUpScurry::Start() {
    // Load textures
    idleTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Wind_Up_Scurry_Idle.png");
    alertaTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Wind_Up_Scurry_Alerta.png");
    antesWalkFastTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet _WindUpScurry_antesdewalkfast.png");
    walkFastTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/SP_WindUpScurry_Walk_Fast.png");
    cansadoTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Wind_Up_Scurry_Cansado.png");
    idleCansadoTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Wind_Up_Idle_Cansado.png");
    hitTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_WindUpScurry_Hit.png");
    dieTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_WindUpScurry_Die.png");

    // Create physics body
    int spawnX = (int)position.getX() + 16;
    int spawnY = (int)position.getY() + 16;

    pbody = Engine::GetInstance().physics->CreateCapsule(spawnX, spawnY, 16, 32, bodyType::DYNAMIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::ENEMY;

    playerRef_ = Engine::GetInstance().scene->player.get();

    // Load animation sets
    idleAnims_.LoadFromTSX("assets/textures/animations/windUpScurryIdle.xml", { {0, "idle"} });
    alertaAnims_.LoadFromTSX("assets/textures/animations/windUpScurryAlerta.xml", { {0, "alerta"} });
    antesWalkFastAnims_.LoadFromTSX("assets/textures/animations/windUpScurryAntesWalkFast.xml", { {0, "antesWalkFast"} });
    walkFastAnims_.LoadFromTSX("assets/textures/animations/windUpScurryWalkFast.xml", { {0, "walkFast"} });
    cansadoAnims_.LoadFromTSX("assets/textures/animations/windUpScurryCansado.xml", { {0, "cansado"} });
    idleCansadoAnims_.LoadFromTSX("assets/textures/animations/windUpScurryIdleCansado.xml", { {0, "idleCansado"} });
    hitAnims_.LoadFromTSX("assets/textures/animations/windUpScurryHit.xml", { {0, "hit"} });
    dieAnims_.LoadFromTSX("assets/textures/animations/windUpScurryDie.xml", { {0, "die"} });

    // Configure non-looping animations
    alertaAnims_.SetLoop("alerta", false);
    antesWalkFastAnims_.SetLoop("antesWalkFast", false);
    cansadoAnims_.SetLoop("cansado", false);
    hitAnims_.SetLoop("hit", false);
    dieAnims_.SetLoop("die", false);

    // Set current clips
    idleAnims_.SetCurrent("idle");
    alertaAnims_.SetCurrent("alerta");
    antesWalkFastAnims_.SetCurrent("antesWalkFast");
    walkFastAnims_.SetCurrent("walkFast");
    cansadoAnims_.SetCurrent("cansado");
    idleCansadoAnims_.SetCurrent("idleCansado");
    hitAnims_.SetCurrent("hit");
    dieAnims_.SetCurrent("die");

    // Start in idle state with a random direction
    ChooseNewPatrolDirection();

    attackFxId = Engine::GetInstance().audio->LoadFx("assets/audio/Enemies/WindUpScurry/ataques.wav");
    deadFxId = Engine::GetInstance().audio->LoadFx("assets/audio/Enemies/WindUpScurry/dead.wav");

    EnterState(State::IDLE);

    return true;
}

bool EnemyWindUpScurry::Update(float dt) {
    playerRef_ = Engine::GetInstance().scene->player.get();

    // Tick attack cooldown — prevents spamming damage/audio every physics frame
    if (attackCooldown_ > 0.0f) {
        attackCooldown_ -= dt;
    }

    // Update position from physics
    if (pbody != nullptr) {
        int x, y;
        pbody->GetPosition(x, y);
        position.setX((float)x);
        position.setY((float)y);
    }

    // Update facing direction based on movement
    if (currentState_ == State::IDLE) {
        facingRight_ = (patrolDirX_ > 0.0f);
    }
    else if (currentState_ == State::WALK_FAST && playerRef_) {
        int px, py;
        playerRef_->pbody->GetPosition(px, py);
        facingRight_ = (px > position.getX());
    }

    // Handle death state separately
    if (currentState_ == State::DEATH) {
        dieAnims_.Update(dt);
        Draw(dt);
        if (dieAnims_.HasFinishedOnce("die")) {
            active = false;
            pendingToDelete = true;
        }
        return true;
    }

    // Update active animations
    switch (currentState_) {
    case State::IDLE:
        idleAnims_.Update(dt);
        break;
    case State::ALERTA:
        alertaAnims_.Update(dt);
        break;
    case State::ANTES_WALK_FAST:
        antesWalkFastAnims_.Update(dt);
        break;
    case State::WALK_FAST:
        walkFastAnims_.Update(dt);
        break;
    case State::CANSADO:
        cansadoAnims_.Update(dt);
        break;
    case State::IDLE_CANSADO:
        idleCansadoAnims_.Update(dt);
        break;
    case State::HIT:
        hitAnims_.Update(dt);
        break;
    default:
        break;
    }

    UpdateFSM(dt);
    Draw(dt);

    return true;
}

bool EnemyWindUpScurry::CleanUp() {
    if (pbody != nullptr) {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }

    if (idleTexture_)          Engine::GetInstance().textures->UnLoad(idleTexture_);
    if (alertaTexture_)        Engine::GetInstance().textures->UnLoad(alertaTexture_);
    if (antesWalkFastTexture_) Engine::GetInstance().textures->UnLoad(antesWalkFastTexture_);
    if (walkFastTexture_)      Engine::GetInstance().textures->UnLoad(walkFastTexture_);
    if (cansadoTexture_)       Engine::GetInstance().textures->UnLoad(cansadoTexture_);
    if (idleCansadoTexture_)   Engine::GetInstance().textures->UnLoad(idleCansadoTexture_);
    if (hitTexture_)           Engine::GetInstance().textures->UnLoad(hitTexture_);
    if (dieTexture_)           Engine::GetInstance().textures->UnLoad(dieTexture_);

    idleTexture_ = nullptr;
    alertaTexture_ = nullptr;
    antesWalkFastTexture_ = nullptr;
    walkFastTexture_ = nullptr;
    cansadoTexture_ = nullptr;
    idleCansadoTexture_ = nullptr;
    hitTexture_ = nullptr;
    dieTexture_ = nullptr;

    return true;
}

void EnemyWindUpScurry::UpdateFSM(float dt) {
    stateTimer_ += dt;

    switch (currentState_) {
    case State::IDLE:
    {
        // Random patrol movement
        patrolTimer_ += dt;
        if (patrolTimer_ >= PATROL_CHANGE_INTERVAL) {
            ChooseNewPatrolDirection();
            patrolTimer_ = 0.0f;
        }

        // Apply patrol movement
        if (pbody) {
            Engine::GetInstance().physics->SetXVelocity(pbody, patrolDirX_ * PATROL_SPEED);
        }

        // Check for player detection
        if (GetDistanceToPlayer() <= DETECTION_RADIUS) {
            EnterState(State::ALERTA);
        }
        break;
    }

    case State::ALERTA:
        // Stop moving during alert
        if (pbody) {
            Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
        }

        // When alert animation finishes, transition to pre-chase
        if (alertaAnims_.HasFinishedOnce("alerta")) {
            EnterState(State::ANTES_WALK_FAST);
        }
        break;

    case State::ANTES_WALK_FAST:
        // Stop moving during transition
        if (pbody) {
            Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
        }

        // When transition animation finishes, start chasing
        if (antesWalkFastAnims_.HasFinishedOnce("antesWalkFast")) {
            EnterState(State::WALK_FAST);
        }
        break;

    case State::WALK_FAST:
    {
        // Chase the player
        if (playerRef_ && playerRef_->pbody) {
            int px, py;
            playerRef_->pbody->GetPosition(px, py);
            float dx = (float)px - position.getX();
            float dirX = (dx > 0.0f) ? 1.0f : -1.0f;

            if (pbody) {
                Engine::GetInstance().physics->SetXVelocity(pbody, dirX * CHASE_SPEED);
            }
        }

        // After CHASE_DURATION ms, enter fatigue
        if (stateTimer_ >= CHASE_DURATION) {
            EnterState(State::CANSADO);
        }
        break;
    }

    case State::CANSADO:
        // Slow down to a stop
        if (pbody) {
            Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
        }

        // When winding-down animation finishes, enter exhausted idle
        if (cansadoAnims_.HasFinishedOnce("cansado")) {
            EnterState(State::IDLE_CANSADO);
        }
        break;

    case State::IDLE_CANSADO:
        // Immobile - vulnerable to attacks
        if (pbody) {
            Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
        }

        // After FATIGUE_DURATION ms, recover and go back to idle
        if (stateTimer_ >= FATIGUE_DURATION) {
            EnterState(State::IDLE);
        }
        break;

    case State::HIT:
        if (pbody) {
            Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
        }

        if (hitAnims_.HasFinishedOnce("hit")) {
            if (health <= 0) {
                EnterState(State::DEATH);
            }
            else {
                EnterState(State::IDLE_CANSADO);
            }
        }
        break;

    default:
        break;
    }
}

void EnemyWindUpScurry::EnterState(State newState) {
    LOG("EnemyWindUpScurry::EnterState: transitioning from %d to %d", (int)currentState_, (int)newState);
    currentState_ = newState;
    stateTimer_ = 0.0f;

    switch (currentState_) {
    case State::IDLE:
        idleAnims_.ResetCurrent();
        ChooseNewPatrolDirection();
        patrolTimer_ = 0.0f;
        break;
    case State::ALERTA:
        alertaAnims_.ResetCurrent();
        break;
    case State::ANTES_WALK_FAST:
        antesWalkFastAnims_.ResetCurrent();
        break;
    case State::WALK_FAST:
        walkFastAnims_.ResetCurrent();
        break;
    case State::CANSADO:
        cansadoAnims_.ResetCurrent();
        break;
    case State::IDLE_CANSADO:
        idleCansadoAnims_.ResetCurrent();
        break;
    case State::HIT:
        hitAnims_.ResetCurrent();
        break;
    case State::DEATH:
        Engine::GetInstance().audio->PlayFxSpatial(deadFxId, position);
        dieAnims_.ResetCurrent();
        if (pbody) {
            Engine::GetInstance().physics->DeletePhysBody(pbody);
            pbody = nullptr;
        }
        break;
    }
}

void EnemyWindUpScurry::ChooseNewPatrolDirection() {
    patrolDirX_ = (rand() % 2 == 0) ? 1.0f : -1.0f;
}

void EnemyWindUpScurry::OnCollision(PhysBody* physA, PhysBody* physB) {
    if (currentState_ == State::DEATH) return;

    if (physA == pbody || physB == pbody) {
        PhysBody* other = (physA == pbody) ? physB : physA;

        // Contact with player deals damage — cooldown prevents spam every physics frame
        if (other->ctype == ColliderType::PLAYER) {
            if ((currentState_ == State::WALK_FAST || currentState_ == State::IDLE)
                && attackCooldown_ <= 0.0f)
            {
                if (other->listener != nullptr) {
                    Engine::GetInstance().audio->PlayFxSpatial(attackFxId, position);
                    other->listener->TakeDamage(1);
                    attackCooldown_ = ATTACK_COOLDOWN;
                }
            }
        }

        // Player attacks this enemy
        if (other->ctype == ColliderType::ATTACK) {
            TakeDamage(1);
        }
        else if (other->ctype == ColliderType::SLINGSHOT_PROJ) {
            TakeDamage(1);
            if (other->listener != nullptr) {
                other->listener->Destroy();
            }
        }

        // Reverse direction on wall collision while patrolling
        if (other->ctype == ColliderType::PLATFORM && currentState_ == State::IDLE) {
            patrolDirX_ = -patrolDirX_;
        }
    }
}

void EnemyWindUpScurry::OnCollisionEnd(PhysBody* physA, PhysBody* physB) {
    // No special collision end logic needed
}

void EnemyWindUpScurry::TakeDamage(int damage) {
    if (currentState_ == State::DEATH || currentState_ == State::HIT) return;

    // Can ONLY take damage during the fatigue state (IDLE_CANSADO)
    if (currentState_ == State::IDLE_CANSADO) {
        health -= damage;
        LOG("WindUpScurry took %d damage -> health: %d/%d", damage, health, maxHealth);
        EnterState(State::HIT);
    }
    else {
        LOG("WindUpScurry is immune right now! (state: %d) — must wait for IDLE_CANSADO", (int)currentState_);
    }
}

void EnemyWindUpScurry::Draw(float dt) {
    SDL_Texture* currentTexture = nullptr;
    SDL_Rect     frameRect = { 0, 0, 0, 0 };

    switch (currentState_) {
    case State::IDLE:
        currentTexture = idleTexture_;
        frameRect = idleAnims_.GetCurrentFrame();
        break;
    case State::ALERTA:
        currentTexture = alertaTexture_;
        frameRect = alertaAnims_.GetCurrentFrame();
        break;
    case State::ANTES_WALK_FAST:
        currentTexture = antesWalkFastTexture_;
        frameRect = antesWalkFastAnims_.GetCurrentFrame();
        break;
    case State::WALK_FAST:
        currentTexture = walkFastTexture_;
        frameRect = walkFastAnims_.GetCurrentFrame();
        break;
    case State::CANSADO:
        currentTexture = cansadoTexture_;
        frameRect = cansadoAnims_.GetCurrentFrame();
        break;
    case State::IDLE_CANSADO:
        currentTexture = idleCansadoTexture_;
        frameRect = idleCansadoAnims_.GetCurrentFrame();
        break;
    case State::HIT:
        currentTexture = hitTexture_;
        frameRect = hitAnims_.GetCurrentFrame();
        break;
    case State::DEATH:
        currentTexture = dieTexture_;
        frameRect = dieAnims_.GetCurrentFrame();
        break;
    }

    if (currentTexture && frameRect.w > 0 && frameRect.h > 0) {
        float scale = 0.5f;
        switch (currentState_) {
        case State::IDLE:
            scale = 1.6f;
            break;
        case State::ALERTA:
            scale = 1.0f;
            break;
        case State::CANSADO:
            scale = 1.1f;
            break;
        case State::IDLE_CANSADO:
            scale = 1.0f;
            break;
        case State::DEATH:
            scale = 1.0f;
            break;
        default:
            scale = 0.5f;
            break;
        }

        int drawW = (int)(frameRect.w * scale);
        int drawH = (int)(frameRect.h * scale);

        // Center horizontally on the body, anchor bottom of drawn sprite to body bottom
        int px = (int)position.getX() - drawW / 2;
        int py = (int)position.getY() + 16 - drawH;

        SDL_FlipMode flip = facingRight_ ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

        Engine::GetInstance().render->DrawTexture(currentTexture, px, py, &frameRect, 1.0f, -1.0f, 0, INT_MAX, INT_MAX, flip, scale);
    }
}

float EnemyWindUpScurry::GetDistanceToPlayer() const {
    if (!playerRef_ || !playerRef_->pbody || !pbody || playerRef_->IsHiding()) return 9999.0f;
    int px, py;
    playerRef_->pbody->GetPosition(px, py);
    int sx, sy;
    pbody->GetPosition(sx, sy);
    float dx = (float)(sx - px);
    float dy = (float)(sy - py);
    return std::sqrt(dx * dx + dy * dy);
}