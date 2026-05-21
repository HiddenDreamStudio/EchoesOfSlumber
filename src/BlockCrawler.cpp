#include "BlockCrawler.h"

#include "Engine.h"
#include "Log.h"
#include "Physics.h"
#include "Player.h"
#include "Render.h"
#include "Scene.h"
#include "Textures.h"
#include "tracy/Tracy.hpp"

#include <algorithm>
#include <cmath>
#include <climits>

namespace
{
constexpr const char* BLOCK_CRAWLER_MOVE_LEFT =
	"assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Block_Crawler_der_a_izq.png";
constexpr const char* BLOCK_CRAWLER_MOVE_RIGHT =
	"assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Block_Crawler_izq_a_der.png";
constexpr const char* BLOCK_CRAWLER_WALL =
	"assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Block_Crawler_Muro.png";
constexpr const char* BLOCK_CRAWLER_DEATH =
	"assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Block_Crawler_Dead.png";
}

BlockCrawler::BlockCrawler()
{
	type = EntityType::BLOCK_CRAWLER;
	name = "BlockCrawler";
	speed = 2.0f;
	health = 3;
	maxHealth = 3;
	texW = bodyW_;
	texH = bodyH_;
}

BlockCrawler::~BlockCrawler() {}

bool BlockCrawler::Start()
{
	auto& textures = Engine::GetInstance().textures;

	moveLeftTexture_ = textures->Load(BLOCK_CRAWLER_MOVE_LEFT);
	moveRightTexture_ = textures->Load(BLOCK_CRAWLER_MOVE_RIGHT);
	wallTexture_ = textures->Load(BLOCK_CRAWLER_WALL);
	deathTexture_ = textures->Load(BLOCK_CRAWLER_DEATH);

	BuildStripAnimation(moveLeftTexture_, moveLeftAnim_, true, MOVE_FRAME_MS);
	BuildStripAnimation(moveRightTexture_, moveRightAnim_, true, MOVE_FRAME_MS);
	BuildStripAnimation(wallTexture_, wallAnim_, false, WALL_FRAME_MS);
	BuildStripAnimation(deathTexture_, deathAnim_, false, DEATH_FRAME_MS);

	CreateNormalBody((int)position.getX() + bodyW_ / 2, (int)position.getY() + bodyH_);

	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);
	if (patrolLeftX_ == 0.0f && patrolRightX_ == 0.0f)
	{
		patrolLeftX_ = (float)bodyX - 200.0f;
		patrolRightX_ = (float)bodyX + 200.0f;
	}

	state_ = BlockCrawlerState::PATROL;
	return true;
}

bool BlockCrawler::Update(float dt)
{
	ZoneScoped;

	if (Engine::GetInstance().scene->isPaused_ || Engine::GetInstance().scene->isGameOver_)
	{
		Draw(0.0f);
		return true;
	}

	GetPhysicsValues();
	UpdateFSM(dt);

	if (state_ != BlockCrawlerState::WALL && state_ != BlockCrawlerState::DYING)
	{
		ApplyPhysics();
	}

	if (state_ == BlockCrawlerState::PATROL)
	{
		if (contactDamageCooldown_ > 0.0f) contactDamageCooldown_ -= dt;
		if (isContactWithPlayer_ && playerListener_ != nullptr && contactDamageCooldown_ <= 0.0f)
		{
			playerListener_->TakeDamage(1);
			contactDamageCooldown_ = CONTACT_DAMAGE_INTERVAL;
		}
	}

	Draw(dt);

	if (state_ == BlockCrawlerState::DYING && deathAnim_.HasFinishedOnce())
	{
		Destroy();
	}

	return true;
}

bool BlockCrawler::CleanUp()
{
	LOG("Cleanup BlockCrawler");
	Engine::GetInstance().textures->UnLoad(moveLeftTexture_);
	Engine::GetInstance().textures->UnLoad(moveRightTexture_);
	Engine::GetInstance().textures->UnLoad(wallTexture_);
	Engine::GetInstance().textures->UnLoad(deathTexture_);

	if (pbody != nullptr)
	{
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}

	return true;
}

void BlockCrawler::SetPatrolPoints(float leftX, float rightX)
{
	patrolLeftX_ = leftX;
	patrolRightX_ = rightX;
}

void BlockCrawler::SetSpawnSize(float width, float height)
{
	if (width > 0.0f) bodyW_ = (int)width;
	if (height > 0.0f) bodyH_ = (int)height;
	texW = bodyW_;
	texH = bodyH_;
	currentBodyW_ = bodyW_;
	currentBodyH_ = bodyH_;
}

