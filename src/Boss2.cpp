#include "Boss2.h"
#include "Engine.h"
#include "Physics.h"
#include "Render.h"
#include "Scene.h"
#include "Textures.h"
#include "Window.h"
#include "Audio.h"
#include "Log.h"

// ─────────────────────────────────────────────────────────────────────────────

Boss2::Boss2() : Boss(EntityType::BOSS_2)
{
    name      = "Boss2";
    health    = 6;
    maxHealth = 6;
    texW      = BOX_W;
    texH      = BOX_H;
}

Boss2::~Boss2() {}

// ─── Start / CleanUp ─────────────────────────────────────────────────────────

bool Boss2::Start()
{
    SpawnBoxBody();
    nextExposeCycle_ = 2 + (rand() % 2);
    triggerRadius_   = 450.0f;

    auto& tex = *Engine::GetInstance().textures;
    const std::string DIR = "assets/textures/spritesheets/SS_Enemics_Nivell3/";

    texSale_ = tex.Load((DIR + "SP_Music_Box_Salircaja_Fase1.png").c_str());
    for (int i = 0; i < 10; i++) animSale_.AddFrame({i * 256, 0, 256, 256}, 80);
    animSale_.SetLoop(false);

    texIdle_ = tex.Load((DIR + "SP_Music_Box_Idle_Fase1.png").c_str());
    for (int i = 0; i < 12; i++) animIdle_.AddFrame({i * 256, 0, 256, 256}, 80);
    animIdle_.SetLoop(true);

    texPunch_ = tex.Load((DIR + "SP_Music_Box_GolpeHorizontal_Fase1.png").c_str());
    for (int i = 0; i < 13; i++) animPunch_.AddFrame({i * 256, 0, 256, 256}, 70);
    animPunch_.SetLoop(false);

    texRetract_ = tex.Load((DIR + "SP_Music_Box_RecogeGolpe2_Fase1.png").c_str());
    for (int i = 0; i < 20; i++) animRetract_.AddFrame({i * 256, 0, 256, 256}, 60);
    animRetract_.SetLoop(false);

    texCansado_ = tex.Load((DIR + "SP_Music_Box_Cansado_Fase1.png").c_str());
    for (int i = 0; i < 9; i++) animCansado_.AddFrame({i * 256, 0, 256, 256}, 80);
    animCansado_.SetLoop(false);

    texRest_ = tex.Load((DIR + "SP_Music_Box_Cansado_Loop_Fase1.png").c_str());
    for (int i = 0; i < 8; i++) animRest_.AddFrame({i * 256, 0, 256, 256}, 80);
    animRest_.SetLoop(true);

    texTapaBtn_ = tex.Load((DIR + "SP_Music_Box_VolverCaja_Fase1.png").c_str());
    for (int i = 0; i < 7; i++) animTapaBtn_.AddFrame({i * 256, 0, 256, 256}, 100);
    animTapaBtn_.SetLoop(false);

    texHit_ = tex.Load((DIR + "SP_Music_Box_Hit_Fase1.png").c_str());
    for (int i = 0; i < 11; i++) animHit_.AddFrame({i * 256, 0, 256, 256}, 80);
    animHit_.SetLoop(false);

    // ── Phase 2 (512×512) ────────────────────────────────────────────────────
    texSale2_ = tex.Load((DIR + "SP_MusicBox_salircaja_fase2.png").c_str());
    for (int i = 0; i < 10; i++) animSale2_.AddFrame({i * 512, 0, 512, 512}, 80);
    animSale2_.SetLoop(false);

    texIdle2_ = tex.Load((DIR + "SP_MusicBox_Idle _fase2.png").c_str());
    for (int i = 0; i < 12; i++) animIdle2_.AddFrame({i * 512, 0, 512, 512}, 80);
    animIdle2_.SetLoop(true);

    texCansado2_ = tex.Load((DIR + "SP_MusicBox_Cansado_Fase2.png").c_str());
    for (int i = 0; i < 9; i++) animCansado2_.AddFrame({i * 512, 0, 512, 512}, 80);
    animCansado2_.SetLoop(false);

    texRest2_ = tex.Load((DIR + "SP_MusicBox_Cansadoloop_fase2.png").c_str());
    for (int i = 0; i < 12; i++) animRest2_.AddFrame({i * 512, 0, 512, 512}, 80);
    animRest2_.SetLoop(true);

    texTapaBtn2_ = tex.Load((DIR + "SP_MusicBox_vuelvecaja_fase2.png").c_str());
    for (int i = 0; i < 9; i++) animTapaBtn2_.AddFrame({i * 512, 0, 512, 512}, 100);
    animTapaBtn2_.SetLoop(false);

    texGroundSlam_ = tex.Load((DIR + "SP_MusixBox_golpesuelo_fase2.png").c_str());
    for (int i = 0; i < 14; i++) animGroundSlam_.AddFrame({i * 512, 0, 512, 512}, 70);
    animGroundSlam_.SetLoop(false);

    texGroundRetract_ = tex.Load((DIR + "SP_MusicBox_recoge_golpesuielo_2_fase2.png").c_str());
    for (int i = 0; i < 14; i++) animGroundRetract_.AddFrame({i * 512, 0, 512, 512}, 60);
    animGroundRetract_.SetLoop(false);

    // Phase 2 horizontal punch — reuses ground-slam textures
    for (int i = 0; i < 14; i++) animPunch2_.AddFrame({i * 512, 0, 512, 512}, 70);
    animPunch2_.SetLoop(false);
    for (int i = 0; i < 10; i++) animRetract2_.AddFrame({i * 512, 0, 512, 512}, 60);
    animRetract2_.SetLoop(false);

    texHit2_ = tex.Load((DIR + "SP__MusicBox_Hit_Fase2.png").c_str());
    for (int i = 0; i < 11; i++) animHit2_.AddFrame({i * 512, 0, 512, 512}, 80);
    animHit2_.SetLoop(false);

    texDeath2_ = tex.Load((DIR + "SP_MusicBox_Die_Fase2.png").c_str());
    for (int i = 0; i < 20; i++) animDeath2_.AddFrame({i * 512, 0, 512, 512}, 80);
    animDeath2_.SetLoop(false);

    // ── Phase 2 texture load check ────────────────────────────────────────────
    LOG("P2 textures: Sale2=%s Idle2=%s Cansado2=%s Rest2=%s TapaBtn2=%s Slam=%s Retract=%s Hit2=%s Death2=%s",
        texSale2_        ? "OK" : "NULL",
        texIdle2_        ? "OK" : "NULL",
        texCansado2_     ? "OK" : "NULL",
        texRest2_        ? "OK" : "NULL",
        texTapaBtn2_     ? "OK" : "NULL",
        texGroundSlam_   ? "OK" : "NULL",
        texGroundRetract_? "OK" : "NULL",
        texHit2_         ? "OK" : "NULL",
        texDeath2_       ? "OK" : "NULL");

    // ── Shared ────────────────────────────────────────────────────────────────
    texShockwave_ = tex.Load("assets/textures/AS_props/SS_efecto_onda.png");
    texWaveIdle_  = tex.Load("assets/textures/AS_props/SS_onda_idle.png");
    texButton_    = tex.Load("assets/textures/spritesheets/SS_Enemics_Nivell3/Boton V3.png");
    for (int i = 0; i < 13; i++) animShockwave_.AddFrame({i * 200, 0, 200, 500}, 60);
    animShockwave_.SetLoop(false);

    // ── Audio ─────────────────────────────────────────────────────────────────
    auto& audio = *Engine::GetInstance().audio;
    fxIntro_     = audio.LoadFx("assets/audio/Enemies/Music Box/MusicBox_Intro.wav");
    fxIdleLoop_  = audio.LoadFx("assets/audio/Enemies/Music Box/MusicBox_Idlle-loop.wav");
    fxPunch_     = audio.LoadFx("assets/audio/Enemies/Music Box/MusicBox_Punch.wav");
    fxShockwave_ = audio.LoadFx("assets/audio/Enemies/Music Box/MusicBox_Shockwave-loop.wav");
    fxExposeBtn_ = audio.LoadFx("assets/audio/Enemies/Music Box/MusicBox_Expose-button.wav");
    fxStunned_   = audio.LoadFx("assets/audio/Enemies/Music Box/MusicBox_Stunned.wav");
    fxDeath_     = audio.LoadFx("assets/audio/Enemies/Music Box/MusicBox_Death.wav");
    fxHit_       = audio.LoadFx("assets/audio/Enemies/Music Box/MusicBox_Hit.wav");

    return true;
}

