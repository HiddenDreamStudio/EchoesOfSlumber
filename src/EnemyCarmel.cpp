#include "EnemyCarmel.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "Audio.h"
#include "CarmelHulls.h"

EnemyCarmel::EnemyCarmel()
{
	name = "EnemyCarmel";
}

EnemyCarmel::~EnemyCarmel() {}

bool EnemyCarmel::Start()
{
	std::unordered_map<int, std::string> idleAliases = { {0, "idle"} };
	anims_.LoadFromTSX("assets/textures/animations/carmelAnimation.xml", idleAliases);
	anims_.SetCurrent("idle");
	texture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Carmel_idle.png");

	std::unordered_map<int, std::string> rollAliases = { {0, "roll"} };
	rollAnims_.LoadFromTSX("assets/textures/animations/carmelRollAnimation.xml", rollAliases);
	rollAnims_.SetCurrent("roll");
	rollTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Carmel_Roll.png");

	std::unordered_map<int, std::string> scaredAliases = { {0, "scared"} };
	scaredAnims_.LoadFromTSX("assets/textures/animations/carmelScaredAnimation.xml", scaredAliases);
	scaredAnims_.SetCurrent("scared");
	scaredAnims_.SetLoop("scared", false);
	scaredTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Carmel_scared.png");

	std::unordered_map<int, std::string> blowupAliases = { {0, "blowup"} };
	blowupAnims_.LoadFromTSX("assets/textures/animations/carmelBlowupAnimation.xml", blowupAliases);
	blowupAnims_.SetCurrent("blowup");
	blowupAnims_.SetLoop("blowup", false);
	blowupTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Carmel_Blowup.png");

	idleFxId_ = Engine::GetInstance().audio->LoadFx("assets/audio/fx/EnemiesLVL1/SpiderCandy/Spider candy idle.wav");
	moveFxId_ = Engine::GetInstance().audio->LoadFx("assets/audio/fx/EnemiesLVL1/SpiderCandy/Move.wav");
	alertFxId_ = Engine::GetInstance().audio->LoadFx("assets/audio/fx/EnemiesLVL1/SpiderCandy/Alert.wav");
	deathFxId_ = Engine::GetInstance().audio->LoadFx("assets/audio/fx/EnemiesLVL1/SpiderCandy/Spider candy_death.wav");
	hitFxId_ = Engine::GetInstance().audio->LoadFx("assets/audio/fx/EnemiesLVL1/SpiderCandy/Spider candy_daño.wav");
	attackFxId_ = Engine::GetInstance().audio->LoadFx("assets/audio/fx/EnemiesLVL1/SpiderCandy/Spider candy_atack.wav");

	idleFxTimer_ = 2000.0f + (rand() % 3000);
	int bx = (int)position.getX() + 32;
	int by = (int)position.getY() + 32;
	
	// Create the sensor body once. It will follow the main body and have its shape updated dynamically.
	pbodySensor_ = Engine::GetInstance().physics->CreateConvexPolygon(bx, by, carmel_hull_idle[0].points, carmel_hull_idle[0].numPoints * 2, bodyType::KINEMATIC);
	pbodySensor_->listener = this;
	pbodySensor_->ctype = ColliderType::ENEMY;
	Engine::GetInstance().physics->UpdateConvexPolygon(pbodySensor_, carmel_hull_idle[0].points, carmel_hull_idle[0].numPoints * 2, 1.0f, 0.6f, true);

	UpdatePhysicsBody(false); // Start small
	TransitionTo(EnemyCarmelState::IDLE);
	return true;
}

bool EnemyCarmel::CleanUp()
{
	if (pbodySensor_ != nullptr) {
		Engine::GetInstance().physics->DeletePhysBody(pbodySensor_);
		pbodySensor_ = nullptr;
	}
	Engine::GetInstance().textures->UnLoad(rollTexture_);
	Engine::GetInstance().textures->UnLoad(scaredTexture_);
	Engine::GetInstance().textures->UnLoad(blowupTexture_);
	return Enemy::CleanUp();
}

void EnemyCarmel::SetPatrolPoints(float leftX, float rightX)
{
	// Unused for Spider Candy since it stays still until activated
}

void EnemyCarmel::UpdatePhysicsBody(bool big)
{
	int bx = (int)position.getX();
	int by = (int)position.getY();

	if (pbody != nullptr) {
		pbody->GetPosition(bx, by);
		Engine::GetInstance().physics->DeletePhysBody(pbody);
	} else {
		// First time creation, adjust to center of 64x64 idle sprite
		bx += 32;
		by += 32;
	}
	
	if (big) {
		pbody = Engine::GetInstance().physics->CreateCapsule(bx, by, 50, 100, bodyType::DYNAMIC);
	} else {
		pbody = Engine::GetInstance().physics->CreateCapsule(bx, by, 20, 50, bodyType::DYNAMIC);
	}
	
	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY;
}

