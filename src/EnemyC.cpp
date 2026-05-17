#include "EnemyC.h"
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
#include "Projectile.h"
#include "tracy/Tracy.hpp"

EnemyC::EnemyC() : Entity(EntityType::ENEMY_C)
{
	name = "EnemyC";
}

EnemyC::~EnemyC() {}

bool EnemyC::Awake() {
	return true;
}

bool EnemyC::Start() {

	// Load walking animation from the TSX (no <animation> nodes, use sequential loader)
	anims.LoadSequentialFromTSX("assets/textures/animations/EnemigoC.tsx", "walk", 100);
	anims.SetCurrent("walk");
	anims.SetLoop("walk", true);

	texture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS enemics C/spritesheet_bloc_walking.png");

	// Physics body
	texW = 128;
	texH = 128;
	pbody = Engine::GetInstance().physics->CreateCapsule(
		(int)position.getX() + texW / 2, (int)position.getY() + texH / 2,
		20, 50, bodyType::DYNAMIC);

	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY;

	// Initialize pathfinding
	pathfinding = std::make_shared<Pathfinding>();
	Vector2D pos = GetPosition();
	Vector2D tilePos = Engine::GetInstance().map->WorldToMap((int)pos.getX(), (int)pos.getY() + 1);
	pathfinding->ResetPath(tilePos);

	// Remember patrol origin
	patrolOriginX_ = position.getX();

	// Save origin position to return when player hides
	originPosition_ = pos;
	originTilePos_ = tilePos;

	// Start in IDLE
	currentState_ = State::IDLE;
	idleTimer_ = 0.0f;

	return true;
}

bool EnemyC::Update(float dt)
{
	ZoneScoped;

	if (Engine::GetInstance().scene->isPaused_ || Engine::GetInstance().scene->isGameOver_) {
		Draw(0.0f);
		return true;
	}

	// Tick cooldowns
	if (shootCooldown_ > 0.0f) shootCooldown_ -= dt;
	if (attackCooldown_ > 0.0f) attackCooldown_ -= dt;

	// Contact damage while touching player
	if (isContactWithPlayer_ && playerListener_ != nullptr && attackCooldown_ <= 0.0f)
	{
		playerListener_->TakeDamage(1);
		attackCooldown_ = ATTACK_INTERVAL;
	}

	// ?? Hide behaviour: return to origin while player is hiding ??????????????
	auto playerShared = Engine::GetInstance().scene->player;
	bool playerIsHiding = playerShared && playerShared->IsHiding();

	if (playerIsHiding)
	{
		wasPlayerHiding_ = true;

		int bodyX, bodyY;
		pbody->GetPosition(bodyX, bodyY);
		Vector2D tilePos = Engine::GetInstance().map->WorldToMap(bodyX, bodyY);

		int distToOrigin = std::abs((int)tilePos.getX() - (int)originTilePos_.getX()) +
			std::abs((int)tilePos.getY() - (int)originTilePos_.getY());

		if (distToOrigin <= 1)
		{
			pathfinding->pathTiles.clear();
			GetPhysicsValues();
			velocity.x = 0.0f;
			ApplyPhysics();
		}
		else
		{
			pathfinding->ResetPath(tilePos);
			while (pathfinding->CanPropagateAStar(originTilePos_))
				pathfinding->PropagateAStar(SQUARED);

			GetPhysicsValues();
			MoveToward(originPosition_.getX());
			ApplyPhysics();
		}

		Draw(dt);
		return true;
	}

	if (wasPlayerHiding_)
	{
		wasPlayerHiding_ = false;
		EnterState(State::IDLE);
		int bodyX, bodyY;
		pbody->GetPosition(bodyX, bodyY);
		Vector2D tilePos = Engine::GetInstance().map->WorldToMap(bodyX, bodyY);
		pathfinding->ResetPath(tilePos);
		LOG("EnemyC re-acquired player — pathfinding reset");
	}
	// ?????????????????????????????????????????????????????????????????????????

	// FSM
	UpdateFSM(dt);

	Draw(dt);

	return true;
}

