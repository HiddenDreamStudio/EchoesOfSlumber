#include "EnemyB.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "Window.h"
#include "tracy/Tracy.hpp"

EnemyB::EnemyB() : Entity(EntityType::ENEMY_B)
{
	name = "EnemyB";
}

EnemyB::~EnemyB() {}

bool EnemyB::Awake() { return true; }

bool EnemyB::Start()
{
	std::unordered_map<int, std::string> walkAliases = { {0, "walk"} };
	walkAnims_.LoadFromTSX("assets/textures/animations/blocWalkingAnimation.xml", walkAliases);
	walkAnims_.SetCurrent("walk");
	walkTexture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS enemics C/spritesheet_bloc_walking.png");

	std::unordered_map<int, std::string> turnAliases = { {0, "turn"} };
	turnAnims_.LoadFromTSX("assets/textures/animations/blocTurnaroundAnimation.xml", turnAliases);
	turnAnims_.SetCurrent("turn");
	turnAnims_.SetLoop("turn", false);
	turnTexture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS enemics C/spritesheet_bloc_turnaround.png");

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

	TransitionTo(EnemyBState::IDLE);
	return true;
}

bool EnemyB::Update(float dt)
{
	ZoneScoped;

	if (Engine::GetInstance().scene->isPaused_) {
		Draw(0.0f);
		return true;
	}

	GetPhysicsValues();
	UpdateFSM(dt);
	ApplyPhysics();

	if (contactDamageCooldown_ > 0.0f) contactDamageCooldown_ -= dt;
	if (isContactWithPlayer_ && playerListener_ != nullptr && contactDamageCooldown_ <= 0.0f)
	{
		playerListener_->TakeDamage(1);
		contactDamageCooldown_ = CONTACT_DAMAGE_INTERVAL;
	}

	Draw(dt);
	return true;
}

void EnemyB::UpdateFSM(float dt)
{
	if (state_ == EnemyBState::DEATH) return;

	auto& player = Engine::GetInstance().scene->player;
	if (player != nullptr && player->IsDead())
	{
		velocity.x = 0.0f;
		return;
	}

	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	int playerBodyX, playerBodyY;
	Engine::GetInstance().scene->player->pbody->GetPosition(playerBodyX, playerBodyY);
	float dx           = (float)playerBodyX - (float)bodyX;
	float dy           = (float)playerBodyY - (float)bodyY;
	float distToPlayer = std::abs(dx);

	switch (state_)
	{
	case EnemyBState::IDLE:
		velocity.x = 0.0f;
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f)
			TransitionTo(EnemyBState::PATROL);
		if (distToPlayer < DETECTION_RADIUS)
			TransitionTo(EnemyBState::CHASE);
		break;

	case EnemyBState::PATROL:
		if (distToPlayer < DETECTION_RADIUS)
		{
			TransitionTo(EnemyBState::CHASE);
			break;
		}
		if (patrolDirX_ > 0.0f && (float)bodyX >= patrolRightX_)
		{
			TransitionTo(EnemyBState::TURNING);
			break;
		}
		if (patrolDirX_ < 0.0f && (float)bodyX <= patrolLeftX_)
		{
			TransitionTo(EnemyBState::TURNING);
			break;
		}
		velocity.x   = patrolDirX_ * speed;
		facingRight_ = (patrolDirX_ < 0.0f);
		break;

	case EnemyBState::TURNING:
		velocity.x = 0.0f;
		if (turnAnims_.HasFinishedOnce("turn"))
		{
			patrolDirX_  = -patrolDirX_;
			facingRight_ = (patrolDirX_ < 0.0f);
			TransitionTo(EnemyBState::PATROL);
		}
		break;

	case EnemyBState::CHASE:
		if (distToPlayer > DETECTION_RADIUS * 1.5f)
		{
			TransitionTo(EnemyBState::PATROL);
			break;
		}
		if (distToPlayer < ATTACK_RANGE && std::abs(dy) < ATTACK_RANGE_Y)
		{
			TransitionTo(EnemyBState::ATTACK);
			break;
		}
		velocity.x   = (dx > 0.0f) ? speed : -speed;
		facingRight_ = (dx < 0.0f);
		break;

	case EnemyBState::ATTACK:
		velocity.x = 0.0f;
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f)
			TransitionTo(EnemyBState::STUNNED);
		break;

	case EnemyBState::STUNNED:
		velocity.x = 0.0f;
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f)
		{
			if (distToPlayer < DETECTION_RADIUS)
				TransitionTo(EnemyBState::CHASE);
			else
				TransitionTo(EnemyBState::PATROL);
		}
		break;
	}
}

