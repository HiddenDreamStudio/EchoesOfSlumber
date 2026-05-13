#include "EnemyBlockCrawler.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include <cstdlib>   // rand
#include <algorithm>  // std::min / std::max

// ──────────────────────────── helpers ────────────────────────────
static float Lerp(float a, float b, float t) { return a + (b - a) * t; }
static float RandF(float lo, float hi) {
	return lo + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (hi - lo)));
}

// ──────────────────────────── ctor / dtor ────────────────────────
EnemyBlockCrawler::EnemyBlockCrawler()
{
	name = "EnemyBlockCrawler";
	health    = 5;
	maxHealth = 5;
	speed     = PATROL_SPEED;
}

EnemyBlockCrawler::~EnemyBlockCrawler() {}

// ──────────────────────────── Start ─────────────────────────────
bool EnemyBlockCrawler::Start()
{
	// Load texture (single image – we sample a square from the centre)
	texture = Engine::GetInstance().textures->Load(
		"assets/textures/spritesheets/SS enemics C/test.jpg");

	// Source rect: take a 64×64 region from the centre of the image
	// (the image is an eye; each block will render this cropped square)
	blockSrcRect_ = { 96, 96, 64, 64 };

	// ── Create physics body (capsule, same pattern as EnemyCarmel) ──
	int halfW = (int)(BLOCK_SIZE) / 2;
	int halfH = (int)(BLOCK_SIZE * BLOCK_COUNT) / 2;
	pbody = Engine::GetInstance().physics->CreateRectangle(
		(int)position.getX() + halfW,
		(int)position.getY() + halfH,
		(int)BLOCK_SIZE + 4,
		(int)(BLOCK_SIZE * BLOCK_COUNT),
		bodyType::DYNAMIC, 0.0f);

	pbody->listener = this;
	pbody->ctype    = ColliderType::ENEMY;

	// ── Default patrol range ──
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);
	if (patrolLeftX_ == 0.0f && patrolRightX_ == 0.0f)
	{
		patrolLeftX_  = (float)bodyX - 200.0f;
		patrolRightX_ = (float)bodyX + 200.0f;
	}

	// ── Initialise blocks ──
	blocks_.resize(BLOCK_COUNT);
	for (int i = 0; i < BLOCK_COUNT; ++i)
	{
		// Stack from bottom (i=0) to top (i=N-1)
		float homeY = (float)(BLOCK_COUNT / 2 - i) * BLOCK_SIZE;
		blocks_[i].homeX    = 0.0f;
		blocks_[i].homeY    = homeY;
		blocks_[i].offsetX  = 0.0f;
		blocks_[i].offsetY  = homeY;
		blocks_[i].bobPhase = (float)i * 1.2f; // stagger phases
	}

	GenerateScatterPositions();
	TransitionTo(BlockCrawlerState::COMPACT);

	LOG("EnemyBlockCrawler started at %.0f, %.0f", position.getX(), position.getY());
	return true;
}

// ──────────────────────────── CleanUp ───────────────────────────
bool EnemyBlockCrawler::CleanUp()
{
	return Enemy::CleanUp();
}

// ──────────────────────────── Patrol points ─────────────────────
void EnemyBlockCrawler::SetPatrolPoints(float leftX, float rightX)
{
	patrolLeftX_  = leftX;
	patrolRightX_ = rightX;
}

// ──────────────────────────── Scatter positions ─────────────────
void EnemyBlockCrawler::GenerateScatterPositions()
{
	for (int i = 0; i < BLOCK_COUNT; ++i)
	{
		// Spread blocks evenly across the scatter range, with slight randomness
		float baseX = -SCATTER_RANGE_X + (2.0f * SCATTER_RANGE_X / (BLOCK_COUNT - 1)) * i;
		blocks_[i].scatterX = baseX + RandF(-8.0f, 8.0f);
		blocks_[i].scatterY = RandF(-SCATTER_RANGE_Y, SCATTER_RANGE_Y * 0.3f);
	}
}

