#include "DropDoll.h"
#include "Engine.h"
#include "Physics.h"
#include "Render.h"
#include "Scene.h"
#include "Input.h"
#include "Textures.h"
#include "Audio.h"
#include "Log.h"
#include <cstdlib>

DropDoll::DropDoll() : Entity(EntityType::DROP_DOLL)
{
    name = "DropDoll";
}

DropDoll::~DropDoll() {}

bool DropDoll::Start()
{
    const std::string UI = "assets/textures/UI/Minigame Drop Doll/";
    auto& tex = *Engine::GetInstance().textures;
    //texShape_      = tex.Load((UI + "UI_DropDoll_Shape.png").c_str());
    texBlackBase_  = tex.Load((UI + "UI_DropDoll_Game_BlackBase.png").c_str());
    texFrame_      = tex.Load((UI + "UI_DropDoll_Game_Frame 2.png").c_str());
    texInside_     = tex.Load((UI + "UI_DropDoll_Game_Inside.png").c_str());
    texIndicator_  = tex.Load((UI + "UI_DropDoll_Game_Desplazable (1).png").c_str());
    texPressSpace_ = tex.Load((UI + "UI_DropDoll_Game_Press_Space (1).png").c_str());

    const std::string DIR = "assets/textures/spritesheets/SS_Enemics_Nivell3/";

    texIdle_ = tex.Load((DIR + "SP_DropDoll_Idle.png").c_str());
    for (int i = 0; i < 20; i++) animIdle_.AddFrame({ i * 256, 0, 256, 256 }, 100);
    animIdle_.SetLoop(true);

    texSoltarse_ = tex.Load((DIR + "SP_DropDoll_Soltarsecuerda.png").c_str());
    for (int i = 0; i < 10; i++) animSoltarse_.AddFrame({ i * 256, 0, 256, 256 }, 70);
    animSoltarse_.SetLoop(false);

    texAire_ = tex.Load((DIR + "SP_Drop_Doll_AireLoop.png").c_str());
    for (int i = 0; i < 5; i++) animAire_.AddFrame({ i * 256, 0, 256, 256 }, 80);
    animAire_.SetLoop(true);

    texEnganchar_ = tex.Load((DIR + "SP_DropDoll_Engancharse.png").c_str());
    for (int i = 0; i < 14; i++) animEnganchar_.AddFrame({ i * 256, 0, 256, 256 }, 60);
    animEnganchar_.SetLoop(false);

    texEngancharLoop_ = tex.Load((DIR + "SP_DropDoll_(enganchando_loop).png").c_str());
    for (int i = 0; i < 17; i++) animEngancharLoop_.AddFrame({ i * 256, 0, 256, 256 }, 90);
    animEngancharLoop_.SetLoop(true);

    texDesenganchar_ = tex.Load((DIR + "SP_DropDoll_Desengancharse.png").c_str());
    for (int i = 0; i < 14; i++) animDesenganchar_.AddFrame({ i * 256, 0, 256, 256 }, 80);
    animDesenganchar_.SetLoop(false);

    texDie_ = tex.Load((DIR + "SP_DropDoll_Die.png").c_str());
    for (int i = 0; i < 19; i++) animDie_.AddFrame({ i * 256, 0, 256, 256 }, 80);
    animDie_.SetLoop(false);

    auto& audio = *Engine::GetInstance().audio;
    fxCaida_   = audio.LoadFx("assets/audio/Enemies/Drop Doll/Dropdoll-Caida.wav");
    fxAttach_  = audio.LoadFx("assets/audio/Enemies/Drop Doll/Dropdoll-Attach.wav");
    fxMolesta_ = audio.LoadFx("assets/audio/Enemies/Drop Doll/Dropdoll-Molestando.wav");

    return true;
}

bool DropDoll::CleanUp()
{
    if (pbody_)
    {
        Engine::GetInstance().physics->DeletePhysBody(pbody_);
        pbody_ = nullptr;
    }

    // Safety: ensure player is freed if DropDoll is cleaned up mid-grab
    auto& scn = *Engine::GetInstance().scene;
    if (scn.player && (state_ == DropDollState::GRABBING || state_ == DropDollState::ATTACHED || state_ == DropDollState::DETACHING))
        scn.player->isDollGrabbed_ = false;

    auto& tex = *Engine::GetInstance().textures;
    tex.UnLoad(texIdle_);
    tex.UnLoad(texSoltarse_);
    tex.UnLoad(texAire_);
    tex.UnLoad(texEnganchar_);
    tex.UnLoad(texEngancharLoop_);
    tex.UnLoad(texDesenganchar_);
    tex.UnLoad(texDie_);

    return true;
}

// ─── Physics body ─────────────────────────────────────────────────────────────

