#include "Boss1.h"
#include <SDL3/SDL.h>
#include "Engine.h"
#include "Window.h"
#include "Physics.h"
#include "Render.h"
#include "Scene.h"
#include "Input.h"
#include "Player.h"
#include "Textures.h"
#include "Animation.h"
#include "Audio.h"
#include "Log.h"

// ─────────────────────────────────────────────────────────────────────────────

Boss1::Boss1() : Boss(EntityType::BOSS_1)
{
    name      = "Boss1";
    health    = 4;
    maxHealth = 4;
    texW      = BODY_W;
    texH      = BODY_H;
}

Boss1::~Boss1() {}

bool Boss1::Start()
{
    if (floorY_ == 0.0f) floorY_ = position.getY();
    undergroundX_ = position.getX();

    // Default wall bounds if not set from Tiled
    if (p2WallLeft_ == 0.0f && p2WallRight_ == 0.0f)
    {
        p2WallLeft_  = position.getX() - 500.0f;
        p2WallRight_ = position.getX() + 500.0f;
    }

    // ── Load animations (all Drowning Plush, square frames) ──────────────────
    auto& tex = *Engine::GetInstance().textures;
    const char* DIR = "assets/textures/spritesheets/SS_Enemics_Level2/";

    texSpit_   = tex.Load("assets/textures/AS_props/AS_escupitajo.png");
    texPuddle_ = tex.Load("assets/textures/AS_props/AS_Charco.png");

    texMove_ = tex.Load((std::string(DIR) + "SP_DrowningPlush_movimiento_bajo_suelo_fase1.png").c_str());
    for (int i = 0; i < 10; i++) animMove_.AddFrame({i * 256, 0, 256, 256}, 60);
    animMove_.SetLoop(true);

    // Cansado: 5-frame intro plays once, then the 12-frame loop takes over while exposed
    texCansado_ = tex.Load((std::string(DIR) + "SP_DrowningPlush_cansado_fase1.png").c_str());
    for (int i = 0; i < 5; i++) animCansado_.AddFrame({i * 256, 0, 256, 256}, 80);
    animCansado_.SetLoop(false);

    texCansadoLoop_ = tex.Load((std::string(DIR) + "SP_DrowningPlush_cansado_loop_fase1.png").c_str());
    for (int i = 0; i < 12; i++) animCansadoLoop_.AddFrame({i * 256, 0, 256, 256}, 80);
    animCansadoLoop_.SetLoop(true);

    texAttack_ = tex.Load((std::string(DIR) + "SP_DrowningPlush_salir_suelo_fase1.png").c_str());
    for (int i = 0; i < 32; i++) animAttack_.AddFrame({i * 256, 0, 256, 256}, 60);
    animAttack_.SetLoop(false);

    texHit_ = tex.Load((std::string(DIR) + "SP_DrowningPlush_hit_fase1.png").c_str());
    for (int i = 0; i < 6; i++) animHit_.AddFrame({i * 256, 0, 256, 256}, 100);
    animHit_.SetLoop(false);

    texDive_ = tex.Load((std::string(DIR) + "SP_Drowning_Plush_sumergirse.png").c_str());
    for (int i = 0; i < 18; i++) animDive_.AddFrame({i * 256, 0, 256, 256}, 80);
    animDive_.SetLoop(false);

    texP2Move_ = tex.Load((std::string(DIR) + "SS_Drowningplush2afase.png").c_str());
    for (int i = 0; i < 12; i++) animP2Move_.AddFrame({i * 512,    0, 512, 512}, 70);
    for (int i = 0; i < 8;  i++) animP2Move_.AddFrame({i * 512,  512, 512, 512}, 70);
    for (int i = 0; i < 5;  i++) animP2Move_.AddFrame({i * 512, 1024, 512, 512}, 70);
    for (int i = 0; i < 11; i++) animP2Move_.AddFrame({i * 512, 1536, 512, 512}, 70);
    animP2Move_.SetLoop(true);

    // Escupir — fila 2 (y=512), columnas 3-7 (5 frames donde el boss escupe;
    // la columna 8 de esa fila está vacía en la spritesheet)
    for (int i = 3; i <= 7; i++) animP2Spit_.AddFrame({i * 512, 512, 512, 512}, 100);
    animP2Spit_.SetLoop(false);

    // ── Calibrate floorY_ immediately via downward raycast so the first jump arc
    // and underground strip are correct from the start (no waiting for first landing).
    float hitX, hitY;
    int rcX     = (int)undergroundX_;
    int rcStart = (int)floorY_ - 100; // start well above expected floor
    int rcEnd   = (int)floorY_ + 600; // search far below to guarantee hitting the floor
    if (Engine::GetInstance().physics->RayCastWorld(rcX, rcStart, rcX, rcEnd, hitX, hitY))
    {
        floorY_          = hitY;
        floorCalibrated_ = true;
    }

    // ── Audio ─────────────────────────────────────────────────────────────────
    auto& audio = *Engine::GetInstance().audio;
    fxJump_         = audio.LoadFx("assets/audio/Enemies/Drowning Plush/Drowningplush_jump.wav");
    fxAterrizaje_   = audio.LoadFx("assets/audio/Enemies/Drowning Plush/Drowningplush_death hit.wav");
    fxHit_          = audio.LoadFx("assets/audio/Enemies/Drowning Plush/Drowningplush_hit.wav");
    fxBocaAbierta_  = audio.LoadFx("assets/audio/Enemies/Drowning Plush/Drowningplush_abriendo boca.wav");
    fxDisparo_      = audio.LoadFx("assets/audio/Enemies/Drowning Plush/Drowningplush_Escupitajo.wav");
    fxImpactoSuelo_ = audio.LoadFx("assets/audio/Enemies/Drowning Plush/Drowningplush_Escupitajo Suelo.wav");
    fxDive_         = audio.LoadFx("assets/audio/Enemies/Drowning Plush/Drowingplush_sumergirse.wav");

    return true;
}

