#pragma once
#include "Boss.h"
#include "Animation.h"
#include <cstdlib>
#include <SDL3/SDL.h>

// ─────────────────────────────────────────────────────────────────────────────
// Boss1State — full state machine for the first boss.
//
// Phase 1 loop:  UNDERGROUND → JUMP → VULNERABLE → DIVE → UNDERGROUND …
//
// Phase 2 loop:  P2_WALL_MOVE (wall-to-wall, instant kill) →
//                  occasionally P2_SPIT (projectile + sticky puddle)
//                  occasionally UNDERGROUND (P1-style surface attack, faster)
//                  → back to P2_WALL_MOVE
// ─────────────────────────────────────────────────────────────────────────────
enum class Boss1State
{
    // ── Common ────────────────────────────────────────────────────────────────
    WAITING,          // Dormant until the proximity trigger fires
    STUNNED,          // Brief hit-reaction during a vulnerable window
    DEATH,

    // ── Phase 1 ───────────────────────────────────────────────────────────────
    UNDERGROUND,      // Tracking player X below the floor (no physics body)
    JUMP,             // Emerging from ground — ballistic arc, contact damage
    VULNERABLE,       // On surface, attackable — timer-driven
    DIVE,             // Sinking back underground

    // ── Phase 2 ───────────────────────────────────────────────────────────────
    P2_INTRO,         // Brief pause when entering phase 2 (player prep time)
    P2_WALL_MOVE,     // Underground wall-to-wall, mouth open — instant kill on contact
    P2_SPIT,          // Pauses at wall, spits a projectile toward the player
};

class Boss1 : public Boss
{
public:
    Boss1();
    ~Boss1() override;

    bool Start()   override;
    bool CleanUp() override;

    void TakeDamage(int damage) override;
    void OnCollision(PhysBody* physA, PhysBody* physB) override;
    void OnCollisionEnd(PhysBody* physA, PhysBody* physB) override;

    const char* GetBossName() const override { return "Drowning Plush"; }

    // Called by Map.cpp after spawning
    void SetUndergroundDepth(float d)         { undergroundDepth_ = d; }
    void SetFloorY(float y)                   { floorY_ = y; }
    void SetWallBounds(float left, float right){ p2WallLeft_ = left; p2WallRight_ = right; }

protected:
    void UpdateFSM(float dt)         override;
    void Draw(float dt)              override;
    void OnPhaseChange(int newPhase) override;

private:
    void TransitionTo(Boss1State newState);
    void SpawnPhysicsBody();
    void DestroyPhysicsBody();

    // ── Phase 2 helpers ───────────────────────────────────────────────────────
    void SpawnSpit(float dirX);
    void DestroySpit();
    void SpawnPuddle(float x);
    void DestroyPuddle();
    void UpdateP2(float dt);  // spit flight + puddle timer each frame

    // ─────────────────────────────────────────────────────────────────────────

    Boss1State state_      = Boss1State::WAITING;
    float      stateTimer_ = 0.0f;

    // ── Underground tracking ──────────────────────────────────────────────────
    float undergroundX_    = 0.0f;
    float floorY_          = 0.0f;
    float undergroundDepth_= 200.0f;
    float alignedTimer_    = 0.0f;
    float accumulateTimer_ = 0.0f;
    float exitPauseTimer_  = 0.0f;

    // ── Jump (manual ballistic arc) ───────────────────────────────────────────
    bool  floorCalibrated_ = false;

    static constexpr float JUMP_V0_PX        = 10.0f;
    static constexpr float GAME_GRAVITY_PX   = 500.0f;
    static constexpr float JUMP_CONTACT_RANGE = 80.0f;
    static constexpr float MIN_JUMP_TIME      = 400.0f;
    static constexpr float MAX_JUMP_TIME      = 3500.0f;

    // ── Alignment ─────────────────────────────────────────────────────────────
    static constexpr float PLAYER_SPRITE_ALIGN_X = 16.0f;

    // ── Timers / thresholds ───────────────────────────────────────────────────
    static constexpr float MIN_ALIGN_TIME         = 600.0f;  // P1 — ms player must be over strip
    static constexpr float MIN_ALIGN_TIME_P2      = 250.0f;  // P2 — faster
    static constexpr float EXIT_PAUSE_DURATION    = 400.0f;
    static constexpr float PRE_JUMP_DELAY         = 900.0f;  // P1 tell duration
    static constexpr float PRE_JUMP_DELAY_P2      = 400.0f;  // P2 tell duration
    static constexpr float JUMP_ALIGN_THRESHOLD   = 24.0f;
    static constexpr float VULNERABLE_DURATION_P1 = 4000.0f;
    static constexpr float VULNERABLE_DURATION_P2 = 2500.0f;
    static constexpr float STUN_DURATION          = 380.0f;
    static constexpr float DIVE_DURATION          = 300.0f;
    static constexpr float DEATH_DURATION         = 1200.0f;