void BlockCrawler::UpdateFSM(float dt)
{
	if (state_ == BlockCrawlerState::DYING) return;

	if (wallCooldownTimer_ > 0.0f)
	{
		wallCooldownTimer_ -= dt;
		if (wallCooldownTimer_ < 0.0f) wallCooldownTimer_ = 0.0f;
	}

	auto& player = Engine::GetInstance().scene->player;
	if (player != nullptr && player->IsDead())
	{
		velocity.x = 0.0f;
		return;
	}

	if (state_ == BlockCrawlerState::WALL)
	{
		velocity.x = 0.0f;
		velocity.y = 0.0f;
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f)
		{
			ExitWallState();
		}
		return;
	}

	if (wallCooldownTimer_ <= 0.0f && CanDetectPlayer())
	{
		EnterWallState();
		return;
	}

	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	if (patrolDirX_ > 0.0f && (float)bodyX >= patrolRightX_)
	{
		patrolDirX_ = -1.0f;
	}
	else if (patrolDirX_ < 0.0f && (float)bodyX <= patrolLeftX_)
	{
		patrolDirX_ = 1.0f;
	}

	velocity.x = patrolDirX_ * speed;
}

void BlockCrawler::EnterWallState()
{
	state_ = BlockCrawlerState::WALL;
	stateTimer_ = WALL_DURATION;
	velocity.x = 0.0f;
	velocity.y = 0.0f;
	isContactWithPlayer_ = false;
	playerListener_ = nullptr;
	contactDamageCooldown_ = CONTACT_DAMAGE_INTERVAL;
	wallAnim_.Reset();

	ReplaceBodyForWall();
	LOG("BlockCrawler: WALL");
}

void BlockCrawler::ExitWallState()
{
	state_ = BlockCrawlerState::PATROL;
	wallCooldownTimer_ = WALL_COOLDOWN;

	ReplaceBodyForPatrol();
	LOG("BlockCrawler: PATROL cooldown");
}

void BlockCrawler::EnterDeathState()
{
	state_ = BlockCrawlerState::DYING;
	velocity.x = 0.0f;
	velocity.y = 0.0f;
	isContactWithPlayer_ = false;
	playerListener_ = nullptr;
	deathAnim_.Reset();

	Engine::GetInstance().physics->SetLinearVelocity(pbody, 0.0f, 0.0f);
	b2Body_SetType(pbody->body, b2_staticBody);
	LOG("BlockCrawler: DEATH");
}

void BlockCrawler::CreateNormalBody(int centerX, int bottomY)
{
	currentBodyW_ = bodyW_;
	currentBodyH_ = bodyH_;
	pbody = Engine::GetInstance().physics->CreateRectangle(
		centerX,
		bottomY - currentBodyH_ / 2,
		currentBodyW_,
		currentBodyH_,
		bodyType::DYNAMIC,
		0.9f);
	ConfigureBody();
}

void BlockCrawler::CreateWallBody(int centerX, int bottomY)
{
	currentBodyW_ = WALL_BODY_W;
	currentBodyH_ = WALL_BODY_H;
	pbody = Engine::GetInstance().physics->CreateCapsule(
		centerX,
		bottomY - currentBodyH_ / 2,
		currentBodyW_,
		currentBodyH_,
		bodyType::STATIC,
		0.9f);
	ConfigureBody();
}

void BlockCrawler::ConfigureBody()
{
	if (pbody == nullptr) return;

	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY;
	b2Body_SetFixedRotation(pbody->body, true);
	Engine::GetInstance().physics->SetLinearVelocity(pbody, 0.0f, 0.0f);
}

void BlockCrawler::ReplaceBodyForWall()
{
	if (pbody == nullptr) return;

	int centerX, centerY;
	pbody->GetPosition(centerX, centerY);
	const int bottomY = centerY + currentBodyH_ / 2;

	Engine::GetInstance().physics->DeletePhysBody(pbody);
	pbody = nullptr;
	CreateWallBody(centerX, bottomY);
}

void BlockCrawler::ReplaceBodyForPatrol()
{
	if (pbody == nullptr) return;

	int centerX, centerY;
	pbody->GetPosition(centerX, centerY);
	const int bottomY = centerY + currentBodyH_ / 2;

	Engine::GetInstance().physics->DeletePhysBody(pbody);
	pbody = nullptr;
	CreateNormalBody(centerX, bottomY);
}

bool BlockCrawler::CanDetectPlayer() const
{
	auto& player = Engine::GetInstance().scene->player;
	if (player == nullptr || player->pbody == nullptr || player->IsDead() || player->IsHiding())
	{
		return false;
	}

	int bodyX, bodyY;
	int playerX, playerY;
	pbody->GetPosition(bodyX, bodyY);
	player->pbody->GetPosition(playerX, playerY);

	const float dx = (float)playerX - (float)bodyX;
	const float dy = (float)playerY - (float)bodyY;

	return std::fabs(dx) <= DETECTION_RANGE_X && std::fabs(dy) <= DETECTION_RANGE_Y;
}

