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
<<<<<<< HEAD

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
=======

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
>>>>>>> 28353b0763df53baf241986e7590583d5de99a33

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

<<<<<<< HEAD
	// Tick attack cooldown and apply contact damage
	if (attackCooldown_ > 0.0f) attackCooldown_ -= dt;
	if (isContactWithPlayer_ && playerListener_ != nullptr && attackCooldown_ <= 0.0f && health > 0)
=======
	// Contact damage tick
	if (contactDamageCooldown_ > 0.0f) contactDamageCooldown_ -= dt;
	if (isContactWithPlayer_ && playerListener_ != nullptr && contactDamageCooldown_ <= 0.0f)
>>>>>>> 28353b0763df53baf241986e7590583d5de99a33
	{
		playerListener_->TakeDamage(1);
		contactDamageCooldown_ = CONTACT_DAMAGE_INTERVAL;
	}

	Draw(dt);
	return true;
}

<<<<<<< HEAD
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
=======
void Enemy::UpdateFSM(float dt)
{
	if (state_ == EnemyState::DEATH) return;

	auto& player = Engine::GetInstance().scene->player;
	if (player != nullptr && player->IsDead())
	{
		velocity.x = 0.0f;
		return;
	}

>>>>>>> 28353b0763df53baf241986e7590583d5de99a33
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	float dx = playerPos.getX() - (float)bodyX;
	float distToPlayer = std::abs(dx);

<<<<<<< HEAD
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

=======
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
>>>>>>> 28353b0763df53baf241986e7590583d5de99a33
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

<<<<<<< HEAD
	if (Engine::GetInstance().physics->IsDebug())
		pathfinding->DrawPath();

	SDL_FlipMode flip = facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
	Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2, &animFrame, 1.0f, 0, INT_MAX, INT_MAX, flip);

	// If dead and animation finishes, destroy entity
	if (currentState == EnemyState::DEAD && anims.HasFinishedOnce("death")) {
		Destroy();
=======
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
>>>>>>> 28353b0763df53baf241986e7590583d5de99a33
	}
}

bool Enemy::CleanUp() {
	LOG("Cleanup enemy");
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().textures->UnLoad(rollTexture_);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	return true;
}

bool Enemy::Destroy() {
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

<<<<<<< HEAD
void Enemy::OnCollision(PhysBody* physA, PhysBody* physB) {
	if (physB->ctype == ColliderType::ATTACK) {
		LOG("Enemy hit by player attack");
=======
void Enemy::SetPatrolPoints(float leftX, float rightX)
{
	patrolLeftX_  = leftX;
	patrolRightX_ = rightX;
}

void Enemy::OnCollision(PhysBody* physA, PhysBody* physB)
{
	if (physB->ctype == ColliderType::ATTACK)
	{
>>>>>>> 28353b0763df53baf241986e7590583d5de99a33
		TakeDamage(1);
	}
	else if (physB->ctype == ColliderType::PLAYER) {
		isContactWithPlayer_ = true;
<<<<<<< HEAD
		playerListener_ = physB->listener;

		if (attackCooldown_ <= 0.0f && health > 0) {
=======
		playerListener_      = physB->listener;
		if (contactDamageCooldown_ <= 0.0f)
		{
>>>>>>> 28353b0763df53baf241986e7590583d5de99a33
			playerListener_->TakeDamage(1);
			contactDamageCooldown_ = CONTACT_DAMAGE_INTERVAL;
		}
	}
}

void Enemy::OnCollisionEnd(PhysBody* physA, PhysBody* physB) {
	if (physB->ctype == ColliderType::PLAYER) {
		isContactWithPlayer_ = false;
		playerListener_      = nullptr;
	}
}

<<<<<<< HEAD
void Enemy::TakeDamage(int damage) {
	if (health <= 0) return;
=======
void Enemy::TakeDamage(int damage)
{
	if (state_ == EnemyState::DEATH) return;
>>>>>>> 28353b0763df53baf241986e7590583d5de99a33

	health -= damage;
	LOG("Enemy took %d damage -> health: %d/%d", damage, health, maxHealth);

<<<<<<< HEAD
	// Basic knockback
=======
	// Knockback
>>>>>>> 28353b0763df53baf241986e7590583d5de99a33
	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);
	float dirX = ((float)bodyX > playerPos.getX()) ? 1.0f : -1.0f;
	Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, dirX * 5.0f, -3.0f, true);

	if (health <= 0) {
		health = 0;
<<<<<<< HEAD
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
=======
		TransitionTo(EnemyState::DEATH);
	}
	else
	{
		TransitionTo(EnemyState::STUNNED);
	}
}
>>>>>>> 28353b0763df53baf241986e7590583d5de99a33
