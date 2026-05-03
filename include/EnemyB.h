#pragma once

#include "Entity.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include <cmath>

struct SDL_Texture;

enum class EnemyBState { IDLE, PATROL, TURNING, CHASE, ATTACK, STUNNED, DEATH };

class EnemyB : public Entity
{
public:
	EnemyB();
	virtual ~EnemyB();
	bool Awake();
	bool Start();
	bool Update(float dt);
	bool CleanUp();
	void OnCollision(PhysBody* physA, PhysBody* physB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB);
	void SetPosition(Vector2D pos);
	Vector2D GetPosition();
	bool Destroy();
	void TakeDamage(int damage) override;
	void SetPatrolPoints(float leftX, float rightX);

public:
	float speed = 2.0f;
	SDL_Texture* walkTexture = nullptr;
	SDL_Texture* turnTexture = nullptr;
	int texW = 128;
	int texH = 128;
	PhysBody* pbody = nullptr;

private:
	void UpdateFSM(float dt);
	void TransitionTo(EnemyBState newState);
	void GetPhysicsValues();
	void ApplyPhysics();
	void Draw(float dt);

	b2Vec2 velocity = { 0.0f, 0.0f };
	bool   facingRight_ = false;

	AnimationSet walkAnims_;
	AnimationSet turnAnims_;

	EnemyBState state_ = EnemyBState::IDLE;
	float       stateTimer_ = 0.0f;

	// Tunable constants
	static constexpr float DETECTION_RADIUS = 250.0f;
	static constexpr float ATTACK_RANGE = 75.0f;
	static constexpr float ATTACK_RANGE_Y = 60.0f;
	static constexpr float IDLE_DURATION = 1000.0f;
	static constexpr float ATTACK_DURATION = 400.0f;
	static constexpr float STUN_DURATION = 1800.0f;

	// Patrol
	float patrolLeftX_ = 0.0f;
	float patrolRightX_ = 0.0f;
	float patrolDirX_ = 1.0f;

	// Contact damage
	static constexpr float CONTACT_DAMAGE_INTERVAL = 1000.0f;
	bool    isContactWithPlayer_ = false;
	float   contactDamageCooldown_ = 0.0f;
	Entity* playerListener_ = nullptr;

	// Return-to-origin when player hides
	Vector2D originPosition_;
	bool wasPlayerHiding_ = false;
};