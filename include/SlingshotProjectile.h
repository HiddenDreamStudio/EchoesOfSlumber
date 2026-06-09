#pragma once

#include "Entity.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>

struct SDL_Texture;

class SlingshotProjectile : public Entity
{
public:
	SlingshotProjectile();
	virtual ~SlingshotProjectile();

	bool Start();
	bool Update(float dt);
	bool CleanUp();
	bool Destroy();

	void OnCollision(PhysBody* physA, PhysBody* physB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB);

	void SetPosition(Vector2D pos);
	Vector2D GetPosition();

	// Set direction and power (speed magnitude) for the launch impulse
	void SetLaunch(float dirX, float dirY, float power);

public:
	SDL_Texture* texture = nullptr;
	PhysBody* pbody = nullptr;

private:
	static constexpr float MAX_LIFETIME = 4000.0f;  // ms
	static constexpr int   PROJ_RADIUS = 8;          // physics radius
	static constexpr float DRAW_SCALE = 0.5f;       // Reduced size to fit better 
	static constexpr int   DAMAGE = 1;

	float dirX_ = 1.0f;
	float dirY_ = 0.0f;
	float power_ = 5.0f;
	float lifetime_ = 0.0f;

	int texW = 0;
	int texH = 0;
};