bool Boss2::CleanUp()
{
    DestroyPunchHitbox();
    DestroyShockwave();
    DestroySlamHitbox();
    DestroyTallWave();
    DestroyButtonSensor();
    if (pbody)
    {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }
    return true;
}

// ─── Physics helpers ─────────────────────────────────────────────────────────

void Boss2::SpawnBoxBody()
{
    if (pbody) return;
    // position = top-left of Tiled object; center the body
    int cx = (int)position.getX() + BOX_W / 2;
    int cy = (int)position.getY() + BOX_H / 2;
    pbody = Engine::GetInstance().physics->CreateRectangle(
        cx, cy, 70, 80, bodyType::STATIC, 0.8f);
    pbody->listener = this;
    pbody->ctype    = ColliderType::UNKNOWN; // box itself isn't a damage source
}

void Boss2::SpawnPunchHitbox()
{
    if (punchBody_) return;
    int bx, by;
    pbody->GetPosition(bx, by);
    // Fist starts at the box edge — moves each frame to follow the arm tip
    int hx = bx + (int)(attackDir_ * BOX_W / 2.0f);
    int hy = by;
    punchBody_ = Engine::GetInstance().physics->CreateRectangleSensor(
        hx, hy, 44, (int)PUNCH_H_SIZE, bodyType::STATIC);
    punchBody_->listener = this;
    punchBody_->ctype    = ColliderType::ENEMY;
}

void Boss2::DestroyPunchHitbox()
{
    if (!punchBody_) return;
    Engine::GetInstance().physics->DeletePhysBody(punchBody_);
    punchBody_ = nullptr;
}

