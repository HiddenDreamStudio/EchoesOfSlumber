#include "EnemyPlush.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Map.h"
#include "Player.h"
#include "tracy/Tracy.hpp"
#include <cmath>
#include <algorithm>

EnemyPlush::EnemyPlush() : Entity(EntityType::ENEMY_PLUSH)
{
	name = "EnemyPlush";
	health = 4;
	maxHealth = 4;
}

EnemyPlush::~EnemyPlush() {}

bool EnemyPlush::Awake() {
	return true;
}

bool EnemyPlush::Start() {
	// Create capsule physics body
	texW = 60;
	texH = 80;
	pbody = Engine::GetInstance().physics->CreateCapsule(
		(int)position.getX() + texW / 2, (int)position.getY() + texH / 2,
		24, 60, bodyType::DYNAMIC);

	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY;

	patrolOriginX_ = position.getX();
	originPosition_ = position;

	currentState_ = State::PATROL;
	isJumping_ = false;
	lastJumpWasShort_ = false;

	return true;
}

bool EnemyPlush::Update(float dt)
{
	ZoneScoped;

	if (Engine::GetInstance().scene->isPaused_ || Engine::GetInstance().scene->isGameOver_) {
		Draw(0.0f);
		return true;
	}

	// Update cooldowns
	if (attackCooldown_ > 0.0f) attackCooldown_ -= dt;

	// Process contact damage
	if (isContactWithPlayer_ && playerListener_ != nullptr && attackCooldown_ <= 0.0f)
	{
		playerListener_->TakeDamage(1);
		attackCooldown_ = ATTACK_INTERVAL;
	}

	// Hide behavior: Stand still when player is hiding
	auto playerShared = Engine::GetInstance().scene->player;
	bool playerIsHiding = playerShared && playerShared->IsHiding();

	if (playerIsHiding)
	{
		wasPlayerHiding_ = true;
		GetPhysicsValues();
		velocity.x = 0.0f;
		ApplyPhysics();
		Draw(dt);
		return true;
	}

	if (wasPlayerHiding_)
	{
		wasPlayerHiding_ = false;
		EnterState(State::PATROL);
	}

	// FSM Update
	UpdateFSM(dt);

	// Render colored placeholder
	Draw(dt);

	return true;
}

float EnemyPlush::GetDistanceToPlayer() const
{
	int x, y;
	pbody->GetPosition(x, y);
	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	// Offset playerPos to center
	float px = playerPos.getX() + 32.0f;
	float py = playerPos.getY() + 32.0f;

	float dx = (float)x - px;
	float dy = (float)y - py;
	return std::sqrt(dx * dx + dy * dy);
}

