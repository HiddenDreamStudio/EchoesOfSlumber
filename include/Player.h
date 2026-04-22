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

private:

	void GetPhysicsValues();
	void Move();
	void Jump();
	void Dash(float dt);
	void Attack(float dt);
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

	PhysBody* pbody = nullptr;
	float jumpForce = 10.0f; // The force to apply when jumping
	float doubleJumpForce = 11.0f; // The force to apply when double jumping
	bool isJumping = false; // Flag to check if the player is currently jumping
	bool canDoubleJump = false; // Flag to allow a second jump in the air
	bool hasDoubleJumped = false; // Flag to track if double jump was already used

private:
	b2Vec2 velocity = { 0.0f, 0.0f };
	AnimationSet anims;
	Animation wakeUpAnim;
	SDL_Texture* wakeUpTexture = nullptr;
	// Walkthrough/Cinematic state (from main)
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

	// Combat - visual feedback
	static constexpr float DAMAGE_FLASH_DURATION = 150.0f; // ms
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
	static constexpr int LEDGE_RAY_MARGIN  = 10;   // Margin for ledge detection raycasts

	// Dash
	static constexpr float DASH_SPEED    = 15.0f;
	static constexpr float DASH_DURATION = 200.0f; // ms
	static constexpr float DASH_COOLDOWN = 800.0f; // ms after dash ends
	bool  isDashing_    = false;
	float dashTimer_    = 0.0f;
	float dashCooldown_ = 0.0f;
	float dashDirX_     = 1.0f;
};
