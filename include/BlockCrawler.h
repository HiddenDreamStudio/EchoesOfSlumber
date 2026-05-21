#pragma once

#include "Enemy.h"
#include "Animation.h"

enum class BlockCrawlerState
{
	PATROL,
	WALL,
	DYING
};

class BlockCrawler : public Enemy
{
public:
	BlockCrawler();
	~BlockCrawler() override;

	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OnCollision(PhysBody* physA, PhysBody* physB) override;
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB) override;
	void TakeDamage(int damage) override;

	void SetPatrolPoints(float leftX, float rightX);
	void SetSpawnSize(float width, float height);

private:
	void UpdateFSM(float dt) override;
	void Draw(float dt) override;

	void EnterWallState();
	void ExitWallState();
	void EnterDeathState();
	void CreateNormalBody(int centerX, int bottomY);
	void CreateWallBody(int centerX, int bottomY);
	void ConfigureBody();
	void ReplaceBodyForWall();
	void ReplaceBodyForPatrol();
	bool CanDetectPlayer() const;
	void BuildStripAnimation(SDL_Texture* sourceTexture, Animation& animation, bool loop, int frameDurationMs) const;

	SDL_Texture* moveLeftTexture_ = nullptr;
	SDL_Texture* moveRightTexture_ = nullptr;
	SDL_Texture* wallTexture_ = nullptr;
	SDL_Texture* deathTexture_ = nullptr;

	Animation moveLeftAnim_;
	Animation moveRightAnim_;
	Animation wallAnim_;
	Animation deathAnim_;

	BlockCrawlerState state_ = BlockCrawlerState::PATROL;
	float stateTimer_ = 0.0f;
	float wallCooldownTimer_ = 0.0f;

	float patrolLeftX_ = 0.0f;
	float patrolRightX_ = 0.0f;
	float patrolDirX_ = 1.0f;

	int bodyW_ = 96;
	int bodyH_ = 96;
	int currentBodyW_ = 96;
	int currentBodyH_ = 96;

	static constexpr int WALL_BODY_W = 40;
	static constexpr int WALL_BODY_H = 100;
	static constexpr float DETECTION_RANGE_X = 300.0f;
	static constexpr float DETECTION_RANGE_Y = 140.0f;
	static constexpr float WALL_DURATION = 5000.0f;
	static constexpr float WALL_COOLDOWN = 5000.0f;
	static constexpr int MOVE_FRAME_MS = 90;
	static constexpr int WALL_FRAME_MS = 90;
	static constexpr int DEATH_FRAME_MS = 80;
};