void DropDoll::SpawnBody()
{
    if (pbody_) return;
    pbody_ = Engine::GetInstance().physics->CreateRectangle(
        (int)position.getX(),
        (int)position.getY(),
        DOLL_W, DOLL_H,
        bodyType::DYNAMIC,
        0.0f);
    pbody_->listener = this;
    pbody_->ctype    = ColliderType::ENEMY;
}

// ─── Update ──────────────────────────────────────────────────────────────────

bool DropDoll::Update(float dt)
{
    if (pendingToDelete) return true;

    switch (state_)
    {
    case DropDollState::HANGING:
    {
        animIdle_.Update(dt);
        auto& scn = *Engine::GetInstance().scene;
        if (scn.player && scn.player->pbody)
        {
            int px, py;
            scn.player->pbody->GetPosition(px, py);
            float dx = fabsf((float)px - position.getX());
            if (dx < triggerWidth_ * 0.5f)
            {
                animSoltarse_.Reset();
                state_ = DropDollState::RELEASING;
            }
        }
        break;
    }

    case DropDollState::RELEASING:
        animSoltarse_.Update(dt);
        if (animSoltarse_.HasFinishedOnce())
        {
            SpawnBody();
            animAire_.Reset();
            fallElapsedTime_ = 0.0f;
            hasBeenFalling_  = false;
            Engine::GetInstance().audio->PlayFxSpatial(fxCaida_, position);
            state_ = DropDollState::FALLING;
        }
        break;

    case DropDollState::FALLING:
        if (isPostDetachFall_)
        {
            // Hold the let-go pose (its last frame, since the animation has
            // already finished and doesn't loop) instead of switching back to
            // the aerial spin loop — it just released the player, it isn't
            // tumbling through the air like a doll that missed.
            animDesenganchar_.Update(dt);

            // No physics body here — fall manually in a straight line so it
            // passes clean through the player it just released, and stop the
            // instant it reaches the floor we found via raycast on detach.
            float dtSec = dt / 1000.0f;
            postDetachVelocityY_ += POST_DETACH_FALL_ACCEL * dtSec;
            position.setY(position.getY() + postDetachVelocityY_ * dtSec);

            if (position.getY() >= postDetachGroundY_)
            {
                position.setY(postDetachGroundY_);
                animDie_.Reset();
                state_ = DropDollState::DYING;
            }
            break;
        }

        animAire_.Update(dt);
        if (pbody_)
        {
            int bx, by;
            pbody_->GetPosition(bx, by);
            position.setX((float)bx);
            position.setY((float)by);

            // Arm the landing check once enough real time has passed since
            // letting go — see the comment on hasBeenFalling_ for why this has
            // to be a time check and not a velocity one.
            fallElapsedTime_ += dt;
            if (!hasBeenFalling_ && fallElapsedTime_ > MIN_FALL_TIME_BEFORE_LANDING_CHECK)
                hasBeenFalling_ = true;

            // Missed the player — keep falling under gravity until it
            // actually reaches the ground, then play the death animation
            // there instead of mid-air.
            if (hasMissedPlayer_)
            {
                missedTimer_ += dt;

                // "Landed" = solid ground sits right beneath its feet RIGHT
                // NOW — checked geometrically with a short downward raycast
                // from its center. This catches the touch-down the instant it
                // happens, instead of waiting for the physics body's
                // velocity/position to "settle" — which can lag well behind
                // the visual landing, since a resting frictionless body keeps
                // jittering near (but not at) zero speed from the contact
                // solver for a noticeable while.
                bool atRest = false;
                if (hasBeenFalling_)
                {
                    float hitX = 0.0f, hitY = 0.0f;
                    atRest = Engine::GetInstance().physics->RayCastWorld(
                        bx, by, bx, by + DOLL_H / 2 + 4, hitX, hitY);
                }

                // Backstop: in case it somehow never reads as settled (e.g. it
                // falls into a pit with no floor) — without this the doll would
                // stay falling/resting forever, never playing its death animation.
                bool timedOut = missedTimer_ > MAX_FALL_AFTER_MISS;

                if (atRest || timedOut)
                {
                    Engine::GetInstance().physics->DeletePhysBody(pbody_);
                    pbody_ = nullptr;
                    animDie_.Reset();
                    state_ = DropDollState::DYING;
                }
            }
        }
        break;

    case DropDollState::GRABBING:
    {
        animEnganchar_.Update(dt);
        auto& scn = *Engine::GetInstance().scene;
        if (scn.player && scn.player->pbody)
        {
            int px, py;
            scn.player->pbody->GetPosition(px, py);
            position.setX((float)px);
            position.setY((float)py - DOLL_H);
        }
        if (animEnganchar_.HasFinishedOnce())
        {
            // Randomize success zone for variety
            float zoneW = 0.10f;
            float maxStart = 0.80f - zoneW;
            zoneStart_ = 0.10f + (rand() % (int)((maxStart - 0.10f) * 100)) / 100.0f;
            zoneEnd_   = zoneStart_ + zoneW;

            indicatorPos_   = 0.0f;
            indicatorDir_   = 1.0f;
            indicatorSpeed_ = 0.75f;
            failedAttempts_ = 0;
            attachTimer_    = 0.0f;
            molesTimer_     = 0.0f;
            animEngancharLoop_.Reset();
            Engine::GetInstance().audio->PlayFxSpatial(fxMolesta_, position);
            state_ = DropDollState::ATTACHED;
        }
        break;
    }

    case DropDollState::ATTACHED:
    {
        animEngancharLoop_.Update(dt);
        attachTimer_ += dt;

        auto& scn = *Engine::GetInstance().scene;

        // Follow player position
        if (scn.player && scn.player->pbody)
        {
            int px, py;
            scn.player->pbody->GetPosition(px, py);
            position.setX((float)px);
            position.setY((float)py - DOLL_H);
        }

        // Loop molesta SFX
        molesTimer_ += dt;
        if (molesTimer_ >= MOLES_INTERVAL)
        {
            molesTimer_ = 0.0f;
            Engine::GetInstance().audio->PlayFxSpatial(fxMolesta_, position);
        }

        // Move indicator
        float dtSec = dt / 1000.0f;
        indicatorPos_ += indicatorDir_ * indicatorSpeed_ * dtSec;
        if (indicatorPos_ >= 1.0f) { indicatorPos_ = 1.0f; indicatorDir_ = -1.0f; }
        if (indicatorPos_ <= 0.0f) { indicatorPos_ = 0.0f; indicatorDir_ =  1.0f; }

        // Passive damage tick every DAMAGE_INTERVAL ms
        if (attachTimer_ >= DAMAGE_INTERVAL)
        {
            if (scn.player) scn.player->TakeDamage(1);
            attachTimer_ = 0.0f;
        }

        // Check Space press
        auto& input = *Engine::GetInstance().input;
        if (input.GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN)
        {
            if (indicatorPos_ >= zoneStart_ && indicatorPos_ <= zoneEnd_)
            {
                // Success — play the detach animation; isDollGrabbed_ stays true
                // until it finishes (see DETACHING below). Clearing it here would
                // let Player::Jump() see the very same Space press as a jump input.
                animDesenganchar_.Reset();
                state_ = DropDollState::DETACHING;
                break;
            }
            else
            {
                // Failed press — deal damage and speed up indicator
                if (scn.player) scn.player->TakeDamage(1);
                failedAttempts_++;
                indicatorSpeed_ += SPEED_INCREASE;
            }
        }

        DrawMinigameBar();
        break;
    }

    case DropDollState::DETACHING:
        animDesenganchar_.Update(dt);
        if (animDesenganchar_.HasFinishedOnce())
        {
            auto& scn = *Engine::GetInstance().scene;
            if (scn.player) scn.player->isDollGrabbed_ = false;

            // Don't just vanish at head height — drop it from here and let it
            // disintegrate on the ground, same as a doll that misses. No
            // physics body this time (it would just rest on top of the player
            // it released instead of falling through them); raycast straight
            // down to find the floor and fall there manually.
            //
            // Cast from the PLAYER's center, not the doll's: a ray whose
            // origin starts inside a shape can't register a hit on that same
            // shape (there's no "entry" — it's already inside), so casting
            // from inside the player's own collider makes the ray skip clean
            // through their head/body and land on the actual floor below
            // instead of mistaking them for the ground.
            float hitX = 0.0f, hitY = 0.0f;
            float groundY = position.getY() + 400.0f; // fallback if no floor is found
            if (scn.player && scn.player->pbody)
            {
                int px, py;
                scn.player->pbody->GetPosition(px, py);
                if (Engine::GetInstance().physics->RayCastWorld(px, py, px, py + 2000, hitX, hitY))
                    groundY = hitY;
            }

            postDetachGroundY_   = groundY - DOLL_H * 0.5f;
            postDetachVelocityY_ = 0.0f;
            isPostDetachFall_    = true;

            state_ = DropDollState::FALLING;
        }
        break;

    case DropDollState::DYING:
        animDie_.Update(dt);
        if (animDie_.HasFinishedOnce())
            pendingToDelete = true;
        break;
    }

    return true;
}

