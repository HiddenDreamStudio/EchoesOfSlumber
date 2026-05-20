#pragma once
#include "Enemy.h"
#include <string>

class Boss : public Enemy
{
public:
    Boss(EntityType type);
    virtual ~Boss() = default;
    bool Update(float dt) override;

    bool  IsEngaged()        const { return isEngaged_; }
    bool  IsDead()           const { return isDead_; }
    int   GetPhase()         const { return phase_; }
    float GetHealthPercent() const;

    virtual const char* GetBossName() const = 0;
    void SetTriggerRadius(float r) { triggerRadius_ = r; }
    void SetArenaLimits(float minX, float maxX);

protected:
    virtual void OnPhaseChange(int newPhase) {}
    bool CheckProximityTrigger() const;
    void CheckPhaseTransition();
    void ClampToArena();

    bool  isEngaged_     = false;
    bool  isDead_        = false;
    int   phase_         = 1;
    float triggerRadius_ = 500.0f;
    float arenaMinX_     = 0.0f;
    float arenaMaxX_     = 99999.0f;

    static constexpr float PHASE2_THRESHOLD = 0.5f;
};
