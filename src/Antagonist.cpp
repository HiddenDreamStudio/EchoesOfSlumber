#include "Antagonist.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "Scene.h"
#include "Log.h"
#include <cmath>

Antagonist::Antagonist() : Entity(EntityType::ANTAGONIST)
{
    name = "Antagonist";
}

Antagonist::~Antagonist() {}

bool Antagonist::Start()
{
    texIdle_ = Engine::GetInstance().textures->Load(
        "assets/textures/spritesheets/SS_Antagonista/SS_Antagonista_idle.png");
    texDisappear_ = Engine::GetInstance().textures->Load(
        "assets/textures/spritesheets/SS_Antagonista/SS_Antagonistadesapareixent.png");

    for (int i = 0; i < IDLE_FRAMES; ++i)
        animIdle_.AddFrame({ i * IDLE_FRAME_W, 0, IDLE_FRAME_W, IDLE_FRAME_H }, (int)MS_PER_FRAME);
    animIdle_.SetLoop(true);

    for (int i = 0; i < DISAPP_FRAMES; ++i)
        animDisappear_.AddFrame({ i * DISAPP_FRAME_W, 0, DISAPP_FRAME_W, DISAPP_FRAME_H }, (int)MS_PER_FRAME);
    animDisappear_.SetLoop(false);

    // Snap feet to the floor below the placed position so the designer only
    // needs to place the object roughly in the right column — no manual Y tuning.
    float hitX = 0.0f, hitY = 0.0f;
    int rcX = (int)position.getX();
    int rcY = (int)position.getY();
    if (Engine::GetInstance().physics->RayCastWorld(rcX, rcY, rcX, rcY + 2000, hitX, hitY))
    {
        // Visual half-height is 128*scale_ for both animations (512*scale_*0.5 or 256*scale_).
        // The sprite has transparent space at the bottom of the frame, so we push it
        // down by FOOT_OFFSET so the character's feet land on the floor instead of the frame edge.
        position.setY(hitY - 128.0f * scale_ + FOOT_OFFSET);
    }

    return true;
}

bool Antagonist::CleanUp()
{
    if (texIdle_)      { Engine::GetInstance().textures->UnLoad(texIdle_);      texIdle_      = nullptr; }
    if (texDisappear_) { Engine::GetInstance().textures->UnLoad(texDisappear_); texDisappear_ = nullptr; }
    return true;
}

bool Antagonist::Update(float dt)
{
    if (pendingToDelete) return true;

    switch (state_)
    {
    case AntagonistState::IDLE:
    {
        // If the end-game sequence already fired (e.g. map reloaded after death),
        // remove this entity silently so it can never trigger the cinematic again.
        auto& scn = *Engine::GetInstance().scene;
        if (scn.IsEndGameTriggered()) {
            pendingToDelete = true;
            break;
        }

        animIdle_.Update(dt);

        if (scn.player && scn.player->pbody)
        {
            int px, py;
            scn.player->pbody->GetPosition(px, py);
            float dx = (float)px - position.getX();
            float dy = (float)py - position.getY();
            if (sqrtf(dx * dx + dy * dy) < appearRange_)
            {
                animDisappear_.Reset();
                state_ = AntagonistState::DISAPPEARING;
            }
        }
        break;
    }

    case AntagonistState::DISAPPEARING:
        animDisappear_.Update(dt);
        if (animDisappear_.HasFinishedOnce()) {
            pendingToDelete = true;
            Engine::GetInstance().scene->TriggerEndGameCinematic();
        }
        break;
    }

    Draw();
    return true;
}

void Antagonist::Draw()
{
    auto& render = *Engine::GetInstance().render;

    SDL_Texture*    tex;
    const SDL_Rect& frame     = (state_ == AntagonistState::IDLE)
                                ? animIdle_.GetCurrentFrame()
                                : animDisappear_.GetCurrentFrame();
    // Idle frames are 512px; disappear frames are 256px — use half scale for
    // idle so both animations render at the same on-screen size.
    float drawScale = (state_ == AntagonistState::IDLE) ? scale_ * 0.5f : scale_;
    tex             = (state_ == AntagonistState::IDLE) ? texIdle_ : texDisappear_;

    if (!tex) return;

    int drawX = (int)(position.getX() - frame.w * drawScale * 0.5f);
    int drawY = (int)(position.getY() - frame.h * drawScale * 0.5f);

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(tex, alpha_);
    render.DrawTexture(tex, drawX, drawY, &frame, 1.0f, 0.0, INT_MAX, INT_MAX, SDL_FLIP_NONE, drawScale);
    SDL_SetTextureAlphaMod(tex, 255);
}