void Boss2::SpawnShockwave()
{
    if (shockwaveBody_) return;
    int bx, by;
    pbody->GetPosition(bx, by);
    // Starts at the floor-level edge of the box, travels toward the player
    static constexpr int GROUND_OFFSET = 50;
    int sx  = bx + (int)(attackDir_ * BOX_W / 2.0f);
    int physY = by + BOX_H / 2 - SHOCKWAVE_H / 2; // physics body at actual ground
    int drawY = by + BOX_H / 2 + GROUND_OFFSET;   // visual ground (matches sprite offset)
    shockwaveBody_ = Engine::GetInstance().physics->CreateRectangleSensor(
        sx, physY, SHOCKWAVE_W, SHOCKWAVE_H, bodyType::STATIC);
    shockwaveBody_->listener = this;
    shockwaveBody_->ctype    = ColliderType::ENEMY;
    shockwaveX_          = (float)sx;
    shockwaveY_          = (float)drawY;
    shockwavePhysY_      = (float)physY;
    shockwaveStart_      = (float)sx;
    shockwaveSpeed_      = (phase_ >= 2) ? SHOCKWAVE_SPEED_P2 : SHOCKWAVE_SPEED;
    shockwaveActive_     = true;
    shockwaveFrame_      = 0;
    shockwaveIdleFrame_  = 0;
    shockwaveFrameTimer_ = 0.0f;
    shockwaveOutro_      = false;
    Engine::GetInstance().audio->PlayFxSpatial(fxShockwave_, position);
}

void Boss2::UpdateShockwave(float dt)
{
    if (!shockwaveActive_ || !shockwaveBody_) return;

    static constexpr int   HOLD_FRAME   = 7;
    static constexpr float FRAME_DUR    = 60.0f;
    static constexpr int   TOTAL_FRAMES = 13;
    float outroDist = (TOTAL_FRAMES - HOLD_FRAME - 1) * FRAME_DUR * shockwaveSpeed_ / 1000.0f;

    shockwaveX_ += attackDir_ * shockwaveSpeed_ * (dt / 1000.0f);
    shockwaveBody_->SetPosition((int)shockwaveX_, (int)shockwavePhysY_);
    float dist = fabsf(shockwaveX_ - shockwaveStart_);

    shockwaveFrameTimer_ += dt;

    if (!shockwaveOutro_)
    {
        if (shockwaveFrameTimer_ >= FRAME_DUR)
        {
            shockwaveFrameTimer_ -= FRAME_DUR;
            if (shockwaveFrame_ < HOLD_FRAME)
                shockwaveFrame_++;
            else
                shockwaveIdleFrame_ = (shockwaveIdleFrame_ + 1) % 6; // loop idle anim
        }
        if (dist >= SHOCKWAVE_RANGE - outroDist)
        {
            shockwaveOutro_      = true;
            shockwaveFrameTimer_ = 0.0f;
            shockwaveFrame_      = HOLD_FRAME + 1;
        }
    }
    else
    {
        // Play frames 9→12, then destroy
        if (shockwaveFrameTimer_ >= FRAME_DUR)
        {
            shockwaveFrameTimer_ -= FRAME_DUR;
            shockwaveFrame_++;
            if (shockwaveFrame_ >= TOTAL_FRAMES)
                DestroyShockwave();
        }
    }
}

void Boss2::DestroyShockwave()
{
    if (!shockwaveBody_) return;
    Engine::GetInstance().physics->DeletePhysBody(shockwaveBody_);
    shockwaveBody_  = nullptr;
    shockwaveActive_ = false;
}

void Boss2::SpawnSlamHitbox()
{
    if (slamBody_) return;
    int bx, by;
    pbody->GetPosition(bx, by);
    int hx = bx + (int)(attackDir_ * (BOX_W / 2 + 40));
    int hy = by + BOX_H / 2;
    slamBody_ = Engine::GetInstance().physics->CreateRectangleSensor(
        hx, hy, 56, 95, bodyType::STATIC);
    slamBody_->listener = this;
    slamBody_->ctype    = ColliderType::ENEMY;
}

void Boss2::DestroySlamHitbox()
{
    if (!slamBody_) return;
    Engine::GetInstance().physics->DeletePhysBody(slamBody_);
    slamBody_ = nullptr;
}

void Boss2::SpawnTallWave()
{
    if (tallWaveBody_) return;
    int bx, by;
    pbody->GetPosition(bx, by);
    int sx    = bx + (int)(attackDir_ * BOX_W / 2.0f);
    int physY = by + BOX_H / 2 - TALL_WAVE_H / 2;
    static constexpr int GROUND_OFFSET = 50;
    int drawY = by + BOX_H / 2 + GROUND_OFFSET;
    tallWaveBody_ = Engine::GetInstance().physics->CreateRectangleSensor(
        sx, physY, TALL_WAVE_W, TALL_WAVE_H, bodyType::STATIC);
    tallWaveBody_->listener = this;
    tallWaveBody_->ctype    = ColliderType::ENEMY;
    tallWaveX_          = (float)sx;
    tallWaveY_          = (float)drawY;
    tallWavePhysY_      = (float)physY;
    tallWaveStart_      = (float)sx;
    tallWaveActive_     = true;
    tallWaveFrame_      = 0;
    tallWaveIdleFrame_  = 0;
    tallWaveFrameTimer_ = 0.0f;
    tallWaveOutro_      = false;
    Engine::GetInstance().audio->PlayFxSpatial(fxShockwave_, position);
}