void EnemyB::TransitionTo(EnemyBState newState)
{
	state_ = newState;

	switch (newState)
	{
	case EnemyBState::IDLE:
		stateTimer_ = IDLE_DURATION;
		break;

	case EnemyBState::TURNING:
		turnAnims_.SetCurrent("turn");
		turnAnims_.ResetCurrent();
		velocity.x = 0.0f;
		break;

	case EnemyBState::ATTACK:
	{
		stateTimer_ = ATTACK_DURATION;
		int bodyX, bodyY;
		pbody->GetPosition(bodyX, bodyY);
		int playerBX, playerBY;
		Engine::GetInstance().scene->player->pbody->GetPosition(playerBX, playerBY);
		facingRight_ = ((float)playerBX < (float)bodyX);
		auto& pl = Engine::GetInstance().scene->player;
		if (pl && !pl->IsDead()) pl->TakeDamage(1);
		LOG("EnemyB: ATTACK");
		break;
	}

	case EnemyBState::STUNNED:
		stateTimer_ = STUN_DURATION;
		velocity.x  = 0.0f;
		break;

	case EnemyBState::DEATH:
		LOG("EnemyB: DEATH");
		Destroy();
		break;

	default:
		break;
	}
}

void EnemyB::GetPhysicsValues()
{
	velocity   = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity.x = 0.0f;
}

void EnemyB::ApplyPhysics()
{
	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}

void EnemyB::Draw(float dt)
{
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	if (Engine::GetInstance().physics->IsDebug())
	{
		auto& render = Engine::GetInstance().render;
		int scale    = Engine::GetInstance().window->GetScale();
		int cx = render->camera.x + x * scale;
		int cy = render->camera.y + y * scale;

		// Detection radius circle
		constexpr int SEGMENTS = 32;
		int r = (int)(DETECTION_RADIUS * scale);
		SDL_SetRenderDrawColor(render->renderer, 255, 255, 0, 180);
		for (int i = 0; i < SEGMENTS; i++)
		{
			float a0 = (float)i       / SEGMENTS * 6.2832f;
			float a1 = (float)(i + 1) / SEGMENTS * 6.2832f;
			SDL_RenderLine(render->renderer,
				cx + (int)(SDL_cosf(a0) * r), cy + (int)(SDL_sinf(a0) * r),
				cx + (int)(SDL_cosf(a1) * r), cy + (int)(SDL_sinf(a1) * r));
		}

		// Line to player only while chasing or attacking
		if (state_ == EnemyBState::CHASE || state_ == EnemyBState::ATTACK)
		{
			Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
			int sx2 = render->camera.x + (int)playerPos.getX() * scale;
			int sy2 = render->camera.y + (int)playerPos.getY() * scale;
			SDL_SetRenderDrawColor(render->renderer, 255, 50, 50, 255);
			SDL_RenderLine(render->renderer, cx, cy, sx2, sy2);
		}
	}

	SDL_FlipMode flip = facingRight_ ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

	if (state_ == EnemyBState::TURNING)
	{
		turnAnims_.Update(dt);
		const SDL_Rect& frame = turnAnims_.GetCurrentFrame();
		Engine::GetInstance().render->DrawTexture(turnTexture, x - texW / 2, y - texH / 2, &frame, 1.0f, 0, INT_MAX, INT_MAX, flip, 1.0f);
	}
	else
	{
		walkAnims_.Update(dt);
		const SDL_Rect& frame = walkAnims_.GetCurrentFrame();
		Engine::GetInstance().render->DrawTexture(walkTexture, x - texW / 2, y - texH / 2, &frame, 1.0f, 0, INT_MAX, INT_MAX, flip, 1.0f);
	}
}

bool EnemyB::CleanUp()
{
	LOG("Cleanup EnemyB");
	Engine::GetInstance().textures->UnLoad(walkTexture);
	Engine::GetInstance().textures->UnLoad(turnTexture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	return true;
}

bool EnemyB::Destroy()
{
	LOG("Destroying EnemyB");
	active = false;
	pendingToDelete = true;
	return true;
}

void EnemyB::SetPosition(Vector2D pos)
{
	pbody->SetPosition((int)pos.getX(), (int)pos.getY());
}

Vector2D EnemyB::GetPosition()
{
	int x, y;
	pbody->GetPosition(x, y);
	return Vector2D((float)x - texW / 2, (float)y - texH / 2);
}

void EnemyB::SetPatrolPoints(float leftX, float rightX)
{
	patrolLeftX_  = leftX;
	patrolRightX_ = rightX;
}

void EnemyB::OnCollision(PhysBody* physA, PhysBody* physB)
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

void EnemyB::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	if (physB->ctype == ColliderType::PLAYER)
	{
		isContactWithPlayer_ = false;
		playerListener_      = nullptr;
	}
}

void EnemyB::TakeDamage(int damage)
{
	if (state_ == EnemyBState::DEATH) return;

	health -= damage;
	LOG("EnemyB recibio %d de dano -> vida: %d/%d", damage, health, maxHealth);

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);
	float dirX = ((float)bodyX > playerPos.getX()) ? 1.0f : -1.0f;
	Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, dirX * 5.0f, -3.0f, true);

	if (health <= 0)
	{
		health = 0;
		TransitionTo(EnemyBState::DEATH);
	}
	else
	{
		TransitionTo(EnemyBState::STUNNED);
	}
}
