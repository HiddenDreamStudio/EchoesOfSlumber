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

Enemy::~Enemy() {

}

bool Enemy::Awake() {
	return true;
}

bool Enemy::Start() {

	std::unordered_map<int, std::string> aliases = { {0, "idle"} };
	anims.LoadFromTSX("assets/textures/animations/carmelAnimation.xml", aliases);
	anims.SetCurrent("idle");

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

	// Guardar posici�n de origen para volver cuando el jugador se esconde
	originPosition_ = pos;
	originTilePos_ = tilePos;

	return true;
}

bool Enemy::Update(float dt)
{
	ZoneScoped;

	if (Engine::GetInstance().scene->isPaused_ || Engine::GetInstance().scene->isGameOver_) {
		Draw(0.0f);
		return true;
	}

	PerformPathfinding();
	GetPhysicsValues();
	Move();
	ApplyPhysics();

	// Tick attack cooldown and apply contact damage while touching player
	if (attackCooldown_ > 0.0f) attackCooldown_ -= dt;
	if (isContactWithPlayer_ && playerListener_ != nullptr && attackCooldown_ <= 0.0f)
	{
		playerListener_->TakeDamage(1);
		attackCooldown_ = ATTACK_INTERVAL;
	}

	Draw(dt);

	return true;
}

void Enemy::PerformPathfinding() {

	auto playerShared = Engine::GetInstance().scene->player;
	bool playerIsHiding = playerShared && playerShared->IsHiding();

	if (playerIsHiding)
	{
		wasPlayerHiding_ = true;

		Vector2D pos = GetPosition();
		Vector2D tilePos = Engine::GetInstance().map->WorldToMap((int)pos.getX(), (int)pos.getY());

		// Si ya estamos en el tile de origen, limpiar path y parar
		int distToOrigin = std::abs((int)tilePos.getX() - (int)originTilePos_.getX()) +
			std::abs((int)tilePos.getY() - (int)originTilePos_.getY());
		if (distToOrigin <= 1)
		{
			pathfinding->pathTiles.clear();
			return;
		}

		// Pathfinding normal pero con destino = origen
		pathfinding->ResetPath(tilePos);
		while (pathfinding->CanPropagateAStar(originTilePos_))
			pathfinding->PropagateAStar(SQUARED);

		return;
	}

	if (wasPlayerHiding_)
	{
		// Player just stopped hiding � fully reset pathfinding from current tile
		Vector2D pos = GetPosition();
		Vector2D tilePos = Engine::GetInstance().map->WorldToMap((int)pos.getX(), (int)pos.getY());
		pathfinding->ResetPath(tilePos);
		wasPlayerHiding_ = false;
		LOG("Enemy re-acquired player (player stopped hiding) � pathfinding reset");
	}

	// Normal pathfinding (player is visible)

	Vector2D pos = GetPosition();
	Vector2D tilePos = Engine::GetInstance().map.get()->WorldToMap((int)pos.getX(), (int)pos.getY());

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	Vector2D playerTilePos = Engine::GetInstance().map->WorldToMap((int)playerPos.getX(), (int)playerPos.getY());

	// Security check: don't pathfind if player is too far (avoids FPS drop)
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

void Enemy::GetPhysicsValues() {
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y };
}

void Enemy::Move() {

	// Don't move if we have no path
	if (pathfinding->pathTiles.size() < 2) return;

	auto playerShared = Engine::GetInstance().scene->player;
	bool playerIsHiding = playerShared && playerShared->IsHiding();

	// Si el jugador se est� escondiendo, moverse hacia la posici�n de origen
	if (playerIsHiding)
	{
		int bodyX, bodyY;
		pbody->GetPosition(bodyX, bodyY);

		float originCenterX = originPosition_.getX() + texW * 0.5f;
		const float TOLERANCE = 2.0f;

		if (originCenterX > bodyX + TOLERANCE) velocity.x = speed;
		else if (originCenterX < bodyX - TOLERANCE) velocity.x = -speed;
		else                                         velocity.x = 0.0f;
		return;
	}

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();

	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	float playerCenterX = playerPos.getX() + texW * 0.5f;
	const float POSITION_TOLERANCE = 2.0f;

	if (playerCenterX > bodyX + POSITION_TOLERANCE) {
		velocity.x = speed;
	}
	else if (playerCenterX < bodyX - POSITION_TOLERANCE) {
		velocity.x = -speed;
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

	Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2, &animFrame);
}

bool Enemy::CleanUp()
{
	LOG("Cleanup enemy");
	Engine::GetInstance().textures->UnLoad(texture);
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

void Enemy::SetPosition(Vector2D pos) {
	pbody->SetPosition((int)(pos.getX()), (int)(pos.getY()));
}

Vector2D Enemy::GetPosition() {
	int x, y;
	pbody->GetPosition(x, y);
	return Vector2D((float)x - texW / 2, (float)y - texH / 2);
}

void Enemy::OnCollision(PhysBody* physA, PhysBody* physB) {
	if (physB->ctype == ColliderType::ATTACK)
	{
		LOG("Enemy hit by player attack");
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

void Enemy::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	if (physB->ctype == ColliderType::PLAYER)
	{
		isContactWithPlayer_ = false;
		playerListener_ = nullptr;
	}
}

void Enemy::TakeDamage(int damage)
{
	health -= damage;
	LOG("Enemy took %d damage -> health: %d/%d", damage, health, maxHealth);

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	float dirX = (bodyX > playerPos.getX()) ? 1.0f : -1.0f;
	Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, dirX * 5.0f, -3.0f, true);

	if (health <= 0)
	{
		health = 0;
		LOG("Enemy is dead");
		Destroy();
	}
}