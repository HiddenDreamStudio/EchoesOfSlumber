#pragma once
#include "Entity.h"
#include "Physics.h"

enum class DropDollState { HANGING, FALLING, ATTACHED };

class DropDoll : public Entity
{
public:
    DropDoll();
    ~DropDoll();

    bool Start()           override;
    bool Update(float dt)  override;
    bool CleanUp()         override;

    void OnCollision(PhysBody* physA, PhysBody* physB) override;

    void SetTriggerWidth(float w) { triggerWidth_ = w; }

private:
    void SpawnBody();
    void Draw();

    DropDollState state_             = DropDollState::HANGING;
    PhysBody*     pbody_             = nullptr;
    float         triggerWidth_      = 80.0f;
    float         attachTimer_       = 0.0f;  // ms elapsed while attached
    float         originalSpeed_     = 4.0f;  // saved before slowing player

    static constexpr int   DOLL_W        = 40;
    static constexpr int   DOLL_H        = 56;
    static constexpr int   ROPE_LENGTH   = 140;
    static constexpr float ATTACH_DURATION = 4000.0f; // ms stuck on player
    static constexpr float SLOW_FACTOR     = 0.4f;    // fraction of normal speed
};
