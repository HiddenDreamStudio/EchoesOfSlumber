#include "Boss2.h"
#include "Engine.h"
#include "Physics.h"
#include "Render.h"
#include "Scene.h"
#include "Textures.h"
#include "Window.h"
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

    auto& tex = *Engine::GetInstance().textures;
    const std::string DIR = "assets/textures/spritesheets/SS_Enemics_Nivell3/";

    texSale_ = tex.Load((DIR + "spritesheet_Music_Box_Sale_la_caja.png").c_str());
    for (int i = 0; i < 9; i++) animSale_.AddFrame({i * 228, 0, 228, 228}, 80);
    animSale_.SetLoop(true);

    texPunch_ = tex.Load((DIR + "spritesheet_Music_box_Golpe_Punyo _Horizontal.png").c_str());
    for (int i = 0; i < 11; i++) animPunch_.AddFrame({i * 186, 0, 186, 186}, 70);
    animPunch_.SetLoop(false);

    texRest_ = tex.Load((DIR + "spritesheet_Music_Box_Cansado.png").c_str());
    for (int i = 0; i < 20; i++) animRest_.AddFrame({i * 102, 0, 102, 102}, 80);
    animRest_.SetLoop(true);

    texDestapaBtn_ = tex.Load((DIR + "spritesheet_Music_Box_destapa_el_boton.png").c_str());
    for (int i = 0; i < 6; i++) animDestapaBtn_.AddFrame({i * 256, 0, 256, 256}, 100);
    animDestapaBtn_.SetLoop(false);

    texTapaBtn_ = tex.Load((DIR + "spritesheet_Musi_Box_Tapar_Boton.png").c_str());
    for (int i = 0; i < 5; i++) animTapaBtn_.AddFrame({i * 256, 0, 256, 256}, 100);
    animTapaBtn_.SetLoop(false);

    texHit_ = tex.Load((DIR + "spritesheet_Music_Box_Hit.png").c_str());
    for (int i = 0; i < 6; i++) animHit_.AddFrame({i * 256, 0, 256, 256}, 80);
    animHit_.SetLoop(false);

    texShockwave_ = tex.Load("assets/textures/spritesheets/SS_Pols_01.png");
    for (int i = 0; i < 11; i++) animShockwave_.AddFrame({i * 866, 0, 866, 202}, 60);
    animShockwave_.SetLoop(true);

    return true;
}

bool Boss2::CleanUp()
{
    DestroyPunchHitbox();
    DestroyShockwave();
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
        cx, cy, BOX_W, BOX_H, bodyType::STATIC, 0.8f);
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
    shockwaveX_     = (float)sx;
    shockwaveY_     = (float)drawY;
    shockwavePhysY_ = (float)physY;
    shockwaveStart_ = (float)sx;
    shockwaveActive_ = true;
}

void Boss2::UpdateShockwave(float dt)
{
    if (!shockwaveActive_ || !shockwaveBody_) return;
    shockwaveX_ += attackDir_ * SHOCKWAVE_SPEED * (dt / 1000.0f);
    shockwaveBody_->SetPosition((int)shockwaveX_, (int)shockwavePhysY_);
    if (fabsf(shockwaveX_ - shockwaveStart_) >= SHOCKWAVE_RANGE)
        DestroyShockwave();
}

void Boss2::DestroyShockwave()
{
    if (!shockwaveBody_) return;
    Engine::GetInstance().physics->DeletePhysBody(shockwaveBody_);
    shockwaveBody_  = nullptr;
    shockwaveActive_ = false;
}

