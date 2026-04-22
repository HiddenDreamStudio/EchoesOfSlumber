#pragma once
#include "Entity.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

struct SDL_Texture;

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
	void PerformPathfinding();
	void GetPhysicsValues();
	void Move();
	void ApplyPhysics();
	void Draw(float dt);

public:
	float speed = 2.5f;
	SDL_Texture* texture = NULL;
	int texW, texH;
	PhysBody* pbody;

private:
	b2Vec2 velocity;
	AnimationSet anims;
	std::shared_ptr<Pathfinding> pathfinding;

	// Contact damage
	static constexpr float ATTACK_INTERVAL = 1000.0f;
	bool    isContactWithPlayer_ = false;
	float   attackCooldown_ = 0.0f;
	Entity* playerListener_ = nullptr;

	// Tracks whether the player was hiding on the previous frame.
	bool wasPlayerHiding_ = false;

	// ?? Return-to-origin ?????????????????????????????????????????????????????
	Vector2D originPosition_;
	Vector2D originTilePos_;
};