int EnemyC::GetDistanceToPlayer() const
{
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);
	Vector2D tilePos = Engine::GetInstance().map->WorldToMap(bodyX, bodyY);

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	Vector2D playerTilePos = Engine::GetInstance().map->WorldToMap((int)playerPos.getX(), (int)playerPos.getY());

	return std::abs((int)tilePos.getX() - (int)playerTilePos.getX()) +
		std::abs((int)tilePos.getY() - (int)playerTilePos.getY());
}

void EnemyC::UpdateFSM(float dt)
{
	int dist = GetDistanceToPlayer();

	switch (currentState_)
	{
	case State::IDLE:
	{
		// Stand still
		GetPhysicsValues();
		velocity.x = 0.0f;
		ApplyPhysics();

		idleTimer_ += dt;

		// Transition to PATROL after idle duration
		if (idleTimer_ >= IDLE_DURATION) {
			EnterState(State::PATROL);
		}
		// Transition to ALERT if player detected
		if (dist <= DETECT_RANGE) {
			EnterState(State::ALERT);
		}
		break;
	}

	case State::PATROL:
	{
		GetPhysicsValues();

		// Simple back-and-forth patrol
		int bodyX, bodyY;
		pbody->GetPosition(bodyX, bodyY);

		float offsetFromOrigin = (float)bodyX - patrolOriginX_;

		if (patrolDirection_ > 0.0f && offsetFromOrigin > PATROL_RANGE) {
			patrolDirection_ = -1.0f;
		}
		else if (patrolDirection_ < 0.0f && offsetFromOrigin < -PATROL_RANGE) {
			patrolDirection_ = 1.0f;
		}

		velocity.x = patrolDirection_ * PATROL_SPEED;
		facingRight_ = (velocity.x > 0.0f);
		ApplyPhysics();

		// Transition to ALERT if player detected
		if (dist <= DETECT_RANGE) {
			EnterState(State::ALERT);
		}
		break;
	}

	case State::ALERT:
	{
		// Perform pathfinding toward player
		PerformPathfinding();
		GetPhysicsValues();

		// Check transitions
		if (dist > DETECT_RANGE) {
			EnterState(State::IDLE);
			break;
		}

		if (dist <= FLEE_RANGE) {
			EnterState(State::FLEE);
			break;
		}

		if (dist <= SHOOT_RANGE && shootCooldown_ <= 0.0f) {
			EnterState(State::SHOOT);
			break;
		}

		// Move toward player (slowly)
		Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
		MoveToward(playerPos.getX());
		ApplyPhysics();
		break;
	}

	case State::SHOOT:
	{
		GetPhysicsValues();
		velocity.x = 0.0f; // Stand still to shoot
		ApplyPhysics();

		// Face player
		Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
		int bodyX, bodyY;
		pbody->GetPosition(bodyX, bodyY);
		facingRight_ = (playerPos.getX() > (float)bodyX);

		// Shoot
		Shoot();
		shootCooldown_ = SHOOT_COOLDOWN;

		// After shooting, go back to ALERT
		EnterState(State::ALERT);
		break;
	}

	case State::FLEE:
	{
		PerformPathfinding();
		GetPhysicsValues();

		// Check transitions
		if (dist > DETECT_RANGE) {
			EnterState(State::IDLE);
			break;
		}

		if (dist > FLEE_RANGE) {
			EnterState(State::ALERT);
			break;
		}

		// Move away from player
		Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
		MoveAwayFrom(playerPos.getX());

		// Can still shoot while fleeing
		if (shootCooldown_ <= 0.0f) {
			Shoot();
			shootCooldown_ = SHOOT_COOLDOWN;
		}

		ApplyPhysics();
		break;
	}
	}
}

void EnemyC::EnterState(State newState)
{
	currentState_ = newState;

	switch (newState)
	{
	case State::IDLE:
		idleTimer_ = 0.0f;
		break;
	case State::PATROL:
		speed = PATROL_SPEED;
		break;
	case State::ALERT:
		speed = PATROL_SPEED;
		break;
	case State::SHOOT:
		break;
	case State::FLEE:
		speed = FLEE_SPEED;
		break;
	}
}

