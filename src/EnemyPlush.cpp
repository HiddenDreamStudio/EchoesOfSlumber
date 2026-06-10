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

	// Load XML animations
	idleAnims_.LoadFromTSX("assets/textures/animations/jumpingPlushIdle.xml", { {0, "idle"} });
	alertAnims_.LoadFromTSX("assets/textures/animations/jumpingPlushAlert.xml", { {0, "alert"} });
	jumpStartAnims_.LoadFromTSX("assets/textures/animations/jumpingPlushJumpStart.xml", { {0, "jump_start"} });
	jumpLoopAnims_.LoadFromTSX("assets/textures/animations/jumpingPlushJumpLoop.xml", { {0, "jump_loop"} });
	jumpEndAnims_.LoadFromTSX("assets/textures/animations/jumpingPlushJumpEnd.xml", { {0, "jump_end"} });
	dizzyAnims_.LoadFromTSX("assets/textures/animations/jumpingPlushDizzy.xml", { {0, "dizzy"} });
	hitAnims_.LoadFromTSX("assets/textures/animations/jumpingPlushHit.xml", { {0, "hit"} });
	dieAnims_.LoadFromTSX("assets/textures/animations/jumpingPlushDie.xml", { {0, "die"} });

	// Set loop properties and defaults
	idleAnims_.SetCurrent("idle");
	alertAnims_.SetCurrent("alert");
	
	jumpStartAnims_.SetCurrent("jump_start");
	jumpStartAnims_.SetLoop("jump_start", false);
	
	jumpLoopAnims_.SetCurrent("jump_loop");
	
	jumpEndAnims_.SetCurrent("jump_end");
	jumpEndAnims_.SetLoop("jump_end", false);
	
	dizzyAnims_.SetCurrent("dizzy");
	
	hitAnims_.SetCurrent("hit");
	hitAnims_.SetLoop("hit", false);
	
	dieAnims_.SetCurrent("die");
	dieAnims_.SetLoop("die", false);

	// Load spritesheet textures
	idleTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/Spritesheet_Jumping_Plush_Idle.png");
	alertTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Jumping_Plush_Alerta.png");
	jumpStartTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Jumping_Plush_Jump_inicial.png");
	jumpLoopTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Jump_Loop.png");
	jumpEndTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Jumping_Plush_Jump final.png");
	dizzyTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Jumping_Plush_Cansado.png");
	hitTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Jumping_Plush_Hit.png");
	dieTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level2/spritesheet_Jumpin_Plush_Die.png");

	return true;
}