// ──────────────────────────── FSM ───────────────────────────────
void EnemyBlockCrawler::UpdateFSM(float dt)
{
	if (state_ == BlockCrawlerState::DEATH) return;

	// Stop if player is dead
	auto& player = Engine::GetInstance().scene->player;
	if (player != nullptr && player->IsDead())
	{
		velocity.x = 0.0f;
		return;
	}

	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	stateTimer_ -= dt;

	switch (state_)
	{
	// ─────────── COMPACT: blocks stacked, patrol left-right ───────────
	case BlockCrawlerState::COMPACT:
	{
		// Patrol logic
		if (patrolDirX_ > 0.0f && (float)bodyX >= patrolRightX_)
			patrolDirX_ = -1.0f;
		else if (patrolDirX_ < 0.0f && (float)bodyX <= patrolLeftX_)
			patrolDirX_ = 1.0f;

		velocity.x   = patrolDirX_ * PATROL_SPEED;
		facingRight_ = (patrolDirX_ > 0.0f);

		// Bob each block with sinusoidal vertical offset
		for (auto& b : blocks_)
		{
			b.bobPhase += BOB_FREQUENCY * dt;
			b.offsetX = b.homeX;
			b.offsetY = b.homeY + std::sin(b.bobPhase) * BOB_AMPLITUDE;
		}

		// Timer → start dispersing
		if (stateTimer_ <= 0.0f)
			TransitionTo(BlockCrawlerState::DISPERSING);

		break;
	}

	// ─────────── DISPERSING: blocks lerp outward ─────────────────────
	case BlockCrawlerState::DISPERSING:
	{
		velocity.x = patrolDirX_ * DISPERSED_SPEED;

		float progress = 1.0f - (stateTimer_ / TRANSITION_DURATION);
		progress = std::min(std::max(progress, 0.0f), 1.0f);
		// Smooth ease-out
		float t = 1.0f - (1.0f - progress) * (1.0f - progress);

		for (auto& b : blocks_)
		{
			b.offsetX = Lerp(b.homeX, b.scatterX, t);
			b.offsetY = Lerp(b.homeY, b.scatterY, t);
		}

		if (stateTimer_ <= 0.0f)
			TransitionTo(BlockCrawlerState::DISPERSED);

		break;
	}

	// ─────────── DISPERSED: blocks scattered, jittering ──────────────
	case BlockCrawlerState::DISPERSED:
	{
		// Slow patrol
		if (patrolDirX_ > 0.0f && (float)bodyX >= patrolRightX_)
			patrolDirX_ = -1.0f;
		else if (patrolDirX_ < 0.0f && (float)bodyX <= patrolLeftX_)
			patrolDirX_ = 1.0f;

		velocity.x = patrolDirX_ * DISPERSED_SPEED;

		// Per-block jitter
		for (auto& b : blocks_)
		{
			// Randomise jitter periodically
			if (RandF(0.0f, 1.0f) < 0.02f)
			{
				b.jitterVx = RandF(-JITTER_STRENGTH, JITTER_STRENGTH);
				b.jitterVy = RandF(-JITTER_STRENGTH, JITTER_STRENGTH);
			}

			b.offsetX = b.scatterX + b.jitterVx * (dt * 0.1f);
			b.offsetY = b.scatterY + b.jitterVy * (dt * 0.1f);

			// Keep scatter positions updated for smooth jitter
			b.scatterX += b.jitterVx * (dt * 0.05f);
			b.scatterY += b.jitterVy * (dt * 0.05f);

			// Clamp so they don't drift too far
			b.scatterX = std::min(std::max(b.scatterX, -SCATTER_RANGE_X * 1.3f), SCATTER_RANGE_X * 1.3f);
			b.scatterY = std::min(std::max(b.scatterY, -SCATTER_RANGE_Y * 1.5f), SCATTER_RANGE_Y * 1.5f);
		}

		if (stateTimer_ <= 0.0f)
			TransitionTo(BlockCrawlerState::REASSEMBLING);

		break;
	}

	// ─────────── REASSEMBLING: blocks lerp back to stack ─────────────
	case BlockCrawlerState::REASSEMBLING:
	{
		velocity.x = patrolDirX_ * DISPERSED_SPEED;

		float progress = 1.0f - (stateTimer_ / TRANSITION_DURATION);
		progress = std::min(std::max(progress, 0.0f), 1.0f);
		// Smooth ease-in
		float t = progress * progress;

		for (auto& b : blocks_)
		{
			b.offsetX = Lerp(b.scatterX, b.homeX, t);
			b.offsetY = Lerp(b.scatterY, b.homeY, t);
		}

		if (stateTimer_ <= 0.0f)
		{
			GenerateScatterPositions(); // new scatter targets for next cycle
			TransitionTo(BlockCrawlerState::COMPACT);
		}

		break;
	}

	case BlockCrawlerState::DEATH:
		break;
	}

	// ── Manual block-vs-player collision (for dispersed blocks) ──
	CheckBlockPlayerCollision();
}

