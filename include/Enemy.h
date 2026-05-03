#pragma once

#include "Entity.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>

struct SDL_Texture;

class Enemy : public Entity
{
public:
	Enemy();
	virtual ~Enemy();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OnCollision(PhysBody* physA, PhysBody* physB) override;
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB) override;

	void SetPosition(Vector2D pos);
	Vector2D GetPosition();

	bool Destroy() override;
	void TakeDamage(int damage) override;

	float speed = 2.5f;
	SDL_Texture* texture = nullptr;
	int texW = 64;
	int texH = 64;
	PhysBody* pbody = nullptr;

protected:
	virtual void UpdateFSM(float dt) = 0;
	virtual void Draw(float dt) = 0;

	void GetPhysicsValues();
	void ApplyPhysics();

	b2Vec2 velocity = { 0.0f, 0.0f };

	static constexpr float CONTACT_DAMAGE_INTERVAL = 1000.0f;
	bool    isContactWithPlayer_   = false;
	float   contactDamageCooldown_ = 0.0f;
	Entity* playerListener_        = nullptr;
};
