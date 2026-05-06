#include "EnemyCarmel.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include <cmath>

EnemyCarmel::EnemyCarmel()
{
	name = "EnemyCarmel";
}

EnemyCarmel::~EnemyCarmel() {}

bool EnemyCarmel::Start()
{
	// --- Idle animation (64x64, 5 frames) ---
	std::unordered_map<int, std::string> idleAliases = { {0, "idle"} };
	idleAnims_.LoadFromTSX("assets/textures/animations/carmelAnimation.xml", idleAliases);
	idleAnims_.SetCurrent("idle");
	texture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS enemics C/spritesheet_Carmel_idle.png");

	// --- Roll animation (256x256, 7 frames) ---
	std::unordered_map<int, std::string> rollAliases = { {0, "roll"} };
	rollAnims_.LoadFromTSX("assets/textures/animations/carmelRollAnimation.xml", rollAliases);
	rollAnims_.SetCurrent("roll");
	rollTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS enemics C/spritesheet_Carmel_Roll.png");

	// --- Scared animation (64x64, 11 frames, no loop) ---
	std::unordered_map<int, std::string> scaredAliases = { {0, "scared"} };
	scaredAnims_.LoadFromTSX("assets/textures/animations/carmelScaredAnimation.xml", scaredAliases);
	scaredAnims_.SetCurrent("scared");
	scaredAnims_.SetLoop("scared", false);
	scaredTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS enemics C/spritesheet_Carmel_scared.png");

	// --- Blowup animation (256x256, 6 frames, no loop) ---
	std::unordered_map<int, std::string> blowupAliases = { {0, "blowup"} };
	blowupAnims_.LoadFromTSX("assets/textures/animations/carmelBlowupAnimation.xml", blowupAliases);
	blowupAnims_.SetCurrent("blowup");
	blowupAnims_.SetLoop("blowup", false);
	blowupTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS enemics C/spritesheet_Carmel_Blowup.png");

	// --- Physics body ---
	pbody = Engine::GetInstance().physics->CreateCapsule(
		(int)position.getX() + texW / 2,
		(int)position.getY() + texH / 2,
		20, 50, bodyType::DYNAMIC);

	pbody->listener = this;
	pbody->ctype    = ColliderType::ENEMY;

	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);
	if (patrolLeftX_ == 0.0f && patrolRightX_ == 0.0f)
	{
		patrolLeftX_  = (float)bodyX - 200.0f;
		patrolRightX_ = (float)bodyX + 200.0f;
	}

	TransitionTo(EnemyCarmelState::IDLE);
	return true;
}

bool EnemyCarmel::CleanUp()
{
	Engine::GetInstance().textures->UnLoad(rollTexture_);
	Engine::GetInstance().textures->UnLoad(scaredTexture_);
	Engine::GetInstance().textures->UnLoad(blowupTexture_);
	return Enemy::CleanUp();
}

void EnemyCarmel::SetPatrolPoints(float leftX, float rightX)
{
	patrolLeftX_  = leftX;
	patrolRightX_ = rightX;
}

// ─── Collision overrides: NO contact damage ───────────────────────────
void EnemyCarmel::OnCollision(PhysBody* physA, PhysBody* physB)
{
	// Only react to attacks — never deal contact damage to the player
	if (physB->ctype == ColliderType::ATTACK)
	{
		TakeDamage(1);
	}
	// Intentionally NOT calling Enemy::OnCollision to avoid contact damage
}

void EnemyCarmel::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	// Nothing to do — no contact tracking needed
}