bool DropDoll::PostUpdate()
{
    // Drawn after every entity's Update (and thus after the player) so the
    // doll always renders in front of — never behind — the character.
    if (!pendingToDelete) Draw();
    return true;
}

// ─── Collision ────────────────────────────────────────────────────────────────

void DropDoll::OnCollision(PhysBody* physA, PhysBody* physB)
{
    if (state_ != DropDollState::FALLING || pendingToDelete) return;

    if (physB->ctype == ColliderType::PLAYER && physB->listener)
    {
        auto& scn = *Engine::GetInstance().scene;
        if (scn.player) scn.player->suppressDamageAnim_ = true;

        physB->listener->TakeDamage(1);

        Engine::GetInstance().physics->DeletePhysBody(pbody_);
        pbody_ = nullptr;

        // Freeze the player and play the latch-on animation before the minigame starts
        if (scn.player) scn.player->isDollGrabbed_ = true;
        Engine::GetInstance().audio->PlayFxSpatial(fxAttach_, position);
        animEnganchar_.Reset();
        state_ = DropDollState::GRABBING;
    }
    else if (physB->ctype != ColliderType::ATTACK)
    {
        // Missed the player — this contact alone doesn't prove it's the
        // ground (the doll can graze ledges/walls on the way down), so just
        // flag the miss and let the FALLING update confirm the real landing
        // once it actually comes to rest.
        hasMissedPlayer_ = true;
    }
}

