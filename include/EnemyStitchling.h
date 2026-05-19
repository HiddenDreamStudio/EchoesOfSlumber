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
		SETUP,         // Setting up the trap (releasing string)
		IDLE,          // Waiting for player to step on trap
		TRAP_ACTIVE,   // Player trapped, slowing them down and hurting them
		REWIND_SLOW,   // Rewinding slowly (vulnerable)
		REWIND_FAST,   // Rewinding fast (invulnerable/deflecting)
		HIT,           // Taking hit
		DEATH          // Dead
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

private:
	State currentState_ = State::SETUP;
	float stateTimer_ = 0.0f;
	
	// Trap mechanic
	bool isPlayerInTrap_ = false;
	int  escapeMashes_ = 0;
	static constexpr int MASHES_TO_ESCAPE = 5;
	static constexpr float TRAP_DAMAGE_INTERVAL = 2000.0f; // ms
	float trapDamageTimer_ = 0.0f;

	Player* playerRef_ = nullptr;
	bool    playerWasSlowed_ = false;

	// Box2D bodies
	PhysBody* pbody = nullptr;
	PhysBody* trapSensor_ = nullptr;

	// Animations
	AnimationSet idleAnims_;
	AnimationSet alertAnims_;
	AnimationSet setupAnims_;
	AnimationSet grabAnims_;
	AnimationSet rewindAnims_;
	AnimationSet hitAnims_;
	AnimationSet dieAnims_;

	// Textures
	SDL_Texture* idleTexture_ = nullptr;
	SDL_Texture* alertTexture_ = nullptr;
	SDL_Texture* setupTexture_ = nullptr;
	SDL_Texture* grabTexture_ = nullptr;
	SDL_Texture* rewindTexture_ = nullptr;
	SDL_Texture* hitTexture_ = nullptr;
	SDL_Texture* dieTexture_ = nullptr;

	bool facingRight_ = false;
};