bool Boss1::CleanUp()
{
    DestroySpit();
    DestroyPuddle();
    DestroyPhysicsBody();
    auto& tex = *Engine::GetInstance().textures;
    tex.UnLoad(texSpit_);
    tex.UnLoad(texPuddle_);
    tex.UnLoad(texMove_);
    tex.UnLoad(texCansado_);
    tex.UnLoad(texCansadoLoop_);
    tex.UnLoad(texAttack_);
    tex.UnLoad(texHit_);
    tex.UnLoad(texDive_);
    tex.UnLoad(texP2Move_);
    return true;
}

// ─── Physics body helpers ─────────────────────────────────────────────────────

void Boss1::SpawnPhysicsBody()
{
    if (pbody) return;

    // Spawn with capsule bottom at floorY_. Called only on landing (JUMP→VULNERABLE).
    int spawnX = (int)undergroundX_;
    int spawnY = (int)(floorY_ - (float)BODY_H * 0.5f);

    pbody = Engine::GetInstance().physics->CreateCapsule(
        spawnX, spawnY, BODY_W / 2, BODY_H, bodyType::DYNAMIC, 0.0f);
    pbody->listener = this;
    pbody->ctype    = ColliderType::ENEMY;
}

void Boss1::DestroyPhysicsBody()
{
    if (!pbody) return;
    Engine::GetInstance().physics->DeletePhysBody(pbody);
    pbody               = nullptr;
    isContactWithPlayer_ = false;
    playerListener_      = nullptr;
}

// ─── Phase 2 helpers ─────────────────────────────────────────────────────────

void Boss1::SpawnSpit(float dirX)
{
    if (p2SpitBody_) return;
    int sx = (int)undergroundX_;
    int sy = (int)(floorY_ - BODY_H_OPEN * 0.5f); // spawn at "mouth" height
    p2SpitBody_ = Engine::GetInstance().physics->CreateCircle(
        sx, sy, P2_SPIT_R, bodyType::DYNAMIC);
    p2SpitBody_->listener = this;
    p2SpitBody_->ctype    = ColliderType::ENEMY;

    // Calculate VX so spit always reaches the player.
    // Flight time: t = 2 * |VY| / gravity  (gravity = 10 m/s² downward)
    constexpr float FLIGHT_TIME = 2.0f * (-P2_SPIT_VY) / 10.0f;
    float vx = P2_SPIT_VX;
    {
        auto& scn2 = *Engine::GetInstance().scene;
        if (scn2.player && scn2.player->pbody)
        {
            int tpx, tpy;
            scn2.player->pbody->GetPosition(tpx, tpy);
            float dxPx  = fabsf((float)tpx - (float)sx);
            float neededVX = PIXEL_TO_METERS(dxPx) / FLIGHT_TIME;
            if (neededVX > vx) vx = neededVX;
            if (vx > 15.0f)    vx = 15.0f; // cap so it doesn't look absurd
        }
    }
    b2Vec2 vel = { dirX * vx, P2_SPIT_VY };
    b2Body_SetLinearVelocity(p2SpitBody_->body, vel);
    p2SpitHitPlayer_ = false;
    p2SpitHitFloor_  = false;
    Engine::GetInstance().audio->PlayFxSpatial(fxDisparo_, position);
}

