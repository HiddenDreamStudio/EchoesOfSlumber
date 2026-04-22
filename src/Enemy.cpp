#include "Enemy.h"
#include "Engine.h"
#include "Textures.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "tracy/Tracy.hpp"

Enemy::Enemy() : Entity(EntityType::ENEMY)
{
	name = "Enemy";
}

Enemy::~Enemy() {}

bool Enemy::Awake() { return true; }
bool Enemy::Start() { return true; }

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

	if (contactDamageCooldown_ > 0.0f) contactDamageCooldown_ -= dt;
	if (isContactWithPlayer_ && playerListener_ != nullptr && contactDamageCooldown_ <= 0.0f)
	{
		playerListener_->TakeDamage(1);
		contactDamageCooldown_ = CONTACT_DAMAGE_INTERVAL;
	}

	Draw(dt);
	return true;
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

void Enemy::GetPhysicsValues()
{
	velocity   = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity.x = 0.0f;
}

void Enemy::ApplyPhysics()
{
	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}

void Enemy::TakeDamage(int damage)
{
	health -= damage;
	LOG("Enemy took %d damage -> health: %d/%d", damage, health, maxHealth);

	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);
	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	float dirX = ((float)bodyX > playerPos.getX()) ? 1.0f : -1.0f;
	Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, dirX * 5.0f, -3.0f, true);

	if (health <= 0) {
		health = 0;
		Destroy();
	}
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
