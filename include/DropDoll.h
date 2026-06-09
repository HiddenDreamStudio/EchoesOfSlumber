#pragma once
#include "Entity.h"
#include "Physics.h"
#include "Animation.h"
#include <SDL3/SDL.h>

// HANGING → RELEASING → FALLING → GRABBING → ATTACHED → DETACHING (success)
//                          └─────────────────────────────→ DYING (misses the player)
enum class DropDollState { HANGING, RELEASING, FALLING, GRABBING, ATTACHED, DETACHING, DYING };

class DropDoll : public Entity
{
public:
    DropDoll();
    ~DropDoll();

    bool Start()           override;
    bool Update(float dt)  override;
    bool PostUpdate()      override;
    bool CleanUp()         override;

    void OnCollision(PhysBody* physA, PhysBody* physB) override;

    void SetTriggerWidth(float w) { triggerWidth_ = w; }

private:
    void SpawnBody();
    void Draw();
    void DrawMinigameBar();

    DropDollState state_         = DropDollState::HANGING;
    PhysBody*     pbody_         = nullptr;
    float         triggerWidth_  = 80.0f;
    float         attachTimer_   = 0.0f;

    // Set on the first non-player collision while falling; the doll keeps
    // falling under gravity until it comes to rest, then plays its death
    // animation on the ground instead of mid-air.
    bool  hasMissedPlayer_ = false;
    // How long the doll has been in FALLING (counts from the moment it lets
    // go of the rope, not from the miss). Used purely to arm the landing
    // check below — see hasBeenFalling_.
    float fallElapsedTime_ = 0.0f;
    // Guards against a false "landed" reading on the very first frames after
    // letting go. NOTE: this can't be a velocity check — when the very first
    // collision IS the floor (a clean drop with nothing grazed on the way
    // down), the contact solver has already zeroed vy by the time Update
    // reads it back, so a "has it been moving fast" test would never arm and
    // it'd fall through to the timeout every single time. Elapsed time isn't
    // fooled by that: any real fall takes well over this long, so by the time
    // it can land for real, the check is already armed — while the doll's
    // brief moment of near-zero velocity right at spawn (before gravity has
    // had time to act) safely stays under it.
    bool  hasBeenFalling_  = false;
    static constexpr float MIN_FALL_TIME_BEFORE_LANDING_CHECK = 150.0f; // ms
    // Absolute last-resort backstop in case it somehow never reads as settled
    // (e.g. falls into a bottomless pit) — long enough to never fire while
    // genuinely still falling.
    float missedTimer_     = 0.0f;
    static constexpr float MAX_FALL_AFTER_MISS = 2500.0f; // ms — absolute safety net

    // Post-detach fall (after a successful grab + minigame): once it lets go
    // of the player's head it must still drop to the ground and disintegrate
    // there, just like a miss — but WITHOUT a physics body. A solid body
    // spawned right where it detached would simply rest on top of the player
    // it just released (their colliders overlap) instead of passing through.
    // So instead it free-falls manually in a straight line down to a
    // raycast-found floor, landing the instant it gets there — no physics
    // jitter, no waiting.
    bool  isPostDetachFall_    = false;
    float postDetachGroundY_   = 0.0f;
    float postDetachVelocityY_ = 0.0f;
    static constexpr float POST_DETACH_FALL_ACCEL = 500.0f; // px/s² — matches world gravity (10 m/s² × 50 px/m)

    // Minigame
    float indicatorPos_   = 0.0f;
    float indicatorDir_   = 1.0f;
    float indicatorSpeed_ = 0.75f;
    float zoneStart_      = 0.35f;
    float zoneEnd_        = 0.55f;
    int   failedAttempts_ = 0;

    // Minigame textures
    SDL_Texture* texShape_      = nullptr; // doll hanging (background)
    SDL_Texture* texBlackBase_  = nullptr; // dark overlay strip
    SDL_Texture* texFrame_      = nullptr; // stone frame
    SDL_Texture* texInside_     = nullptr; // success zone diamond
    SDL_Texture* texIndicator_  = nullptr; // moving indicator
    SDL_Texture* texPressSpace_ = nullptr; // "PULSA SPACE" text

    // Doll animations (SS_Enemics_Nivell3, all sheets 256x256 frames)
    SDL_Texture* texIdle_          = nullptr; // hangs from the rope, swaying
    SDL_Texture* texSoltarse_      = nullptr; // lets go of the rope
    SDL_Texture* texAire_          = nullptr; // falling loop
    SDL_Texture* texEnganchar_     = nullptr; // latches onto the player
    SDL_Texture* texEngancharLoop_ = nullptr; // wriggles on the player's head
    SDL_Texture* texDesenganchar_  = nullptr; // lets go of the player
    SDL_Texture* texDie_           = nullptr; // disintegrates

    Animation animIdle_;
    Animation animSoltarse_;
    Animation animAire_;
    Animation animEnganchar_;
    Animation animEngancharLoop_;
    Animation animDesenganchar_;
    Animation animDie_;

    static constexpr int   DOLL_W          = 40;
    static constexpr int   DOLL_H          = 56;
    static constexpr float DAMAGE_INTERVAL = 5000.0f;
    static constexpr float SPEED_INCREASE  = 0.08f;

    // Audio
    int   fxCaida_   = -1;
    int   fxAttach_  = -1;
    int   fxMolesta_ = -1;
    float molesTimer_ = 0.0f;
    static constexpr float MOLES_INTERVAL = 2200.0f; // ms — replay interval for the loop
};
