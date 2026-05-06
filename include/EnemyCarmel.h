#pragma once

#include "Enemy.h"
#include "Animation.h"

enum class EnemyCarmelState { IDLE, PATROL, CHASE, SCARED, BLOWUP, DEATH };

class EnemyCarmel : public Enemy
{
public:
	EnemyCarmel();
	~EnemyCarmel() override;

	bool Start() override;
	bool CleanUp() override;
	void TakeDamage(int damage) override;

	void SetPatrolPoints(float leftX, float rightX);

	// Override collision so this enemy does NOT deal contact damage
	void OnCollision(PhysBody* physA, PhysBody* physB) override;
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB) override;

private:
	void UpdateFSM(float dt) override;
	void TransitionTo(EnemyCarmelState newState);
	void Draw(float dt) override;
	void DealExplosionDamage();

	// Animation sets & textures for each state
	AnimationSet idleAnims_;
	AnimationSet rollAnims_;
	AnimationSet scaredAnims_;
	AnimationSet blowupAnims_;

	SDL_Texture* rollTexture_    = nullptr;
	SDL_Texture* scaredTexture_  = nullptr;
	SDL_Texture* blowupTexture_  = nullptr;

	EnemyCarmelState state_      = EnemyCarmelState::IDLE;
	float            stateTimer_ = 0.0f;
	bool             facingRight_ = false;
	bool             hasExploded_ = false;

	float patrolLeftX_  = 0.0f;
	float patrolRightX_ = 0.0f;
	float patrolDirX_   = 1.0f;

	// Pulse timer for the explosion radius indicator
	float pulseTimer_ = 0.0f;

	static constexpr float DETECTION_RADIUS  = 300.0f;
	static constexpr float ARRIVE_RANGE      = 60.0f;   // stops next to player, not on top
	static constexpr float IDLE_DURATION     = 1500.0f;
	static constexpr float EXPLOSION_RADIUS  = 90.0f;   // area of effect in pixels
	static constexpr float EXPLOSION_DAMAGE  = 2;
	static constexpr float ROLL_DRAW_SCALE   = 0.5f;
	static constexpr float BLOWUP_DRAW_SCALE = 0.5f;
};