void Boss2::SpawnButtonSensor()
{
    if (buttonBody_) return;
    int bx, by;
    pbody->GetPosition(bx, by);
    // Button sits on the face of the box (the side facing the player), lower third
    int btnX = bx + (int)(attackDir_ * (BOX_W / 2.0f + BUTTON_W / 2.0f));
    int btnY = by + BOX_H / 4;
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

    switch (newState)
    {
    case Boss2State::INTRO:         animSale_.Reset(); animSale_.SetLoop(false); break;
    case Boss2State::IDLE:          animRest_.Reset(); animRest_.SetLoop(true);  break;
    case Boss2State::CHARGE:        animRest_.Reset(); animRest_.SetLoop(true);  break;
    case Boss2State::PUNCH:         animPunch_.Reset();      break;
    case Boss2State::REST:          animRest_.Reset();       break;
    case Boss2State::EXPOSE_BUTTON: animDestapaBtn_.Reset(); break;
    case Boss2State::CLOSE_BUTTON:  animTapaBtn_.Reset();    break;
    case Boss2State::STUNNED:       animHit_.Reset();        break;
    case Boss2State::DEATH:         animHit_.Reset();        deathTeleportTimer_ = 0.0f; deathSequenceDone_ = false; break;
    default: break;
    }
}

// ─── TakeDamage ──────────────────────────────────────────────────────────────

