#include "EnemyCarmel.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"

EnemyCarmel::EnemyCarmel()
{
	name = "EnemyCarmel";
}

EnemyCarmel::~EnemyCarmel() {}

bool EnemyCarmel::Start()
{
	std::unordered_map<int, std::string> idleAliases = { {0, "idle"} };
	anims_.LoadFromTSX("assets/textures/animations/carmelAnimation.xml", idleAliases);
	anims_.SetCurrent("idle");
	texture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS enemics C/spritesheet_Carmel_idle.png");

	std::unordered_map<int, std::string> rollAliases = { {0, "roll"} };
	rollAnims_.LoadFromTSX("assets/textures/animations/carmelRollAnimation.xml", rollAliases);
	rollAnims_.SetCurrent("roll");
	rollAnims_.SetLoop("roll", false);
	rollTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS enemics C/spritesheet_Carmel_Roll.png");

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
	return Enemy::CleanUp();
}

void EnemyCarmel::SetPatrolPoints(float leftX, float rightX)
{
	patrolLeftX_  = leftX;
	patrolRightX_ = rightX;
}

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
	case EnemyCarmelState::IDLE:
		velocity.x = 0.0f;
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f)
			TransitionTo(EnemyCarmelState::PATROL);
		if (distToPlayer < DETECTION_RADIUS)
			TransitionTo(EnemyCarmelState::CHASE);
		break;

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
		facingRight_  = (patrolDirX_ < 0.0f);
		break;

	case EnemyCarmelState::CHASE:
		if (distToPlayer > DETECTION_RADIUS * 1.5f)
		{
			TransitionTo(EnemyCarmelState::PATROL);
			break;
		}
		if (distToPlayer < ATTACK_RANGE)
		{
			TransitionTo(EnemyCarmelState::ATTACK);
			break;
		}
		velocity.x   = (dx > 0.0f) ? speed : -speed;
		facingRight_ = (dx < 0.0f);
		break;

	case EnemyCarmelState::ATTACK:
		stateTimer_ -= dt;
		velocity.x = attackDirX_ * ATTACK_SPEED;
		if (stateTimer_ <= 0.0f)
			TransitionTo(EnemyCarmelState::STUNNED);
		break;

	case EnemyCarmelState::STUNNED:
		velocity.x = 0.0f;
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f)
		{
			if (distToPlayer < DETECTION_RADIUS)
				TransitionTo(EnemyCarmelState::CHASE);
			else
				TransitionTo(EnemyCarmelState::PATROL);
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
		break;

	case EnemyCarmelState::ATTACK:
	{
		stateTimer_ = ATTACK_DURATION;
		int bodyX, bodyY;
		pbody->GetPosition(bodyX, bodyY);
		Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
		attackDirX_  = (playerPos.getX() > (float)bodyX) ? 1.0f : -1.0f;
		facingRight_ = (attackDirX_ < 0.0f);
		rollAnims_.ResetCurrent();
		LOG("EnemyCarmel: ATTACK");
		break;
	}

	case EnemyCarmelState::STUNNED:
		stateTimer_ = STUN_DURATION;
		velocity.x  = 0.0f;
		LOG("EnemyCarmel: STUNNED");
		break;

	case EnemyCarmelState::DEATH:
		LOG("EnemyCarmel: DEATH");
		Destroy();
		break;

	default:
		break;
	}
}

void EnemyCarmel::TakeDamage(int damage)
{
	if (state_ == EnemyCarmelState::DEATH) return;

	health -= damage;
	LOG("EnemyCarmel took %d damage -> health: %d/%d", damage, health, maxHealth);

	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);
	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	float dirX = ((float)bodyX > playerPos.getX()) ? 1.0f : -1.0f;
	Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, dirX * 5.0f, -3.0f, true);

	if (health <= 0)
	{
		health = 0;
		TransitionTo(EnemyCarmelState::DEATH);
	}
	else
	{
		TransitionTo(EnemyCarmelState::STUNNED);
	}
}

void EnemyCarmel::Draw(float dt)
{
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	SDL_FlipMode flip = facingRight_ ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

	if (state_ == EnemyCarmelState::ATTACK)
	{
		rollAnims_.Update(dt);
		const SDL_Rect& frame = rollAnims_.GetCurrentFrame();
		int drawX = x - (int)(256.0f * ROLL_DRAW_SCALE) / 2;
		int drawY = y - (int)(256.0f * ROLL_DRAW_SCALE) / 2;
		Engine::GetInstance().render->DrawTexture(rollTexture_, drawX, drawY, &frame, 1.0f, 0, INT_MAX, INT_MAX, flip, ROLL_DRAW_SCALE);
	}
	else
	{
		anims_.Update(dt);
		const SDL_Rect& frame = anims_.GetCurrentFrame();
		Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2, &frame, 1.0f, 0, INT_MAX, INT_MAX, flip, 1.0f);
	}
}