void EnemyPlush::UpdateFSM(float dt)
{
	float dist = GetDistanceToPlayer();

	switch (currentState_)
	{
	case State::PATROL:
	{
		GetPhysicsValues();

		// Back and forth patrol movement
		int bodyX, bodyY;
		pbody->GetPosition(bodyX, bodyY);
		float offset = (float)bodyX - patrolOriginX_;

		if (patrolDirection_ > 0.0f && offset > PATROL_RANGE) {
			patrolDirection_ = -1.0f;
		}
		else if (patrolDirection_ < 0.0f && offset < -PATROL_RANGE) {
			patrolDirection_ = 1.0f;
		}

		velocity.x = patrolDirection_ * PATROL_SPEED;
		facingRight_ = (velocity.x > 0.0f);
		ApplyPhysics();

		// Transition to TENSE state if player detected
		if (dist <= DETECT_RANGE) {
			EnterState(State::TENSE);
		}
		break;
	}

	case State::TENSE:
	{
		GetPhysicsValues();
		velocity.x = 0.0f; // Stand still while tensing up
		ApplyPhysics();

		tenseTimer_ += dt;

		// Face the player
		Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
		int bodyX, bodyY;
		pbody->GetPosition(bodyX, bodyY);
		facingRight_ = (playerPos.getX() > (float)bodyX);

		// Shake visual effect
		tenseVisualOffset_ = (std::sin(tenseTimer_ * 0.1f) > 0.0f) ? 3.0f : -3.0f;

		if (tenseTimer_ >= TENSE_DURATION)
		{
			tenseVisualOffset_ = 0.0f;
			tenseTimer_ = 0.0f;

			// Decide jump direction
			jumpDirectionRight_ = (playerPos.getX() > (float)bodyX);

			// Decide jump type based on horizontal distance
			float diffX = std::abs((playerPos.getX() + 32.0f) - (float)bodyX);

			if (diffX < 120.0f && !lastJumpWasShort_)
			{
				// Perform short jump
				GetPhysicsValues();
				float jvx = jumpDirectionRight_ ? JUMP_SPEED_X_SHORT : -JUMP_SPEED_X_SHORT;
				Engine::GetInstance().physics->SetLinearVelocity(pbody, jvx, JUMP_SPEED_Y_SHORT);
				lastJumpWasShort_ = true;
			}
			else
			{
				// Perform long jump
				GetPhysicsValues();
				float jvx = jumpDirectionRight_ ? JUMP_SPEED_X_LONG : -JUMP_SPEED_X_LONG;
				Engine::GetInstance().physics->SetLinearVelocity(pbody, jvx, JUMP_SPEED_Y_LONG);
				lastJumpWasShort_ = false;
			}

			EnterState(State::JUMPING);
		}
		break;
	}

	case State::JUMPING:
	{
		jumpAirTime_ += dt;
		GetPhysicsValues();

		// Ensure horizontal jump velocity is maintained in mid-air
		float targetVx = lastJumpWasShort_ ? JUMP_SPEED_X_SHORT : JUMP_SPEED_X_LONG;
		velocity.x = jumpDirectionRight_ ? targetVx : -targetVx;
		ApplyPhysics();

		// Check if we have landed: y velocity is extremely small and we have been in air for at least 200ms
		if (jumpAirTime_ > 200.0f && std::abs(velocity.y) < 0.02f)
		{
			GetPhysicsValues();
			velocity.x = 0.0f;
			ApplyPhysics();

			if (lastJumpWasShort_)
			{
				// Chains immediately into a long jump as requested!
				EnterState(State::TENSE);
			}
			else
			{
				// Long jump finishes, gets dizzy/dazed
				EnterState(State::DIZZY);
			}
		}
		break;
	}

	case State::DIZZY:
	{
		GetPhysicsValues();
		velocity.x = 0.0f;
		ApplyPhysics();

		dizzyTimer_ += dt;

		// Orbiting stars angle update
		dizzyVisualAngle_ += dt * 0.3f;

		if (dizzyTimer_ >= DIZZY_DURATION)
		{
			dizzyTimer_ = 0.0f;
			dizzyVisualAngle_ = 0.0f;
			// Reset and go back to charging/patrolling
			EnterState(State::PATROL);
		}
		break;
	}
	}
}

void EnemyPlush::EnterState(State newState)
{
	currentState_ = newState;

	switch (newState)
	{
	case State::PATROL:
		speed = PATROL_SPEED;
		break;
	case State::TENSE:
		tenseTimer_ = 0.0f;
		tenseVisualOffset_ = 0.0f;
		break;
	case State::JUMPING:
		isJumping_ = true;
		jumpAirTime_ = 0.0f;
		break;
	case State::DIZZY:
		isJumping_ = false;
		dizzyTimer_ = 0.0f;
		dizzyVisualAngle_ = 0.0f;
		break;
	}
}

void EnemyPlush::GetPhysicsValues()
{
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
}

void EnemyPlush::ApplyPhysics()
{
	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}

