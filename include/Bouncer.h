#pragma once

#include "Enemy.h"
#include "Animation.h"
#include <array>
#include <cstdint>

enum class BouncerState { IDLE, JUMP_START, AIRBORNE, LANDING, TIRED, HIT, DEATH };
enum class BouncerAnim { IDLE, JUMP_START, JUMP_LOOP, JUMP_END, TIRED, HIT, DIE, COUNT };
enum class ColliderType;

class Bouncer : public Enemy
{
public:
	Bouncer();
	~Bouncer() override;

	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OnCollision(PhysBody* physA, PhysBody* physB) override;
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB) override;
	void TakeDamage(int damage) override;

	void SetSpawnSize(float width, float height);

private:
	struct Clip
	{
		SDL_Texture* texture = nullptr;
		Animation anim;
		int sheetW = 0;
		int sheetH = 0;
		int frameCount = 0;
	};

	void UpdateFSM(float dt) override;
	void Draw(float dt) override;

	bool LoadClip(BouncerAnim anim, const char* path, int frames, int frameDurationMs, bool loop);
	void SetClip(BouncerAnim anim, bool reset = true);
	void TransitionTo(BouncerState newState);
	void Launch(bool highJump);
	void HandleSurfaceContacts();
	void HandleMapBounds();
	void ApplyPlayerTether(float dt);
	bool TryGetPlayerDeltaX(float& outDeltaX) const;
	float NextVariation(float amount);
	void ClampHorizontalSpeed();
	bool IsBlockingSurface(ColliderType type) const;

	static constexpr int   DEFAULT_DIAMETER = 96;
	static constexpr float PHYSICS_RADIUS_FACTOR = 0.5f;
	static constexpr float BASE_HORIZONTAL_SPEED = 3.1f;
	static constexpr float MIN_HORIZONTAL_SPEED = 2.3f;
	static constexpr float MAX_HORIZONTAL_SPEED = 4.2f;
	static constexpr float PLAYER_FOLLOW_DISTANCE = 360.0f;
	static constexpr float PLAYER_HARD_LIMIT_DISTANCE = 620.0f;
	static constexpr float NORMAL_JUMP_SPEED = 7.2f;
	static constexpr float HIGH_JUMP_SPEED = 10.0f;
	static constexpr float MAX_FALL_SPEED = 11.0f;
	static constexpr float LANDING_DURATION = 150.0f;
	static constexpr float TIRED_DURATION = 900.0f;
	static constexpr float TIRED_AFTER_MOVEMENT = 10000.0f;
	static constexpr float INITIAL_IDLE_DURATION = 220.0f;
	static constexpr float HIT_DURATION = 420.0f;
	static constexpr float VISUAL_GROUND_OVERLAP = 5.0f;
	static constexpr int   BOUNCES_BEFORE_HIGH_JUMP = 5;

	std::array<Clip, static_cast<size_t>(BouncerAnim::COUNT)> clips_;
	BouncerState state_ = BouncerState::IDLE;
	BouncerAnim currentClip_ = BouncerAnim::IDLE;
	float stateTimer_ = 0.0f;
	float spawnWidth_ = (float)DEFAULT_DIAMETER;
	float spawnHeight_ = (float)DEFAULT_DIAMETER;
	float drawDiameter_ = (float)DEFAULT_DIAMETER;
	float physicsRadius_ = (float)DEFAULT_DIAMETER * PHYSICS_RADIUS_FACTOR;
	float moveDirX_ = 1.0f;
	float activeMovementTimer_ = 0.0f;
	int bounceCount_ = 0;
	bool highJumpQueued_ = false;
	bool tiredQueued_ = false;
	uint32_t variationSeed_ = 0xA53C9E2Du;
};