void Boss2::UpdateTallWave(float dt)
{
    if (!tallWaveActive_ || !tallWaveBody_) return;

    static constexpr int   HOLD_FRAME   = 7;
    static constexpr float FRAME_DUR    = 60.0f;
    static constexpr int   TOTAL_FRAMES = 13;
    float outroDist = (TOTAL_FRAMES - HOLD_FRAME - 1) * FRAME_DUR * TALL_WAVE_SPEED / 1000.0f;

    tallWaveX_ += attackDir_ * TALL_WAVE_SPEED * (dt / 1000.0f);
    tallWaveBody_->SetPosition((int)tallWaveX_, (int)tallWavePhysY_);
    float dist = fabsf(tallWaveX_ - tallWaveStart_);

    tallWaveFrameTimer_ += dt;

    if (!tallWaveOutro_)
    {
        if (tallWaveFrameTimer_ >= FRAME_DUR)
        {
            tallWaveFrameTimer_ -= FRAME_DUR;
            if (tallWaveFrame_ < HOLD_FRAME)
                tallWaveFrame_++;
            else
                tallWaveIdleFrame_ = (tallWaveIdleFrame_ + 1) % 6; // loop idle anim
        }
        if (dist >= SHOCKWAVE_RANGE - outroDist)
        {
            tallWaveOutro_      = true;
            tallWaveFrameTimer_ = 0.0f;
            tallWaveFrame_      = HOLD_FRAME + 1;
        }
    }
    else
    {
        if (tallWaveFrameTimer_ >= FRAME_DUR)
        {
            tallWaveFrameTimer_ -= FRAME_DUR;
            tallWaveFrame_++;
            if (tallWaveFrame_ >= TOTAL_FRAMES)
                DestroyTallWave();
        }
    }
}

void Boss2::DestroyTallWave()
{
    if (!tallWaveBody_) return;
    Engine::GetInstance().physics->DeletePhysBody(tallWaveBody_);
    tallWaveBody_   = nullptr;
    tallWaveActive_ = false;
}

void Boss2::SpawnButtonSensor()
{
    if (buttonBody_) return;
    int bx, by;
    pbody->GetPosition(bx, by);
    int btnX = bx + (int)(attackDir_ * (phase_ >= 2 ? 40 : 33));
    int btnY = by + (phase_ >= 2 ? 43 : 55);
    buttonBody_ = Engine::GetInstance().physics->CreateRectangleSensor(
        btnX, btnY, BUTTON_W, BUTTON_H, bodyType::STATIC);
    buttonBody_->listener = this;
    buttonBody_->ctype    = ColliderType::ENEMY;
}

void Boss2::DestroyButtonSensor()
{
    if (!buttonBody_) return;
    Engine::GetInstance().physics->DeletePhysBody(buttonBody_);
    buttonBody_ = nullptr;
}

// ─── State transitions ────────────────────────────────────────────────────────

void Boss2::TransitionTo(Boss2State newState)
{
    state_      = newState;
    stateTimer_ = 0.0f;

    if (phase_ >= 2)
    {
        switch (newState)
        {
        case Boss2State::INTRO:         animSale2_.Reset();                             break;
        case Boss2State::IDLE:          animIdle2_.Reset(); animIdle2_.SetLoop(true);   break;
        case Boss2State::CHARGE:        animIdle2_.Reset(); animIdle2_.SetLoop(true);   break;
        case Boss2State::PUNCH:         animPunch_.Reset();                             break;
        case Boss2State::RETRACT:       animRetract_.Reset();                           break;
        case Boss2State::REST:          animIdle2_.Reset();                             break;
        case Boss2State::EXPOSE_BUTTON: animCansado2_.Reset(); animRest2_.Reset();      break;
        case Boss2State::CLOSE_BUTTON:  animTapaBtn2_.Reset();                         break;
        case Boss2State::STUNNED:       animHit2_.Reset();                             break;
        case Boss2State::DEATH:         animDeath2_.Reset();                           break;
        case Boss2State::GROUND_SLAM:   animGroundSlam_.Reset();                       break;
        case Boss2State::GROUND_PUNCH:  /* animGroundSlam_ holds last frame */           break;
        case Boss2State::GROUND_RETRACT:animGroundRetract_.Reset();                    break;
        case Boss2State::PHASE_TRANSITION:
            transSubPhase_ = 0;
            animTapaBtn_.Reset();
            break;
        default: break;
        }
    }
    else
    {
        switch (newState)
        {
        case Boss2State::INTRO:         animSale_.Reset();                          break;
        case Boss2State::IDLE:          animIdle_.Reset(); animIdle_.SetLoop(true); break;
        case Boss2State::CHARGE:        animIdle_.Reset(); animIdle_.SetLoop(true); break;
        case Boss2State::PUNCH:         animPunch_.Reset();                         break;
        case Boss2State::RETRACT:       animRetract_.Reset();                       break;
        case Boss2State::REST:          animIdle_.Reset();                          break;
        case Boss2State::EXPOSE_BUTTON: animCansado_.Reset(); animRest_.Reset();    break;
        case Boss2State::CLOSE_BUTTON:  animTapaBtn_.Reset();                       break;
        case Boss2State::STUNNED:       animHit_.Reset();                           break;
        case Boss2State::DEATH:         animHit_.Reset();                           break;
        case Boss2State::GROUND_SLAM:   animIdle_.Reset(); animIdle_.SetLoop(true); break;
        case Boss2State::GROUND_PUNCH:  animIdle_.Reset();                          break;
        case Boss2State::GROUND_RETRACT:animIdle_.Reset();                          break;
        default: break;
        }
    }

    // Audio
    auto& audio = *Engine::GetInstance().audio;
    switch (newState)
    {
    case Boss2State::INTRO:         audio.PlayFxSpatial(fxIntro_, position);     break;
    case Boss2State::PUNCH:         audio.PlayFxSpatial(fxPunch_, position);     break;
    case Boss2State::GROUND_SLAM:   audio.PlayFxSpatial(fxPunch_, position);     break;
    case Boss2State::EXPOSE_BUTTON: audio.PlayFxSpatial(fxExposeBtn_, position); break;
    case Boss2State::STUNNED:       audio.PlayFxSpatial(fxStunned_, position);   break;
    case Boss2State::DEATH:         audio.PlayFxSpatial(fxDeath_, position);     break;
    default: break;
    }
}

