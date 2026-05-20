#pragma once
#include "Enemy.h"

class Boss : public Enemy
{
public:
    Boss(EntityType type);
    bool Update(float dt) override;

    bool  IsEngaged()        const { return isEngaged_; }
    bool  IsDead()           const { return isDead_; }
    int   GetPhase()         const { return phase_; }
    float GetHealthPercent() const;

    virtual const char* GetBossName() const = 0;
    void SetTriggerRadius(float r) { triggerRadius_ = r; }

protected:
    virtual void OnPhaseChange(int newPhase) {}
    bool CheckProximityTrigger() const;
    void CheckPhaseTransition();

    bool  isEngaged_    = false;
    bool  isDead_       = false;
    int   phase_        = 1;
    float triggerRadius_ = 500.0f;

    static constexpr float PHASE2_THRESHOLD = 0.5f;
};
