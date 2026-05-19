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
    // Textures
    idleTexture_   = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Stitichling_Idle.png");
    alertTexture_  = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Stitichling_Alerta.png");
    setupTexture_  = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_stitchling_Suelta_Cuerda.png");
    grabTexture_   = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Stitchling_Agarrar.png");
    rewindTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Stitchling_Recoge_Cuerda.png");
    hitTexture_    = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Stitchling_Hit.png");
    dieTexture_    = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Stitichling_Die.png");

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

    // Load AnimationSets
    idleAnims_.LoadFromTSX("assets/textures/animations/stitchlingIdle.xml", { {0, "idle"} });
    alertAnims_.LoadFromTSX("assets/textures/animations/stitchlingAlerta.xml", { {0, "alert"} });
    setupAnims_.LoadFromTSX("assets/textures/animations/stitchlingSueltaCuerda.xml", { {0, "setup"} });
    grabAnims_.LoadFromTSX("assets/textures/animations/stitchlingAgarrar.xml", { {0, "grab"} });
    rewindAnims_.LoadFromTSX("assets/textures/animations/stitchlingRecogeCuerda.xml", { {0, "rewind"} });
    hitAnims_.LoadFromTSX("assets/textures/animations/stitchlingHit.xml", { {0, "hit"} });
    dieAnims_.LoadFromTSX("assets/textures/animations/stitchlingDie.xml", { {0, "die"} });

    setupAnims_.SetLoop("setup", false);
    rewindAnims_.SetLoop("rewind", false);
    hitAnims_.SetLoop("hit", false);
    dieAnims_.SetLoop("die", false);

    idleAnims_.SetCurrent("idle");
    alertAnims_.SetCurrent("alert");
    setupAnims_.SetCurrent("setup");
    grabAnims_.SetCurrent("grab");
    rewindAnims_.SetCurrent("rewind");
    hitAnims_.SetCurrent("hit");
    dieAnims_.SetCurrent("die");

    EnterState(State::SETUP);
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
    case State::SETUP:
        setupAnims_.Update(dt);
        break;
    case State::IDLE:
        idleAnims_.Update(dt);
        break;
    case State::TRAP_ACTIVE:
        grabAnims_.Update(dt);
        break;
    case State::REWIND_SLOW:
        rewindAnims_.Update(dt * 0.5f); // Half speed
        break;
    case State::REWIND_FAST:
        rewindAnims_.Update(dt * 2.0f); // Double speed
        break;
    case State::HIT:
        hitAnims_.Update(dt);
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
    if (setupTexture_) Engine::GetInstance().textures->UnLoad(setupTexture_);
    if (grabTexture_) Engine::GetInstance().textures->UnLoad(grabTexture_);
    if (rewindTexture_) Engine::GetInstance().textures->UnLoad(rewindTexture_);
    if (hitTexture_) Engine::GetInstance().textures->UnLoad(hitTexture_);
    if (dieTexture_) Engine::GetInstance().textures->UnLoad(dieTexture_);

    idleTexture_ = nullptr;
    alertTexture_ = nullptr;
    setupTexture_ = nullptr;
    grabTexture_ = nullptr;
    rewindTexture_ = nullptr;
    hitTexture_ = nullptr;
    dieTexture_ = nullptr;

    return true;
}

void EnemyStitchling::UpdateFSM(float dt) {
    stateTimer_ += dt;

    switch (currentState_) {
    case State::SETUP:
        if (setupAnims_.HasFinishedOnce("setup")) {
            EnterState(State::IDLE);
        }
        break;

    case State::IDLE:
        break;

    case State::TRAP_ACTIVE:
        if (playerRef_) {
            // Apply damage if stuck for too long
            trapDamageTimer_ += dt;
            if (trapDamageTimer_ >= TRAP_DAMAGE_INTERVAL) {
                playerRef_->TakeDamage(1);
                trapDamageTimer_ = 0.0f;
            }

            // Player can mash SPACE to escape
            if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN) {
                escapeMashes_++;
                if (escapeMashes_ >= MASHES_TO_ESCAPE) {
                    // Escaped!
                    RemovePlayerSlowdown();
                    if (rand() % 2 == 0) {
                        EnterState(State::REWIND_SLOW);
                    } else {
                        EnterState(State::REWIND_FAST);
                    }
                }
            }
        }
        break;

    case State::REWIND_SLOW:
        if (rewindAnims_.HasFinishedOnce("rewind")) {
            EnterState(State::SETUP);
        }
        break;

    case State::REWIND_FAST:
        if (rewindAnims_.HasFinishedOnce("rewind")) {
            EnterState(State::SETUP);
        }
        break;

    case State::HIT:
        if (hitAnims_.HasFinishedOnce("hit")) {
            EnterState(State::DEATH);
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
    case State::SETUP:
        setupAnims_.ResetCurrent();
        break;
    case State::IDLE:
        idleAnims_.ResetCurrent();
        break;
    case State::TRAP_ACTIVE:
        grabAnims_.ResetCurrent();
        trapDamageTimer_ = 0.0f;
        escapeMashes_ = 0;
        ApplyPlayerSlowdown();
        break;
    case State::REWIND_SLOW:
        rewindAnims_.ResetCurrent();
        break;
    case State::REWIND_FAST:
        rewindAnims_.ResetCurrent();
        break;
    case State::HIT:
        hitAnims_.ResetCurrent();
        RemovePlayerSlowdown();
        break;
    case State::DEATH:
        dieAnims_.ResetCurrent();
        RemovePlayerSlowdown();
        
        if (pbody) {
            Engine::GetInstance().physics->DeletePhysBody(pbody);
            pbody = nullptr;
        }
        if (trapSensor_) {
            Engine::GetInstance().physics->DeletePhysBody(trapSensor_);
            trapSensor_ = nullptr;
        }
        break;
    }
}