void BlockCrawler::BuildStripAnimation(SDL_Texture* sourceTexture, Animation& animation, bool loop, int frameDurationMs) const
{
	if (sourceTexture == nullptr) return;

	int textureW = 0;
	int textureH = 0;
	Engine::GetInstance().textures->GetSize(sourceTexture, textureW, textureH);
	if (textureW <= 0 || textureH <= 0) return;

	const int frameW = textureH;
	const int frameCount = std::max(1, (int)std::round((float)textureW / (float)frameW));

	animation.SetLoop(loop);
	for (int i = 0; i < frameCount; ++i)
	{
		const int frameX = i * frameW;
		if (frameX >= textureW) break;

		SDL_Rect frame{ frameX, 0, std::min(frameW, textureW - frameX), textureH };
		animation.AddFrame(frame, frameDurationMs);
	}
	animation.Reset();
}

void BlockCrawler::Draw(float dt)
{
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	SDL_Texture* textureToDraw = nullptr;
	Animation* animationToDraw = nullptr;
	bool isPatrolAnimation = false;

	switch (state_)
	{
	case BlockCrawlerState::WALL:
		textureToDraw = wallTexture_;
		animationToDraw = &wallAnim_;
		break;

	case BlockCrawlerState::DYING:
		textureToDraw = deathTexture_;
		animationToDraw = &deathAnim_;
		break;

	case BlockCrawlerState::PATROL:
	default:
		isPatrolAnimation = true;
		if (patrolDirX_ < 0.0f)
		{
			textureToDraw = moveLeftTexture_;
			animationToDraw = &moveLeftAnim_;
		}
		else
		{
			textureToDraw = moveRightTexture_;
			animationToDraw = &moveRightAnim_;
		}
		break;
	}

	if (textureToDraw == nullptr || animationToDraw == nullptr) return;

	animationToDraw->Update(dt);
	const SDL_Rect& frame = animationToDraw->GetCurrentFrame();
	float drawScale = 1.0f;
	if (isPatrolAnimation)
	{
		const SDL_Rect& wallFrame = wallAnim_.GetCurrentFrame();
		if (frame.h > 0 && wallFrame.h > 0)
		{
			drawScale = (float)wallFrame.h / (float)frame.h;
		}
	}

	const int drawW = (int)((float)frame.w * drawScale);
	const int drawH = (int)((float)frame.h * drawScale);
	const int drawX = x - drawW / 2;
	const int drawY = y + currentBodyH_ / 2 - drawH;

	Engine::GetInstance().render->DrawTexture(
		textureToDraw,
		drawX,
		drawY,
		&frame,
		1.0f,
		0,
		INT_MAX,
		INT_MAX,
		SDL_FLIP_NONE,
		drawScale);
}

void BlockCrawler::OnCollision(PhysBody* physA, PhysBody* physB)
{
	if (state_ == BlockCrawlerState::WALL || state_ == BlockCrawlerState::DYING)
	{
		if (physB->ctype == ColliderType::SLINGSHOT_PROJ && physB->listener != nullptr)
		{
			physB->listener->Destroy();
		}
		return;
	}

	if (physB->ctype == ColliderType::ATTACK)
	{
		TakeDamage(1);
	}
	else if (physB->ctype == ColliderType::SLINGSHOT_PROJ)
	{
		TakeDamage(1);
		if (physB->listener != nullptr)
		{
			physB->listener->Destroy();
		}
	}
	else if (physB->ctype == ColliderType::PLAYER)
	{
		isContactWithPlayer_ = true;
		playerListener_ = physB->listener;
		if (playerListener_ != nullptr && contactDamageCooldown_ <= 0.0f)
		{
			playerListener_->TakeDamage(1);
			contactDamageCooldown_ = CONTACT_DAMAGE_INTERVAL;
		}
	}
}

void BlockCrawler::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	if (physB->ctype == ColliderType::PLAYER)
	{
		isContactWithPlayer_ = false;
		playerListener_ = nullptr;
	}
}

void BlockCrawler::TakeDamage(int damage)
{
	if (state_ == BlockCrawlerState::WALL || state_ == BlockCrawlerState::DYING)
	{
		return;
	}

	health -= damage;
	LOG("BlockCrawler took %d damage -> health: %d/%d", damage, health, maxHealth);

	if (health <= 0)
	{
		health = 0;
		EnterDeathState();
	}
}