void Boss1::DestroySpit()
{
    if (!p2SpitBody_) return;
    Engine::GetInstance().physics->DeletePhysBody(p2SpitBody_);
    p2SpitBody_      = nullptr;
    p2SpitHitPlayer_ = false;
    p2SpitHitFloor_  = false;
}

void Boss1::SpawnPuddle(float x)
{
    DestroyPuddle(); // only one puddle at a time
    int px = (int)x;
    int py = (int)(floorY_ - P2_PUDDLE_H / 2);
    p2PuddleBody_ = Engine::GetInstance().physics->CreateRectangleSensor(
        px, py, P2_PUDDLE_W, P2_PUDDLE_H, bodyType::STATIC);
    p2PuddleBody_->listener = this;
    p2PuddleBody_->ctype    = ColliderType::PUDDLE;
    p2PuddleTimer_   = P2_PUDDLE_DURATION;
    p2PlayerInPuddle_= false;
    Engine::GetInstance().audio->PlayFxSpatial(fxImpactoSuelo_, position);
}

void Boss1::DestroyPuddle()
{
    if (!p2PuddleBody_) return;
    // Restore player speed and jump if they were inside
    if (p2PlayerInPuddle_)
    {
        auto& scn = *Engine::GetInstance().scene;
        if (scn.player)
        {
            scn.player->speed     = p2OriginalSpeed_;
            scn.player->jumpForce = p2OriginalJumpForce_;
        }
        p2PlayerInPuddle_ = false;
    }
    Engine::GetInstance().physics->DeletePhysBody(p2PuddleBody_);
    p2PuddleBody_ = nullptr;
}

void Boss1::UpdateP2(float dt)
{
    // ── Spit in flight ────────────────────────────────────────────────────────
    if (p2SpitHitPlayer_)
    {
        auto& scn = *Engine::GetInstance().scene;
        if (scn.player) scn.player->TakeDamage(1);
        DestroySpit();
    }
    else if (p2SpitHitFloor_)
    {
        SpawnPuddle(p2SpitFloorX_);
        DestroySpit();
    }

    // ── Puddle lifetime ───────────────────────────────────────────────────────
    if (p2PuddleBody_)
    {
        p2PuddleTimer_ -= dt;
        if (p2PuddleTimer_ <= 0.0f) DestroyPuddle();
    }
}

// ─── State transitions ────────────────────────────────────────────────────────

void Boss1::TransitionTo(Boss1State newState)
{
    state_           = newState;
    stateTimer_      = 0.0f;
    alignedTimer_    = 0.0f;
    accumulateTimer_ = 0.0f;
    exitPauseTimer_  = 0.0f;

    // Reset non-looping animations on entry
    auto& audio = *Engine::GetInstance().audio;
    switch (newState)
    {
    case Boss1State::DIVE:       animDive_.Reset();    audio.PlayFxSpatial(fxDive_, position); break;
    case Boss1State::DEATH:      animDive_.Reset();    audio.PlayFxSpatial(fxDive_, position); deathTeleportTimer_ = 0.0f; deathSequenceDone_ = false; break;
    case Boss1State::STUNNED:    animHit_.Reset();     break;
    case Boss1State::JUMP:       animAttack_.Reset();  audio.PlayFxSpatial(fxDive_, position); audio.PlayFxSpatial(fxJump_, position); break;
    case Boss1State::VULNERABLE: animCansado_.Reset(); audio.PlayFxSpatial(fxAterrizaje_, position);  break;
    case Boss1State::P2_SPIT:    animP2Spit_.Reset();  audio.PlayFxSpatial(fxBocaAbierta_, position); break;
    default: break;
    }
}

// ─── TakeDamage ──────────────────────────────────────────────────────────────

