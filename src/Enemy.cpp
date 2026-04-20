#include "Enemy.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "tracy/Tracy.hpp"

Enemy::Enemy() : Entity(EntityType::ENEMY)
{
	name = "Enemy";
}

Enemy::~Enemy() {}

bool Enemy::Awake() { return true; }

bool Enemy::Start()
{
	// Idle animation
	std::unordered_map<int, std::string> idleAliases = { {0, "idle"} };
	anims.LoadFromTSX("assets/textures/animations/carmelAnimation.xml", idleAliases);
	anims.SetCurrent("idle");
	texture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS enemics C/spritesheet_Carmel_idle.png");

	// Roll animation (attack)
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

	// Default patrol range around spawn if not set via SetPatrolPoints
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);
	if (patrolLeftX_ == 0.0f && patrolRightX_ == 0.0f)
	{
		patrolLeftX_  = (float)bodyX - 200.0f;
		patrolRightX_ = (float)bodyX + 200.0f;
	}

	TransitionTo(EnemyState::IDLE);
	return true;
}

bool Enemy::Update(float dt)
{
	ZoneScoped;

	if (Engine::GetInstance().scene->isPaused_) {
		Draw(0.0f);
		return true;
	}

	GetPhysicsValues();
	UpdateFSM(dt);
	ApplyPhysics();

	// Contact damage tick
	if (contactDamageCooldown_ > 0.0f) contactDamageCooldown_ -= dt;
	if (isContactWithPlayer_ && playerListener_ != nullptr && contactDamageCooldown_ <= 0.0f)
	{
		playerListener_->TakeDamage(1);
		contactDamageCooldown_ = CONTACT_DAMAGE_INTERVAL;
	}

	Draw(dt);
	return true;
}

void Enemy::UpdateFSM(float dt)
{
	if (state_ == EnemyState::DEATH) return;

	auto& player = Engine::GetInstance().scene->player;
	if (player != nullptr && player->IsDead())
	{
		velocity.x = 0.0f;
		return;
	}

	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	float dx = playerPos.getX() - (float)bodyX;
	float distToPlayer = std::abs(dx);

	switch (state_)
	{
	case EnemyState::IDLE:
		velocity.x = 0.0f;
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f)
			TransitionTo(EnemyState::PATROL);
		if (distToPlayer < DETECTION_RADIUS)
			TransitionTo(EnemyState::CHASE);
		break;

	case EnemyState::PATROL:
		if (distToPlayer < DETECTION_RADIUS)
		{
			TransitionTo(EnemyState::CHASE);
			break;
		}
		if (patrolDirX_ > 0.0f && (float)bodyX >= patrolRightX_)
			patrolDirX_ = -1.0f;
		else if (patrolDirX_ < 0.0f && (float)bodyX <= patrolLeftX_)
			patrolDirX_ = 1.0f;

		velocity.x     = patrolDirX_ * speed;
		facingRight_   = (patrolDirX_ < 0.0f);
		break;

	case EnemyState::CHASE:
		if (distToPlayer > DETECTION_RADIUS * 1.5f)
		{
			TransitionTo(EnemyState::PATROL);
			break;
		}
		if (distToPlayer < ATTACK_RANGE)
		{
			TransitionTo(EnemyState::ATTACK);
			break;
		}
		velocity.x   = (dx > 0.0f) ? speed : -speed;
		facingRight_ = (dx < 0.0f);
		break;

	case EnemyState::ATTACK:
		stateTimer_ -= dt;
		velocity.x = attackDirX_ * ATTACK_SPEED;
		if (stateTimer_ <= 0.0f)
			TransitionTo(EnemyState::STUNNED);
		break;

	case EnemyState::STUNNED:
		velocity.x = 0.0f;
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f)
		{
			if (distToPlayer < DETECTION_RADIUS)
				TransitionTo(EnemyState::CHASE);
			else
				TransitionTo(EnemyState::PATROL);
		}
		break;
	}
}

