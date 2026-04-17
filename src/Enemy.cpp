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

	// load
	std::unordered_map<int, std::string> aliases = { {0, "idle"} };
	anims.LoadFromTSX("assets/textures/animations/carmelAnimation.xml", aliases);
	anims.SetCurrent("idle");

	texture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS enemics C/spritesheet_Carmel_idle.png");

	//Add physics to the enemy - initialize physics body
	texW = 64;
	texH = 64;
	pbody = Engine::GetInstance().physics->CreateCapsule((int)position.getX()+texW/2, (int)position.getY()+texH/2, 20, 50, bodyType::DYNAMIC);

	//Assign enemy class (using "this") to the listener of the pbody. This makes the Physics module to call the OnCollision method
	pbody->listener = this;

	//ssign collider type
	pbody->ctype = ColliderType::ENEMY;

	// Initialize pathfinding
	pathfinding = std::make_shared<Pathfinding>();
	//Get the position of the enemy
	Vector2D pos = GetPosition();
	//Convert to tile coordinates
	Vector2D tilePos = Engine::GetInstance().map->WorldToMap((int)pos.getX(), (int)pos.getY()+1);
	//Reset pathfinding
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

	//Get the position of the enemy
	Vector2D pos = GetPosition();
	//Convert to tile coordinates
	Vector2D tilePos = Engine::GetInstance().map.get()->WorldToMap((int)pos.getX(), (int)pos.getY());
	//Reset pathfinding
	pathfinding->ResetPath(tilePos);

	while(pathfinding->CanPropagateAStar(tilePos)) {
		pathfinding->PropagateAStar(SQUARED);
	}
}

void Enemy::GetPhysicsValues() {
	// Read current velocity
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y }; 
}

void Enemy::Move() {

	if (pathfinding->pathTiles.size() < 2) return;

	// Path exists: move horizontally toward the player
	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();

	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	float playerCenterX = playerPos.getX() + texW * 0.5f;

	const float POSITION_TOLERANCE = 2.0f;
	if (playerCenterX > bodyX + POSITION_TOLERANCE) {
		velocity.x = speed;
	} else if (playerCenterX < bodyX - POSITION_TOLERANCE) {
		velocity.x = -speed;
	} else {
		velocity.x = 0;
	}
}

void Enemy::ApplyPhysics() {

	// Apply velocity via helper
	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}

void Enemy::Draw(float dt) {

	anims.Update(dt);
	const SDL_Rect& animFrame = anims.GetCurrentFrame();

	// Update render position using your PhysBody helper
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	// Draw pathfinding debug only when F9 debug mode is active
	if (Engine::GetInstance().physics->IsDebug())
		pathfinding->DrawPath();

	//Draw the enemy using the texture and the current animation frame
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
	// Adjust for center
	return Vector2D((float)x-texW/2,(float)y-texH/2);
}

//Define OnCollision function for the enemy.
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
		// Deal damage immediately on first contact if cooldown allows
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

	// Knockback: push the enemy away from the player
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

