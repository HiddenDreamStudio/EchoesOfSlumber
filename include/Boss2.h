#pragma once
#include "Boss.h"
#include "Animation.h"
#include <SDL3/SDL.h>
#include <cstdlib>

// ─────────────────────────────────────────────────────────────────────────────
// Boss2State — music box boss FSM
//
// Phase 1 loop:  IDLE → CHARGE → PUNCH → RETRACT → REST → IDLE …
//                Every 2-3 cycles: REST → EXPOSE_BUTTON → IDLE
// Phase 2 (TBD): extended in a future branch when room + art are ready.
// ─────────────────────────────────────────────────────────────────────────────
enum class Boss2State
{
    WAITING,        // dormant until proximity trigger
    INTRO,          // Sale_la_caja plays once at fight start
    IDLE,           // Cansado loop between attacks
    CHARGE,         // arm telegraph
    PUNCH,          // arm extended, hitbox active
    RETRACT,        // arm retracts, shockwave spawns
    REST,           // cooldown before next cycle
    EXPOSE_BUTTON,  // button visible and attackable
    CLOSE_BUTTON,   // Tapar_Boton plays when button window expires
    STUNNED,        // hit reaction
    DEATH
};

class Boss2 : public Boss
{
public:
    Boss2();
    ~Boss2() override;

    bool Start()   override;
    bool Update(float dt) override;   // overrides Boss::Update (static box, no velocity)
    bool CleanUp() override;

    void TakeDamage(int damage) override;
    void OnCollision(PhysBody* physA, PhysBody* physB) override;

    const char* GetBossName() const override { return "Music Box"; }

protected:
    void UpdateFSM(float dt)         override;
    void Draw(float dt)              override;
    void OnPhaseChange(int newPhase) override;

private:
    void TransitionTo(Boss2State newState);

    void SpawnBoxBody();
    void SpawnPunchHitbox();
    void DestroyPunchHitbox();
    void SpawnShockwave();
    void UpdateShockwave(float dt);
    void DestroyShockwave();
    void SpawnButtonSensor();
    void DestroyButtonSensor();

    Boss2State state_      = Boss2State::WAITING;
    float      stateTimer_ = 0.0f;
    float      deathTeleportTimer_ = 0.0f; // counts up once the death animation finishes
    bool       deathSequenceDone_  = false; // guards the teleport/portal-seal so they fire exactly once
    float      attackDir_  = 1.0f;  // +1 = right, -1 = left (toward player, fixed at fight start)

    // ── Extra physics bodies ──────────────────────────────────────────────────
    PhysBody* punchBody_     = nullptr; // sensor active during PUNCH
    PhysBody* shockwaveBody_ = nullptr; // moving sensor at floor level
    PhysBody* buttonBody_    = nullptr; // sensor active during EXPOSE_BUTTON

    // ── Shockwave state ───────────────────────────────────────────────────────
    bool  shockwaveActive_ = false;
    float shockwaveX_      = 0.0f;
    float shockwaveY_      = 0.0f; // visual Y (for drawing)
    float shockwavePhysY_  = 0.0f; // physics body Y (actual ground)
    float shockwaveStart_  = 0.0f;

    // ── Attack cycle ─────────────────────────────────────────────────────────
    int attackCycle_     = 0;
    int nextExposeCycle_ = 2; // expose button after this many attack cycles (2 or 3)

    // ── Box / clown dimensions ────────────────────────────────────────────────
    static constexpr int BOX_W       = 120; // px
    static constexpr int BOX_H       = 100; // px
    static constexpr int CLOWN_W     = 60;
    static constexpr int CLOWN_H     = 70;
    static constexpr int BUTTON_W    = 22;
    static constexpr int BUTTON_H    = 32;

    // ── Punch ─────────────────────────────────────────────────────────────────
    static constexpr float PUNCH_REACH  = 320.0f; // px — fixed reach toward player
    static constexpr float PUNCH_H_SIZE =  50.0f; // hitbox height (player can duck or jump)

    // ── Shockwave ─────────────────────────────────────────────────────────────
    static constexpr float SHOCKWAVE_SPEED = 250.0f; // px/s
    static constexpr float SHOCKWAVE_RANGE = 700.0f; // max distance before despawn
    static constexpr int   SHOCKWAVE_W     =  28;
    static constexpr int   SHOCKWAVE_H     =  30;

    // ── Timers ────────────────────────────────────────────────────────────────
    static constexpr float INTRO_DURATION   =  720.0f; // 9 frames * 80ms
    static constexpr float CLOSE_DURATION  =  500.0f; // 5 frames * 100ms
    static constexpr float IDLE_DURATION    =  800.0f;
    static constexpr float CHARGE_DURATION  =  800.0f;
    static constexpr float PUNCH_DURATION   =  770.0f; // 11 frames * 70ms
    static constexpr float RETRACT_DURATION =  500.0f;
    static constexpr float REST_DURATION    = 1600.0f; // 20 frames * 80ms
    static constexpr float EXPOSE_DURATION  = 3000.0f;
    static constexpr float STUN_DURATION    =  500.0f;
    static constexpr float DEATH_DURATION   = 1500.0f;
    static constexpr float DEATH_TELEPORT_DELAY = 2000.0f; // ms after the death animation finishes before returning the player to the hub

    // ── Textures & Animations ─────────────────────────────────────────────────
    SDL_Texture* texSale_        = nullptr;
    SDL_Texture* texPunch_       = nullptr;
    SDL_Texture* texRest_        = nullptr;
    SDL_Texture* texDestapaBtn_  = nullptr;
    SDL_Texture* texTapaBtn_     = nullptr;
    SDL_Texture* texHit_         = nullptr;
    SDL_Texture* texShockwave_   = nullptr;

    Animation animSale_;
    Animation animPunch_;
    Animation animRest_;
    Animation animDestapaBtn_;
    Animation animTapaBtn_;
    Animation animHit_;
    Animation animShockwave_;
};
