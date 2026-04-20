#pragma once

#include "Entity.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"
#include <memory>

struct SDL_Texture;

// FSM states for the enemy
enum class EnemyState {
	IDLE,
	PATROL,
	CHASE,
	ATTACK,
	STUNNED,
	DEAD
};

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

private:
	void UpdateFSM(float dt);
	void PerformPathfinding();
	void GetPhysicsValues();
	void Move();
	void ApplyPhysics();
	void Draw(float dt);

public:
	float speed = 2.5f;
	SDL_Texture* texture = nullptr;

	// Initialized to fix C26495 warnings
	int texW = 64;
	int texH = 64;
	PhysBody* pbody = nullptr;

private:
	b2Vec2 velocity = { 0.0f, 0.0f };
	AnimationSet anims;
	std::shared_ptr<Pathfinding> pathfinding;

	// FSM parameters
	EnemyState currentState = EnemyState::IDLE;
	float stateTimer = 0.0f;
	float detectionRadius = 350.0f;
	float attackRadius = 100.0f;
	float dashSpeed = 12.0f;
	bool facingRight = true;

	// Contact damage
	static constexpr float ATTACK_INTERVAL = 1000.0f;
	bool    isContactWithPlayer_ = false;
	float   attackCooldown_ = 0.0f;
	Entity* playerListener_ = nullptr;
};