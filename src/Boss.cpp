#include "Boss.h"
#include "Engine.h"
#include "Physics.h"
#include "Scene.h"
#include "tracy/Tracy.hpp"

Boss::Boss(EntityType type) : Enemy()
{
    this->type = type;
}

bool Boss::Update(float dt)
{
    ZoneScoped;

    if (Engine::GetInstance().scene->isPaused_ || Engine::GetInstance().scene->isGameOver_)
    {
        if (pbody) Draw(0.0f);
        return true;
    }

    bool hadBody = (pbody != nullptr);
    if (hadBody) GetPhysicsValues();
    UpdateFSM(dt);
    if (pbody && hadBody) ApplyPhysics();

    if (pbody)
    {
        if (contactDamageCooldown_ > 0.0f) contactDamageCooldown_ -= dt;
        if (isContactWithPlayer_ && playerListener_ != nullptr && contactDamageCooldown_ <= 0.0f)
        {
            playerListener_->TakeDamage(1);
            contactDamageCooldown_ = CONTACT_DAMAGE_INTERVAL;
        }
    }

    Draw(dt);
    return true;
}

float Boss::GetHealthPercent() const
{
    if (maxHealth <= 0) return 0.0f;
    return (float)health / (float)maxHealth;
}

bool Boss::CheckProximityTrigger() const
{
    Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
    float dx = playerPos.getX() - position.getX();
    float dy = playerPos.getY() - position.getY();
    return (dx * dx + dy * dy) < (triggerRadius_ * triggerRadius_);
}

void Boss::SetArenaLimits(float minX, float maxX)
{
    arenaMinX_ = minX;
    arenaMaxX_ = maxX;
}

void Boss::ClampToArena()
{
    if (!pbody) return;
    int bx, by;
    pbody->GetPosition(bx, by);
    if (bx < (int)arenaMinX_) pbody->SetPosition((int)arenaMinX_, by);
    if (bx > (int)arenaMaxX_) pbody->SetPosition((int)arenaMaxX_, by);
}

void Boss::CheckPhaseTransition()
{
    if (phase_ == 1 && GetHealthPercent() <= PHASE2_THRESHOLD)
    {
        phase_ = 2;
        OnPhaseChange(2);
    }
}
