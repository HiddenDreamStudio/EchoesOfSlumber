#pragma once

#include "Entity.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>

class EnemyPlush : public Entity
{
public:
	enum class State {
		PATROL,      // Normal state, moves slowly back and forth
		TENSE,       // Tenses up and prepares to jump
		JUMPING,     // Mid-air quick jump direct towards player
		DIZZY,       // Marejat / desorientat for a few seconds
		DEATH        // Death state with animation before deletion
	};

	enum class JumpPhase {
		START,
		LOOP,
		END
	};

	EnemyPlush();
	virtual ~EnemyPlush();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;
	bool Destroy() override;

	void OnCollision(PhysBody* physA, PhysBody* physB) override;
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB) override;

	void SetPosition(Vector2D pos);
	Vector2D GetPosition();
	void TakeDamage(int damage) override;
	void TakeDamage(int damage, bool applyKnockback);

public:
	float speed = 1.0f;
	PhysBody* pbody = nullptr;
	int texW = 64, texH = 64;

private:
	void UpdateFSM(float dt);
	void EnterState(State newState);
	void GetPhysicsValues();
	void ApplyPhysics();
	void Draw(float dt);

	float GetDistanceToPlayer() const;

private:
	State currentState_ = State::PATROL;
	b2Vec2 velocity = { 0.0f, 0.0f };

	// Animations
	AnimationSet idleAnims_;
	AnimationSet alertAnims_;
	AnimationSet jumpStartAnims_;
	AnimationSet jumpLoopAnims_;
	AnimationSet jumpEndAnims_;
	AnimationSet dizzyAnims_;
	AnimationSet hitAnims_;
	AnimationSet dieAnims_;

	// Textures
	SDL_Texture* idleTexture_ = nullptr;
	SDL_Texture* alertTexture_ = nullptr;
	SDL_Texture* jumpStartTexture_ = nullptr;
	SDL_Texture* jumpLoopTexture_ = nullptr;
	SDL_Texture* jumpEndTexture_ = nullptr;
	SDL_Texture* dizzyTexture_ = nullptr;
	SDL_Texture* hitTexture_ = nullptr;
	SDL_Texture* dieTexture_ = nullptr;

	// Hit details
	bool isHit_ = false;
	float hitTimer_ = 0.0f;

	// FSM parameters
	static constexpr float PATROL_SPEED = 1.0f;
	static constexpr float JUMP_SPEED_X_LONG = 8.0f;
	static constexpr float JUMP_SPEED_Y_LONG = -8.0f;
	static constexpr float JUMP_SPEED_X_SHORT = 4.0f;
	static constexpr float JUMP_SPEED_Y_SHORT = -5.0f;

	static constexpr float DETECT_RANGE = 250.0f;    // pixels
	static constexpr float TENSE_DURATION = 600.0f;   // ms
	static constexpr float DIZZY_DURATION = 2000.0f;  // ms
	static constexpr float ATTACK_INTERVAL = 1000.0f;

	float tenseTimer_ = 0.0f;
	float dizzyTimer_ = 0.0f;

	// Jump properties
	bool lastJumpWasShort_ = false;
	bool isJumping_ = false;
	bool jumpDirectionRight_ = true;
	float jumpAirTime_ = 0.0f;
	JumpPhase jumpPhase_ = JumpPhase::START;

	// Patrol variables
	float patrolOriginX_ = 0.0f;
	float patrolDirection_ = 1.0f;
	static constexpr float PATROL_RANGE = 120.0f;

	// Contact damage
	bool isContactWithPlayer_ = false;
	float attackCooldown_ = 0.0f;
	Entity* playerListener_ = nullptr;

	// Facing
	bool facingRight_ = true;

	// Return-to-origin when player hides
	Vector2D originPosition_;
	bool wasPlayerHiding_ = false;

	// Aesthetic color variables
	float tenseVisualOffset_ = 0.0f;
	float dizzyVisualAngle_ = 0.0f;

	// Audio Fx IDs
	int damageFxId = -1;
	int jumpFxId = -1;
};