// ─── TakeDamage ──────────────────────────────────────────────────────────────

void Boss2::TakeDamage(int damage)
{
    if (state_ != Boss2State::EXPOSE_BUTTON) return;

    Engine::GetInstance().audio->PlayFxSpatial(fxHit_, position);

    health -= damage;
    if (health < 0) health = 0;

    int prevPhase = phase_;
    CheckPhaseTransition();

    if (health <= 0)
    {
        isDead_ = true;
        DestroyPunchHitbox();
        DestroyShockwave();
        DestroyButtonSensor();
        TransitionTo(Boss2State::DEATH);
    }
    else if (phase_ > prevPhase)
    {
        DestroyButtonSensor();
        TransitionTo(Boss2State::PHASE_TRANSITION);
    }
    else
    {
        DestroyButtonSensor();
        TransitionTo(Boss2State::STUNNED);
    }
}

// ─── Collision ────────────────────────────────────────────────────────────────

void Boss2::OnCollision(PhysBody* physA, PhysBody* physB)
{
    if (physA == punchBody_)
    {
        if (physB->ctype == ColliderType::PLAYER && physB->listener)
            physB->listener->TakeDamage(1);
    }
    else if (physA == shockwaveBody_)
    {
        if (physB->ctype == ColliderType::PLAYER && physB->listener)
            physB->listener->TakeDamage(1);
    }
    else if (physA == slamBody_)
    {
        if (physB->ctype == ColliderType::PLAYER && physB->listener)
            physB->listener->TakeDamage(1);
    }
    else if (physA == tallWaveBody_)
    {
        if (physB->ctype == ColliderType::PLAYER && physB->listener)
            physB->listener->TakeDamage(1);
    }
    else if (physA == buttonBody_)
    {
        bool isHit = physB->ctype == ColliderType::ATTACK || physB->ctype == ColliderType::SLINGSHOT_PROJ;
        if (isHit && state_ == Boss2State::EXPOSE_BUTTON)
            TakeDamage(1);
    }
}

// ─── Phase change ─────────────────────────────────────────────────────────────

void Boss2::OnPhaseChange(int newPhase)
{
    LOG("Boss2 entering phase %d!", newPhase);
    attackCycle_     = 0;
    nextExposeCycle_ = 1 + (rand() % 2); // phase 2: 1 or 2 horizontal attacks before slam
}

// ─── Update (overrides Boss::Update — static box, no velocity) ───────────────

bool Boss2::Update(float dt)
{
    if (Engine::GetInstance().scene->isPaused_ || Engine::GetInstance().scene->isGameOver_)
    {
        Draw(0.0f);
        return true;
    }

    UpdateFSM(dt);
    UpdateShockwave(dt);
    UpdateTallWave(dt);
    Draw(dt);
    return true;
}

// ─── FSM ─────────────────────────────────────────────────────────────────────

