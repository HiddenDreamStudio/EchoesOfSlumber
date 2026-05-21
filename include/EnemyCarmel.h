#pragma once

#include "Enemy.h"
#include "Animation.h"

enum class EnemyCarmelState { IDLE, SCARED, BLOWUP, CHASE, DEFLATE, DEATH };

class EnemyCarmel : public Enemy
{
public:
	EnemyCarmel();
	~EnemyCarmel() override;

	bool Start() override;
	bool CleanUp() override;
	void TakeDamage(int damage) override;

	void SetPatrolPoints(float leftX, float rightX); // Kept for compatibility but unused

private:
	void UpdateFSM(float dt) override;
	void TransitionTo(EnemyCarmelState newState);
	void Draw(float dt) override;

	void UpdatePhysicsBody(bool big);

	AnimationSet anims_;
	AnimationSet rollAnims_;
	AnimationSet scaredAnims_;
	AnimationSet blowupAnims_;

	SDL_Texture* rollTexture_ = nullptr;
	SDL_Texture* scaredTexture_ = nullptr;
	SDL_Texture* blowupTexture_ = nullptr;

	EnemyCarmelState state_      = EnemyCarmelState::IDLE;
	float            stateTimer_ = 0.0f;
	bool             facingRight_ = false;

	float currentDirX_ = 1.0f;
	float turnDelayTimer_ = 0.0f;

	// Distances
	static constexpr float DETECTION_RADIUS = 150.0f;
	static constexpr float LOSE_RADIUS      = 400.0f;

	// Durations (ms)
	static constexpr float SCARED_DURATION  = 1100.0f; // 11 frames * 100ms
	static constexpr float BLOWUP_DURATION  = 900.0f;  // 6 frames * 150ms
	static constexpr float CHASE_SPEED      = 2.5f;
	static constexpr float TURN_DELAY       = 600.0f;  // Delay before changing direction

	// Sizes
	static constexpr float ROLL_DRAW_SCALE  = 0.5f; // Draw roll at half size (128x128)
};
