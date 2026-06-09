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
    DEATH,
    PHASE_TRANSITION, // VolverCaja_F1 → hold 2s → SalirCaja_F2 → Phase 2 IDLE
    // ── Phase 2 ──────────────────────────────────────────────────────────────
    GROUND_SLAM,    // telegraph for vertical punch
    GROUND_PUNCH,   // fist hits ground, hitbox active
    GROUND_RETRACT  // arm retracts, tall wave spawns → leads to EXPOSE_BUTTON
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
    void SpawnSlamHitbox();
    void DestroySlamHitbox();
    void SpawnTallWave();
    void UpdateTallWave(float dt);
    void DestroyTallWave();
    void SpawnButtonSensor();
    void DestroyButtonSensor();

    Boss2State state_      = Boss2State::WAITING;
    float      stateTimer_ = 0.0f;
    float      attackDir_  = 1.0f;  // +1 = right, -1 = left (toward player, fixed at fight start)

    // ── Extra physics bodies ──────────────────────────────────────────────────
    PhysBody* punchBody_     = nullptr; // sensor active during PUNCH
    PhysBody* shockwaveBody_ = nullptr; // moving sensor at floor level
    PhysBody* buttonBody_    = nullptr; // sensor active during EXPOSE_BUTTON
    PhysBody* slamBody_      = nullptr; // sensor active during GROUND_PUNCH
    PhysBody* tallWaveBody_  = nullptr; // tall moving sensor (phase 2)

    // ── Shockwave state ───────────────────────────────────────────────────────
    bool  shockwaveActive_     = false;
    float shockwaveX_          = 0.0f;
    float shockwaveY_          = 0.0f;
    float shockwavePhysY_      = 0.0f;
    float shockwaveStart_      = 0.0f;
    float shockwaveSpeed_      = 0.0f; // set per-spawn based on phase
    int   shockwaveFrame_      = 0;
    int   shockwaveIdleFrame_  = 0;
    float shockwaveFrameTimer_ = 0.0f;
    bool  shockwaveOutro_      = false;

    // ── Tall wave state (phase 2) ─────────────────────────────────────────────
    bool  tallWaveActive_      = false;
    float tallWaveX_           = 0.0f;
    float tallWaveY_           = 0.0f;
    float tallWavePhysY_       = 0.0f;
    float tallWaveStart_       = 0.0f;
    int   tallWaveFrame_       = 0;
    int   tallWaveIdleFrame_   = 0;
    float tallWaveFrameTimer_  = 0.0f;
    bool  tallWaveOutro_       = false;

    // ── Attack cycle ─────────────────────────────────────────────────────────
    int attackCycle_     = 0;
    int nextExposeCycle_ = 2; // expose button after this many attack cycles (2 or 3)
    int transSubPhase_   = 0; // sub-phase during PHASE_TRANSITION (0=VolverCaja,1=hold,2=SalirCaja2)

    // ── Box / clown dimensions ────────────────────────────────────────────────
    static constexpr int BOX_W       = 120; // px
    static constexpr int BOX_H       = 100; // px
    static constexpr int CLOWN_W     = 60;
    static constexpr int CLOWN_H     = 70;
    static constexpr int BUTTON_W    = 40;
    static constexpr int BUTTON_H    = 40;

    // ── Punch ─────────────────────────────────────────────────────────────────
    static constexpr float PUNCH_REACH  = 200.0f; // px — fixed reach toward player (matches fist sprite extension)
    static constexpr float PUNCH_H_SIZE =  50.0f; // hitbox height (player can duck or jump)

    // ── Shockwave ─────────────────────────────────────────────────────────────
    static constexpr float SHOCKWAVE_SPEED = 250.0f; // px/s
    static constexpr float SHOCKWAVE_RANGE = 700.0f; // max distance before despawn
    static constexpr int   SHOCKWAVE_W     =  28;
    static constexpr int   SHOCKWAVE_H     =  30;

    // ── Timers (phase 1) ──────────────────────────────────────────────────────
    static constexpr float INTRO_DURATION   =  720.0f;
    static constexpr float CLOSE_DURATION   =  500.0f;
    static constexpr float IDLE_DURATION    =  800.0f;
    static constexpr float CHARGE_DURATION  =  800.0f;
    static constexpr float PUNCH_DURATION   =  770.0f;
    static constexpr float RETRACT_DURATION =  500.0f;
    static constexpr float REST_DURATION    = 1600.0f;
    static constexpr float EXPOSE_DURATION  = 3000.0f;
    static constexpr float STUN_DURATION    =  500.0f;
    static constexpr float DEATH_DURATION   = 1500.0f;

    // ── Timers (phase 2 overrides) ────────────────────────────────────────────
    static constexpr float CHARGE_DURATION_P2       =  500.0f;
    static constexpr float REST_DURATION_P2         =  900.0f;
    static constexpr float EXPOSE_DURATION_P2       = 4000.0f;
    static constexpr float GROUND_SLAM_DURATION     =  700.0f;
    static constexpr float GROUND_PUNCH_DURATION    = 1100.0f;
    static constexpr float GROUND_RETRACT_DURATION  =  400.0f;

    // ── Shockwave speeds ──────────────────────────────────────────────────────
    static constexpr float SHOCKWAVE_SPEED_P2  = 370.0f;

    // ── Tall wave ─────────────────────────────────────────────────────────────
    static constexpr float TALL_WAVE_SPEED = 320.0f;
    static constexpr int   TALL_WAVE_W     =  28;
    static constexpr int   TALL_WAVE_H     =  68; // jumpeable but tight

    // ── Textures & Animations ─────────────────────────────────────────────────
    SDL_Texture* texWaveIdle_    = nullptr; // looping animation during wave hold phase

    // ── Phase 1 ───────────────────────────────────────────────────────────────
    SDL_Texture* texSale_        = nullptr;
    SDL_Texture* texIdle_        = nullptr;
    SDL_Texture* texPunch_       = nullptr;
    SDL_Texture* texRetract_     = nullptr;
    SDL_Texture* texCansado_     = nullptr;
    SDL_Texture* texRest_        = nullptr;
    SDL_Texture* texTapaBtn_     = nullptr;
    SDL_Texture* texHit_         = nullptr;

    Animation animSale_;
    Animation animIdle_;
    Animation animPunch_;
    Animation animRetract_;
    Animation animCansado_;
    Animation animRest_;
    Animation animTapaBtn_;
    Animation animHit_;

    // ── Phase 2 ───────────────────────────────────────────────────────────────
    SDL_Texture* texSale2_        = nullptr;
    SDL_Texture* texIdle2_        = nullptr;
    SDL_Texture* texCansado2_     = nullptr;
    SDL_Texture* texRest2_        = nullptr;
    SDL_Texture* texTapaBtn2_     = nullptr;
    SDL_Texture* texGroundSlam_   = nullptr;
    SDL_Texture* texGroundRetract_= nullptr;
    SDL_Texture* texHit2_         = nullptr;
    SDL_Texture* texDeath2_       = nullptr;

    Animation animSale2_;
    Animation animIdle2_;
    Animation animCansado2_;
    Animation animRest2_;
    Animation animTapaBtn2_;
    Animation animGroundSlam_;
    Animation animGroundRetract_;
    Animation animPunch2_;    // Phase 2 horizontal punch  (texGroundSlam_, 14 frames)
    Animation animRetract2_;  // Phase 2 horizontal retract (texGroundRetract_, 10 frames)
    Animation animHit2_;
    Animation animDeath2_;

    // ── Shared ────────────────────────────────────────────────────────────────
    SDL_Texture* texShockwave_   = nullptr;
    SDL_Texture* texButton_      = nullptr;
    Animation animShockwave_;
};
