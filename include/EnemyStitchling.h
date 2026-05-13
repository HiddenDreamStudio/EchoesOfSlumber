#pragma once

#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>

enum class StitchlingState { IDLE, ACTIVATED, TRAPPING, REWIND_SLOW, REWIND_FAST, DEATH };

class EnemyStitchling : public Enemy
{
public:
	EnemyStitchling();
	virtual ~EnemyStitchling();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OnCollision(PhysBody* physA, PhysBody* physB) override;
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB) override;

	void TakeDamage(int damage) override;

private:
	void UpdateFSM(float dt) override;
	void Draw(float dt) override;

	void TransitionTo(StitchlingState newState);

	StitchlingState state_ = StitchlingState::IDLE;
	float stateTimer_ = 0.0f;

	PhysBody* trapSensor_ = nullptr;
	bool isPlayerInTrap_ = false;

	// Trap mechanics
	float trapDuration_ = 3000.0f; // 3 seconds
	float damageInterval_ = 1000.0f;
	float damageTimer_ = 0.0f;
	int escapeMashes_ = 0;
	static constexpr int REQUIRED_MASHES = 10;

	// Visuals
	Animation idleAnim_;
	Animation activatedAnim_;
	Animation trappingAnim_;
	Animation rewindAnim_;
	
	SDL_Texture* stitchTexture_ = nullptr;

	Vector2D anchorPos_;
};