void Boss1::TakeDamage(int damage)
{
    // Boss is only damageable during VULNERABLE or STUNNED
    if (state_ != Boss1State::VULNERABLE &&
        state_ != Boss1State::STUNNED)
        return;

    // In phase 2 surface attack, track whether the player hit us during VULNERABLE
    if (phase_ == 2 && state_ == Boss1State::VULNERABLE)
        p2WasHitInVulnerable_ = true;

    health -= damage;
    health = (health < 0) ? 0 : health;

    int oldPhase = phase_;
    CheckPhaseTransition();
    bool justEnteredP2 = (phase_ == 2 && oldPhase == 1);

    Engine::GetInstance().audio->PlayFxSpatial(fxHit_, position);

    if (health <= 0)
    {
        isDead_ = true;
        DestroyPhysicsBody();
        TransitionTo(Boss1State::DEATH);
    }
    else
    {
        // Knockback impulse
        if (pbody)
        {
            Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
            float dirX = (position.getX() > playerPos.getX()) ? 1.0f : -1.0f;
            Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, dirX * 4.0f, -2.0f, true);
        }
        TransitionTo(Boss1State::STUNNED);
    }
}

// ─── Collision ────────────────────────────────────────────────────────────────

void Boss1::OnCollision(PhysBody* physA, PhysBody* physB)
{
    // ── Phase 2 spit hit something ────────────────────────────────────────────
    if (physA == p2SpitBody_)
    {
        if (physB->ctype == ColliderType::PLAYER)
            p2SpitHitPlayer_ = true;
        else if (physB->ctype != ColliderType::ATTACK)
        {
            // Hit floor or wall — record X for puddle
            int sx, sy;
            p2SpitBody_->GetPosition(sx, sy);
            p2SpitFloorX_ = (float)sx;
            p2SpitHitFloor_ = true;
        }
        return;
    }

    // ── Phase 2 puddle — player entered ──────────────────────────────────────
    if (physA == p2PuddleBody_ && physB->ctype == ColliderType::PLAYER)
    {
        auto& scn = *Engine::GetInstance().scene;
        if (scn.player && !p2PlayerInPuddle_)
        {
            if (scn.player)
            {
                p2OriginalSpeed_     = scn.player->speed;
                p2OriginalJumpForce_ = scn.player->jumpForce;
                scn.player->speed     *= P2_PUDDLE_SLOW;
                scn.player->jumpForce *= P2_PUDDLE_SLOW;
            }
            p2PlayerInPuddle_ = true;
        }
        return;
    }

    // ── Normal attack damage ──────────────────────────────────────────────────
    if (physB->ctype == ColliderType::ATTACK)
        TakeDamage(1);
}

void Boss1::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
    // Player left puddle — restore speed
    if (physA == p2PuddleBody_ && physB->ctype == ColliderType::PLAYER)
    {
        auto& scn = *Engine::GetInstance().scene;
        if (scn.player && p2PlayerInPuddle_)
        {
            if (auto* p = dynamic_cast<Player*>(scn.player.get()))
            {
                p->speed     = p2OriginalSpeed_;
                p->jumpForce = p2OriginalJumpForce_;
            }
            p2PlayerInPuddle_ = false;
        }
    }
}

// ─── Phase change ─────────────────────────────────────────────────────────────

void Boss1::OnPhaseChange(int newPhase)
{
    attackCount_ = 0;
    if (newPhase == 2)
    {
        // No destruir el cuerpo ni transicionar aquí — TakeDamage irá a STUNNED
        // y DIVE usará p2WasHitInVulnerable_ para ir a P2_INTRO después
        p2WallCrosses_        = 0;
        p2NextSurface_        = 7 + rand() % 2;
        p2WasHitInVulnerable_ = true; // para que DIVE vaya a P2_INTRO
        auto& scn = *Engine::GetInstance().scene;
        Vector2D pp = scn.GetPlayerPosition();
        p2MoveDir_ = (pp.getX() > undergroundX_) ? 1.0f : -1.0f;
    }
}

// ─── FSM ─────────────────────────────────────────────────────────────────────