void Boss2::UpdateFSM(float dt)
{
    stateTimer_ += dt;

    switch (state_)
    {
    // ── WAITING ──────────────────────────────────────────────────────────────
    case Boss2State::WAITING:
        if (CheckProximityTrigger())
        {
            isEngaged_ = true;
            // Determine attack direction from player position (fixed for the whole fight)
            if (pbody)
            {
                int bx, by;
                pbody->GetPosition(bx, by);
                Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
                attackDir_ = (playerPos.getX() > (float)bx) ? 1.0f : -1.0f;
            }
            LOG("Boss2 fight started! attackDir=%.0f", attackDir_);
            TransitionTo(Boss2State::INTRO);
        }
        break;

    // ── INTRO ────────────────────────────────────────────────────────────────
    case Boss2State::INTRO:
        if (animSale_.HasFinishedOnce())
            TransitionTo(Boss2State::IDLE);
        break;

    // ── IDLE ─────────────────────────────────────────────────────────────────
    case Boss2State::IDLE:
        if (stateTimer_ >= IDLE_DURATION)
        {
            // Recalculate direction toward player before each attack
            if (pbody)
            {
                int bx, by;
                pbody->GetPosition(bx, by);
                Vector2D pp = Engine::GetInstance().scene->GetPlayerPosition();
                attackDir_ = (pp.getX() > (float)bx) ? 1.0f : -1.0f;
            }
            TransitionTo(Boss2State::CHARGE);
        }
        break;

    // ── CHARGE ───────────────────────────────────────────────────────────────
    case Boss2State::CHARGE:
    {
        float chargeDur = (phase_ >= 2) ? CHARGE_DURATION_P2 : CHARGE_DURATION;
        if (stateTimer_ >= chargeDur)
        {
            SpawnPunchHitbox();
            TransitionTo(Boss2State::PUNCH);
        }
        break;
    }

    // ── PUNCH ─────────────────────────────────────────────────────────────────
    case Boss2State::PUNCH:
    {
        // Move fist hitbox to the arm tip each frame
        if (punchBody_ && pbody)
        {
            int bx, by;
            pbody->GetPosition(bx, by);
            float progress = std::min(stateTimer_ / PUNCH_DURATION, 1.0f);
            float armNow   = PUNCH_REACH * progress;
            int hx = bx + (int)(attackDir_ * (BOX_W / 2.0f + armNow));
            punchBody_->SetPosition(hx, by);
        }
        if (stateTimer_ >= PUNCH_DURATION)
        {
            DestroyPunchHitbox();
            TransitionTo(Boss2State::RETRACT);
        }
        break;
    }

    // ── RETRACT ──────────────────────────────────────────────────────────────
    case Boss2State::RETRACT:
        if (stateTimer_ >= RETRACT_DURATION)
        {
            SpawnShockwave();
            TransitionTo(Boss2State::REST);
        }
        break;

    // ── REST ─────────────────────────────────────────────────────────────────
    case Boss2State::REST:
    {
        float restDur = (phase_ >= 2) ? REST_DURATION_P2 : REST_DURATION;
        if (stateTimer_ >= restDur)
        {
            attackCycle_++;
            if (attackCycle_ >= nextExposeCycle_)
            {
                attackCycle_     = 0;
                nextExposeCycle_ = (phase_ >= 2) ? 1 + (rand() % 2) : 2 + (rand() % 2);
                if (phase_ >= 2)
                    TransitionTo(Boss2State::GROUND_SLAM); // slam before button in phase 2
                else
                {
                    SpawnButtonSensor();
                    TransitionTo(Boss2State::EXPOSE_BUTTON);
                }
            }
            else
            {
                TransitionTo(Boss2State::IDLE);
            }
        }
        break;
    }

    // ── GROUND_SLAM ──────────────────────────────────────────────────────────
    case Boss2State::GROUND_SLAM:
        if (animGroundSlam_.HasFinishedOnce())
        {
            SpawnSlamHitbox();
            TransitionTo(Boss2State::GROUND_RETRACT);
        }
        break;

    // ── GROUND_PUNCH (no longer active — kept for FSM completeness) ──────────
    case Boss2State::GROUND_PUNCH:
        break;

    // ── GROUND_RETRACT ───────────────────────────────────────────────────────
    case Boss2State::GROUND_RETRACT:
        if (animGroundRetract_.HasFinishedOnce())
        {
            DestroySlamHitbox();
            SpawnTallWave();
            SpawnButtonSensor();
            TransitionTo(Boss2State::EXPOSE_BUTTON);
        }
        break;

    // ── EXPOSE_BUTTON ─────────────────────────────────────────────────────────
    case Boss2State::EXPOSE_BUTTON:
    {
        float exposeDur = (phase_ >= 2) ? EXPOSE_DURATION_P2 : EXPOSE_DURATION;
        if (stateTimer_ >= exposeDur)
        {
            DestroyButtonSensor();
            TransitionTo(Boss2State::IDLE);
        }
        break;
    }

    // ── CLOSE_BUTTON ─────────────────────────────────────────────────────────
    case Boss2State::CLOSE_BUTTON:
        if (stateTimer_ >= CLOSE_DURATION)
            TransitionTo(Boss2State::IDLE);
        break;

    // ── STUNNED ──────────────────────────────────────────────────────────────
    case Boss2State::STUNNED:
        if (stateTimer_ >= STUN_DURATION)
            TransitionTo(Boss2State::IDLE);
        break;

    // ── PHASE_TRANSITION ─────────────────────────────────────────────────────
    case Boss2State::PHASE_TRANSITION:
        if (transSubPhase_ == 0)
        {
            if (animTapaBtn_.HasFinishedOnce())
            {
                transSubPhase_ = 1;
                stateTimer_    = 0.0f;
            }
        }
        else if (transSubPhase_ == 1)
        {
            if (stateTimer_ >= 2000.0f)
            {
                animSale2_.Reset();
                transSubPhase_ = 2;
            }
        }
        else
        {
            if (animSale2_.HasFinishedOnce())
                TransitionTo(Boss2State::IDLE);
        }
        break;

    // ── DEATH ────────────────────────────────────────────────────────────────
    case Boss2State::DEATH:
        if (stateTimer_ >= DEATH_DURATION && !deathSequenceDone_)
        {
            deathTeleportTimer_ += dt;
            if (deathTeleportTimer_ >= DEATH_TELEPORT_DELAY)
            {
                deathSequenceDone_ = true;
                isEngaged_ = false;
                auto& scn = *Engine::GetInstance().scene;
                // Seal the entrance portal (MapLvl3ZonaAlta.tmx, object id 395) — boss is gone, no reason to come back
                scn.SealBossPortal(2948.0f, 3694.64f, 680.0f, 686.728f);
                pendingToDelete = true;
            }
        }
        break;
    }
}

// ─── Draw ─────────────────────────────────────────────────────────────────────

