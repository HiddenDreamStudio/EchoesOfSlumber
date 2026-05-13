#include "EnemyStitchling.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "Player.h"
#include "Input.h"
#include <cstdlib>
#include <cmath>

EnemyStitchling::EnemyStitchling() : Enemy()
{
	name = "Stitchling";
	type = EntityType::STITCHLING;
	health = 1; // Dies in one hit when vulnerable
}

EnemyStitchling::~EnemyStitchling() {}

bool EnemyStitchling::Awake() { return true; }

bool EnemyStitchling::Start()
{
	const char* path = "assets/textures/spritesheets/SS enemics C/testenemylvl2.jpg";
	SDL_Surface* surface = IMG_Load(path);
	if (surface) {
		SDL_SetSurfaceColorKey(surface, 1, SDL_MapRGB(SDL_GetPixelFormatDetails(surface->format), nullptr, 255, 255, 255));
		stitchTexture_ = Engine::GetInstance().textures->LoadSurface(surface);
		SDL_DestroySurface(surface);
	}
	
	texW = 64;
	texH = 64;

	// Body: the yo-yo itself. Usually stays hidden/quiet.
	pbody = Engine::GetInstance().physics->CreateCircle(
		(int)position.getX(),
		(int)position.getY(),
		15, bodyType::STATIC);

	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY;

	// Sensor: the trap area.
	trapSensor_ = Engine::GetInstance().physics->CreateRectangleSensor(
		(int)position.getX(),
		(int)position.getY() + 32,
		160, 40, bodyType::STATIC);
	trapSensor_->listener = this;
	trapSensor_->ctype = ColliderType::UNKNOWN;

	anchorPos_ = position;
	TransitionTo(StitchlingState::IDLE);

	return true;
}

bool EnemyStitchling::Update(float dt)
{
	// Base class handles FSM call and physics
	return Enemy::Update(dt);
}

bool EnemyStitchling::CleanUp()
{
	if (stitchTexture_) Engine::GetInstance().textures->UnLoad(stitchTexture_);
	if (trapSensor_) Engine::GetInstance().physics->DeletePhysBody(trapSensor_);
	
	// Reset player if we were trapping them
	auto player = Engine::GetInstance().scene->player;
	if (player && state_ == StitchlingState::TRAPPING) {
		// If needed, reset player state here
	}

	return Enemy::CleanUp();
}

void EnemyStitchling::OnCollision(PhysBody* physA, PhysBody* physB)
{
	if (physA == trapSensor_ && physB->ctype == ColliderType::PLAYER)
	{
		isPlayerInTrap_ = true;
		if (state_ == StitchlingState::IDLE)
		{
			TransitionTo(StitchlingState::ACTIVATED);
		}
	}
	
	// Handle damage from player attacks
	if (physB->ctype == ColliderType::ATTACK)
	{
		TakeDamage(1);
	}
}

void EnemyStitchling::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	if (physA == trapSensor_ && physB->ctype == ColliderType::PLAYER)
	{
		isPlayerInTrap_ = false;
	}
}

void EnemyStitchling::TakeDamage(int damage)
{
	// Only vulnerable during REWIND_SLOW
	if (state_ == StitchlingState::REWIND_SLOW)
	{
		health -= damage;
		if (health <= 0) TransitionTo(StitchlingState::DEATH);
	}
	else {
		LOG("Stitchling is protected right now!");
	}
}

