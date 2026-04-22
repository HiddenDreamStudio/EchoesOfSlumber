#include "Projectile.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "Log.h"
#include "EntityManager.h"
#include "tracy/Tracy.hpp"
#include "Scene.h"
#include <cmath>

Projectile::Projectile() : Entity(EntityType::PROJECTILE)
{
	name = "Projectile";
}

Projectile::~Projectile() {}

bool Projectile::Start()
{
	// Load projectile animation from the spritesheet
	anims.LoadSequentialFromTSX("assets/textures/animations/ProjectileAnim.tsx", "fly", 80);
	anims.SetCurrent("fly");
	anims.SetLoop("fly", true);

	texture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS enemics C/spritesheet_pilota.png");

	// Create a sensor AABB for the projectile (sensor = no physical blocking, still triggers OnCollision)
	pbody = Engine::GetInstance().physics->CreateRectangleSensor(
		(int)position.getX(), (int)position.getY(),
		PROJ_SIZE * 2, PROJ_SIZE * 2, bodyType::DYNAMIC);

	pbody->listener = this;
	pbody->ctype = ColliderType::PROJECTILE;

	// Disable gravity for projectile (flies straight)
	b2Body_SetGravityScale(pbody->body, 0.0f);

	// Set initial velocity
	Engine::GetInstance().physics->SetLinearVelocity(pbody, dirX_ * SPEED, dirY_ * SPEED);

	return true;
}

bool Projectile::Update(float dt)
{
	ZoneScoped;

	if (Engine::GetInstance().scene->isPaused_ || Engine::GetInstance().scene->isGameOver_) {
		// Still draw but don't move
		anims.Update(0.0f);
		int x, y;
		pbody->GetPosition(x, y);
		int drawSize = (int)(texW * DRAW_SCALE);
		const SDL_Rect& frame = anims.GetCurrentFrame();
		Engine::GetInstance().render->DrawTexture(texture, x - drawSize / 2, y - drawSize / 2, &frame, 1.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, DRAW_SCALE);
		return true;
	}

	// Check lifetime
	lifetime_ += dt;
	if (lifetime_ >= MAX_LIFETIME) {
		Destroy();
		return true;
	}

	// Ensure velocity is maintained (in case physics slows it)
	Engine::GetInstance().physics->SetLinearVelocity(pbody, dirX_ * SPEED, dirY_ * SPEED);

	// Update animation
	anims.Update(dt);
	const SDL_Rect& frame = anims.GetCurrentFrame();

	// Draw
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	int drawSize = (int)(texW * DRAW_SCALE);
	Engine::GetInstance().render->DrawTexture(texture, x - drawSize / 2, y - drawSize / 2, &frame, 1.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, DRAW_SCALE);

	return true;
}

void Projectile::OnCollision(PhysBody* physA, PhysBody* physB)
{
	switch (physB->ctype)
	{
	case ColliderType::PLAYER:
		// Damage is handled by the Player's own OnCollision (ColliderType::PROJECTILE case)
		LOG("Projectile hit player");
		Destroy();
		break;
	case ColliderType::PLATFORM:
		LOG("Projectile hit platform");
		Destroy();
		break;
	default:
		break;
	}
}

void Projectile::OnCollisionEnd(PhysBody* physA, PhysBody* physB) {}

bool Projectile::CleanUp()
{
	LOG("Cleanup projectile");
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

bool Projectile::Destroy()
{
	LOG("Destroying Projectile");
	active = false;
	pendingToDelete = true;
	return true;
}

void Projectile::SetPosition(Vector2D pos)
{
	if (pbody) pbody->SetPosition((int)(pos.getX()), (int)(pos.getY()));
}

Vector2D Projectile::GetPosition()
{
	int x, y;
	pbody->GetPosition(x, y);
	return Vector2D((float)x, (float)y);
}

void Projectile::SetDirection(float dX, float dY)
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
}
