#pragma once

#include "Entity.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>

class Player;

class EnemyStitchling : public Entity
{
public:
	enum class State {
		IDLE,          // Yoyo sitting on the ground, waiting
		ALERT,         // Player is near, yoyo vibrates
		TRAP_ACTIVE,   // Player stepped on rope: yoyo disappears then throws ropes
		DEATH          // Dead / cleaned up
	};

	EnemyStitchling();
	virtual ~EnemyStitchling();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OnCollision(PhysBody* physA, PhysBody* physB) override;
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB) override;

	void TakeDamage(int damage) override;

public:
	int texW = 256, texH = 256;

private:
	void UpdateFSM(float dt);
	void EnterState(State newState);
	void Draw(float dt);
	void ApplyPlayerSlowdown();
	void RemovePlayerSlowdown();
	float GetDistanceToPlayer() const;

private:
	State currentState_ = State::IDLE;
	float stateTimer_ = 0.0f;
	
	// Trap mechanic
	bool disappearDone_ = false;  // true once the desaparecer animation has finished
	int  escapeMashes_ = 0;
	static constexpr int MASHES_TO_ESCAPE = 5;

	Player* playerRef_ = nullptr;
	bool    playerWasSlowed_ = false;

	// Box2D bodies
	PhysBody* pbody = nullptr;
	PhysBody* trapSensor_ = nullptr;

	// Animations (only 3 needed for the yoyo)
	AnimationSet idleAnims_;
	AnimationSet alertAnims_;
	AnimationSet dieAnims_;

	// Textures (only 3 needed for the yoyo)
	SDL_Texture* idleTexture_ = nullptr;
	SDL_Texture* alertTexture_ = nullptr;
	SDL_Texture* dieTexture_ = nullptr;

	bool facingRight_ = false;

	// Audio Fx IDs
	int attackFxId = -1;
	int disappearFxId = -1;
};
