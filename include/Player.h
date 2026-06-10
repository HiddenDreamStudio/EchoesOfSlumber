#pragma once

#include "Entity.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>

struct SDL_Texture;

class Player : public Entity
{
public:
	enum class EquippedItem { NONE, BLANKET, SLINGSHOT, STUFFED_ANIMAL };

	// Equipped Item state
	EquippedItem GetEquippedItem() const { return equippedItem_; }
	void SetEquippedItem(EquippedItem item) { equippedItem_ = item; }

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
	void TakeDamage(int damage, bool applyKnockback);
	void Revive();
	void StartWakeUp(float speedMultiplier = 1.0f);
	bool IsDead() const { return isDead_; }

	// Map viewer helpers
	SDL_Rect GetCurrentAnimationRect() const;
	bool     IsFacingRight() const;

	// Hiding state queried by enemies to disable detection
	bool IsHiding() const { return isHiding_ || isHidingBehindRock_; }

	// Blanket (cape) collectible – must be collected before hiding is available
	bool HasBlanket() const { return hasBlanket_; }
	void SetHasBlanket(bool v) { hasBlanket_ = v; }

	// Push rock state — queried externally if needed
	bool IsPushing() const { return isPushing_; }

	// Slingshot (tirachinas) collectible — must be collected before firing is available
	bool HasSlingshot() const { return hasSlingshot_; }
	void SetHasSlingshot(bool v) { hasSlingshot_ = v; }
	bool IsAiming() const { return isAiming_; }

	// Stuffed Animal / Plush collectible
	bool HasStuffedAnimal() const { return hasStuffedAnimal_; }
	void SetHasStuffedAnimal(bool v) { hasStuffedAnimal_ = v; }
	bool IsBearMode() const { return isBearMode_; }

private:

	void GetPhysicsValues();
	void Move();
	void Jump();
	void Attack(float dt);
	void Hide(float dt);
	void Slingshot(float dt);
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
	int landFxId = -1;
	int stepsFxId = -1;
	int gameOverFxId = -1;
	int slingshotAimFxId = -1;
	int slingshotShootFxId = -1;
	int slingshotThudFxId = -1;
	int capeFxId = -1;

	// Bear Audio
	int bearAppearFxId = -1;
	int bearDeathFxId = -1;
	int bearStepsFxId = -1;
	int bearDamageFxId = -1;
	int bearSlapFxId = -1;

	PhysBody* pbody = nullptr;
	PhysBody* platformBelow = nullptr;
	
	float platformDropTimer_ = 0.0f;
	float knockbackX_     = 0.0f;
	float knockbackTimer_ = 0.0f;
	float jumpForce = 10.0f;
	bool suppressDamageAnim_ = false; // set by traps that skip the damage animation
	bool isJumping = false;

	bool isWakingUp = true;
	bool wakeUpAnimStarted = false;
	
	int keys = 0;

	// God mode flag
	bool godMode_ = false;

private:
	b2Vec2 velocity = { 0.0f, 0.0f };
	AnimationSet anims;
	Animation wakeUpAnim;
	SDL_Texture* wakeUpTexture = nullptr;
	float wakeUpAnimSpeed_ = 1.0f;
	float drawScale = 0.5f;
	bool facingRight = false; // false = facing right (variable is named counter-intuitively; true = facing left)
	PhysBody* currentGround = nullptr;

	//  Hide cooldown 
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
	static constexpr float STEP_COOLDOWN = 550.0f;

	// Hide (press H to crouch behind rocks  enemies lose sight of player)
	bool  isHiding_ = false;
	bool  isExitingHide_ = false;
	bool  isHidingBehindRock_ = false;
	// Visual: gentle alpha pulse while hidden to signal stealth state
	float hideAlphaTime_ = 0.0f;
	SDL_Texture* hideTexture_ = nullptr;
	Animation    hideAnim_;      // player body crouching
	Animation    hideExitAnim_; // player body standing up
	Animation    hideCapeAnim_; // cape crouching
	Animation    hideExitCapeAnim_; // cape standing up

public:
	void SetHidingBehindRock(bool hiding);
private:

	// Blanket (cape) collectible flag
	bool hasBlanket_ = false;
	bool hasStuffedAnimal_ = false;
	EquippedItem equippedItem_ = EquippedItem::NONE;

	// Push rock state
	bool  isPushing_ = false;
	int   pushContactCount_ = 0;   // track overlapping push_rock contacts
	float pushDir_ = 0.0f;         // -1 = left, +1 = right
	PhysBody* pushedRockBody_ = nullptr;
	SDL_Texture* pushTexture_ = nullptr;
	Animation    pushAnim_;
	static constexpr float PUSH_SPEED_FACTOR = 0.35f; // 35% of normal speed

	// Slingshot (tirachinas) weapon
	bool  hasSlingshot_ = false;
	bool  isAiming_ = false;
	bool  isAimingWithGamepad_ = false;
	float chargeTimer_ = 0.0f;
	float aimAngle_ = 0.0f;
	float slingshotCooldown_ = 0.0f;
	static constexpr float MAX_CHARGE_TIME = 1500.0f;    // ms
	static constexpr float MIN_LAUNCH_SPEED = 3.0f;
	static constexpr float MAX_LAUNCH_SPEED = 12.0f;
	static constexpr float SLINGSHOT_COOLDOWN = 500.0f;   // ms
	SDL_Texture* slingshotShootTexture_ = nullptr;
	Animation    slingshotAnim_;       // charge-up (row 3: 12 frames, plays once)
	Animation    slingshotHoldAnim_;   // hold loop (row 0: 6 frames, loops)
	bool         slingshotCharged_ = false; // true once charge anim finishes

	// Bear mode states, textures, and animations
	bool         isBearMode_ = false;
	bool         isBearTransforming_ = false;
	bool         isThrowingBear_ = false;
	bool         isKidSleeping_ = false;
	float        bearSleepTimer_ = 0.0f;
	float        bearCooldownTimer_ = 0.0f;
	Vector2D     bearSummonPosition_;
	bool         bearSummonFacingRight_ = false;
	int          bearHealth_ = 3;
	SDL_Texture* bearAppearTexture_ = nullptr;
	SDL_Texture* bearIdleTexture_ = nullptr;
	SDL_Texture* bearWalkTexture_ = nullptr;
	SDL_Texture* bearAttackTexture_ = nullptr;
	SDL_Texture* bearDeathTexture_ = nullptr;
	SDL_Texture* throwBearTexture_ = nullptr;
	AnimationSet bearAppearAnims_;
	AnimationSet bearIdleAnims_;
	AnimationSet bearWalkAnims_;
	AnimationSet bearAttackAnims_;
	AnimationSet bearDeathAnims_;
	AnimationSet throwBearAnims_;

	// Yoyo trap animation (played when caught by Stitchling)
	SDL_Texture* yoyoTrapTexture_ = nullptr;

	// Drop Doll minigame animation (played while grabbed by the doll)
	SDL_Texture* dollGrabbedTexture_ = nullptr;
	Animation    dollGrabbedAnim_;

public:
	bool         isYoyoTrapped_ = false;
	AnimationSet yoyoTrapAnims_;
	bool         isDollGrabbed_ = false;
};