void Boss2::TakeDamage(int damage)
{
    if (state_ != Boss2State::EXPOSE_BUTTON) return;

    health -= damage;
    if (health < 0) health = 0;

    CheckPhaseTransition();

    if (health <= 0)
    {
        isDead_ = true;
        DestroyPunchHitbox();
        DestroyShockwave();
        DestroyButtonSensor();
        TransitionTo(Boss2State::DEATH);
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
    else if (physA == buttonBody_)
    {
        if (physB->ctype == ColliderType::ATTACK && state_ == Boss2State::EXPOSE_BUTTON)
            TakeDamage(1);
    }
}

// ─── Phase change ─────────────────────────────────────────────────────────────

void Boss2::OnPhaseChange(int newPhase)
{
    LOG("Boss2 entering phase %d!", newPhase);
    attackCycle_     = 0;
    nextExposeCycle_ = 2 + (rand() % 2);
    // TODO: speed up attacks, add phase 2 patterns when art/design confirmed
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
    UpdateShockwave(dt); // runs every frame regardless of state
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
        if (stateTimer_ >= CHARGE_DURATION)
        {
            SpawnPunchHitbox();
            TransitionTo(Boss2State::PUNCH);
        }
        break;

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
        if (stateTimer_ >= REST_DURATION)
        {
            attackCycle_++;
            if (attackCycle_ >= nextExposeCycle_)
            {
                attackCycle_     = 0;
                nextExposeCycle_ = 2 + (rand() % 2);
                SpawnButtonSensor();
                TransitionTo(Boss2State::EXPOSE_BUTTON);
            }
            else
            {
                TransitionTo(Boss2State::IDLE);
            }
        }
        break;

    // ── EXPOSE_BUTTON ─────────────────────────────────────────────────────────
    case Boss2State::EXPOSE_BUTTON:
        if (stateTimer_ >= EXPOSE_DURATION)
        {
            DestroyButtonSensor();
            TransitionTo(Boss2State::CLOSE_BUTTON);
        }
        break;

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

    // ── DEATH — once the death animation finishes, wait a beat then send the
    //           player back to the hub (MapLvl3ZonaAlta — spawn "J") ─────────
    case Boss2State::DEATH:
        if (stateTimer_ >= DEATH_DURATION && !deathSequenceDone_)
        {
            deathTeleportTimer_ += dt;
            if (deathTeleportTimer_ >= DEATH_TELEPORT_DELAY)
            {
                deathSequenceDone_ = true;
                isEngaged_ = false;
                auto& scn = *Engine::GetInstance().scene;
                scn.RequestSubMapTeleport("MapLvl3ZonaAlta.tmx", "J");
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

    SDL_FlipMode flip = (attackDir_ > 0.0f) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

    // ── Main sprite ───────────────────────────────────────────────────────────
    SDL_Texture* activeTex = nullptr;
    SDL_FRect    frameRect = {};
    int drawW = 0, drawH = 0;

    auto toFRect = [](const SDL_Rect& r) -> SDL_FRect {
        return { (float)r.x, (float)r.y, (float)r.w, (float)r.h };
    };

    static constexpr float SCALE_SALE  = 0.9f;
    static constexpr float SCALE_PUNCH = 1.2f;
    static constexpr float SCALE_REST  = 2.0f;
    static constexpr float SCALE_BTN   = 0.9f;
    static constexpr float SCALE_HIT   = 0.9f;

    switch (state_)
    {
    case Boss2State::INTRO:
        animSale_.Update(dt);
        activeTex = texSale_; frameRect = toFRect(animSale_.GetCurrentFrame());
        drawW = (int)(228 * SCALE_SALE); drawH = (int)(228 * SCALE_SALE);
        break;
    case Boss2State::IDLE:
    case Boss2State::CHARGE:
        animRest_.Update(dt);
        activeTex = texRest_; frameRect = toFRect(animRest_.GetCurrentFrame());
        drawW = (int)(102 * SCALE_REST); drawH = (int)(102 * SCALE_REST);
        break;
    case Boss2State::PUNCH:
        animPunch_.Update(dt);
        activeTex = texPunch_; frameRect = toFRect(animPunch_.GetCurrentFrame());
        drawW = (int)(186 * SCALE_PUNCH); drawH = (int)(186 * SCALE_PUNCH);
        break;
    case Boss2State::RETRACT:
        animPunch_.UpdateBackwards(dt);
        activeTex = texPunch_; frameRect = toFRect(animPunch_.GetCurrentFrame());
        drawW = (int)(186 * SCALE_PUNCH); drawH = (int)(186 * SCALE_PUNCH);
        break;
    case Boss2State::REST:
        animRest_.Update(dt);
        activeTex = texRest_; frameRect = toFRect(animRest_.GetCurrentFrame());
        drawW = (int)(102 * SCALE_REST); drawH = (int)(102 * SCALE_REST);
        break;
    case Boss2State::EXPOSE_BUTTON:
        if (!animDestapaBtn_.HasFinishedOnce()) animDestapaBtn_.Update(dt);
        activeTex = texDestapaBtn_; frameRect = toFRect(animDestapaBtn_.GetCurrentFrame());
        drawW = (int)(256 * SCALE_BTN); drawH = (int)(256 * SCALE_BTN);
        break;
    case Boss2State::CLOSE_BUTTON:
        animTapaBtn_.Update(dt);
        activeTex = texTapaBtn_; frameRect = toFRect(animTapaBtn_.GetCurrentFrame());
        drawW = (int)(256 * SCALE_BTN); drawH = (int)(256 * SCALE_BTN);
        break;
    case Boss2State::STUNNED:
    case Boss2State::DEATH:
        animHit_.Update(dt);
        activeTex = texHit_; frameRect = toFRect(animHit_.GetCurrentFrame());
        drawW = (int)(256 * SCALE_HIT); drawH = (int)(256 * SCALE_HIT);
        break;
    default:
        break;
    }

    if (activeTex)
    {
        static constexpr int GROUND_OFFSET = 50;
        int drawX = bx - drawW / 2;
        int drawY = (by + BOX_H / 2) - drawH + GROUND_OFFSET;
        SDL_FRect dst = worldToScreen(drawX, drawY, drawW, drawH);
        SDL_RenderTextureRotated(render.renderer, activeTex, &frameRect, &dst,
                                 0.0, nullptr, flip);
    }


    // ── Shockwave ─────────────────────────────────────────────────────────────
    if (shockwaveActive_ && texShockwave_)
    {
        animShockwave_.Update(dt);
        SDL_Rect srcR = animShockwave_.GetCurrentFrame();
        SDL_FRect srcF = { (float)srcR.x, (float)srcR.y, (float)srcR.w, (float)srcR.h };
        static constexpr float SW_SCALE = 0.4f;
        int swW = (int)(866 * SW_SCALE);
        int swH = (int)(202 * SW_SCALE);
        int swX = (int)shockwaveX_ - swW / 2;
        int swY = (int)shockwaveY_ - swH;  // bottom of sprite at ground level
        SDL_FRect dst = worldToScreen(swX, swY, swW, swH);
        SDL_FlipMode swFlip = (attackDir_ > 0.0f) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_RenderTextureRotated(render.renderer, texShockwave_, &srcF, &dst, 0.0, nullptr, swFlip);
    }
}
