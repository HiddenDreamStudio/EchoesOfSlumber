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
    // ?? Textures ?????????????????????????????????????????????????????????????
    idleTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Wind_Up_Scurry_Idle.png");
    alertaTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Wind_Up_Scurry_Alerta.png");
    antesWalkFastTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet _WindUpScurry_antesdewalkfast.png");
    walkFastTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/SP_WindUpScurry_Walk_Fast.png");
    
    biteTexture_ = walkFastTexture_;   // placeholder visual
    biteRetreatTexture_ = walkFastTexture_;   // placeholder visual
    cansadoTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Wind_Up_Scurry_Cansado.png");
    idleCansadoTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Wind_Up_Idle_Cansado.png");
    hitTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_WindUpScurry_Hit.png");
    dieTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_WindUpScurry_Die.png");

    // ?? Physics: BOX collider (more accurate than capsule for a small toy) ???
    int spawnX = (int)position.getX() + 16;
    int spawnY = (int)position.getY() + 16;
    pbody = Engine::GetInstance().physics->CreateRectangle(spawnX, spawnY, 22, 28, bodyType::DYNAMIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::ENEMY;

    playerRef_ = Engine::GetInstance().scene->player.get();

    // ?? Animation sets ???????????????????????????????????????????????????????
    idleAnims_.LoadFromTSX("assets/textures/animations/windUpScurryIdle.xml", { {0, "idle"} });
    alertaAnims_.LoadFromTSX("assets/textures/animations/windUpScurryAlerta.xml", { {0, "alerta"} });
    antesWalkFastAnims_.LoadFromTSX("assets/textures/animations/windUpScurryAntesWalkFast.xml", { {0, "antesWalkFast"} });
    walkFastAnims_.LoadFromTSX("assets/textures/animations/windUpScurryWalkFast.xml", { {0, "walkFast"} });
    // DESPUÉS — usa los XMLs que SÍ existen como placeholder
    biteAnims_.LoadFromTSX("assets/textures/animations/windUpScurryWalkFast.xml", { {0, "bite"} });
    biteRetreatAnims_.LoadFromTSX("assets/textures/animations/windUpScurryWalkFast.xml", { {0, "biteRetreat"} });
    cansadoAnims_.LoadFromTSX("assets/textures/animations/windUpScurryCansado.xml", { {0, "cansado"} });
    idleCansadoAnims_.LoadFromTSX("assets/textures/animations/windUpScurryIdleCansado.xml", { {0, "idleCansado"} });
    hitAnims_.LoadFromTSX("assets/textures/animations/windUpScurryHit.xml", { {0, "hit"} });
    dieAnims_.LoadFromTSX("assets/textures/animations/windUpScurryDie.xml", { {0, "die"} });

    // ?? Non-looping animations ???????????????????????????????????????????????
    alertaAnims_.SetLoop("alerta", false);
    antesWalkFastAnims_.SetLoop("antesWalkFast", false);
    biteAnims_.SetLoop("bite", true);
    // biteRetreat is a run-cycle so it loops until the timer expires
    biteRetreatAnims_.SetLoop("biteRetreat", true);
    cansadoAnims_.SetLoop("cansado", false);
    hitAnims_.SetLoop("hit", false);
    dieAnims_.SetLoop("die", false);

    // ?? Set current clips ????????????????????????????????????????????????????
    idleAnims_.SetCurrent("idle");
    alertaAnims_.SetCurrent("alerta");
    antesWalkFastAnims_.SetCurrent("antesWalkFast");
    walkFastAnims_.SetCurrent("walkFast");
    biteAnims_.SetCurrent("bite");
    biteRetreatAnims_.SetCurrent("biteRetreat");
    cansadoAnims_.SetCurrent("cansado");
    idleCansadoAnims_.SetCurrent("idleCansado");
    hitAnims_.SetCurrent("hit");
    dieAnims_.SetCurrent("die");

    ChooseNewPatrolDirection();

    attackFxId = Engine::GetInstance().audio->LoadFx("assets/audio/Enemies/WindUpScurry/ataques.wav");
    deadFxId = Engine::GetInstance().audio->LoadFx("assets/audio/Enemies/WindUpScurry/dead.wav");

    EnterState(State::IDLE);
    return true;
}