    // ── Movement ─────────────────────────────────────────────────────────────
    static constexpr float UNDERGROUND_SPEED_P1 = 180.0f;
    static constexpr float UNDERGROUND_SPEED_P2 = 280.0f;  // surface attack in P2

    // ── Phase 2 wall-to-wall ──────────────────────────────────────────────────
    float p2WallLeft_      = 0.0f;  // set by Tiled property
    float p2WallRight_     = 0.0f;
    float p2MoveDir_       = 1.0f;  // +1 right, -1 left
    int   p2WallCrosses_   = 0;     // crossings since last surface attack
    int   p2NextSpit_      = 2;     // crossing to spit at (1–3, randomised)
    int   p2NextSurface_   = 9;     // crossing to surface at (8–10, randomised)

    static constexpr float P2_INTRO_DURATION  = 2000.0f; // ms pause before wall move starts
    static constexpr float P2_WALL_SPEED      = 350.0f; // px/s
    static constexpr float P2_WALL_THRESHOLD  = 30.0f;  // px from wall → reached
    static constexpr float P2_KILL_HALF_W     = 48.0f;  // = BODY_W/2, matches visible strip exactly
    static constexpr float PLAYER_BODY_HALF_H = 50.0f;  // player capsule half-height (height=100)

    // ── Phase 2 spit ─────────────────────────────────────────────────────────
    PhysBody* p2SpitBody_        = nullptr;
    bool      p2SpitHitPlayer_   = false;  // deferred collision flags
    bool      p2SpitHitFloor_    = false;
    float     p2SpitFloorX_      = 0.0f;

    static constexpr float P2_SPIT_PAUSE    = 500.0f;  // ms pause at wall before spit
    static constexpr float P2_SPIT_VX       = 7.0f;    // m/s horizontal minimum
    static constexpr float P2_SPIT_VY       = -6.0f;   // m/s upward (higher = more arc)
    static constexpr int   P2_SPIT_R        = 14;      // px radius

    // ── Phase 2 puddle ────────────────────────────────────────────────────────
    PhysBody* p2PuddleBody_      = nullptr;
    float     p2PuddleTimer_     = 0.0f;
    bool      p2PlayerInPuddle_  = false;
    float     p2OriginalSpeed_   = 4.0f;

    static constexpr float P2_PUDDLE_DURATION = 8000.0f; // ms
    static constexpr float P2_PUDDLE_SLOW     = 0.4f;    // fraction of normal speed/jump
    static constexpr int   P2_PUDDLE_W        = 90;
    static constexpr int   P2_PUDDLE_H        = 18;

    float p2OriginalJumpForce_ = 10.0f;

    // ── Phase 2 attack counter (P1-style surface attack) ─────────────────────
    int  attackCount_          = 0;
    bool p2WasHitInVulnerable_ = false; // true → DIVE goes to P2_WALL_MOVE; false → loops surface

    // ── Sprite dimensions ─────────────────────────────────────────────────────
    static constexpr int BODY_W      = 96;
    static constexpr int BODY_H      = 128;
    static constexpr int BODY_H_OPEN = 40;

    // ── Animations (all Drowning Plush — frame width = frame height) ──────────
    bool facingRight_ = false;

    SDL_Texture* texSpit_    = nullptr; // Escupitajo prop
    SDL_Texture* texPuddle_  = nullptr; // Charco prop

    SDL_Texture* texMove_    = nullptr; // Moverse bajo suelo  — 73×73,  28 frames
    SDL_Texture* texAlert_   = nullptr; // Idle Cansado        — 128×128, 16 frames
    SDL_Texture* texAttack_  = nullptr; // Ataque              — 79×79,  26 frames
    SDL_Texture* texHit_     = nullptr; // Hit                 — 256×256, 6 frames
    SDL_Texture* texDive_    = nullptr; // Sumergirse          — 114×114, 18 frames

    Animation animMove_;
    Animation animAlert_;
    Animation animAttack_;
    Animation animHit_;
    Animation animDive_;
};
