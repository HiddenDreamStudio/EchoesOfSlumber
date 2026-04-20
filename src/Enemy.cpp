#include "Enemy.h"
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
#include "tracy/Tracy.hpp"

Enemy::Enemy() : Entity(EntityType::ENEMY)
{
	name = "Enemy";
}

Enemy::~Enemy() {}

bool Enemy::Awake() {
	return true;
}

bool Enemy::Start() {
	// Load animations with their aliases
	std::unordered_map<int, std::string> aliases = {
		{0, "idle"},
		{10, "walk"},
		{20, "attack"},
		{30, "stun"},
		{40, "death"}
	};
	anims.LoadFromTSX("assets/textures/animations/carmelAnimation.xml", aliases);
	anims.SetCurrent("idle");
	anims.SetLoop("death", false);

	texture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS enemics C/spritesheet_Carmel_idle.png");

	texW = 64;
	texH = 64;
	pbody = Engine::GetInstance().physics->CreateCapsule((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, 20, 50, bodyType::DYNAMIC);
	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY;

	pathfinding = std::make_shared<Pathfinding>();
	Vector2D pos = GetPosition();
	Vector2D tilePos = Engine::GetInstance().map->WorldToMap((int)pos.getX(), (int)pos.getY() + 1);
	pathfinding->ResetPath(tilePos);

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

	// Tick attack cooldown and apply contact damage
	if (attackCooldown_ > 0.0f) attackCooldown_ -= dt;
	if (isContactWithPlayer_ && playerListener_ != nullptr && attackCooldown_ <= 0.0f && health > 0)
	{
		playerListener_->TakeDamage(1);
		attackCooldown_ = ATTACK_INTERVAL;
	}

	Draw(dt);
	return true;
}

void Enemy::UpdateFSM(float dt) {
	if (currentState == EnemyState::DEAD) return;

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	Vector2D myPos = GetPosition();
	float distToPlayer = myPos.distanceEuclidean(playerPos);

	switch (currentState) {
	case EnemyState::IDLE:
		velocity.x = 0.0f;
		if (anims.Has("idle")) anims.SetCurrent("idle");
		stateTimer -= dt;

		if (distToPlayer < detectionRadius) {
			currentState = EnemyState::CHASE;
		}
		else if (stateTimer <= 0.0f) {
			currentState = EnemyState::PATROL;
			stateTimer = 2000.0f;
			facingRight = !facingRight;
		}
		break;

	case EnemyState::PATROL:
		if (anims.Has("walk")) anims.SetCurrent("walk");
		velocity.x = facingRight ? speed : -speed;
		stateTimer -= dt;

		if (distToPlayer < detectionRadius) {
			currentState = EnemyState::CHASE;
		}
		else if (stateTimer <= 0.0f) {
			currentState = EnemyState::IDLE;
			stateTimer = 1500.0f;
		}
		break;

	case EnemyState::CHASE:
		if (anims.Has("walk")) anims.SetCurrent("walk");

		if (distToPlayer > detectionRadius * 1.5f) {
			currentState = EnemyState::IDLE;
			stateTimer = 1000.0f;
		}
		else if (distToPlayer <= attackRadius) {
			// Start dash preparation
			currentState = EnemyState::ATTACK;
			stateTimer = 600.0f;
			velocity.x = 0.0f;
		}
		else {
			PerformPathfinding();
			Move();
		}
		break;

	case EnemyState::ATTACK:
		if (stateTimer > 0.0f) {
			// Windup / Anticipation
			if (anims.Has("idle")) anims.SetCurrent("idle");
			velocity.x = 0.0f;
			facingRight = (playerPos.getX() > myPos.getX());
			stateTimer -= dt;
		}
		else if (stateTimer > -400.0f) {
			// Dash / Charge
			if (anims.Has("attack")) anims.SetCurrent("attack");
			velocity.x = facingRight ? dashSpeed : -dashSpeed;
			stateTimer -= dt;
		}
		else {
			// Becomes vulnerable / stunned
			currentState = EnemyState::STUNNED;
			stateTimer = 1200.0f;
		}
		break;

	case EnemyState::STUNNED:
		if (anims.Has("stun")) anims.SetCurrent("stun");
		velocity.x = 0.0f;
		stateTimer -= dt;
		if (stateTimer <= 0.0f) {
			currentState = EnemyState::IDLE;
			stateTimer = 500.0f;
		}
		break;

	case EnemyState::DEAD:
		break;
	}
}

void Enemy::PerformPathfinding() {
	Vector2D myPos = GetPosition();
	Vector2D tilePos = Engine::GetInstance().map->WorldToMap((int)myPos.getX(), (int)myPos.getY());

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	Vector2D playerTilePos = Engine::GetInstance().map->WorldToMap((int)playerPos.getX(), (int)playerPos.getY());

	int dist = std::abs((int)tilePos.getX() - (int)playerTilePos.getX()) + std::abs((int)tilePos.getY() - (int)playerTilePos.getY());
	if (dist > 25) {
		pathfinding->pathTiles.clear();
		return;
	}

	pathfinding->ResetPath(tilePos);
	while (pathfinding->CanPropagateAStar(tilePos)) {
		pathfinding->PropagateAStar(SQUARED);
	}
}

void Enemy::GetPhysicsValues() {
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y };
}