bool EnemyWindUpScurry::Update(float dt) {
    playerRef_ = Engine::GetInstance().scene->player.get();

    if (attackCooldown_ > 0.0f) attackCooldown_ -= dt;

    // Sync position from physics
    if (pbody != nullptr) {
        int x, y;
        pbody->GetPosition(x, y);
        position.setX((float)x);
        position.setY((float)y);
    }

    // Facing direction
    if (currentState_ == State::IDLE) {
        facingRight_ = (patrolDirX_ > 0.0f);
    }
    else if (currentState_ == State::WALK_FAST && playerRef_) {
        int px, py;
        playerRef_->pbody->GetPosition(px, py);
        facingRight_ = (px > position.getX());
    }
    else if (currentState_ == State::BITE) {
        // Keep facing the player during the bite
        if (playerRef_ && playerRef_->pbody) {
            int px, py;
            playerRef_->pbody->GetPosition(px, py);
            facingRight_ = (px > position.getX());
        }
    }
    else if (currentState_ == State::BITE_RETREAT) {
        // Face the retreat direction (away from player)
        facingRight_ = (biteReturnDirX_ > 0.0f);
    }

    // ?? Death is handled separately ??????????????????????????????????????????
    if (currentState_ == State::DEATH) {
        dieAnims_.Update(dt);
        Draw(dt);
        if (dieAnims_.HasFinishedOnce("die")) {
            active = false;
            pendingToDelete = true;
        }
        return true;
    }

    // ?? Update active animation ??????????????????????????????????????????????
    switch (currentState_) {
    case State::IDLE:            idleAnims_.Update(dt);           break;
    case State::ALERTA:          alertaAnims_.Update(dt);         break;
    case State::ANTES_WALK_FAST: antesWalkFastAnims_.Update(dt);  break;
    case State::WALK_FAST:       walkFastAnims_.Update(dt);       break;
    case State::BITE:            biteAnims_.Update(dt);           break;
    case State::BITE_RETREAT:    biteRetreatAnims_.Update(dt);    break;
    case State::CANSADO:         cansadoAnims_.Update(dt);        break;
    case State::IDLE_CANSADO:    idleCansadoAnims_.Update(dt);    break;
    case State::HIT:             hitAnims_.Update(dt);            break;
    default: break;
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

    auto& tex = Engine::GetInstance().textures;
    if (idleTexture_)          tex->UnLoad(idleTexture_);
    if (alertaTexture_)        tex->UnLoad(alertaTexture_);
    if (antesWalkFastTexture_) tex->UnLoad(antesWalkFastTexture_);
    if (walkFastTexture_)      tex->UnLoad(walkFastTexture_);
    // biteTexture_ y biteRetreatTexture_ son alias de walkFastTexture_ mientras no haya spritesheet propio
    // — no hacer UnLoad o liberará el mismo puntero dos veces
    // tex->UnLoad(biteTexture_);
    // tex->UnLoad(biteRetreatTexture_);
    if (cansadoTexture_)       tex->UnLoad(cansadoTexture_);
    if (idleCansadoTexture_)   tex->UnLoad(idleCansadoTexture_);
    if (hitTexture_)           tex->UnLoad(hitTexture_);
    if (dieTexture_)           tex->UnLoad(dieTexture_);

    idleTexture_ = alertaTexture_ = antesWalkFastTexture_ = walkFastTexture_ = nullptr;
    biteTexture_ = biteRetreatTexture_ = nullptr;
    cansadoTexture_ = idleCansadoTexture_ = hitTexture_ = dieTexture_ = nullptr;

    return true;
}

// ?????????????????????????????????????????????????????????????????????????????
//  FSM
// ?????????????????????????????????????????????????????????????????????????????

void EnemyWindUpScurry::UpdateFSM(float dt) {
    stateTimer_ += dt;

    switch (currentState_) {

        // ?? IDLE: random patrol ??????????????????????????????????????????????????
    case State::IDLE:
    {
        patrolTimer_ += dt;
        if (patrolTimer_ >= PATROL_CHANGE_INTERVAL) {
            ChooseNewPatrolDirection();
            patrolTimer_ = 0.0f;
        }
        if (pbody) Engine::GetInstance().physics->SetXVelocity(pbody, patrolDirX_ * PATROL_SPEED);
        if (GetDistanceToPlayer() <= DETECTION_RADIUS) EnterState(State::ALERTA);
        break;
    }

    // ?? ALERTA: spotted the player ???????????????????????????????????????????
    case State::ALERTA:
        if (pbody) Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
        if (alertaAnims_.HasFinishedOnce("alerta")) EnterState(State::ANTES_WALK_FAST);
        break;

        // ?? ANTES_WALK_FAST: wind-up before chase ????????????????????????????????
    case State::ANTES_WALK_FAST:
        if (pbody) Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
        if (antesWalkFastAnims_.HasFinishedOnce("antesWalkFast")) EnterState(State::WALK_FAST);
        break;

        // ?? WALK_FAST: chase until bite range ????????????????????????????????????
    case State::WALK_FAST:
    {
        if (playerRef_ && playerRef_->pbody) {
            int px, py;
            playerRef_->pbody->GetPosition(px, py);
            float dx = (float)px - position.getX();
            float dirX = (dx > 0.0f) ? 1.0f : -1.0f;

            if (pbody) Engine::GetInstance().physics->SetXVelocity(pbody, dirX * CHASE_SPEED);

            // Close enough ? bite!
            if (std::abs(dx) <= BITE_RANGE) {
                EnterState(State::BITE);
                break;
            }
        }
        // Safety timeout — go exhausted if player is unreachable
        if (stateTimer_ >= CHASE_DURATION) EnterState(State::CANSADO);
        break;
    }

    // ?? BITE: stop, play bite anim, deal damage once, then retreat ???????????
    case State::BITE:
    {
        if (pbody) Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);

        if (!biteDamageDealt_ && playerRef_ && playerRef_->pbody) {
            if (playerRef_->pbody->listener != nullptr && attackCooldown_ <= 0.0f) {
                Engine::GetInstance().audio->PlayFxSpatial(attackFxId, position);
                playerRef_->pbody->listener->TakeDamage(1);
                attackCooldown_ = ATTACK_COOLDOWN;
                biteDamageDealt_ = true;
            }
        }

        // Sin XML propio: avanzar por timer (600ms = duración aproximada de un mordisco)
        if (stateTimer_ >= 600.0f) {
            EnterState(State::BITE_RETREAT);
        }
        break;
    }
    // ?? BITE_RETREAT: run away in opposite direction ?????????????????????????
    case State::BITE_RETREAT:
    {
        if (pbody) Engine::GetInstance().physics->SetXVelocity(pbody, biteReturnDirX_ * BITE_RETREAT_SPEED);

        if (stateTimer_ >= BITE_RETREAT_DURATION) {
            EnterState(State::CANSADO);
        }
        break;
    }

    // ?? CANSADO: winding down animation ?????????????????????????????????????
    case State::CANSADO:
        if (pbody) Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
        if (cansadoAnims_.HasFinishedOnce("cansado")) EnterState(State::IDLE_CANSADO);
        break;

        // ?? IDLE_CANSADO: exhausted & vulnerable ?????????????????????????????????
    case State::IDLE_CANSADO:
        if (pbody) Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
        if (stateTimer_ >= FATIGUE_DURATION) EnterState(State::IDLE);
        break;

        // ?? HIT: taking damage ???????????????????????????????????????????????????
    case State::HIT:
        if (pbody) Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
        if (hitAnims_.HasFinishedOnce("hit")) {
            EnterState(health <= 0 ? State::DEATH : State::IDLE_CANSADO);
        }
        break;

    default:
        break;
    }
}