void EnemyStitchling::UpdateFSM(float dt)
{
	auto& engine = Engine::GetInstance();
	auto player = engine.scene->player;

	switch (state_)
	{
	case StitchlingState::IDLE:
		// Stays still. Cord blends with scenario (drawn faintly in Draw)
		break;

	case StitchlingState::ACTIVATED:
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f)
		{
			TransitionTo(StitchlingState::TRAPPING);
		}
		break;

	case StitchlingState::TRAPPING:
		stateTimer_ -= dt;
		damageTimer_ -= dt;

		if (player && isPlayerInTrap_)
		{
			// Slow movement penalty: we fight the player velocity
			b2Vec2 pVel = engine.physics->GetLinearVelocity(player->pbody);
			pVel.x *= 0.4f; // Heavy slow
			engine.physics->SetLinearVelocity(player->pbody, pVel);

			// Damage over time
			if (damageTimer_ <= 0.0f)
			{
				player->TakeDamage(1);
				damageTimer_ = damageInterval_;
			}

			// Escape mechanic: Mash SPACE or E
			if (engine.input->GetKey(SDL_SCANCODE_E) == KEY_DOWN || 
				engine.input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN ||
				engine.input->GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_DOWN)
			{
				escapeMashes_++;
				if (escapeMashes_ >= REQUIRED_MASHES)
				{
					// Randomly choose rewind mode
					TransitionTo((std::rand() % 2 == 0) ? StitchlingState::REWIND_SLOW : StitchlingState::REWIND_FAST);
				}
			}
		}

		if (stateTimer_ <= 0.0f)
		{
			TransitionTo((std::rand() % 2 == 0) ? StitchlingState::REWIND_SLOW : StitchlingState::REWIND_FAST);
		}
		break;

	case StitchlingState::REWIND_SLOW:
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f) TransitionTo(StitchlingState::IDLE);
		break;

	case StitchlingState::REWIND_FAST:
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f) TransitionTo(StitchlingState::IDLE);
		break;

	case StitchlingState::DEATH:
		break;
	}
}

void EnemyStitchling::TransitionTo(StitchlingState newState)
{
	state_ = newState;
	stateTimer_ = 0.0f;

	switch (newState)
	{
	case StitchlingState::IDLE:
		escapeMashes_ = 0;
		LOG("Stitchling: IDLE");
		break;
	case StitchlingState::ACTIVATED:
		stateTimer_ = 400.0f; 
		LOG("Stitchling: ACTIVATED");
		break;
	case StitchlingState::TRAPPING:
		stateTimer_ = trapDuration_;
		damageTimer_ = damageInterval_;
		LOG("Stitchling: TRAPPING PLAYER");
		break;
	case StitchlingState::REWIND_SLOW:
		stateTimer_ = 4000.0f;
		LOG("Stitchling: REWINDING SLOWLY (VULNERABLE)");
		break;
	case StitchlingState::REWIND_FAST:
		stateTimer_ = 1200.0f;
		LOG("Stitchling: REWINDING FAST");
		break;
	case StitchlingState::DEATH:
		LOG("Stitchling: DEATH");
		Destroy();
		break;
	}
}

void EnemyStitchling::Draw(float dt)
{
	auto& render = Engine::GetInstance().render;
	int x, y;
	pbody->GetPosition(x, y);

	// Cord drawing logic
	if (state_ == StitchlingState::IDLE)
	{
		// Faint "seams" on the floor
		render->DrawRectangle({x - 80, y + 32, 160, 4}, 70, 70, 100, 100, true, false);
	}
	else if (state_ == StitchlingState::ACTIVATED || state_ == StitchlingState::TRAPPING)
	{
		// Vibrant threads pulling the player
		auto playerPos = Engine::GetInstance().scene->GetPlayerPosition();
		render->DrawLine(x, y, (int)playerPos.getX(), (int)playerPos.getY(), 100, 150, 255, 255);
		// Trap area highlights
		render->DrawRectangle({x - 80, y + 32, 160, 8}, 120, 180, 255, 180, true, false);
	}

	// Yo-yo body
	float baseScale = 0.25f;
	float scale = baseScale;
	if (state_ == StitchlingState::ACTIVATED) scale = baseScale * 1.2f;
	if (state_ == StitchlingState::TRAPPING) scale = baseScale * (1.1f + 0.1f * std::sin(stateTimer_ * 0.01f));
	
	int w, h;
	Engine::GetInstance().textures->GetSize(stitchTexture_, w, h);
	int drawX = x - (int)(w * scale / 2);
	int drawY = y - (int)(h * scale / 2);

	render->DrawTexture(stitchTexture_, drawX, drawY, nullptr, 1.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, scale);

	// Vulnerability indicator
	if (state_ == StitchlingState::REWIND_SLOW)
	{
		static float alphaTimer = 0.0f;
		alphaTimer += 0.01f * dt;
		int a = (int)(140 + 100 * std::sin(alphaTimer));
		render->DrawCircle(x, y, 40, 255, 80, 80, a);
	}
}
