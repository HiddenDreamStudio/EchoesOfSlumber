#include "DropDoll.h"
#include "Engine.h"
#include "Physics.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"

DropDoll::DropDoll() : Entity(EntityType::DROP_DOLL)
{
    name = "DropDoll";
}

DropDoll::~DropDoll() {}

bool DropDoll::Start()
{
    // position is set by Map.cpp to the center of the Tiled object
    return true;
}

bool DropDoll::CleanUp()
{
    if (pbody_)
    {
        Engine::GetInstance().physics->DeletePhysBody(pbody_);
        pbody_ = nullptr;
    }
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
        // Trigger when player enters the horizontal zone (no Y limit)
        auto& scn = *Engine::GetInstance().scene;
        if (scn.player && scn.player->pbody)
        {
            int px, py;
            scn.player->pbody->GetPosition(px, py);
            float dx = fabsf((float)px - position.getX());
            if (dx < triggerWidth_ * 0.5f)
            {
                SpawnBody();
                state_ = DropDollState::FALLING;
            }
        }
        break;
    }

    case DropDollState::FALLING:
        if (pbody_)
        {
            int bx, by;
            pbody_->GetPosition(bx, by);
            position.setX((float)bx);
            position.setY((float)by);
        }
        break;

    case DropDollState::ATTACHED:
    {
        attachTimer_ += dt;

        // Follow player position
        auto& scn = *Engine::GetInstance().scene;
        if (scn.player && scn.player->pbody)
        {
            int px, py;
            scn.player->pbody->GetPosition(px, py);
            position.setX((float)px);
            position.setY((float)py - DOLL_H);
        }

        if (attachTimer_ >= ATTACH_DURATION)
        {
            // Restore player speed and disappear
            if (scn.player) scn.player->speed = originalSpeed_;
            pendingToDelete = true;
        }
        break;
    }
    }

    Draw();
    return true;
}

// ─── Collision ────────────────────────────────────────────────────────────────

void DropDoll::OnCollision(PhysBody* physA, PhysBody* physB)
{
    if (state_ != DropDollState::FALLING || pendingToDelete) return;

    if (physB->ctype == ColliderType::PLAYER && physB->listener)
    {
        // Suppress the damage animation — doll attachment has its own visual
        auto& scn = *Engine::GetInstance().scene;
        if (scn.player) scn.player->suppressDamageAnim_ = true;

        physB->listener->TakeDamage(1);

        // Destroy physics body and attach to player
        Engine::GetInstance().physics->DeletePhysBody(pbody_);
        pbody_ = nullptr;

        if (scn.player)
        {
            originalSpeed_ = scn.player->speed;
            scn.player->speed *= SLOW_FACTOR;
        }
        attachTimer_ = 0.0f;
        state_ = DropDollState::ATTACHED;
    }
    else if (physB->ctype != ColliderType::ATTACK)
    {
        // Hit floor or wall
        pendingToDelete = true;
    }
}

// ─── Draw ─────────────────────────────────────────────────────────────────────

void DropDoll::Draw()
{
    auto& render = *Engine::GetInstance().render;

    int cx = (int)position.getX();
    int cy = (int)position.getY();

    if (state_ == DropDollState::HANGING)
    {
        // Rope: line from above down to doll top
        render.DrawLine(cx, cy - ROPE_LENGTH, cx, cy - DOLL_H / 2,
                        180, 140, 80, 255, true);

        // Doll body placeholder (brown rectangle)
        SDL_Rect body = { cx - DOLL_W / 2, cy - DOLL_H / 2, DOLL_W, DOLL_H };
        render.DrawRectangle(body, 160, 100, 60, 230, true, true);
        render.DrawRectangle(body, 20, 10, 0, 220, false, true);
    }
    else if (state_ == DropDollState::FALLING)
    {
        SDL_Rect body = { cx - DOLL_W / 2, cy - DOLL_H / 2, DOLL_W, DOLL_H };
        render.DrawRectangle(body, 200, 80, 40, 230, true, true);
        render.DrawRectangle(body, 20, 10, 0, 220, false, true);
    }
    else // ATTACHED — purple tint, fades slightly as timer progresses
    {
        float pct   = 1.0f - (attachTimer_ / ATTACH_DURATION);
        Uint8 alpha = (Uint8)(180.0f * pct + 60.0f);
        SDL_Rect body = { cx - DOLL_W / 2, cy - DOLL_H / 2, DOLL_W, DOLL_H };
        render.DrawRectangle(body, 160, 40, 200, alpha, true, true);
        render.DrawRectangle(body, 20, 10, 0, alpha, false, true);
    }
}
