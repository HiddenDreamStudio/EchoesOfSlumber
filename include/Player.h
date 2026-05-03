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

	// Push rock state — queried externally if needed
	bool IsPushing() const { return isPushing_; }

private:

	void GetPhysicsValues();
	void Move();
	void Jump();
	void Attack(float dt);
	void Hide(float dt);
	void Teleport();
	void ApplyPhysics();
	void Draw(float dt);
	void UpdateClimb(float dt);

public:

	//Declare player parameters
	float speed = 4.0f;
	SDL_Texture* texture = NULL;

	int texW = 0, texH = 0;

	//Audio fx
	int pickCoinFxId = -1;
<<<<<<< HEAD

	PhysBody* pbody = nullptr;
	float jumpForce = 10.0f; // The force to apply when jumping
	float doubleJumpForce = 11.0f; // The force to apply when double jumping
	bool isJumping = false; // Flag to check if the player is currently jumping
	bool canDoubleJump = false; // Flag to allow a second jump in the air
	bool hasDoubleJumped = false; // Flag to track if double jump was already used
=======
	int jumpFxId = -1;
	int stepsFxId = -1;
	int gameOverFxId = -1;

	PhysBody* pbody = nullptr;
	float jumpForce = 10.0f;
	bool isJumping = false;

	bool isWakingUp = true;
>>>>>>> main

private:
	b2Vec2 velocity = { 0.0f, 0.0f };
	AnimationSet anims;
	Animation wakeUpAnim;
	SDL_Texture* wakeUpTexture = nullptr;
<<<<<<< HEAD
	// Walkthrough/Cinematic state (from main)
	bool isWakingUp = true;
=======
>>>>>>> main
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
	float damageFlashTimer_ = 0.0f;

	// Ledge climb
	AnimationSet climbAnims;
	SDL_Texture* climbTexture = nullptr;
	static constexpr float CLIMB_DRAW_SCALE = 0.5f; // 256->128 visual match
	bool isClimbing_ = false;
	float climbTargetX_ = 0.0f;  // World X to teleport after climb
	float climbTargetY_ = 0.0f;  // World Y to teleport after climb (top of ledge)

	// Ledge detection via raycasts (auto-detect, no Tiled objects needed)
	void CheckLedge();
	static constexpr int LEDGE_RAY_REACH   = 30;  // horizontal raycast distance (px)
	static constexpr int LEDGE_HEAD_OFFSET = 40;   // how far above body center to cast the "head" ray
	static constexpr int LEDGE_BODY_OFFSET = 10;   // how far above body center to cast the "body" ray

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

	// Push rock state
	bool  isPushing_ = false;
	int   pushContactCount_ = 0;   // track overlapping push_rock contacts
	float pushDir_ = 0.0f;         // -1 = left, +1 = right
	PhysBody* pushedRockBody_ = nullptr;
	SDL_Texture* pushTexture_ = nullptr;
	Animation    pushAnim_;
	static constexpr float PUSH_SPEED_FACTOR = 0.35f; // 35% of normal speed
	};