// ─── FSM ──────────────────────────────────────────────────────────────
void EnemyCarmel::UpdateFSM(float dt)
{
	if (state_ == EnemyCarmelState::DEATH) return;

	auto& player = Engine::GetInstance().scene->player;
	if (player != nullptr && player->IsDead())
	{
		velocity.x = 0.0f;
		return;
	}

	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	float dx           = playerPos.getX() - (float)bodyX;
	float distToPlayer = std::abs(dx);

	switch (state_)
	{
	// ── IDLE: stand still, wait, then patrol. Chase if player detected ──
	case EnemyCarmelState::IDLE:
		velocity.x = 0.0f;
		stateTimer_ -= dt;
		if (distToPlayer < DETECTION_RADIUS)
			TransitionTo(EnemyCarmelState::CHASE);
		else if (stateTimer_ <= 0.0f)
			TransitionTo(EnemyCarmelState::PATROL);
		break;

	// ── PATROL: move back and forth, chase if player detected ──
	case EnemyCarmelState::PATROL:
		if (distToPlayer < DETECTION_RADIUS)
		{
			TransitionTo(EnemyCarmelState::CHASE);
			break;
		}
		if (patrolDirX_ > 0.0f && (float)bodyX >= patrolRightX_)
			patrolDirX_ = -1.0f;
		else if (patrolDirX_ < 0.0f && (float)bodyX <= patrolLeftX_)
			patrolDirX_ = 1.0f;

		velocity.x    = patrolDirX_ * speed;
		facingRight_  = (patrolDirX_ > 0.0f);
		break;

	// ── CHASE: roll towards the player, stop next to them ──
	case EnemyCarmelState::CHASE:
		if (distToPlayer > DETECTION_RADIUS * 1.5f)
		{
			TransitionTo(EnemyCarmelState::PATROL);
			break;
		}
		if (distToPlayer < ARRIVE_RANGE)
		{
			// Arrived next to the player — start scared sequence
			TransitionTo(EnemyCarmelState::SCARED);
			break;
		}
		velocity.x   = (dx > 0.0f) ? speed : -speed;
		facingRight_  = (dx > 0.0f);
		break;

	// ── SCARED: stop, play scared animation, then transition to blowup ──
	case EnemyCarmelState::SCARED:
		velocity.x = 0.0f;
		pulseTimer_ += dt;
		if (scaredAnims_.HasFinishedOnce("scared"))
		{
			TransitionTo(EnemyCarmelState::BLOWUP);
		}
		break;

	// ── BLOWUP: play blowup animation, then explode and die ──
	case EnemyCarmelState::BLOWUP:
		velocity.x = 0.0f;
		pulseTimer_ += dt;
		if (blowupAnims_.HasFinishedOnce("blowup") && !hasExploded_)
		{
			hasExploded_ = true;
			DealExplosionDamage();
			TransitionTo(EnemyCarmelState::DEATH);
		}
		break;

	case EnemyCarmelState::DEATH:
		break;
	}
}

void EnemyCarmel::TransitionTo(EnemyCarmelState newState)
{
	state_ = newState;

	switch (newState)
	{
	case EnemyCarmelState::IDLE:
		stateTimer_ = IDLE_DURATION;
		idleAnims_.ResetCurrent();
		LOG("EnemyCarmel: IDLE");
		break;

	case EnemyCarmelState::PATROL:
		rollAnims_.ResetCurrent();
		rollAnims_.SetLoop("roll", true);
		LOG("EnemyCarmel: PATROL");
		break;

	case EnemyCarmelState::CHASE:
		rollAnims_.ResetCurrent();
		rollAnims_.SetLoop("roll", true);
		LOG("EnemyCarmel: CHASE");
		break;

	case EnemyCarmelState::SCARED:
		velocity.x = 0.0f;
		pulseTimer_ = 0.0f;
		scaredAnims_.ResetCurrent();
		LOG("EnemyCarmel: SCARED");
		break;

	case EnemyCarmelState::BLOWUP:
		velocity.x = 0.0f;
		blowupAnims_.ResetCurrent();
		LOG("EnemyCarmel: BLOWUP");
		break;

	case EnemyCarmelState::DEATH:
		LOG("EnemyCarmel: DEATH");
		Destroy();
		break;
	}
}

// ─── Explosion damage ─────────────────────────────────────────────────
void EnemyCarmel::DealExplosionDamage()
{
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	float dx = playerPos.getX() - (float)bodyX;
	float dy = playerPos.getY() - (float)bodyY;
	float dist = std::sqrt(dx * dx + dy * dy);

	if (dist < EXPLOSION_RADIUS)
	{
		auto& player = Engine::GetInstance().scene->player;
		if (player != nullptr && !player->IsDead())
		{
			player->TakeDamage((int)EXPLOSION_DAMAGE);
			LOG("EnemyCarmel: Explosion hit player! (dist=%.1f)", dist);
		}
	}
	else
	{
		LOG("EnemyCarmel: Explosion missed player (dist=%.1f, radius=%.1f)", dist, EXPLOSION_RADIUS);
	}
}