void Enemy::TransitionTo(EnemyState newState)
{
	state_ = newState;

	switch (newState)
	{
	case EnemyState::IDLE:
		stateTimer_ = IDLE_DURATION;
		break;

	case EnemyState::ATTACK:
	{
		stateTimer_ = ATTACK_DURATION;
		int bodyX, bodyY;
		pbody->GetPosition(bodyX, bodyY);
		Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
		attackDirX_  = (playerPos.getX() > (float)bodyX) ? 1.0f : -1.0f;
		facingRight_ = (attackDirX_ < 0.0f);
		rollAnims_.ResetCurrent();
		LOG("Enemy: ATTACK");
		break;
	}

	case EnemyState::STUNNED:
		stateTimer_ = STUN_DURATION;
		velocity.x  = 0.0f;
		LOG("Enemy: STUNNED");
		break;

	case EnemyState::DEATH:
		LOG("Enemy: DEATH");
		Destroy();
		break;

	default:
		break;
	}
}

void Enemy::GetPhysicsValues()
{
	velocity   = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity.x = 0.0f;
}

void Enemy::ApplyPhysics()
{
	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}

void Enemy::Draw(float dt)
{
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	SDL_FlipMode flip = facingRight_ ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

	if (state_ == EnemyState::ATTACK)
	{
		rollAnims_.Update(dt);
		const SDL_Rect& frame = rollAnims_.GetCurrentFrame();
		int drawX = x - (int)(256.0f * ROLL_DRAW_SCALE) / 2;
		int drawY = y - (int)(256.0f * ROLL_DRAW_SCALE) / 2;
		Engine::GetInstance().render->DrawTexture(rollTexture_, drawX, drawY, &frame, 1.0f, 0, INT_MAX, INT_MAX, flip, ROLL_DRAW_SCALE);
	}
	else
	{
		anims.Update(dt);
		const SDL_Rect& frame = anims.GetCurrentFrame();
		Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2, &frame, 1.0f, 0, INT_MAX, INT_MAX, flip, 1.0f);
	}
}

bool Enemy::CleanUp()
{
	LOG("Cleanup enemy");
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().textures->UnLoad(rollTexture_);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	return true;
}

bool Enemy::Destroy()
{
	LOG("Destroying Enemy");
	active = false;
	pendingToDelete = true;
	return true;
}

void Enemy::SetPosition(Vector2D pos)
{
	pbody->SetPosition((int)pos.getX(), (int)pos.getY());
}

Vector2D Enemy::GetPosition()
{
	int x, y;
	pbody->GetPosition(x, y);
	return Vector2D((float)x - texW / 2, (float)y - texH / 2);
}

void Enemy::SetPatrolPoints(float leftX, float rightX)
{
	patrolLeftX_  = leftX;
	patrolRightX_ = rightX;
}

void Enemy::OnCollision(PhysBody* physA, PhysBody* physB)
{
	if (physB->ctype == ColliderType::ATTACK)
	{
		TakeDamage(1);
	}
	else if (physB->ctype == ColliderType::PLAYER)
	{
		isContactWithPlayer_ = true;
		playerListener_      = physB->listener;
		if (contactDamageCooldown_ <= 0.0f)
		{
			playerListener_->TakeDamage(1);
			contactDamageCooldown_ = CONTACT_DAMAGE_INTERVAL;
		}
	}
}

void Enemy::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	if (physB->ctype == ColliderType::PLAYER)
	{
		isContactWithPlayer_ = false;
		playerListener_      = nullptr;
	}
}

void Enemy::TakeDamage(int damage)
{
	if (state_ == EnemyState::DEATH) return;

	health -= damage;
	LOG("Enemy took %d damage -> health: %d/%d", damage, health, maxHealth);

	// Knockback
	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);
	float dirX = ((float)bodyX > playerPos.getX()) ? 1.0f : -1.0f;
	Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, dirX * 5.0f, -3.0f, true);

	if (health <= 0)
	{
		health = 0;
		TransitionTo(EnemyState::DEATH);
	}
	else
	{
		TransitionTo(EnemyState::STUNNED);
	}
}
