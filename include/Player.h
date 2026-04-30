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
	void Revive();
	bool IsDead() const { return isDead_; }

	// Map viewer helpers
	SDL_Rect GetCurrentAnimationRect() const;
	bool     IsFacingRight() const;

	// Hiding state � queried by enemies to disable detection
	bool IsHiding() const { return isHiding_; }

	// Blanket (cape) collectible – must be collected before hiding is available
	bool HasBlanket() const { return hasBlanket_; }
	void SetHasBlanket(bool v) { hasBlanket_ = v; }

private:

	void GetPhysicsValues();
	void Move();
	void Jump();
	void Attack(float dt);
	void Hide(float dt);
	void Teleport();
	void ApplyPhysics();
	void Draw(float dt);

public:

	//Declare player parameters
	float speed = 4.0f;
	SDL_Texture* texture = NULL;

	int texW = 0, texH = 0;

	//Audio fx
	int pickCoinFxId = -1;
	int jumpFxId = -1;
	int stepsFxId = -1;
	int gameOverFxId = -1;

	PhysBody* pbody = nullptr;
	float jumpForce = 10.0f;
	bool isJumping = false;

	bool isWakingUp = true;

private:
	b2Vec2 velocity = { 0.0f, 0.0f };
	AnimationSet anims;
	Animation wakeUpAnim;
	SDL_Texture* wakeUpTexture = nullptr;
	float drawScale = 1.0f;
	bool facingRight = true;
	// ?? Hide cooldown 
	static constexpr float HIDE_COOLDOWN = 15000.0f; // ms
	float hideCooldown_ = 0.0f;
	// Combat - attack
	static constexpr float ATTACK_DURATION = 300.0f;
	static constexpr float ATTACK_COOLDOWN = 600.0f;
	static constexpr int   ATTACK_DAMAGE = 1;
	static constexpr int   HITBOX_W = 60;
	static constexpr int   HITBOX_H = 80;
	static constexpr int   HITBOX_OFFSET = 50;
	bool      isAttacking_ = false;
	float     attackTimer_ = 0.0f;
	float     attackCooldown_ = 0.0f;
	PhysBody* attackHitbox_ = nullptr;

	// Combat - i-frames
	static constexpr float IFRAME_DURATION = 1000.0f;
	bool  isInvincible_ = false;
	float iFrameTimer_ = 0.0f;

	// Death state
	bool isDead_ = false;

	// Hit/Death state flag
	bool isShowingDamageAnim_ = false;

	// Combat - visual feedback
	static constexpr float DAMAGE_FLASH_DURATION = 150.0f;
	float damageFlashTimer_ = 0.0f;

	float stepTimer_ = 0.0f;
	static constexpr float STEP_COOLDOWN = 350.0f;

	// Hide (press H to crouch behind rocks  enemies lose sight of player)
	bool  isHiding_ = false;
	bool  isExitingHide_ = false;
	// Visual: gentle alpha pulse while hidden to signal stealth state
	float hideAlphaTime_ = 0.0f;

	// Blanket (cape) collectible flag
	bool hasBlanket_ = false;
	};