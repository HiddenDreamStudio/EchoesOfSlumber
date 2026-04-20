#pragma once

#include "Entity.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include <cmath>

struct SDL_Texture;

enum class EnemyState { IDLE, PATROL, CHASE, ATTACK, STUNNED, DEATH };

class Enemy : public Entity
{
public:

	Enemy();
	virtual ~Enemy();
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

private:
	void UpdateFSM(float dt);
	void TransitionTo(EnemyState newState);
	void GetPhysicsValues();
	void ApplyPhysics();
	void Draw(float dt);

public:
	float speed = 2.5f;
	SDL_Texture* texture = nullptr;
	int texW = 64;
	int texH = 64;
	PhysBody* pbody = nullptr;

private:
	b2Vec2 velocity = { 0.0f, 0.0f };
	bool   facingRight_ = false;

	// Animations
	AnimationSet anims;
	AnimationSet rollAnims_;
	SDL_Texture* rollTexture_ = nullptr;

	// FSM
	EnemyState state_      = EnemyState::IDLE;
	float      stateTimer_ = 0.0f;

	// Tunable constants
	static constexpr float DETECTION_RADIUS      = 300.0f;  // px to start chasing
	static constexpr float ATTACK_RANGE          = 100.0f;  // px to trigger attack
	static constexpr float IDLE_DURATION         = 1500.0f; // ms before starting patrol
	static constexpr float STUN_DURATION         = 600.0f;  // ms stunned after attack or hit
	static constexpr float ATTACK_SPEED          = 12.0f;   // rush speed
	static constexpr float ATTACK_DURATION       = 350.0f;  // ms of rush
	static constexpr float ROLL_DRAW_SCALE       = 0.5f;    // 256px frame drawn at 128px

	// Patrol
	float patrolLeftX_  = 0.0f;
	float patrolRightX_ = 0.0f;
	float patrolDirX_   = 1.0f;

	// Attack direction (set when entering ATTACK state)
	float attackDirX_ = 1.0f;

	// Contact damage
	static constexpr float CONTACT_DAMAGE_INTERVAL = 1000.0f;
	bool    isContactWithPlayer_    = false;
	float   contactDamageCooldown_  = 0.0f;
	Entity* playerListener_         = nullptr;
};