void Enemy::Move() {
	if (pathfinding->pathTiles.size() < 2) return;

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	float playerCenterX = playerPos.getX() + texW * 0.5f;

	const float POSITION_TOLERANCE = 2.0f;
	if (playerCenterX > bodyX + POSITION_TOLERANCE) {
		velocity.x = speed;
		facingRight = true;
	}
	else if (playerCenterX < bodyX - POSITION_TOLERANCE) {
		velocity.x = -speed;
		facingRight = false;
	}
	else {
		velocity.x = 0;
	}
}

void Enemy::ApplyPhysics() {
	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}

void Enemy::Draw(float dt) {
	anims.Update(dt);
	const SDL_Rect& animFrame = anims.GetCurrentFrame();

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	if (Engine::GetInstance().physics->IsDebug())
		pathfinding->DrawPath();

	SDL_FlipMode flip = facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
	Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2, &animFrame, 1.0f, 0, INT_MAX, INT_MAX, flip);

	// If dead and animation finishes, destroy entity
	if (currentState == EnemyState::DEAD && anims.HasFinishedOnce("death")) {
		Destroy();
	}
}

bool Enemy::CleanUp() {
	LOG("Cleanup enemy");
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	return true;
}

bool Enemy::Destroy() {
	LOG("Destroying Enemy");
	active = false;
	pendingToDelete = true;
	return true;
}

void Enemy::SetPosition(Vector2D pos) {
	pbody->SetPosition((int)(pos.getX()), (int)(pos.getY()));
}

Vector2D Enemy::GetPosition() {
	int x, y;
	pbody->GetPosition(x, y);
	return Vector2D((float)x - texW / 2, (float)y - texH / 2);
}

void Enemy::OnCollision(PhysBody* physA, PhysBody* physB) {
	if (physB->ctype == ColliderType::ATTACK) {
		LOG("Enemy hit by player attack");
		TakeDamage(1);
	}
	else if (physB->ctype == ColliderType::PLAYER) {
		isContactWithPlayer_ = true;
		playerListener_ = physB->listener;

		if (attackCooldown_ <= 0.0f && health > 0) {
			playerListener_->TakeDamage(1);
			attackCooldown_ = ATTACK_INTERVAL;
		}
	}
}

void Enemy::OnCollisionEnd(PhysBody* physA, PhysBody* physB) {
	if (physB->ctype == ColliderType::PLAYER) {
		isContactWithPlayer_ = false;
		playerListener_ = nullptr;
	}
}

void Enemy::TakeDamage(int damage) {
	if (health <= 0) return;

	health -= damage;
	LOG("Enemy took %d damage -> health: %d/%d", damage, health, maxHealth);

	// Basic knockback
	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	float dirX = (bodyX > playerPos.getX()) ? 1.0f : -1.0f;
	Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, dirX * 5.0f, -3.0f, true);

	if (health <= 0) {
		health = 0;
		LOG("Enemy is dead");
		currentState = EnemyState::DEAD;
		velocity.x = 0.0f;
		if (anims.Has("death")) {
			anims.SetCurrent("death");
		}
		else {
			Destroy(); 
		}
	}
	else {
		// Stunned momentarily when taking damage
		currentState = EnemyState::STUNNED;
		stateTimer = 400.0f;
	}
}