void Boss1::UpdateFSM(float dt)
{
    stateTimer_ += dt;

    auto&    scn       = *Engine::GetInstance().scene;
    Vector2D playerPos = scn.GetPlayerPosition();
    // Physics body center + small correction for sprite art not being pixel-perfect centered.
    // Tweak PLAYER_SPRITE_ALIGN_X in Boss1.h if the strip still looks off.
    float playerCX = playerPos.getX(); // fallback
    if (scn.player && scn.player->pbody)
    {
        int px, py;
        scn.player->pbody->GetPosition(px, py);
        playerCX = (float)px + PLAYER_SPRITE_ALIGN_X;
    }
    float speed = (phase_ == 1) ? UNDERGROUND_SPEED_P1 : UNDERGROUND_SPEED_P2;

    // Puddle and spit timers must tick every frame regardless of boss state
    UpdateP2(dt);

    // Facing direction: always toward player except during wall move (follow movement dir)
    if (state_ == Boss1State::P2_WALL_MOVE)
        facingRight_ = (p2MoveDir_ > 0.0f);
    else if (scn.player)
        facingRight_ = (playerCX > undergroundX_);

    switch (state_)
    {
    // ── WAITING ──────────────────────────────────────────────────────────────
    case Boss1State::WAITING:
        // Proximity trigger (temporary until room system exists)
        if (CheckProximityTrigger())
        {
            isEngaged_ = true;
            TransitionTo(Boss1State::UNDERGROUND);
        }
        break;

    // ── UNDERGROUND ──────────────────────────────────────────────────────────
    case Boss1State::UNDERGROUND:
    {
        float dx   = playerCX - undergroundX_;
        float step = speed * (dt / 1000.0f);

        if (alignedTimer_ > 0.0f)
        {
            // COMMITTED — position locked, counting down the tell before jumping
            alignedTimer_ += dt;
            if (alignedTimer_ >= (phase_ == 2 ? PRE_JUMP_DELAY_P2 : PRE_JUMP_DELAY))
            {
                attackCount_++;
                TransitionTo(Boss1State::JUMP);
            }
        }
        else if (fabsf(dx) < JUMP_ALIGN_THRESHOLD)
        {
            // ACCUMULATING — boss stops so the player can walk through.
            // Reset the exit pause since the player is back in zone.
            exitPauseTimer_ = EXIT_PAUSE_DURATION;
            accumulateTimer_ += dt;
            if (accumulateTimer_ >= (phase_ == 2 ? MIN_ALIGN_TIME_P2 : MIN_ALIGN_TIME))
            {
                undergroundX_ = playerCX;
                alignedTimer_ = dt;
            }
        }
        else
        {
            accumulateTimer_ = 0.0f;
            if (exitPauseTimer_ > 0.0f)
            {
                exitPauseTimer_ -= dt;
            }
            else
            {
                undergroundX_ += (dx > 0.0f) ? step : -step;
            }
        }

        position.setX(undergroundX_);
        position.setY(floorY_);
        break;
    }

    // ── JUMP ─────────────────────────────────────────────────────────────────
    case Boss1State::JUMP:
    {
        // No arc — boss attacks from underground, damages player if standing above
        position.setX(undergroundX_);
        position.setY(floorY_);

        if (contactDamageCooldown_ > 0.0f) contactDamageCooldown_ -= dt;
        if (scn.player && scn.player->pbody && contactDamageCooldown_ <= 0.0f)
        {
            int px, py;
            scn.player->pbody->GetPosition(px, py);
            bool inX = fabsf((float)px - undergroundX_) < JUMP_CONTACT_RANGE;
            bool inY = (float)py + PLAYER_BODY_HALF_H >= floorY_ - 20.0f;
            if (inX && inY)
            {
                scn.player->TakeDamage(1);
                contactDamageCooldown_ = CONTACT_DAMAGE_INTERVAL;
                // Empujar al jugador al lado contrario durante 350ms
                float pushDir = ((float)px >= undergroundX_) ? 1.0f : -1.0f;
                scn.player->knockbackX_     = pushDir * 8.0f;
                scn.player->knockbackTimer_ = 350.0f;
            }
        }

        // When attack animation finishes → spawn body and go vulnerable
        if (animAttack_.HasFinishedOnce())
        {
            floorCalibrated_ = true;
            SpawnPhysicsBody();
            velocity.x = 0.0f;
            TransitionTo(Boss1State::VULNERABLE);
        }
        break;
    }

    // ── VULNERABLE ───────────────────────────────────────────────────────────
    case Boss1State::VULNERABLE:
    {
        velocity.x = 0.0f;
        float dur = (phase_ == 1) ? VULNERABLE_DURATION_P1 : VULNERABLE_DURATION_P2;
        if (stateTimer_ >= dur)
            TransitionTo(Boss1State::DIVE);
        break;
    }

    // ── DIVE ─────────────────────────────────────────────────────────────────
    case Boss1State::DIVE:
        // Destroy the body on the very first frame of DIVE so the player can
        // walk through immediately — the "sinking" is purely visual from here on.
        if (pbody)
        {
            int bx, by;
            pbody->GetPosition(bx, by);
            undergroundX_ = (float)bx;
            floorY_        = (float)by + (float)BODY_H * 0.5f;
            DestroyPhysicsBody();
        }
        velocity.x = 0.0f;
        if (animDive_.HasFinishedOnce())
        {
            if (phase_ == 2 && p2WasHitInVulnerable_)
            {
                p2WasHitInVulnerable_ = false;
                TransitionTo(Boss1State::P2_INTRO);
            }
            else
                TransitionTo(Boss1State::UNDERGROUND);
        }
        break;

    // ── STUNNED ──────────────────────────────────────────────────────────────
    case Boss1State::STUNNED:
        velocity.x = 0.0f;
        if (stateTimer_ >= STUN_DURATION)
            TransitionTo(Boss1State::DIVE);
        break;

    // ── DEATH — reuses the "submerge" animation; once it finishes, wait a beat
    //           then send the player back to the hub (Map2 — spawn "Main") ────
    case Boss1State::DEATH:
        velocity.x = 0.0f;
        if (animDive_.HasFinishedOnce() && !deathSequenceDone_)
        {
            deathTeleportTimer_ += dt;
            if (deathTeleportTimer_ >= DEATH_TELEPORT_DELAY)
            {
                deathSequenceDone_ = true;
                isEngaged_ = false;
                // Seal the entrance portal (Map2.tmx, object id 70) — boss is gone, no reason to come back
                scn.SealBossPortal(1145.33f, 3668.0f, 284.667f, 604.0f);
                Destroy();
            }
        }
        break;

    // ── P2_INTRO ─────────────────────────────────────────────────────────────
    case Boss1State::P2_INTRO:
        position.setX(undergroundX_);
        position.setY(floorY_);
        if (stateTimer_ >= P2_INTRO_DURATION)
            TransitionTo(Boss1State::P2_WALL_MOVE);
        break;

    // ── P2_WALL_MOVE ──────────────────────────────────────────────────────────
    case Boss1State::P2_WALL_MOVE:
    {
        float step = P2_WALL_SPEED * (dt / 1000.0f);
        undergroundX_ += p2MoveDir_ * step;
        position.setX(undergroundX_);
        position.setY(floorY_);

        // Instant kill — player walks into the strip (horizontal + vertical overlap)
        if (contactDamageCooldown_ > 0.0f) contactDamageCooldown_ -= dt;
        if (scn.player && scn.player->pbody && contactDamageCooldown_ <= 0.0f)
        {
            int px, py;
            scn.player->pbody->GetPosition(px, py);
            bool inX = fabsf((float)px - undergroundX_) < P2_KILL_HALF_W;
            // Check player feet (center + half-height) overlap the strip
            bool inY = (float)py + PLAYER_BODY_HALF_H >= floorY_ - (float)BODY_H_OPEN;
            if (inX && inY)
            {
                scn.player->TakeDamage(scn.player->health);
                contactDamageCooldown_ = CONTACT_DAMAGE_INTERVAL;
            }
        }

        // Wall reached?
        float targetWall = (p2MoveDir_ > 0.0f) ? p2WallRight_ : p2WallLeft_;
        if (fabsf(undergroundX_ - targetWall) < P2_WALL_THRESHOLD)
        {
            undergroundX_ = targetWall;
            position.setX(undergroundX_);
            p2WallCrosses_++;

            if (p2WallCrosses_ >= p2NextSurface_)
            {
                // Return to P1-style surface attack (faster timings)
                p2WallCrosses_ = 0;
                p2NextSurface_ = 7 + rand() % 2;
                TransitionTo(Boss1State::UNDERGROUND);
            }
            else
            {
                // Spit every time it reaches a wall — P2_SPIT flips p2MoveDir_
                // after firing, so it naturally alternates between both walls.
                TransitionTo(Boss1State::P2_SPIT);
            }
        }

        break;
    }

    // ── P2_SPIT ───────────────────────────────────────────────────────────────
    case Boss1State::P2_SPIT:
    {
        position.setX(undergroundX_);
        position.setY(floorY_);

        // After the pause, fire once — always away from the wall, into the arena.
        // p2MoveDir_ still holds the direction the boss was moving when it hit the
        // wall (it only flips below, after firing), so -p2MoveDir_ points inward —
        // this naturally alternates sides as the boss alternates walls.
        if (stateTimer_ >= P2_SPIT_PAUSE && !p2SpitBody_)
        {
            SpawnSpit(-p2MoveDir_);
        }
        // Brief moment after firing → flip direction, resume wall move
        if (stateTimer_ >= P2_SPIT_PAUSE + 500.0f)
        {
            p2MoveDir_ = -p2MoveDir_;
            TransitionTo(Boss1State::P2_WALL_MOVE);
        }
        break;
    }
    }
}

