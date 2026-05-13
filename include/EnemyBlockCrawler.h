#pragma once

#include "Enemy.h"
#include <vector>
#include <cmath>

// ── Forward declarations ──
struct SDL_Texture;

// ── FSM states ──
enum class BlockCrawlerState {
	COMPACT,      // blocks stacked, bobbing vertically, patrolling
	DISPERSING,   // blocks spreading outward (transition)
	DISPERSED,    // blocks scattered, jittering, wider danger zone
	REASSEMBLING, // blocks returning to compact form (transition)
	DEATH
};

// ── Per-block data ──
struct CrawlerBlock {
	// Current offset from the enemy's physics-body centre
	float offsetX = 0.0f;
	float offsetY = 0.0f;

	// Target offset (used during transitions & dispersed jitter)
	float targetX = 0.0f;
	float targetY = 0.0f;

	// Compact-state stacking position
	float homeX = 0.0f;
	float homeY = 0.0f;

	// Dispersed-state rest position
	float scatterX = 0.0f;
	float scatterY = 0.0f;

	// Sinusoidal bob phase (compact state)
	float bobPhase = 0.0f;

	// Small random jitter velocity (dispersed state)
	float jitterVx = 0.0f;
	float jitterVy = 0.0f;
};

// ──────────────────────────────────────────────
class EnemyBlockCrawler : public Enemy
{
public:
	EnemyBlockCrawler();
	~EnemyBlockCrawler() override;

	bool Start() override;
	bool CleanUp() override;
	void TakeDamage(int damage) override;

	void SetPatrolPoints(float leftX, float rightX);

private:
	void UpdateFSM(float dt) override;
	void Draw(float dt) override;

	void TransitionTo(BlockCrawlerState newState);
	void GenerateScatterPositions();
	void CheckBlockPlayerCollision();

	// ── Blocks ──
	std::vector<CrawlerBlock> blocks_;
	static constexpr int   BLOCK_COUNT    = 5;
	static constexpr float BLOCK_SIZE     = 24.0f;

	// ── Texture region (sub-rect of test.jpg for each block) ──
	SDL_Rect blockSrcRect_ = { 0, 0, 64, 64 };

	// ── State machine ──
	BlockCrawlerState state_      = BlockCrawlerState::COMPACT;
	float             stateTimer_ = 0.0f;
	bool              facingRight_ = false;

	// ── Patrol ──
	float patrolLeftX_  = 0.0f;
	float patrolRightX_ = 0.0f;
	float patrolDirX_   = 1.0f;

	// ── Tunable constants (in milliseconds / pixels) ──
	static constexpr float COMPACT_DURATION     = 4000.0f;  // time in compact before dispersing
	static constexpr float DISPERSE_DURATION    = 3000.0f;  // time blocks stay scattered
	static constexpr float TRANSITION_DURATION  = 800.0f;   // disperse / reassemble transition time
	static constexpr float PATROL_SPEED         = 1.8f;
	static constexpr float DISPERSED_SPEED      = 0.8f;     // slower patrol while dispersed
	static constexpr float BOB_AMPLITUDE        = 6.0f;     // vertical bob pixels
	static constexpr float BOB_FREQUENCY        = 0.004f;   // radians per ms
	static constexpr float SCATTER_RANGE_X      = 60.0f;    // how far blocks spread horizontally
	static constexpr float SCATTER_RANGE_Y      = 10.0f;    // slight vertical variance when dispersed
	static constexpr float JITTER_STRENGTH      = 0.3f;     // random jitter magnitude
	static constexpr float BLOCK_DRAW_SCALE     = 1.0f;
	static constexpr float BLOCK_DAMAGE_RADIUS  = 18.0f;    // per-block hit radius for manual check
};