// ─── Draw ─────────────────────────────────────────────────────────────────────

void DropDoll::Draw()
{
    auto& render = *Engine::GetInstance().render;

    int cx = (int)position.getX();
    int cy = (int)position.getY();

    static constexpr float DRAW_SCALE = 0.9f; // shrinks the 256x256 frames down to doll size

    auto drawAnim = [&](SDL_Texture* tex, Animation& anim, int centerX, int centerY) {
        if (!tex) return;
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        const SDL_Rect& f = anim.GetCurrentFrame();
        if (f.w == 0 || f.h == 0) return;
        int sw = (int)(f.w * DRAW_SCALE);
        int sh = (int)(f.h * DRAW_SCALE);
        render.DrawTexture(tex, centerX - sw / 2, centerY - sh / 2, &f, 1.0f, -1.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, DRAW_SCALE);
    };

    switch (state_)
    {
    case DropDollState::HANGING:
        drawAnim(texIdle_, animIdle_, cx, cy);
        break;

    case DropDollState::RELEASING:
        drawAnim(texSoltarse_, animSoltarse_, cx, cy);
        break;

    case DropDollState::FALLING:
        // Post-detach drop: keep the let-go pose instead of the aerial spin
        // loop — it just released the player, it isn't tumbling mid-air.
        if (isPostDetachFall_)
            drawAnim(texDesenganchar_, animDesenganchar_, cx, cy);
        else
            drawAnim(texAire_, animAire_, cx, cy);
        break;

    case DropDollState::GRABBING:
        drawAnim(texEnganchar_, animEnganchar_, cx, cy);
        break;

    case DropDollState::ATTACHED:
        // Drawn on the player's head behind the full-screen minigame UI (DrawMinigameBar)
        drawAnim(texEngancharLoop_, animEngancharLoop_, cx, cy);
        break;

    case DropDollState::DETACHING:
        drawAnim(texDesenganchar_, animDesenganchar_, cx, cy);
        break;

    case DropDollState::DYING:
        drawAnim(texDie_, animDie_, cx, cy);
        break;
    }
}

// ─── Minigame UI ─────────────────────────────────────────────────────────────

void DropDoll::DrawMinigameBar()
{
    auto& render = *Engine::GetInstance().render;

    // All elements scaled to 50% (640x360), centered horizontally at x=320
    static constexpr int FX = 320, FY = 0, FW = 640, FH = 360;

    // Bar track region within the scaled frame
    static constexpr int BAR_X = 489; // 320 + (int)(338*0.50)
    static constexpr int BAR_W = 310; // (int)(620*0.50)
    static constexpr int BAR_Y = 242; // (int)(483*0.50)

    // 1 — Doll shape (behind frame)
    //if (texShape_)
    //    render.DrawTextureAlpha(texShape_, FX, FY, FW, FH);

    // 3 — Stone frame
    if (texFrame_)
        render.DrawTextureAlpha(texFrame_, FX, FY, FW, FH);

    // 4 — Success zone diamond (fixed)
    if (texInside_)
    {
        static constexpr int IW = 24, IH = 24;
        float zoneCenter = (zoneStart_ + zoneEnd_) * 0.5f;
        int ix = BAR_X + (int)(zoneCenter * BAR_W) - IW / 2;
        int iy = BAR_Y - IH / 2 - 4;
        render.DrawTextureAlpha(texInside_, ix, iy, IW, IH);
    }

    // 5 — Moving indicator
    if (texIndicator_)
    {
        static constexpr int SW = 14, SH = 43;
        int sx = BAR_X + (int)(indicatorPos_ * BAR_W) - SW / 2;
        int sy = BAR_Y - SH / 2;
        render.DrawTextureAlpha(texIndicator_, sx, sy, SW, SH);
    }

    // 6 — "Press Space" text
    if (texPressSpace_)
        render.DrawTextureAlpha(texPressSpace_, 494, 320, 315, 44);
}
