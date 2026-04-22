#pragma once

#include "Entity.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

struct SDL_Texture;

class EnemyC : public Entity
{
public:
	// FSM states
	enum class State {
		IDLE,
		PATROL,
		ALERT,
		SHOOT,
		FLEE
	};

	EnemyC();
	virtual ~EnemyC();

	bool Awake();
	bool Start();
	bool Update(float dt);
	bool CleanUp();
	bool Destroy();

	void OnCollision(PhysBody* physA, PhysBody* physB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB);

	void SetPosition(Vector2D pos);
	Vector2D GetPosition();
	void TakeDamage(int damage) override;

public:
	float speed = 1.5f;
	SDL_Texture* texture = nullptr;
	int texW = 128, texH = 128;
	PhysBody* pbody = nullptr;

private:
	// FSM
	void UpdateFSM(float dt);
	void EnterState(State newState);

	// Helpers
	void PerformPathfinding();
	void GetPhysicsValues();
	void MoveToward(float targetX);
	void MoveAwayFrom(float targetX);
	void ApplyPhysics();
	void Shoot();
	void Draw(float dt);

	int GetDistanceToPlayer() const;

private:
	State currentState_ = State::IDLE;
	b2Vec2 velocity = { 0.0f, 0.0f };

	AnimationSet anims;
	std::shared_ptr<Pathfinding> pathfinding;

	// FSM parameters
	static constexpr int   DETECT_RANGE = 15;      // tiles (manhattan)
	static constexpr int   SHOOT_RANGE = 10;        // tiles
	static constexpr int   FLEE_RANGE = 4;          // tiles
	static constexpr float PATROL_SPEED = 1.5f;
	static constexpr float FLEE_SPEED = 3.0f;
	static constexpr float SHOOT_COOLDOWN = 2000.0f; // ms
	static constexpr float IDLE_DURATION = 1500.0f;  // ms before patrol

	float shootCooldown_ = 0.0f;
	float idleTimer_ = 0.0f;

	// Patrol
	float patrolOriginX_ = 0.0f;
	float patrolDirection_ = 1.0f;
	static constexpr float PATROL_RANGE = 100.0f; // px from origin

	// Contact damage (same pattern as Enemy)
	static constexpr float ATTACK_INTERVAL = 1000.0f;
	bool    isContactWithPlayer_ = false;
	float   attackCooldown_ = 0.0f;
	Entity* playerListener_ = nullptr;

	// Facing
	bool facingRight_ = false;
};