bool EnemyPlush::Update(float dt)
{
	ZoneScoped;

	if (Engine::GetInstance().scene->isPaused_ || Engine::GetInstance().scene->isGameOver_) {
		Draw(0.0f);
		return true;
	}

	// Update hit timer
	if (isHit_) {
		hitTimer_ -= dt;
		if (hitTimer_ <= 0.0f) {
			isHit_ = false;
			hitTimer_ = 0.0f;
		}
	}

	// Update cooldowns
	if (attackCooldown_ > 0.0f) attackCooldown_ -= dt;

	// Process contact damage (skip if dead)
	if (currentState_ != State::DEATH && isContactWithPlayer_ && playerListener_ != nullptr)
	{
		int enemyX, enemyY;
		pbody->GetPosition(enemyX, enemyY);
		int playerX, playerY;
		auto pl = Engine::GetInstance().scene->player;
		if (pl && pl->pbody)
		{
			pl->pbody->GetPosition(playerX, playerY);
			float dx = (float)enemyX - (float)playerX;
			float dy = (float)enemyY - (float)playerY;
			float distSq = dx * dx + dy * dy;
			if (distSq > 150.0f * 150.0f)
			{
				isContactWithPlayer_ = false;
				playerListener_ = nullptr;
			}
		}
	}
	if (currentState_ != State::DEATH && isContactWithPlayer_ && playerListener_ != nullptr && attackCooldown_ <= 0.0f)
	{
		playerListener_->TakeDamage(1);
		attackCooldown_ = ATTACK_INTERVAL;
	}

	// Hide behavior: Stand still when player is hiding (skip if dead)
	auto playerShared = Engine::GetInstance().scene->player;
	bool playerIsHiding = playerShared && playerShared->IsHiding();

	if (currentState_ != State::DEATH && playerIsHiding)
	{
		wasPlayerHiding_ = true;
		GetPhysicsValues();
		velocity.x = 0.0f;
		ApplyPhysics();
		Draw(dt);
		return true;
	}

	if (wasPlayerHiding_ && currentState_ != State::DEATH)
	{
		wasPlayerHiding_ = false;
		EnterState(State::PATROL);
	}

	// FSM Update
	UpdateFSM(dt);

	// Update the animations
	if (currentState_ == State::DEATH) {
		dieAnims_.Update(dt);
	}
	else if (isHit_) {
		hitAnims_.Update(dt);
	}
	else {
		switch (currentState_) {
		case State::PATROL:
			idleAnims_.Update(dt);
			break;
		case State::TENSE:
			alertAnims_.Update(dt);
			break;
		case State::JUMPING:
			if (jumpPhase_ == JumpPhase::START) {
				jumpStartAnims_.Update(dt);
				if (jumpStartAnims_.HasFinishedOnce("jump_start") || velocity.y >= 0.0f) {
					jumpPhase_ = JumpPhase::LOOP;
				}
			}
			else if (jumpPhase_ == JumpPhase::LOOP) {
				jumpLoopAnims_.Update(dt);
			}
			else if (jumpPhase_ == JumpPhase::END) {
				jumpEndAnims_.Update(dt);
			}
			break;
		case State::DIZZY:
			dizzyAnims_.Update(dt);
			break;
		}
	}

	// Render
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
	if (currentState_ == State::DEATH)
	{
		velocity.x = 0.0f;
		ApplyPhysics();
		if (dieAnims_.HasFinishedOnce("die"))
		{
			Destroy();
		}
		return;
	}

	// Bypass FSM logic when hit: just let the knockback physics resolve
	if (isHit_)
	{
		GetPhysicsValues();
		ApplyPhysics();
		return;
	}

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
		facingRight_ = (patrolDirection_ > 0.0f);
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

		if (tenseTimer_ >= TENSE_DURATION)
		{
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
		facingRight_ = jumpDirectionRight_; // keep facing the jump direction while airborne

		if (jumpPhase_ == JumpPhase::START)
		{
			// Maintain start jump physics
			float targetVx = lastJumpWasShort_ ? JUMP_SPEED_X_SHORT : JUMP_SPEED_X_LONG;
			velocity.x = jumpDirectionRight_ ? targetVx : -targetVx;
			ApplyPhysics();
		}
		else if (jumpPhase_ == JumpPhase::LOOP)
		{
			// Maintain loop jump physics
			float targetVx = lastJumpWasShort_ ? JUMP_SPEED_X_SHORT : JUMP_SPEED_X_LONG;
			velocity.x = jumpDirectionRight_ ? targetVx : -targetVx;
			ApplyPhysics();

			// Check if we have landed: y velocity is extremely small and we have been in air for at least 200ms
			if (jumpAirTime_ > 200.0f && std::abs(velocity.y) < 0.02f)
			{
				jumpPhase_ = JumpPhase::END;
				jumpEndAnims_.ResetCurrent();
				velocity.x = 0.0f;
				ApplyPhysics();
			}
		}
		else if (jumpPhase_ == JumpPhase::END)
		{
			// Keep still on land
			velocity.x = 0.0f;
			ApplyPhysics();

			if (jumpEndAnims_.HasFinishedOnce("jump_end"))
			{
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
		}
		break;
	}

	case State::DIZZY:
	{
		GetPhysicsValues();
		velocity.x = 0.0f;
		ApplyPhysics();

		dizzyTimer_ += dt;

		if (dizzyTimer_ >= DIZZY_DURATION)
		{
			dizzyTimer_ = 0.0f;
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
		idleAnims_.ResetCurrent();
		break;
	case State::TENSE:
		tenseTimer_ = 0.0f;
		tenseVisualOffset_ = 0.0f;
		alertAnims_.ResetCurrent();
		break;
	case State::JUMPING:
		isJumping_ = true;
		jumpAirTime_ = 0.0f;
		jumpPhase_ = JumpPhase::START;
		jumpStartAnims_.ResetCurrent();
		jumpLoopAnims_.ResetCurrent();
		jumpEndAnims_.ResetCurrent();
		break;
	case State::DIZZY:
		isJumping_ = false;
		dizzyTimer_ = 0.0f;
		dizzyAnims_.ResetCurrent();
		break;
	case State::DEATH:
		isJumping_ = false;
		velocity.x = 0.0f;
		ApplyPhysics();
		dieAnims_.ResetCurrent();
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

	SDL_FlipMode flip = facingRight_ ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

	// Pick the correct active animation frame and texture
	SDL_Texture* activeTexture = nullptr;
	SDL_Rect frame = { 0, 0, 0, 0 };
	float scale = 1.0f;
	int charBottom = 0;

	if (currentState_ == State::DEATH) {
		activeTexture = dieTexture_;
		frame = dieAnims_.GetCurrentFrame();
		scale = 1.43f;
		charBottom = 157;
	}
	else if (isHit_) {
		activeTexture = hitTexture_;
		frame = hitAnims_.GetCurrentFrame();
		scale = 1.67f;
		charBottom = 145;
	}
	else {
		switch (currentState_) {
		case State::PATROL:
			activeTexture = idleTexture_;
			frame = idleAnims_.GetCurrentFrame();
			scale = 2.0f;
			charBottom = 97;
			break;
		case State::TENSE:
			activeTexture = alertTexture_;
			frame = alertAnims_.GetCurrentFrame();
			scale = 1.1f;
			charBottom = 162;
			break;
		case State::JUMPING:
			if (jumpPhase_ == JumpPhase::START) {
				activeTexture = jumpStartTexture_;
				frame = jumpStartAnims_.GetCurrentFrame();
				scale = 2.0f;
				charBottom = 70;
			}
			else if (jumpPhase_ == JumpPhase::LOOP) {
				activeTexture = jumpLoopTexture_;
				frame = jumpLoopAnims_.GetCurrentFrame();
				scale = 1.0f;
				charBottom = 0;
			}
			else if (jumpPhase_ == JumpPhase::END) {
				activeTexture = jumpEndTexture_;
				frame = jumpEndAnims_.GetCurrentFrame();
				scale = 2.0f;
				charBottom = 70;
			}
			break;
		case State::DIZZY:
			activeTexture = dizzyTexture_;
			frame = dizzyAnims_.GetCurrentFrame();
			scale = 1.0f;
			charBottom = 101;
			break;
		}
	}

	if (activeTexture != nullptr && frame.w > 0 && frame.h > 0)
	{
		// Center horizontally on the physics body
		int drawX = x - (int)(frame.w * scale) / 2;
		
		int drawY;
		if (charBottom > 0) {
			// Anchor the character's feet to the ground
			drawY = (y + 30) - (int)(charBottom * scale);
		} else {
			// Anchor using the bottom of the frame (default)
			drawY = (y + 30) - (int)(frame.h * scale);
		}

		// Add subtle shaking visual effect if in TENSE/alert state
		if (currentState_ == State::TENSE)
		{
			tenseVisualOffset_ += dt;
			int shake = (std::sin(tenseVisualOffset_ * 0.1f) > 0.0f) ? 3 : -3;
			drawX += shake;
		}

		Engine::GetInstance().render->DrawTexture(activeTexture, drawX, drawY, &frame, 1.0f, -1.0f, 0, INT_MAX, INT_MAX, flip, scale);
	}
}

bool EnemyPlush::CleanUp()
{
	LOG("Cleanup EnemyPlush");

	if (idleTexture_) Engine::GetInstance().textures->UnLoad(idleTexture_);
	if (alertTexture_) Engine::GetInstance().textures->UnLoad(alertTexture_);
	if (jumpStartTexture_) Engine::GetInstance().textures->UnLoad(jumpStartTexture_);
	if (jumpLoopTexture_) Engine::GetInstance().textures->UnLoad(jumpLoopTexture_);
	if (jumpEndTexture_) Engine::GetInstance().textures->UnLoad(jumpEndTexture_);
	if (dizzyTexture_) Engine::GetInstance().textures->UnLoad(dizzyTexture_);
	if (hitTexture_) Engine::GetInstance().textures->UnLoad(hitTexture_);
	if (dieTexture_) Engine::GetInstance().textures->UnLoad(dieTexture_);

	idleTexture_ = nullptr;
	alertTexture_ = nullptr;
	jumpStartTexture_ = nullptr;
	jumpLoopTexture_ = nullptr;
	jumpEndTexture_ = nullptr;
	dizzyTexture_ = nullptr;
	hitTexture_ = nullptr;
	dieTexture_ = nullptr;

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
		TakeDamage(1, false);
		if (physB->listener != nullptr)
			physB->listener->Destroy();
	}
	else if (physB->ctype == ColliderType::PLAYER)
	{
		isContactWithPlayer_ = true;
		playerListener_ = physB->listener;
		if (attackCooldown_ <= 0.0f && currentState_ != State::DEATH)
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
	TakeDamage(damage, true);
}

void EnemyPlush::TakeDamage(int damage, bool applyKnockback)
{
	if (currentState_ == State::DEATH) return;

	health -= damage;
	LOG("EnemyPlush took %d damage -> health: %d/%d", damage, health, maxHealth);

	isHit_ = true;
	hitTimer_ = 350.0f; // ms
	hitAnims_.ResetCurrent();

	if (applyKnockback)
	{
		// Knockback effect
		Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
		int bodyX, bodyY;
		pbody->GetPosition(bodyX, bodyY);

		float dirX = (bodyX > playerPos.getX()) ? 1.0f : -1.0f;
		Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, dirX * 3.5f, 0.0f, true);
	}

	if (health <= 0)
	{
		health = 0;
		LOG("EnemyPlush entering death sequence");
		EnterState(State::DEATH);
	}
}
