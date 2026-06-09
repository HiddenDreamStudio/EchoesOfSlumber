#pragma once

#include "Entity.h"
#include "Animation.h"
#include "Engine.h"
#include "Physics.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>

class Player;

class BlockCrawler : public Entity
{
public:
	enum class State {
		MOVE,          // Moving side to side, invulnerable
		WALL,          // Stopped, tall wall, vulnerable
		HIT,           // Taking damage in wall state
		DEATH          // Dead
	};

	BlockCrawler();
	virtual ~BlockCrawler();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OnCollision(PhysBody* physA, PhysBody* physB) override;
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB) override;

	void TakeDamage(int damage) override;

	void SetPatrolPoints(float left, float right) {
		patrolLeft_ = left;
		patrolRight_ = right;
	}

public:
	int texW = 256, texH = 256;

private:
	void UpdateFSM(float dt);
	void EnterState(State newState);
	void Draw();
	void UpdatePhysicsForCurrentFrame();
	void PlayBlockCrawlerFx(int fxId);

private:
	int moveFxId_ = -1;
	int apilarFxId_ = -1;
	int desapilarFxId_ = -1;
	int hitFxId_ = -1;
	int dieFxId_ = -1;
	int noDamageFxId_ = -1;
	
	float moveFxTimer_ = 0.0f;
	float moveFxInterval_ = 320.0f; // move.wav length

private:
	State currentState_ = State::MOVE;
	float stateTimer_ = 0.0f;
	
	float patrolLeft_ = 0.0f;
	float patrolRight_ = 0.0f;
	float speed_ = 1.0f;
	bool movingRight_ = true;

	static constexpr float MOVE_DURATION = 5000.0f; // ms
	static constexpr float WALL_DURATION = 3000.0f; // ms

	// Contact damage
	static constexpr float CONTACT_DAMAGE_INTERVAL = 3000.0f;
	bool isContactWithPlayer_ = false;
	float contactDamageCooldown_ = 0.0f;
	Entity* playerListener_ = nullptr;

	// Box2D bodies
	PhysBody* pbody = nullptr;
	int currentFrameIndex_ = -1;
	int currentPhysHalfH_ = 40;

	// Animations
	AnimationSet moveAnims_;
	AnimationSet wallAnims_;
	AnimationSet hitAnims_;
	AnimationSet dieAnims_;

	// Textures
	SDL_Texture* moveTexture_ = nullptr;
	SDL_Texture* wallTexture_ = nullptr;
	SDL_Texture* hitTexture_ = nullptr;
	SDL_Texture* dieTexture_ = nullptr;

	bool facingRight_ = false;
};
