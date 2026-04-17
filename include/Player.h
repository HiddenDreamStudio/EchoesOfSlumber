#pragma once

#include "Entity.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>

struct SDL_Texture;

class Player : public Entity
{
public:

	Player();

	virtual ~Player();

	bool Awake();

	bool Start();

	bool Update(float dt);

	bool CleanUp();

	bool Destroy();

	void OnCollision(PhysBody* physA, PhysBody* physB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB);

	Vector2D GetPosition();
	void SetPosition(Vector2D pos);

	void TakeDamage(int damage) override;

private:

	void GetPhysicsValues();
	void Move();
	void Jump();
	void Attack(float dt);
	void Teleport();
	void ApplyPhysics();
	void Draw(float dt);

public:

	//Declare player parameters
	float speed = 4.0f;
	SDL_Texture* texture = NULL;

	int texW, texH;

	//Audio fx
	int pickCoinFxId;

	PhysBody* pbody;
	float jumpForce = 10.0f; // The force to apply when jumping
	float doubleJumpForce = 11.0f; // The force to apply when double jumping
	bool isJumping = false; // Flag to check if the player is currently jumping
	bool canDoubleJump = false; // Flag to allow a second jump in the air
	bool hasDoubleJumped = false; // Flag to track if double jump was already used

private:
	b2Vec2 velocity;
	AnimationSet anims;
	Animation wakeUpAnim;
	SDL_Texture* wakeUpTexture = nullptr;
	bool isWakingUp = true;
	float drawScale = 1.0f;
	bool facingRight = true;

	// Combat - attack
	static constexpr float ATTACK_DURATION = 300.0f;  // ms hitbox active
	static constexpr float ATTACK_COOLDOWN = 600.0f;  // ms between attacks
	static constexpr int   ATTACK_DAMAGE   = 1;
	static constexpr int   HITBOX_W        = 60;
	static constexpr int   HITBOX_H        = 80;
	static constexpr int   HITBOX_OFFSET   = 50;      // px from body center to hitbox center
	bool      isAttacking_    = false;
	float     attackTimer_    = 0.0f;
	float     attackCooldown_ = 0.0f;
	PhysBody* attackHitbox_   = nullptr;

	// Combat - i-frames
	static constexpr float IFRAME_DURATION = 1000.0f; // ms
	bool  isInvincible_ = false;
	float iFrameTimer_  = 0.0f;

	// Death state
	bool isDead_ = false;

	// Hit/Death state flag
	bool isShowingDamageAnim_ = false;
};