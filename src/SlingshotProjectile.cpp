#include "SlingshotProjectile.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "Log.h"
#include "EntityManager.h"
#include "Scene.h"
#include <cmath>

SlingshotProjectile::SlingshotProjectile() : Entity(EntityType::SLINGSHOT_PROJECTILE)
{
	name = "SlingshotProjectile";
}

SlingshotProjectile::~SlingshotProjectile() {}

bool SlingshotProjectile::Start()
{
	// Load rock texture
	texture = Engine::GetInstance().textures->Load("assets/textures/Tirachinas/municio.png");

	// Create a small dynamic circle body for the rock
	pbody = Engine::GetInstance().physics->CreateCircle(
		(int)position.getX(), (int)position.getY(),
		PROJ_RADIUS, bodyType::DYNAMIC);

	pbody->listener = this;
	pbody->ctype = ColliderType::SLINGSHOT_PROJ;

	// Enable gravity for parabolic arc (gravity scale = 1.0 is default)
	b2Body_SetGravityScale(pbody->body, 1.0f);

	// Apply launch velocity (direct velocity, not impulse, so it matches trajectory calculation)
	b2Vec2 v;
	v.x = dirX_ * power_;
	v.y = dirY_ * power_;
	Engine::GetInstance().physics->SetLinearVelocity(pbody, v);

	return true;
}

bool SlingshotProjectile::Update(float dt)
{
	if (Engine::GetInstance().scene->isPaused_ || Engine::GetInstance().scene->isGameOver_) {
		// Still draw but don't advance lifetime
		int x, y;
		pbody->GetPosition(x, y);
		int drawW = (int)(texW * DRAW_SCALE);
		int drawH = (int)(texH * DRAW_SCALE);

		SDL_Rect section = { 0, 0, texW, texH };
		Engine::GetInstance().render->DrawTexture(texture, x - drawW / 2, y - drawH / 2,
			&section, 1.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, DRAW_SCALE);
		return true;
	}

	// Check lifetime
	lifetime_ += dt;
	if (lifetime_ >= MAX_LIFETIME) {
		Destroy();
		return true;
	}

	// Get current velocity for rotation
	b2Vec2 vel = Engine::GetInstance().physics->GetLinearVelocity(pbody);

	// Calculate rotation angle from velocity (in degrees)
	double angle = 0.0;
	if (std::abs(vel.x) > 0.01f || std::abs(vel.y) > 0.01f) {
		angle = std::atan2((double)vel.y, (double)vel.x) * RADTODEG;
	}

	// Draw
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	int drawW = (int)(texW * DRAW_SCALE);
	int drawH = (int)(texH * DRAW_SCALE);

	SDL_Rect section = { 0, 0, texW, texH };

	Engine::GetInstance().render->ApplyAmbientTint(texture);
	Engine::GetInstance().render->DrawTexture(texture, x - drawW / 2, y - drawH / 2,
		&section, 1.0f, angle, drawW / 2, drawH / 2, SDL_FLIP_NONE, DRAW_SCALE);
	Engine::GetInstance().render->ResetAmbientTint(texture);

	return true;
}

void SlingshotProjectile::OnCollision(PhysBody* physA, PhysBody* physB)
{
	switch (physB->ctype)
	{
	case ColliderType::ENEMY:
		LOG("Slingshot rock hit enemy");
		// Damage is handled by the enemy's OnCollision with SLINGSHOT_PROJ
		Destroy();
		break;
	case ColliderType::PLATFORM:
		LOG("Slingshot rock hit platform");
		Destroy();
		break;
	case ColliderType::PUSH_ROCK:
	case ColliderType::BOX:
		LOG("Slingshot rock hit obstacle");
		Destroy();
		break;
	default:
		break;
	}
}

void SlingshotProjectile::OnCollisionEnd(PhysBody* physA, PhysBody* physB) {}

bool SlingshotProjectile::CleanUp()
{
	LOG("Cleanup slingshot projectile");
	if (texture) {
		Engine::GetInstance().textures->UnLoad(texture);
		texture = nullptr;
	}
	if (pbody) {
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}
	return true;
}

bool SlingshotProjectile::Destroy()
{
	LOG("Destroying slingshot projectile");
	active = false;
	pendingToDelete = true;
	return true;
}

void SlingshotProjectile::SetPosition(Vector2D pos)
{
	if (pbody) pbody->SetPosition((int)(pos.getX()), (int)(pos.getY()));
}

Vector2D SlingshotProjectile::GetPosition()
{
	int x, y;
	pbody->GetPosition(x, y);
	return Vector2D((float)x, (float)y);
}

void SlingshotProjectile::SetLaunch(float dX, float dY, float pwr)
{
	// Normalize direction
	float len = std::sqrt(dX * dX + dY * dY);
	if (len > 0.001f) {
		dirX_ = dX / len;
		dirY_ = dY / len;
	}
	else {
		dirX_ = 1.0f;
		dirY_ = 0.0f;
	}
	power_ = pwr;
}