// ──────────────────────────── State transitions ─────────────────
void EnemyBlockCrawler::TransitionTo(BlockCrawlerState newState)
{
	state_ = newState;

	switch (newState)
	{
	case BlockCrawlerState::COMPACT:
		stateTimer_ = COMPACT_DURATION;
		speed       = PATROL_SPEED;
		// Snap blocks to home
		for (auto& b : blocks_) {
			b.offsetX = b.homeX;
			b.offsetY = b.homeY;
		}
		break;

	case BlockCrawlerState::DISPERSING:
		stateTimer_ = TRANSITION_DURATION;
		break;

	case BlockCrawlerState::DISPERSED:
		stateTimer_ = DISPERSE_DURATION;
		// Initialise jitter
		for (auto& b : blocks_) {
			b.jitterVx = RandF(-JITTER_STRENGTH, JITTER_STRENGTH);
			b.jitterVy = RandF(-JITTER_STRENGTH, JITTER_STRENGTH);
		}
		break;

	case BlockCrawlerState::REASSEMBLING:
		stateTimer_ = TRANSITION_DURATION;
		break;

	case BlockCrawlerState::DEATH:
		LOG("EnemyBlockCrawler: DEATH");
		Destroy();
		break;
	}
}

// ──────────────────── Manual block-player collision ──────────────
void EnemyBlockCrawler::CheckBlockPlayerCollision()
{
	// Only do manual checks when blocks are spread out
	if (state_ != BlockCrawlerState::DISPERSED &&
		state_ != BlockCrawlerState::DISPERSING &&
		state_ != BlockCrawlerState::REASSEMBLING)
		return;

	// Respect cooldown from base class
	if (contactDamageCooldown_ > 0.0f)
		return;

	auto& player = Engine::GetInstance().scene->player;
	if (player == nullptr || player->IsDead()) return;

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();

	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	for (const auto& b : blocks_)
	{
		float bx = (float)bodyX + b.offsetX;
		float by = (float)bodyY + b.offsetY;

		float dx = playerPos.getX() - bx;
		float dy = playerPos.getY() - by;
		float dist = std::sqrt(dx * dx + dy * dy);

		if (dist < BLOCK_DAMAGE_RADIUS)
		{
			player->TakeDamage(1);
			contactDamageCooldown_ = CONTACT_DAMAGE_INTERVAL;
			return; // one hit per frame max
		}
	}
}

// ──────────────────────────── TakeDamage ────────────────────────
void EnemyBlockCrawler::TakeDamage(int damage)
{
	if (state_ == BlockCrawlerState::DEATH) return;

	health -= damage;
	LOG("EnemyBlockCrawler took %d damage -> health: %d/%d", damage, health, maxHealth);

	// Knockback
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);
	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	float dirX = ((float)bodyX > playerPos.getX()) ? 1.0f : -1.0f;
	Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, dirX * 4.0f, -2.0f, true);

	if (health <= 0)
	{
		health = 0;
		TransitionTo(BlockCrawlerState::DEATH);
	}
}

// ──────────────────────────── Draw ──────────────────────────────
void EnemyBlockCrawler::Draw(float dt)
{
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	auto& render = Engine::GetInstance().render;

	// Draw each block
	int halfBlock = (int)(BLOCK_SIZE * BLOCK_DRAW_SCALE) / 2;

	for (const auto& b : blocks_)
	{
		int drawX = x + (int)b.offsetX - halfBlock;
		int drawY = y + (int)b.offsetY - halfBlock;

		render->DrawTexture(texture, drawX, drawY,
			&blockSrcRect_, 1.0f, 0, INT_MAX, INT_MAX,
			SDL_FLIP_NONE, BLOCK_DRAW_SCALE * (BLOCK_SIZE / 64.0f));
	}
}
