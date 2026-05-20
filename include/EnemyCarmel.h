#pragma once

#include "Enemy.h"
#include "Animation.h"

enum class EnemyCarmelState { IDLE, PATROL, CHASE, ATTACK, STUNNED, DEATH };

class EnemyCarmel : public Enemy
{
public:
	EnemyCarmel();
	~EnemyCarmel() override;

	bool Start() override;
	bool CleanUp() override;
	void TakeDamage(int damage) override;

	void SetPatrolPoints(float leftX, float rightX);

private:
	void UpdateFSM(float dt) override;
	void TransitionTo(EnemyCarmelState newState);
	void Draw(float dt) override;

	AnimationSet anims_;
	AnimationSet rollAnims_;
	SDL_Texture* rollTexture_ = nullptr;

	EnemyCarmelState state_      = EnemyCarmelState::IDLE;
	float            stateTimer_ = 0.0f;
	bool             facingRight_ = false;
	float            attackDirX_  = 1.0f;

	float patrolLeftX_  = 0.0f;
	float patrolRightX_ = 0.0f;
	float patrolDirX_   = 1.0f;

	static constexpr float DETECTION_RADIUS = 300.0f;
	static constexpr float ATTACK_RANGE     = 100.0f;
	static constexpr float IDLE_DURATION    = 1500.0f;
	static constexpr float STUN_DURATION    = 600.0f;
	static constexpr float ATTACK_SPEED     = 12.0f;
	static constexpr float ATTACK_DURATION  = 350.0f;
	static constexpr float ROLL_DRAW_SCALE  = 0.5f;
};