// ─── Take damage ─────────────────────────────────────────────────────
void EnemyCarmel::TakeDamage(int damage)
{
	if (state_ == EnemyCarmelState::DEATH) return;

	health -= damage;
	LOG("EnemyCarmel took %d damage -> health: %d/%d", damage, health, maxHealth);

	// Knockback
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);
	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	float dirX = ((float)bodyX > playerPos.getX()) ? 1.0f : -1.0f;
	Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, dirX * 5.0f, -3.0f, true);

	if (health <= 0)
	{
		health = 0;
		// Killed before exploding — just die, no explosion
		TransitionTo(EnemyCarmelState::DEATH);
	}
}

// ─── Draw ─────────────────────────────────────────────────────────────
void EnemyCarmel::Draw(float dt)
{
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	SDL_FlipMode flip = facingRight_ ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

	auto& render = *Engine::GetInstance().render;

	// ── Draw explosion radius indicator during SCARED and BLOWUP ──
	if (state_ == EnemyCarmelState::SCARED || state_ == EnemyCarmelState::BLOWUP)
	{
		// Pulsing alpha: oscillates between 40 and 120 for visibility
		float pulse = std::sin(pulseTimer_ * 0.008f) * 0.5f + 0.5f; // 0..1
		Uint8 alpha = (Uint8)(40.0f + pulse * 80.0f);

		// Draw filled circle as a series of horizontal lines for a filled effect
		int radius = (int)EXPLOSION_RADIUS;
		for (int dy = -radius; dy <= radius; dy++)
		{
			int halfWidth = (int)std::sqrt((float)(radius * radius - dy * dy));
			SDL_Rect line;
			line.x = x - halfWidth;
			line.y = y + dy;
			line.w = halfWidth * 2;
			line.h = 1;
			render.DrawRectangle(line, 255, 80, 30, alpha, true, true);
		}

		// Draw the outline circle on top for clarity
		render.DrawCircle(x, y, radius, 255, 50, 10, (Uint8)(alpha + 60), true);
	}

	// ── Draw the sprite based on current state ──
	switch (state_)
	{
	case EnemyCarmelState::IDLE:
	{
		idleAnims_.Update(dt);
		const SDL_Rect& frame = idleAnims_.GetCurrentFrame();
		render.DrawTexture(texture, x - texW / 2, y - texH / 2, &frame, 1.0f, 0, INT_MAX, INT_MAX, flip, 1.0f);
		break;
	}

	case EnemyCarmelState::PATROL:
	case EnemyCarmelState::CHASE:
	{
		rollAnims_.Update(dt);
		const SDL_Rect& frame = rollAnims_.GetCurrentFrame();
		int scaledW = (int)(256.0f * ROLL_DRAW_SCALE);
		int scaledH = (int)(256.0f * ROLL_DRAW_SCALE);
		int drawX = x - scaledW / 2;
		// Align the bottom of the 128x128 sprite with the bottom of the 64x64 idle sprite
		int drawY = (y + texH / 2) - scaledH;
		render.DrawTexture(rollTexture_, drawX, drawY, &frame, 1.0f, 0, INT_MAX, INT_MAX, flip, ROLL_DRAW_SCALE);
		break;
	}

	case EnemyCarmelState::SCARED:
	{
		scaredAnims_.Update(dt);
		const SDL_Rect& frame = scaredAnims_.GetCurrentFrame();
		render.DrawTexture(scaredTexture_, x - texW / 2, y - texH / 2, &frame, 1.0f, 0, INT_MAX, INT_MAX, flip, 1.0f);
		break;
	}

	case EnemyCarmelState::BLOWUP:
	{
		blowupAnims_.Update(dt);
		const SDL_Rect& frame = blowupAnims_.GetCurrentFrame();
		int scaledW = (int)(256.0f * BLOWUP_DRAW_SCALE);
		int scaledH = (int)(256.0f * BLOWUP_DRAW_SCALE);
		int drawX = x - scaledW / 2;
		// Align the bottom of the 128x128 sprite with the bottom of the 64x64 idle sprite
		int drawY = (y + texH / 2) - scaledH;
		render.DrawTexture(blowupTexture_, drawX, drawY, &frame, 1.0f, 0, INT_MAX, INT_MAX, flip, BLOWUP_DRAW_SCALE);
		break;
	}

	case EnemyCarmelState::DEATH:
		break;
	}
}