// ─── Draw ─────────────────────────────────────────────────────────────────────
// Placeholder: colored rectangles communicate state visually during development.
// Replace this method entirely when the final sprite sheet is ready.

void Boss1::Draw(float dt)
{
    auto& render = *Engine::GetInstance().render;

    if (state_ == Boss1State::WAITING) return;

    SDL_FlipMode flip = facingRight_ ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

    // ── P2 overlays: puddle and spit ─────────────────────────────────────────
    int sc = Engine::GetInstance().window->GetScale();
    auto worldToScreen = [&](int wx, int wy, int w, int h) -> SDL_FRect {
        return { (float)(render.camera.x + wx * sc),
                 (float)(render.camera.y + wy * sc),
                 (float)(w * sc), (float)(h * sc) };
    };

    if (p2PuddleBody_ && texPuddle_)
    {
        int ppx, ppy;
        p2PuddleBody_->GetPosition(ppx, ppy);
        static constexpr int PUD_DW = 130, PUD_DH = 20;
        // Center sprite on the floor surface
        int drawY = ppy + P2_PUDDLE_H / 2 - PUD_DH / 2;
        SDL_SetTextureBlendMode(texPuddle_, SDL_BLENDMODE_BLEND);
        SDL_FRect dst = worldToScreen(ppx - PUD_DW / 2, drawY, PUD_DW, PUD_DH);
        SDL_RenderTexture(render.renderer, texPuddle_, nullptr, &dst);
    }
    if (p2SpitBody_ && texSpit_)
    {
        int spx, spy;
        p2SpitBody_->GetPosition(spx, spy);
        static constexpr int SP_DW = 28, SP_DH = 65;

        // Rotate sprite to follow velocity direction
        b2Vec2 vel = b2Body_GetLinearVelocity(p2SpitBody_->body);
        // Texture natural orientation is DOWN; subtract 90° to align with velocity
        float angle = atan2f(vel.y, vel.x) * 57.2957795f - 90.0f;

        SDL_SetTextureBlendMode(texSpit_, SDL_BLENDMODE_BLEND);
        SDL_FRect dst = worldToScreen(spx - SP_DW / 2, spy - SP_DH / 2, SP_DW, SP_DH);
        SDL_FPoint center = { (float)(SP_DW * sc) / 2.0f, (float)(SP_DH * sc) / 2.0f };
        SDL_RenderTextureRotated(render.renderer, texSpit_, nullptr, &dst, angle, &center, SDL_FLIP_NONE);
    }

    // ── Helper lambdas — center on undergroundX_, bottom anchored ────────────
    static constexpr int   GROUND_OFFSET   = 12;
    static constexpr float SPRITE_SCALE    = 0.85f;  // Fase 1 sheets are now uniformly 256×256
    static constexpr float SPRITE_WIDTH_MUL = 1.15f; // extra horizontal stretch on top of SPRITE_SCALE

    auto drawUnder = [&](SDL_Texture* tex, Animation& anim, float scale = 1.0f, float widthMul = 1.0f) {
        if (!tex) return;
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        anim.Update(dt);
        const SDL_Rect& f = anim.GetCurrentFrame();
        if (f.w == 0 || f.h == 0) return;
        float scaleX = scale * widthMul;
        int sw = (int)(f.w * scaleX);
        int sh = (int)(f.h * scale);
        render.DrawTexture(tex, (int)undergroundX_ - sw / 2,
                           (int)floorY_ - sh + GROUND_OFFSET, &f, 1.0f, -1.0f, 0, INT_MAX, INT_MAX, flip, scaleX, scale);
    };
    auto drawSurf = [&](SDL_Texture* tex, Animation& anim, float bottomY, float scale = 1.0f, float widthMul = 1.0f) {
        if (!tex) return;
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        anim.Update(dt);
        const SDL_Rect& f = anim.GetCurrentFrame();
        if (f.w == 0 || f.h == 0) return;
        float scaleX = scale * widthMul;
        int sw = (int)(f.w * scaleX);
        int sh = (int)(f.h * scale);
        render.DrawTexture(tex, (int)undergroundX_ - sw / 2,
                           (int)bottomY - sh + GROUND_OFFSET, &f, 1.0f, -1.0f, 0, INT_MAX, INT_MAX, flip, scaleX, scale);
    };

    // ── P2_INTRO ──────────────────────────────────────────────────────────────
    if (state_ == Boss1State::P2_INTRO)
    {
        drawUnder(texMove_, animMove_);
        return;
    }

    // ── P2_WALL_MOVE / P2_SPIT ────────────────────────────────────────────────
    if (state_ == Boss1State::P2_WALL_MOVE ||
        state_ == Boss1State::P2_SPIT)
    {
        // Play the spit frames once; once they finish, resume the regular looping
        // sprite animation instead of freezing on the last spit frame.
        if (state_ == Boss1State::P2_SPIT && !animP2Spit_.HasFinishedOnce())
            drawUnder(texP2Move_, animP2Spit_, 0.25f);
        else
            drawUnder(texP2Move_, animP2Move_, 0.25f);

        if (Engine::GetInstance().physics->IsDebug())
        {
            SDL_Rect kz = { (int)undergroundX_ - (int)P2_KILL_HALF_W,
                            (int)(floorY_ - BODY_H_OPEN),
                            (int)(P2_KILL_HALF_W * 2.0f), BODY_H_OPEN };
            render.DrawRectangle(kz, 255, 0, 0, 160, false, true);
        }
        return;
    }

    // ── UNDERGROUND ───────────────────────────────────────────────────────────
    if (state_ == Boss1State::UNDERGROUND)
    {
        drawUnder(texMove_, animMove_, SPRITE_SCALE, SPRITE_WIDTH_MUL);
        return;
    }

    // ── JUMP — Salir del suelo (desde el suelo, sin salto) ────────────────────
    if (state_ == Boss1State::JUMP)
    {
        drawUnder(texAttack_, animAttack_, SPRITE_SCALE, SPRITE_WIDTH_MUL);
        return;
    }

    // ── DIVE — Sumergirse animation ────────────────────────────────────────────
    if (state_ == Boss1State::DIVE)
    {
        drawUnder(texDive_, animDive_, SPRITE_SCALE, SPRITE_WIDTH_MUL);
        return;
    }

    // ── DEATH — reutiliza Sumergirse (se hunde) ────────────────────────────────
    if (state_ == Boss1State::DEATH)
    {
        drawUnder(texDive_, animDive_, SPRITE_SCALE, SPRITE_WIDTH_MUL);
        return;
    }

    // ── VULNERABLE / STUNNED (pbody existe) ───────────────────────────────────
    if (!pbody) return;
    int bx, by;
    pbody->GetPosition(bx, by);
    float bodyBottom = (float)by + BODY_H * 0.5f;
    undergroundX_ = (float)bx;

    if (state_ == Boss1State::STUNNED)
        drawSurf(texHit_, animHit_, bodyBottom, SPRITE_SCALE, SPRITE_WIDTH_MUL);
    else // VULNERABLE — Cansado: 5-frame intro plays once, then loops while exposed
    {
        if (!animCansado_.HasFinishedOnce())
            drawSurf(texCansado_, animCansado_, bodyBottom, SPRITE_SCALE, SPRITE_WIDTH_MUL);
        else
            drawSurf(texCansadoLoop_, animCansadoLoop_, bodyBottom, SPRITE_SCALE, SPRITE_WIDTH_MUL);
    }
}