void EnemyStitchling::ApplyPlayerSlowdown() {
    if (playerRef_ && !playerWasSlowed_) {
        playerRef_->speed = 1.0f; 
        playerRef_->isYoyoTrapped_ = true;
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
        if (other->ctype == ColliderType::PLAYER && currentState_ == State::IDLE) {
            LOG("Transitioning to TRAP_ACTIVE!");
            EnterState(State::TRAP_ACTIVE);
        }
    }

    if (physA == pbody || physB == pbody) {
        PhysBody* other = (physA == pbody) ? physB : physA;
        if (other->ctype == ColliderType::ATTACK) {
            LOG("EnemyStitchling hit by player attack");
            TakeDamage(1);
        } else if (other->ctype == ColliderType::SLINGSHOT_PROJ) {
            LOG("EnemyStitchling hit by slingshot projectile");
            TakeDamage(1);
            if (other->listener != nullptr) {
                other->listener->Destroy();
            }
        }
    }
}

void EnemyStitchling::OnCollisionEnd(PhysBody* physA, PhysBody* physB) {
    LOG("EnemyStitchling::OnCollisionEnd triggered: physA ctype = %d, physB ctype = %d", (int)physA->ctype, (int)physB->ctype);
    if (physA == trapSensor_ || physB == trapSensor_) {
        PhysBody* other = (physA == trapSensor_) ? physB : physA;
        LOG("CollisionEnd with trapSensor_! other ctype = %d, currentState_ = %d", (int)other->ctype, (int)currentState_);
        if (other->ctype == ColliderType::PLAYER && currentState_ == State::TRAP_ACTIVE) {
            LOG("Removing player slowdown from CollisionEnd!");
            RemovePlayerSlowdown();
        }
    }
}

void EnemyStitchling::TakeDamage(int damage) {
    if (currentState_ == State::DEATH || currentState_ == State::HIT) return;

    if (currentState_ == State::REWIND_SLOW) {
        health -= damage;
        EnterState(State::HIT);
    } else if (currentState_ == State::REWIND_FAST) {
        LOG("Stitchling is rewinding fast and deflected the attack!");
    } else {
        LOG("Stitchling is immune right now!");
    }
}

void EnemyStitchling::Draw(float dt) {
    SDL_Texture* currentTexture = nullptr;
    SDL_Rect frameRect = { 0, 0, 0, 0 };

    switch (currentState_) {
    case State::SETUP:
        currentTexture = setupTexture_;
        frameRect = setupAnims_.GetCurrentFrame();
        break;
    case State::IDLE:
        currentTexture = idleTexture_;
        frameRect = idleAnims_.GetCurrentFrame();
        break;
    case State::TRAP_ACTIVE:
        currentTexture = grabTexture_;
        frameRect = grabAnims_.GetCurrentFrame();
        break;
    case State::REWIND_SLOW:
    case State::REWIND_FAST:
        currentTexture = rewindTexture_;
        frameRect = rewindAnims_.GetCurrentFrame();
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
        float scale = 0.35f; 
        
        int drawW = (int)(frameRect.w * scale);
        int drawH = (int)(frameRect.h * scale);

        // Center horizontally on the body, anchor bottom of drawn sprite to body bottom (body center + 16px radius)
        int px = (int)position.getX() - drawW / 2;
        int py = (int)position.getY() + 16 - drawH;

        SDL_FlipMode flip = facingRight_ ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        
        Engine::GetInstance().render->DrawTexture(currentTexture, px, py, &frameRect, 1.0f, 0, INT_MAX, INT_MAX, flip, scale);
    }
}