void EnemyCarmel::UpdateFSM(float dt)
{
	if (state_ == EnemyCarmelState::DEATH) return;

	auto& player = Engine::GetInstance().scene->player;
	if (player != nullptr && player->IsDead())
	{
		velocity.x = 0.0f;
		return;
	}

	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	float dx           = playerPos.getX() - (float)bodyX;
	float distToPlayer = std::abs(dx);
	float totalDist = playerPos.distanceEuclidean(Vector2D((float)bodyX, (float)bodyY));

	switch (state_)
	{
	case EnemyCarmelState::IDLE:
		velocity.x = 0.0f;
		idleFxTimer_ -= dt;
		if (idleFxTimer_ <= 0.0f) {
			PlaySpiderFx(idleFxId_);
			idleFxTimer_ = 2000.0f + (rand() % 3000);
		}
		if (totalDist < DETECTION_RADIUS) {
			TransitionTo(EnemyCarmelState::SCARED);
		}
		break;

	case EnemyCarmelState::SCARED:
		velocity.x = 0.0f;
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f) {
			TransitionTo(EnemyCarmelState::BLOWUP);
		}
		break;

	case EnemyCarmelState::BLOWUP:
		velocity.x = 0.0f;
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f) {
			TransitionTo(EnemyCarmelState::CHASE);
		}
		break;

	case EnemyCarmelState::CHASE:
		if (totalDist > LOSE_RADIUS)
		{
			stateTimer_ -= dt;
			if (stateTimer_ <= 0.0f) {
				TransitionTo(EnemyCarmelState::DEFLATE);
			}
		}
		else {
			stateTimer_ = 2000.0f; // Reset lose timer as long as player is in range
		}
		
		if (state_ == EnemyCarmelState::CHASE) {
			float desiredDir = (dx > 0.0f) ? 1.0f : -1.0f;
			
			if (desiredDir != currentDirX_) {
				turnDelayTimer_ -= dt;
				if (turnDelayTimer_ <= 0.0f) {
					currentDirX_ = desiredDir;
					turnDelayTimer_ = TURN_DELAY;
				}
			} else {
				turnDelayTimer_ = TURN_DELAY;
			}

			moveFxTimer_ -= dt;
			if (moveFxTimer_ <= 0.0f) {
				PlaySpiderFx(moveFxId_);
				moveFxTimer_ = moveFxInterval_;
			}

			velocity.x   = currentDirX_ * CHASE_SPEED;
			facingRight_ = (currentDirX_ > 0.0f);
		}
		break;
		
	case EnemyCarmelState::DEFLATE:
		TransitionTo(EnemyCarmelState::IDLE);
		break;

	case EnemyCarmelState::DEATH:
		break;
	}
}

void EnemyCarmel::TransitionTo(EnemyCarmelState newState)
{
	state_ = newState;

	switch (newState)
	{
	case EnemyCarmelState::IDLE:
		lastFrameIndex_ = -1;
		UpdatePhysicsBody(false);
		break;

	case EnemyCarmelState::SCARED:
		stateTimer_ = SCARED_DURATION;
		scaredAnims_.ResetCurrent();
		PlaySpiderFx(alertFxId_);
		lastFrameIndex_ = -1;
		LOG("SpiderCandy: SCARED");
		break;
		
	case EnemyCarmelState::BLOWUP:
		stateTimer_ = BLOWUP_DURATION;
		blowupAnims_.ResetCurrent();
		lastFrameIndex_ = -1;
		UpdatePhysicsBody(true);
		LOG("SpiderCandy: BLOWUP");
		break;

	case EnemyCarmelState::CHASE:
		stateTimer_ = 2000.0f;
		moveFxTimer_ = 0.0f; // Force immediate play
		LOG("SpiderCandy: CHASE");
		break;
		
	case EnemyCarmelState::DEATH:
		LOG("SpiderCandy: DEATH");
		PlaySpiderFx(deathFxId_);
		Destroy();
		break;

	default:
		break;
	}
}

void EnemyCarmel::TakeDamage(int damage)
{
	if (state_ == EnemyCarmelState::DEATH) return;

	health -= damage;
	PlaySpiderFx(hitFxId_);
	LOG("SpiderCandy took %d damage -> health: %d/%d", damage, health, maxHealth);

	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);
	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	float dirX = ((float)bodyX > playerPos.getX()) ? 1.0f : -1.0f;
	Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, dirX * 5.0f, -3.0f, true);

	if (health <= 0)
	{
		health = 0;
		TransitionTo(EnemyCarmelState::DEATH);
	}
}