void EnemyC::PerformPathfinding()
{
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);
	Vector2D tilePos = Engine::GetInstance().map->WorldToMap(bodyX, bodyY);

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	Vector2D playerTilePos = Engine::GetInstance().map->WorldToMap((int)playerPos.getX(), (int)playerPos.getY());

	int dist = std::abs((int)tilePos.getX() - (int)playerTilePos.getX()) +
		std::abs((int)tilePos.getY() - (int)playerTilePos.getY());

	if (dist > 25) {
		pathfinding->pathTiles.clear();
		return;
	}

	pathfinding->ResetPath(tilePos);

	while (pathfinding->CanPropagateAStar(tilePos)) {
		pathfinding->PropagateAStar(SQUARED);
	}
}

void EnemyC::GetPhysicsValues()
{
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity.x = 0.0f;
}

void EnemyC::MoveToward(float targetX)
{
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	const float POSITION_TOLERANCE = 2.0f;
	if (targetX > bodyX + POSITION_TOLERANCE) {
		velocity.x = speed;
		facingRight_ = true;
	}
	else if (targetX < bodyX - POSITION_TOLERANCE) {
		velocity.x = -speed;
		facingRight_ = false;
	}
	else {
		velocity.x = 0.0f;
	}
}

void EnemyC::MoveAwayFrom(float targetX)
{
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	// Move in opposite direction from target
	if (targetX > bodyX) {
		velocity.x = -FLEE_SPEED;
		facingRight_ = false;
	}
	else {
		velocity.x = FLEE_SPEED;
		facingRight_ = true;
	}
}

void EnemyC::ApplyPhysics()
{
	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}

void EnemyC::Shoot()
{
	// Create a projectile aimed at the player
	auto projEntity = Engine::GetInstance().entityManager->CreateEntity(EntityType::PROJECTILE);
	auto proj = std::dynamic_pointer_cast<Projectile>(projEntity);

	if (proj) {
		int bodyX, bodyY;
		pbody->GetPosition(bodyX, bodyY);

		// GetPlayerPosition returns top-left of sprite; offset to body center
		Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
		float playerCenterX = playerPos.getX() + 64.0f;
		float playerCenterY = playerPos.getY() + 64.0f;

		float dirX = playerCenterX - (float)bodyX;
		float dirY = playerCenterY - (float)bodyY;

		proj->SetDirection(dirX, dirY);
		proj->position = Vector2D((float)bodyX, (float)bodyY);
		proj->Start();

		LOG("EnemyC fired projectile toward player");
	}
}

void EnemyC::Draw(float dt)
{
	anims.Update(dt);
	const SDL_Rect& animFrame = anims.GetCurrentFrame();

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	// Draw pathfinding debug
	if (Engine::GetInstance().physics->IsDebug())
		pathfinding->DrawPath();

	// Flip based on facing direction (sprite natively faces left, so invert)
	SDL_FlipMode flip = facingRight_ ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

	// Offset Y upward by 20px so feet don't clip through the floor
	Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2 - 20, &animFrame,
		1.0f, 0, INT_MAX, INT_MAX, flip);
}

bool EnemyC::CleanUp()
{
	LOG("Cleanup EnemyC");
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

bool EnemyC::Destroy()
{
	LOG("Destroying EnemyC");
	active = false;
	pendingToDelete = true;
	return true;
}

void EnemyC::SetPosition(Vector2D pos) {
	pbody->SetPosition((int)(pos.getX()), (int)(pos.getY()));
}

Vector2D EnemyC::GetPosition() {
	int x, y;
	pbody->GetPosition(x, y);
	return Vector2D((float)x - texW / 2, (float)y - texH / 2);
}

void EnemyC::OnCollision(PhysBody* physA, PhysBody* physB)
{
	if (physB->ctype == ColliderType::ATTACK)
	{
		LOG("EnemyC hit by player attack");
		TakeDamage(1);
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

void EnemyC::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	if (physB->ctype == ColliderType::PLAYER)
	{
		isContactWithPlayer_ = false;
		playerListener_ = nullptr;
	}
}

void EnemyC::TakeDamage(int damage)
{
	health -= damage;
	LOG("EnemyC took %d damage -> health: %d/%d", damage, health, maxHealth);

	// Knockback
	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	float dirX = (bodyX > playerPos.getX()) ? 1.0f : -1.0f;
	Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, dirX * 5.0f, -3.0f, true);

	if (health <= 0)
	{
		health = 0;
		LOG("EnemyC is dead");
		Destroy();
	}
}