void EnemyPlush::Draw(float dt)
{
	int x, y;
	pbody->GetPosition(x, y);

	position.setX((float)x);
	position.setY((float)y);

	// Colored shape dimensions
	int rectW = 44;
	int rectH = 68;

	// Visual offset for shaking in TENSE state
	int drawX = x + (int)tenseVisualOffset_;
	int drawY = y;

	SDL_Rect bodyRect = {
		drawX - rectW / 2,
		drawY - rectH / 2,
		rectW,
		rectH
	};

	// State colors (Red/Green/Orange/Purple)
	Uint8 r = 50, g = 200, b = 100; // PATROL (Green)
	if (currentState_ == State::TENSE) {
		r = 255; g = 140; b = 0; // TENSE (Orange)
	}
	else if (currentState_ == State::JUMPING) {
		r = 230; g = 40; b = 40; // JUMPING (Red)
	}
	else if (currentState_ == State::DIZZY) {
		r = 180; g = 100; b = 230; // DIZZY (Purple)
	}

	// 1. Draw main body rect
	Engine::GetInstance().render->DrawRectangle(bodyRect, r, g, b, 230, true);

	// 2. Draw border / stroke for sleek aesthetics
	Engine::GetInstance().render->DrawRectangle(bodyRect, 20, 20, 20, 255, false);

	// 3. Draw face/eyes details
	int eyeY = drawY - 14;
	int leftEyeX = drawX - 10;
	int rightEyeX = drawX + 10;

	if (currentState_ == State::DIZZY)
	{
		// Draw crosses for dizzy eyes
		int crossSize = 4;
		Engine::GetInstance().render->DrawLine(leftEyeX - crossSize, eyeY - crossSize, leftEyeX + crossSize, eyeY + crossSize, 40, 20, 40, 255);
		Engine::GetInstance().render->DrawLine(leftEyeX + crossSize, eyeY - crossSize, leftEyeX - crossSize, eyeY + crossSize, 40, 20, 40, 255);

		Engine::GetInstance().render->DrawLine(rightEyeX - crossSize, eyeY - crossSize, rightEyeX + crossSize, eyeY + crossSize, 40, 20, 40, 255);
		Engine::GetInstance().render->DrawLine(rightEyeX + crossSize, eyeY - crossSize, rightEyeX - crossSize, eyeY + crossSize, 40, 20, 40, 255);

		// Draw orbiting yellow stars above head
		int headY = drawY - rectH / 2 - 12;
		float angle1 = dizzyVisualAngle_;
		float angle2 = dizzyVisualAngle_ + 120.0f;
		float angle3 = dizzyVisualAngle_ + 240.0f;
		int starRad = 4;
		int orbitRad = 20;

		Engine::GetInstance().render->DrawCircle(drawX + (int)(orbitRad * std::cos(angle1 * 0.01745f)), headY + (int)(5 * std::sin(angle1 * 0.01745f)), starRad, 255, 215, 0, 255);
		Engine::GetInstance().render->DrawCircle(drawX + (int)(orbitRad * std::cos(angle2 * 0.01745f)), headY + (int)(5 * std::sin(angle2 * 0.01745f)), starRad, 255, 215, 0, 255);
		Engine::GetInstance().render->DrawCircle(drawX + (int)(orbitRad * std::cos(angle3 * 0.01745f)), headY + (int)(5 * std::sin(angle3 * 0.01745f)), starRad, 255, 215, 0, 255);
	}
	else
	{
		// Draw normal eyes looking in the facing direction
		int eyeRad = 5;
		int gazeOffset = facingRight_ ? 3 : -3;
		Engine::GetInstance().render->DrawCircle(leftEyeX + gazeOffset, eyeY, eyeRad, 30, 30, 30, 255);
		Engine::GetInstance().render->DrawCircle(rightEyeX + gazeOffset, eyeY, eyeRad, 30, 30, 30, 255);

		// Cute mouth button / stitches
		Engine::GetInstance().render->DrawCircle(drawX, drawY + 2, 4, 30, 30, 30, 255);
	}
}

bool EnemyPlush::CleanUp()
{
	LOG("Cleanup EnemyPlush");
	if (pbody) {
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}
	return true;
}

bool EnemyPlush::Destroy()
{
	LOG("Destroying EnemyPlush");
	active = false;
	pendingToDelete = true;
	return true;
}

void EnemyPlush::SetPosition(Vector2D pos) {
	pbody->SetPosition((int)(pos.getX()), (int)(pos.getY()));
}

Vector2D EnemyPlush::GetPosition() {
	int x, y;
	pbody->GetPosition(x, y);
	return Vector2D((float)x - texW / 2, (float)y - texH / 2);
}

void EnemyPlush::OnCollision(PhysBody* physA, PhysBody* physB)
{
	if (physB->ctype == ColliderType::ATTACK)
	{
		LOG("EnemyPlush hit by player attack");
		TakeDamage(1);
	}
	else if (physB->ctype == ColliderType::SLINGSHOT_PROJ)
	{
		LOG("EnemyPlush hit by slingshot projectile");
		TakeDamage(1);
		if (physB->listener != nullptr)
			physB->listener->Destroy();
	}
	else if (physB->ctype == ColliderType::PLAYER)
	{
		isContactWithPlayer_ = true;
		playerListener_ = physB->listener;
		if (attackCooldown_ <= 0.0f)
		{
			playerListener_->TakeDamage(1);
			attackCooldown_ = ATTACK_INTERVAL;
		}
	}
}

void EnemyPlush::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	if (physB->ctype == ColliderType::PLAYER)
	{
		isContactWithPlayer_ = false;
		playerListener_ = nullptr;
	}
}

void EnemyPlush::TakeDamage(int damage)
{
	health -= damage;
	LOG("EnemyPlush took %d damage -> health: %d/%d", damage, health, maxHealth);

	// Knockback effect
	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	float dirX = (bodyX > playerPos.getX()) ? 1.0f : -1.0f;
	Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, dirX * 6.0f, -4.0f, true);

	if (health <= 0)
	{
		health = 0;
		LOG("EnemyPlush dead");
		Destroy();
	}
}