void EnemyCarmel::Draw(float dt)
{
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	SDL_FlipMode flip = facingRight_ ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	int frameIndex = 0;
	FrameHull* hull = nullptr;

	if (state_ == EnemyCarmelState::CHASE)
	{
		rollAnims_.Update(dt);
		frameIndex = rollAnims_.GetCurrentFrameIndex();
		if (frameIndex != lastFrameIndex_) hull = &carmel_hull_roll[frameIndex % 8];

		const SDL_Rect& frame = rollAnims_.GetCurrentFrame();
		// Full large scale for chase/roll
		currentDrawScale_ = ROLL_DRAW_SCALE;
		int renderSize = (int)(256.0f * currentDrawScale_);
		int drawX = x - renderSize / 2;
		// Anchor bottom to ground (big capsule bottom + 7px visual offset = y + 57)
		int drawY = (y + 57) - renderSize;
		Engine::GetInstance().render->DrawTexture(rollTexture_, drawX, drawY, &frame, 1.0f, -1.0f, 0, INT_MAX, INT_MAX, flip, currentDrawScale_);
	}
	else if (state_ == EnemyCarmelState::SCARED)
	{
		scaredAnims_.Update(dt);
		frameIndex = scaredAnims_.GetCurrentFrameIndex();
		if (frameIndex != lastFrameIndex_) hull = &carmel_hull_scared[frameIndex % 11];

		const SDL_Rect& frame = scaredAnims_.GetCurrentFrame();
		Engine::GetInstance().render->DrawTexture(scaredTexture_, x - 32, y - 32, &frame, 1.0f, -1.0f, 0, INT_MAX, INT_MAX, flip, 1.0f);
	}
	else if (state_ == EnemyCarmelState::BLOWUP)
	{
		blowupAnims_.Update(dt);
		frameIndex = blowupAnims_.GetCurrentFrameIndex();
		if (frameIndex != lastFrameIndex_) hull = &carmel_hull_blowup[frameIndex % 6];

		const SDL_Rect& frame = blowupAnims_.GetCurrentFrame();

		// Smooth scale interpolation: ease-out from idle-equivalent to ROLL_DRAW_SCALE
		// Map idle visual size (64px) to blowup frame size (256px): idle equivalent scale = 64/256 = 0.25
		float startScale = 64.0f / 256.0f;  // 0.25 — same visual size as idle sprite
		float t = 1.0f - (stateTimer_ / BLOWUP_DURATION); // 0→1 over the animation
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;
		// Ease-out cubic for a satisfying growth that starts fast, slows at the end
		float eased = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
		currentDrawScale_ = startScale + (ROLL_DRAW_SCALE - startScale) * eased;

		int renderSize = (int)(256.0f * currentDrawScale_);
		int drawX = x - renderSize / 2;
		// Anchor bottom to ground so it grows upwards, not sinking into floor
		int drawY = (y + 57) - renderSize;
		Engine::GetInstance().render->DrawTexture(blowupTexture_, drawX, drawY, &frame, 1.0f, -1.0f, 0, INT_MAX, INT_MAX, flip, currentDrawScale_);
	}
	else
	{
		// IDLE
		anims_.Update(dt);
		frameIndex = anims_.GetCurrentFrameIndex();
		if (frameIndex != lastFrameIndex_) hull = &carmel_hull_idle[frameIndex % 6];

		const SDL_Rect& frame = anims_.GetCurrentFrame();
		Engine::GetInstance().render->DrawTexture(texture, x - 32, y - 32, &frame, 1.0f, -1.0f, 0, INT_MAX, INT_MAX, flip, 1.0f);
	}

	if (pbodySensor_ != nullptr) {
		pbodySensor_->SetPosition(x, y);
	}

	if (hull != nullptr && pbodySensor_ != nullptr) {
		Engine::GetInstance().physics->UpdateConvexPolygon(pbodySensor_, hull->points, hull->numPoints * 2, currentDrawScale_, 0.6f, true);
		lastFrameIndex_ = frameIndex;
	}
}

void EnemyCarmel::PlaySpiderFx(int fxId)
{
	if (fxId <= 0) return;
	auto& scene = Engine::GetInstance().scene;
	if (scene == nullptr || pbody == nullptr || scene->player == nullptr || scene->player->pbody == nullptr) return;

	int bodyX, bodyY;
	int playerX, playerY;
	pbody->GetPosition(bodyX, bodyY);
	scene->player->pbody->GetPosition(playerX, playerY);

	float dx = (float)(playerX - bodyX);
	float dy = (float)(playerY - bodyY);
	float distSq = dx * dx + dy * dy;

	if (distSq < 1000000.0f)
	{
		Engine::GetInstance().audio->PlayFx(fxId);
	}
}

void EnemyCarmel::OnCollision(PhysBody* physA, PhysBody* physB)
{
	if (physB->ctype == ColliderType::PLAYER && state_ == EnemyCarmelState::CHASE) {
		PlaySpiderFx(attackFxId_);
	}
}