void EnemyWindUpScurry::EnterState(State newState) {
    LOG("EnemyWindUpScurry::EnterState: %d -> %d", (int)currentState_, (int)newState);
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
    case State::BITE:
        biteAnims_.ResetCurrent();
        biteDamageDealt_ = false; // reset so each bite can deal damage once
        // Store retreat direction (opposite of where the player is right now)
        if (playerRef_ && playerRef_->pbody) {
            int px, py;
            playerRef_->pbody->GetPosition(px, py);
            biteReturnDirX_ = (px > position.getX()) ? -1.0f : 1.0f;
        }
        break;
    case State::BITE_RETREAT:
        biteRetreatAnims_.ResetCurrent();
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

// ?????????????????????????????????????????????????????????????????????????????
//  Helpers
// ?????????????????????????????????????????????????????????????????????????????

void EnemyWindUpScurry::ChooseNewPatrolDirection() {
    patrolDirX_ = (rand() % 2 == 0) ? 1.0f : -1.0f;
}

// ?????????????????????????????????????????????????????????????????????????????
//  Collision
// ?????????????????????????????????????????????????????????????????????????????

void EnemyWindUpScurry::OnCollision(PhysBody* physA, PhysBody* physB) {
    if (currentState_ == State::DEATH) return;

    if (physA == pbody || physB == pbody) {
        PhysBody* other = (physA == pbody) ? physB : physA;

        // Wall contact damage during IDLE patrol (legacy behaviour, kept)
        if (other->ctype == ColliderType::PLAYER) {
            if (currentState_ == State::IDLE && attackCooldown_ <= 0.0f) {
                if (other->listener != nullptr) {
                    Engine::GetInstance().audio->PlayFxSpatial(attackFxId, position);
                    other->listener->TakeDamage(1);
                    attackCooldown_ = ATTACK_COOLDOWN;
                }
            }
        }

        // Player melee attack hits this enemy
        if (other->ctype == ColliderType::ATTACK) {
            TakeDamage(1);
        }
        else if (other->ctype == ColliderType::SLINGSHOT_PROJ) {
            TakeDamage(1);
            if (other->listener != nullptr) {
                other->listener->Destroy();
            }
        }

        // Reverse patrol direction on wall hit
        if (other->ctype == ColliderType::PLATFORM && currentState_ == State::IDLE) {
            patrolDirX_ = -patrolDirX_;
        }
    }
}

void EnemyWindUpScurry::OnCollisionEnd(PhysBody* physA, PhysBody* physB) {
    // No special logic needed
}

void EnemyWindUpScurry::TakeDamage(int damage) {
    if (currentState_ == State::DEATH || currentState_ == State::HIT) return;

    health -= damage;
    LOG("WindUpScurry took %d damage -> health: %d/%d", damage, health, maxHealth);
    EnterState(State::HIT);
}

// ?????????????????????????????????????????????????????????????????????????????
//  Draw
// ?????????????????????????????????????????????????????????????????????????????

void EnemyWindUpScurry::Draw(float dt) {
    SDL_Texture* currentTexture = nullptr;
    SDL_Rect     frameRect = { 0, 0, 0, 0 };

    switch (currentState_) {
    case State::IDLE:            currentTexture = idleTexture_;          frameRect = idleAnims_.GetCurrentFrame();           break;
    case State::ALERTA:          currentTexture = alertaTexture_;        frameRect = alertaAnims_.GetCurrentFrame();         break;
    case State::ANTES_WALK_FAST: currentTexture = antesWalkFastTexture_; frameRect = antesWalkFastAnims_.GetCurrentFrame();  break;
    case State::WALK_FAST:       currentTexture = walkFastTexture_;      frameRect = walkFastAnims_.GetCurrentFrame();       break;
    case State::BITE:            currentTexture = biteTexture_;          frameRect = biteAnims_.GetCurrentFrame();           break;
    case State::BITE_RETREAT:    currentTexture = biteRetreatTexture_;   frameRect = biteRetreatAnims_.GetCurrentFrame();    break;
    case State::CANSADO:         currentTexture = cansadoTexture_;       frameRect = cansadoAnims_.GetCurrentFrame();        break;
    case State::IDLE_CANSADO:    currentTexture = idleCansadoTexture_;   frameRect = idleCansadoAnims_.GetCurrentFrame();    break;
    case State::HIT:             currentTexture = hitTexture_;           frameRect = hitAnims_.GetCurrentFrame();            break;
    case State::DEATH:           currentTexture = dieTexture_;           frameRect = dieAnims_.GetCurrentFrame();            break;
    }

    if (currentTexture && frameRect.w > 0 && frameRect.h > 0) {
        float scale = 0.5f;
        switch (currentState_) {
        case State::IDLE:         scale = 1.6f; break;
        case State::ALERTA:       scale = 1.0f; break;
        case State::CANSADO:      scale = 1.1f; break;
        case State::IDLE_CANSADO: scale = 1.0f; break;
        case State::DEATH:        scale = 1.0f; break;
        case State::BITE:         scale = 1.0f; break; // ajusta según tu spritesheet
        case State::BITE_RETREAT: scale = 0.5f; break; // ajusta según tu spritesheet
        default:                  scale = 0.5f; break;
        }

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

float EnemyWindUpScurry::GetDistanceToPlayer() const {
    if (!playerRef_ || !playerRef_->pbody || !pbody || playerRef_->IsHiding()) return 9999.0f;
    int px, py, sx, sy;
    playerRef_->pbody->GetPosition(px, py);
    pbody->GetPosition(sx, sy);
    float dx = (float)(sx - px);
    float dy = (float)(sy - py);
    return std::sqrt(dx * dx + dy * dy);
}