void Boss2::Draw(float dt)
{
    if (state_ == Boss2State::WAITING || !pbody) return;

    auto& render = *Engine::GetInstance().render;
    int sc = Engine::GetInstance().window->GetScale();
    int bx, by;
    pbody->GetPosition(bx, by);

    auto worldToScreen = [&](int wx, int wy, int w, int h) -> SDL_FRect {
        return { (float)(render.camera.x + wx * sc),
                 (float)(render.camera.y + wy * sc),
                 (float)(w * sc), (float)(h * sc) };
    };

    SDL_FlipMode flip = (attackDir_ > 0.0f) ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

    // ── Main sprite ───────────────────────────────────────────────────────────
    SDL_Texture* activeTex = nullptr;
    SDL_FRect    frameRect = {};
    int drawW = 0, drawH = 0;

    auto toFRect = [](const SDL_Rect& r) -> SDL_FRect {
        return { (float)r.x, (float)r.y, (float)r.w, (float)r.h };
    };

    // Phase 1: 256×256 sprites
    static constexpr float SCALE   = 1.2f;
    static constexpr int   SZ      = (int)(256 * SCALE);
    static constexpr int   SZW     = (int)(SZ  * 1.25f);
    // Phase 2: 512×512 sprites — slightly larger than phase 1
    static constexpr float SCALE2  = 0.8f;
    static constexpr int   SZ2     = (int)(512 * SCALE2);
    static constexpr int   SZW2    = (int)(SZ2 * 1.25f);

    const int dW = (phase_ >= 2) ? SZW2 : SZW;
    const int dH = (phase_ >= 2) ? SZ2  : SZ;

    // ── PHASE_TRANSITION drawing ──────────────────────────────────────────────
    if (state_ == Boss2State::PHASE_TRANSITION)
    {
        SDL_Texture* ptTex = nullptr;
        SDL_FRect    ptSrc = {};
        int ptW = SZW, ptH = SZ;
        if (transSubPhase_ <= 1)
        {
            if (transSubPhase_ == 0) animTapaBtn_.Update(dt);
            // sub-phase 1: no update → holds last frame
            ptTex = texTapaBtn_;
            ptSrc = toFRect(animTapaBtn_.GetCurrentFrame());
        }
        else
        {
            animSale2_.Update(dt);
            ptTex = texSale2_;
            ptSrc = toFRect(animSale2_.GetCurrentFrame());
            ptW = SZW2; ptH = SZ2;
        }
        if (ptTex)
        {
            int drawX = bx - ptW / 2;
            int drawY = (by + BOX_H / 2) - ptH + 50;
            SDL_FRect dst = worldToScreen(drawX, drawY, ptW, ptH);
            SDL_RenderTextureRotated(render.renderer, ptTex, &ptSrc, &dst, 0.0, nullptr, flip);
        }
        return;
    }

    if (phase_ >= 2)
    {
        switch (state_)
        {
        case Boss2State::INTRO:
            animSale2_.Update(dt);
            activeTex = texSale2_; frameRect = toFRect(animSale2_.GetCurrentFrame());
            break;
        case Boss2State::IDLE:
        case Boss2State::CHARGE:
        case Boss2State::REST:
            animIdle2_.Update(dt);
            activeTex = texIdle2_; frameRect = toFRect(animIdle2_.GetCurrentFrame());
            break;
        case Boss2State::EXPOSE_BUTTON:
            if (!animCansado2_.HasFinishedOnce()) {
                animCansado2_.Update(dt);
                activeTex = texCansado2_; frameRect = toFRect(animCansado2_.GetCurrentFrame());
            } else {
                animRest2_.Update(dt);
                activeTex = texRest2_; frameRect = toFRect(animRest2_.GetCurrentFrame());
            }
            break;
        case Boss2State::PUNCH:
            animPunch_.Update(dt);
            activeTex = texPunch_; frameRect = toFRect(animPunch_.GetCurrentFrame());
            break;
        case Boss2State::RETRACT:
            animRetract_.Update(dt);
            activeTex = texRetract_; frameRect = toFRect(animRetract_.GetCurrentFrame());
            break;
        case Boss2State::CLOSE_BUTTON:
            animTapaBtn2_.Update(dt);
            activeTex = texTapaBtn2_; frameRect = toFRect(animTapaBtn2_.GetCurrentFrame());
            break;
        case Boss2State::GROUND_SLAM:
        case Boss2State::GROUND_PUNCH:
            animGroundSlam_.Update(dt);
            activeTex = texGroundSlam_; frameRect = toFRect(animGroundSlam_.GetCurrentFrame());
            break;
        case Boss2State::GROUND_RETRACT:
            animGroundRetract_.Update(dt);
            activeTex = texGroundRetract_; frameRect = toFRect(animGroundRetract_.GetCurrentFrame());
            break;
        case Boss2State::STUNNED:
            animHit2_.Update(dt);
            activeTex = texHit2_; frameRect = toFRect(animHit2_.GetCurrentFrame());
            break;
        case Boss2State::DEATH:
            animDeath2_.Update(dt);
            activeTex = texDeath2_; frameRect = toFRect(animDeath2_.GetCurrentFrame());
            break;
        default: break;
        }
    }
    else
    {
        switch (state_)
        {
        case Boss2State::INTRO:
            animSale_.Update(dt);
            activeTex = texSale_; frameRect = toFRect(animSale_.GetCurrentFrame());
            break;
        case Boss2State::IDLE:
        case Boss2State::CHARGE:
        case Boss2State::REST:
        case Boss2State::GROUND_SLAM:
        case Boss2State::GROUND_PUNCH:
        case Boss2State::GROUND_RETRACT:
            animIdle_.Update(dt);
            activeTex = texIdle_; frameRect = toFRect(animIdle_.GetCurrentFrame());
            break;
        case Boss2State::EXPOSE_BUTTON:
            if (!animCansado_.HasFinishedOnce()) {
                animCansado_.Update(dt);
                activeTex = texCansado_; frameRect = toFRect(animCansado_.GetCurrentFrame());
            } else {
                animRest_.Update(dt);
                activeTex = texRest_; frameRect = toFRect(animRest_.GetCurrentFrame());
            }
            break;
        case Boss2State::PUNCH:
            animPunch_.Update(dt);
            activeTex = texPunch_; frameRect = toFRect(animPunch_.GetCurrentFrame());
            break;
        case Boss2State::RETRACT:
            animRetract_.Update(dt);
            activeTex = texRetract_; frameRect = toFRect(animRetract_.GetCurrentFrame());
            break;
        case Boss2State::CLOSE_BUTTON:
            animTapaBtn_.Update(dt);
            activeTex = texTapaBtn_; frameRect = toFRect(animTapaBtn_.GetCurrentFrame());
            break;
        case Boss2State::STUNNED:
        case Boss2State::DEATH:
            animHit_.Update(dt);
            activeTex = texHit_; frameRect = toFRect(animHit_.GetCurrentFrame());
            break;
        default: break;
        }
    }

    drawW = dW; drawH = dH;

    if (activeTex)
    {
        static constexpr int GROUND_OFFSET = 50;
        int drawX = bx - drawW / 2;
        int drawY = (by + BOX_H / 2) - drawH + GROUND_OFFSET;
        SDL_FRect dst = worldToScreen(drawX, drawY, drawW, drawH);
        SDL_RenderTextureRotated(render.renderer, activeTex, &frameRect, &dst,
                                 0.0, nullptr, flip);
    }

    // ── Button sprite ─────────────────────────────────────────────────────────
    if (state_ == Boss2State::EXPOSE_BUTTON && buttonBody_ && texButton_)
    {
        int btnX, btnY;
        buttonBody_->GetPosition(btnX, btnY);
        const int BTN_DRAW_W = (phase_ >= 2) ? 48 : 36;
        const int BTN_DRAW_H = BTN_DRAW_W;
        SDL_FRect dst = worldToScreen(btnX - BTN_DRAW_W / 2, btnY - BTN_DRAW_H / 2, BTN_DRAW_W, BTN_DRAW_H);
        SDL_FlipMode btnFlip = (attackDir_ > 0.0f) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_RenderTextureRotated(render.renderer, texButton_, nullptr, &dst, 0.0, nullptr, btnFlip);
    }

    // ── Tall wave (phase 2) ───────────────────────────────────────────────────
    if (tallWaveActive_)
    {
        static constexpr int FW = 200, FH = 500;
        static constexpr int HOLD_FRAME = 7;
        bool twInHold = !tallWaveOutro_ && tallWaveFrame_ >= HOLD_FRAME;
        SDL_Texture* twTex = (twInHold && texWaveIdle_) ? texWaveIdle_ : texShockwave_;
        int srcFrame = twInHold ? tallWaveIdleFrame_ : tallWaveFrame_;
        if (twTex)
        {
            SDL_FRect srcF = { (float)(srcFrame * FW), 0.0f, (float)FW, (float)FH };
            int twW = (int)(FW * 0.3f);
            int twH = (int)(FH * 0.30f);
            int twX = (int)tallWaveX_ - twW / 2;
            int twY = (int)tallWaveY_ - twH;
            SDL_FRect dst = worldToScreen(twX, twY, twW, twH);
            SDL_FlipMode twFlip = (attackDir_ > 0.0f) ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
            SDL_RenderTextureRotated(render.renderer, twTex, &srcF, &dst, 0.0, nullptr, twFlip);
        }
    }


    // ── Shockwave ─────────────────────────────────────────────────────────────
    if (shockwaveActive_)
    {
        static constexpr int FW = 200, FH = 500;
        static constexpr int HOLD_FRAME = 7;
        bool swInHold = !shockwaveOutro_ && shockwaveFrame_ >= HOLD_FRAME;
        SDL_Texture* swTex = (swInHold && texWaveIdle_) ? texWaveIdle_ : texShockwave_;
        int srcFrame = swInHold ? shockwaveIdleFrame_ : shockwaveFrame_;
        if (swTex)
        {
            SDL_FRect srcF = { (float)(srcFrame * FW), 0.0f, (float)FW, (float)FH };
            int swW = (int)(FW * 0.3f);
            int swH = (int)(FH * 0.22f);
            int swX = (int)shockwaveX_ - swW / 2;
            int swY = (int)shockwaveY_ - swH;
            SDL_FRect dst = worldToScreen(swX, swY, swW, swH);
            SDL_FlipMode swFlip = (attackDir_ > 0.0f) ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
            SDL_RenderTextureRotated(render.renderer, swTex, &srcF, &dst, 0.0, nullptr, swFlip);
        }
    }
}
