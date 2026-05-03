#pragma once

#include "Entity.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>

struct SDL_Texture;

class Projectile : public Entity
{
public:
	Projectile();
	virtual ~Projectile();

	bool Start();
	bool Update(float dt);
	bool CleanUp();
	bool Destroy();

	void OnCollision(PhysBody* physA, PhysBody* physB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB);

	void SetPosition(Vector2D pos);
	Vector2D GetPosition();

	// Set the direction the projectile flies in (normalized)
	void SetDirection(float dirX, float dirY);

public:
	SDL_Texture* texture = nullptr;
	PhysBody* pbody = nullptr;

private:
	static constexpr float SPEED = 5.0f;
	static constexpr float MAX_LIFETIME = 3000.0f; // ms
	static constexpr int   PROJ_SIZE = 16;          // AABB half-size for physics
	static constexpr float DRAW_SCALE = 0.5f;       // 64 * 0.5 = 32px on screen

	float dirX_ = 1.0f;
	float dirY_ = 0.0f;
	float lifetime_ = 0.0f;

	AnimationSet anims;
	int texW = 64;
	int texH = 64;
};
