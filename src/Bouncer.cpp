#include "Bouncer.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "Map.h"
#include "tracy/Tracy.hpp"
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
	size_t AnimIndex(BouncerAnim anim)
	{
		return static_cast<size_t>(anim);
	}
}

Bouncer::Bouncer()
{
	type = EntityType::BOUNCER;
	name = "Bouncer";
	texW = DEFAULT_DIAMETER;
	texH = DEFAULT_DIAMETER;
}

Bouncer::~Bouncer() {}

void Bouncer::SetSpawnSize(float width, float height)
{
	if (width > 0.0f) spawnWidth_ = width;
	if (height > 0.0f) spawnHeight_ = height;
	drawDiameter_ = std::max(spawnWidth_, spawnHeight_);
	physicsRadius_ = drawDiameter_ * PHYSICS_RADIUS_FACTOR;
	texW = (int)drawDiameter_;
	texH = (int)drawDiameter_;
}

bool Bouncer::Start()
{
	maxHealth = 3;
	health = maxHealth;

	LoadClip(BouncerAnim::IDLE, "assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Bouncer_Idle.png", 23, 70, true);
	LoadClip(BouncerAnim::JUMP_START, "assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Bouncer_Jump_Inicial.png", 11, 25, false);
	LoadClip(BouncerAnim::JUMP_LOOP, "assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Bouncer_Jump_Loop.png", 6, 70, true);
	LoadClip(BouncerAnim::JUMP_END, "assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Bouncer_Jump_final.png", 10, 35, false);
	LoadClip(BouncerAnim::TIRED, "assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Bouncer_Cansado.png", 10, 80, true);
	LoadClip(BouncerAnim::HIT, "assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Bouncer_Hit.png", 10, 42, false);
	LoadClip(BouncerAnim::DIE, "assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Bouncer_Die.png", 9, 80, false);

	const int centerX = (int)(position.getX() + spawnWidth_ * 0.5f);
	const int centerY = (int)(position.getY() + spawnHeight_ * 0.5f);
	pbody = Engine::GetInstance().physics->CreateCircle(centerX, centerY, (int)physicsRadius_, bodyType::DYNAMIC);
	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY;

	b2Body_SetLinearDamping(pbody->body, 0.0f);
	b2Body_SetFixedRotation(pbody->body, true);

	b2ShapeId shapeId;
	if (b2Body_GetShapes(pbody->body, &shapeId, 1) > 0)
	{
		b2Shape_SetFriction(shapeId, 0.0f);
		b2Shape_SetRestitution(shapeId, 0.0f);
	}

	variationSeed_ ^= ((uint32_t)centerX * 73856093u) ^ ((uint32_t)centerY * 19349663u);
	moveDirX_ = (variationSeed_ & 1u) ? 1.0f : -1.0f;

	TransitionTo(BouncerState::IDLE);
	return true;
}

bool Bouncer::LoadClip(BouncerAnim anim, const char* path, int frames, int frameDurationMs, bool loop)
{
	Clip& clip = clips_[AnimIndex(anim)];
	SDL_Surface* surface = IMG_Load(path);
	if (surface == nullptr)
	{
		LOG("Bouncer could not load surface: %s", path);
		return false;
	}

	clip.texture = Engine::GetInstance().textures->LoadSurface(surface);
	if (clip.texture == nullptr)
	{
		SDL_DestroySurface(surface);
		LOG("Bouncer could not create texture: %s", path);
		return false;
	}

	clip.sheetW = surface->w;
	clip.sheetH = surface->h;
	clip.frameCount = frames;

	for (int i = 0; i < frames; ++i)
	{
		int x0 = (int)std::round((float)i * (float)clip.sheetW / (float)frames);
		int x1 = (int)std::round((float)(i + 1) * (float)clip.sheetW / (float)frames);

		bool foundVisiblePixel = false;
		int minX = x1;
		int maxX = x0;
		int minY = clip.sheetH;
		int maxY = 0;

		for (int py = 0; py < clip.sheetH; ++py)
		{
			for (int px = x0; px < x1; ++px)
			{
				Uint8 r = 0, g = 0, b = 0, a = 0;
				if (!SDL_ReadSurfacePixel(surface, px, py, &r, &g, &b, &a) || a <= 8)
					continue;

				foundVisiblePixel = true;
				minX = std::min(minX, px);
				maxX = std::max(maxX, px);
				minY = std::min(minY, py);
				maxY = std::max(maxY, py);
			}
		}

		SDL_Rect frame = { x0, 0, std::max(1, x1 - x0), clip.sheetH };
		if (foundVisiblePixel)
		{
			minX = std::max(x0, minX - 1);
			maxX = std::min(x1 - 1, maxX + 1);
			minY = std::max(0, minY - 1);
			maxY = std::min(clip.sheetH - 1, maxY + 1);
			frame = { minX, minY, std::max(1, maxX - minX + 1), std::max(1, maxY - minY + 1) };
		}

		clip.anim.AddFrame(frame, frameDurationMs);
	}

	clip.anim.SetLoop(loop);
	clip.anim.Reset();
	SDL_DestroySurface(surface);
	return true;
}

bool Bouncer::Update(float dt)
{
	ZoneScoped;

	if (Engine::GetInstance().scene->isPaused_ || Engine::GetInstance().scene->isGameOver_)
	{
		if (Engine::GetInstance().scene->isGameOver_ && pbody != nullptr)
		{
			velocity = { 0.0f, 0.0f };
			Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
			b2Body_SetAngularVelocity(pbody->body, 0.0f);
		}
		Draw(0.0f);
		return true;
	}

	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	HandleSurfaceContacts();
	HandleMapBounds();
	UpdateFSM(dt);
	ApplyPlayerTether(dt);

	if (velocity.y > MAX_FALL_SPEED) velocity.y = MAX_FALL_SPEED;
	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);

	if (state_ != BouncerState::DEATH)
	{
		if (contactDamageCooldown_ > 0.0f) contactDamageCooldown_ -= dt;
		if (isContactWithPlayer_ && playerListener_ != nullptr && contactDamageCooldown_ <= 0.0f)
		{
			playerListener_->TakeDamage(1);
			contactDamageCooldown_ = CONTACT_DAMAGE_INTERVAL;
		}
	}

	Draw(dt);
	return true;
}

void Bouncer::UpdateFSM(float dt)
{
	if (state_ == BouncerState::JUMP_START || state_ == BouncerState::AIRBORNE ||
		state_ == BouncerState::LANDING)
	{
		activeMovementTimer_ += dt;
		if (activeMovementTimer_ >= TIRED_AFTER_MOVEMENT)
			tiredQueued_ = true;
	}

	switch (state_)
	{
	case BouncerState::IDLE:
		velocity.x = 0.0f;
		velocity.y = 0.0f;
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f)
			Launch(false);
		break;

	case BouncerState::JUMP_START:
		velocity.x = 0.0f;
		velocity.y = 0.0f;
		if (clips_[AnimIndex(currentClip_)].anim.HasFinishedOnce())
		{
			float horizontalSpeed = BASE_HORIZONTAL_SPEED * NextVariation(0.14f);
			horizontalSpeed = std::clamp(horizontalSpeed, MIN_HORIZONTAL_SPEED, MAX_HORIZONTAL_SPEED);
			velocity.x = moveDirX_ * horizontalSpeed;
			velocity.y = -(highJumpQueued_ ? HIGH_JUMP_SPEED : NORMAL_JUMP_SPEED) * NextVariation(highJumpQueued_ ? 0.10f : 0.08f);

			if (highJumpQueued_)
			{
				bounceCount_ = 0;
				highJumpQueued_ = false;
			}
			TransitionTo(BouncerState::AIRBORNE);
		}
		break;

	case BouncerState::AIRBORNE:
		ClampHorizontalSpeed();
		break;

	case BouncerState::LANDING:
		velocity.x = 0.0f;
		velocity.y = 0.0f;
		stateTimer_ -= dt;
		if (clips_[AnimIndex(currentClip_)].anim.HasFinishedOnce() || stateTimer_ <= 0.0f)
		{
			if (highJumpQueued_ || tiredQueued_)
				TransitionTo(BouncerState::TIRED);
			else
				Launch(false);
		}
		break;

	case BouncerState::TIRED:
		velocity.x = 0.0f;
		velocity.y = 0.0f;
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f)
		{
			tiredQueued_ = false;
			activeMovementTimer_ = 0.0f;
			Launch(highJumpQueued_);
		}
		break;

	case BouncerState::HIT:
		stateTimer_ -= dt;
		if (stateTimer_ <= 0.0f || clips_[AnimIndex(currentClip_)].anim.HasFinishedOnce())
			TransitionTo(BouncerState::AIRBORNE);
		break;

	case BouncerState::DEATH:
		velocity.x = 0.0f;
		velocity.y = 0.0f;
		if (clips_[AnimIndex(currentClip_)].anim.HasFinishedOnce())
			Destroy();
		break;
	}
}

void Bouncer::HandleSurfaceContacts()
{
	if (state_ == BouncerState::IDLE || state_ == BouncerState::LANDING ||
		state_ == BouncerState::TIRED || state_ == BouncerState::DEATH ||
		state_ == BouncerState::JUMP_START)
	{
		return;
	}

	const int capacity = b2Body_GetContactCapacity(pbody->body);
	if (capacity <= 0) return;

	std::vector<b2ContactData> contacts((size_t)capacity);
	const int count = b2Body_GetContactData(pbody->body, contacts.data(), capacity);

	bool groundHit = false;
	bool ceilingHit = false;
	bool wallHit = false;
	float wallNormalX = 0.0f;

	for (int i = 0; i < count; ++i)
	{
		const b2ContactData& contact = contacts[(size_t)i];
		if (contact.manifold.pointCount <= 0) continue;

		const bool selfIsA = B2_ID_EQUALS(b2Shape_GetBody(contact.shapeIdA), pbody->body);
		const b2BodyId otherBody = selfIsA ? b2Shape_GetBody(contact.shapeIdB) : b2Shape_GetBody(contact.shapeIdA);
		PhysBody* otherPhys = static_cast<PhysBody*>(b2Body_GetUserData(otherBody));
		if (otherPhys == nullptr || !IsBlockingSurface(otherPhys->ctype)) continue;

		b2Vec2 normal = contact.manifold.normal;
		if (selfIsA)
		{
			normal.x = -normal.x;
			normal.y = -normal.y;
		}

		const float intoSurface = velocity.x * normal.x + velocity.y * normal.y;
		if (intoSurface > 0.05f) continue;

		if (normal.y < -0.55f)
		{
			groundHit = true;
		}
		else if (normal.y > 0.55f)
		{
			ceilingHit = true;
		}
		else if (std::abs(normal.x) > 0.55f)
		{
			wallHit = true;
			wallNormalX = normal.x;
		}
	}

	if (groundHit)
	{
		bounceCount_++;
		highJumpQueued_ = bounceCount_ >= BOUNCES_BEFORE_HIGH_JUMP;
		if (activeMovementTimer_ >= TIRED_AFTER_MOVEMENT)
			tiredQueued_ = true;
		velocity.y = 0.0f;
		if (state_ != BouncerState::HIT)
			TransitionTo(BouncerState::LANDING);
		else
			velocity.y = -NORMAL_JUMP_SPEED * NextVariation(0.08f);
		return;
	}

	if (wallHit)
	{
		moveDirX_ = (wallNormalX >= 0.0f) ? 1.0f : -1.0f;
		velocity.x = moveDirX_ * BASE_HORIZONTAL_SPEED * NextVariation(0.18f);
		if (velocity.y > -1.0f)
			velocity.y -= 1.0f + 0.7f * NextVariation(0.2f);
	}

	if (ceilingHit && velocity.y < 0.0f)
	{
		velocity.y = std::abs(velocity.y) * 0.55f;
	}
}

void Bouncer::HandleMapBounds()
{
	if (Engine::GetInstance().map == nullptr || pbody == nullptr) return;

	Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
	int x, y;
	pbody->GetPosition(x, y);

	if ((float)x - physicsRadius_ < 0.0f)
	{
		pbody->SetPosition((int)physicsRadius_, y);
		moveDirX_ = 1.0f;
		velocity.x = std::abs(velocity.x);
	}
	else if ((float)x + physicsRadius_ > mapSize.getX())
	{
		pbody->SetPosition((int)(mapSize.getX() - physicsRadius_), y);
		moveDirX_ = -1.0f;
		velocity.x = -std::abs(velocity.x);
	}
}

void Bouncer::TransitionTo(BouncerState newState)
{
	state_ = newState;

	switch (newState)
	{
	case BouncerState::IDLE:
		SetClip(BouncerAnim::IDLE);
		stateTimer_ = INITIAL_IDLE_DURATION;
		break;

	case BouncerState::JUMP_START:
		SetClip(BouncerAnim::JUMP_START);
		break;

	case BouncerState::AIRBORNE:
		SetClip(BouncerAnim::JUMP_LOOP);
		break;

	case BouncerState::LANDING:
		SetClip(BouncerAnim::JUMP_END);
		stateTimer_ = LANDING_DURATION;
		break;

	case BouncerState::TIRED:
		SetClip(BouncerAnim::TIRED);
		stateTimer_ = TIRED_DURATION;
		break;

	case BouncerState::HIT:
		SetClip(BouncerAnim::HIT);
		stateTimer_ = HIT_DURATION;
		break;

	case BouncerState::DEATH:
		SetClip(BouncerAnim::DIE);
		isContactWithPlayer_ = false;
		playerListener_ = nullptr;
		velocity.x = 0.0f;
		velocity.y = 0.0f;
		if (pbody != nullptr)
			pbody->ctype = ColliderType::UNKNOWN;
		break;
	}
}

void Bouncer::Launch(bool highJump)
{
	float playerDeltaX = 0.0f;
	if (TryGetPlayerDeltaX(playerDeltaX) && std::abs(playerDeltaX) > PLAYER_FOLLOW_DISTANCE * 0.65f)
		moveDirX_ = (playerDeltaX > 0.0f) ? 1.0f : -1.0f;

	TransitionTo(BouncerState::JUMP_START);
}

void Bouncer::ApplyPlayerTether(float dt)
{
	float playerDeltaX = 0.0f;
	if (!TryGetPlayerDeltaX(playerDeltaX)) return;

	const float distance = std::abs(playerDeltaX);
	if (distance <= PLAYER_FOLLOW_DISTANCE) return;

	const float desiredDir = (playerDeltaX > 0.0f) ? 1.0f : -1.0f;
	moveDirX_ = desiredDir;

	if (state_ == BouncerState::IDLE || state_ == BouncerState::LANDING ||
		state_ == BouncerState::TIRED || state_ == BouncerState::DEATH ||
		state_ == BouncerState::JUMP_START)
	{
		return;
	}

	const float excess = std::min(distance - PLAYER_FOLLOW_DISTANCE, 260.0f);
	float targetSpeed = BASE_HORIZONTAL_SPEED + excess / 220.0f;
	targetSpeed = std::clamp(targetSpeed, MIN_HORIZONTAL_SPEED, MAX_HORIZONTAL_SPEED);

	if (distance >= PLAYER_HARD_LIMIT_DISTANCE)
	{
		velocity.x = desiredDir * targetSpeed;
		return;
	}

	const float followBlend = std::clamp(dt * 0.006f, 0.0f, 0.16f);
	velocity.x += (desiredDir * targetSpeed - velocity.x) * followBlend;

	if (velocity.x * desiredDir < MIN_HORIZONTAL_SPEED * 0.7f)
		velocity.x = desiredDir * MIN_HORIZONTAL_SPEED * 0.7f;
}

bool Bouncer::TryGetPlayerDeltaX(float& outDeltaX) const
{
	auto& scene = Engine::GetInstance().scene;
	if (scene == nullptr || pbody == nullptr || scene->player == nullptr ||
		scene->player->pbody == nullptr || scene->player->IsDead())
	{
		return false;
	}

	int bodyX, bodyY;
	int playerX, playerY;
	pbody->GetPosition(bodyX, bodyY);
	scene->player->pbody->GetPosition(playerX, playerY);
	outDeltaX = (float)playerX - (float)bodyX;
	return true;
}

void Bouncer::ClampHorizontalSpeed()
{
	if (std::abs(velocity.x) < MIN_HORIZONTAL_SPEED)
		velocity.x = moveDirX_ * MIN_HORIZONTAL_SPEED;
	if (std::abs(velocity.x) > MAX_HORIZONTAL_SPEED)
		velocity.x = moveDirX_ * MAX_HORIZONTAL_SPEED;
}

float Bouncer::NextVariation(float amount)
{
	variationSeed_ = variationSeed_ * 1664525u + 1013904223u;
	const float unit = (float)((variationSeed_ >> 8) & 0xFFFFu) / 65535.0f;
	return 1.0f + (unit * 2.0f - 1.0f) * amount;
}

bool Bouncer::IsBlockingSurface(ColliderType type) const
{
	return type == ColliderType::PLATFORM ||
		type == ColliderType::BOX ||
		type == ColliderType::PUSH_ROCK ||
		type == ColliderType::DOOR;
}

void Bouncer::SetClip(BouncerAnim anim, bool reset)
{
	currentClip_ = anim;
	if (reset)
		clips_[AnimIndex(currentClip_)].anim.Reset();
}

void Bouncer::Draw(float dt)
{
	Clip& clip = clips_[AnimIndex(currentClip_)];
	if (dt > 0.0f)
		clip.anim.Update(dt);

	const SDL_Rect& frame = clip.anim.GetCurrentFrame();
	if (clip.texture == nullptr || frame.w <= 0 || frame.h <= 0) return;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	const float drawScale = drawDiameter_ / (float)std::max(frame.w, frame.h);
	const int drawW = (int)((float)frame.w * drawScale);
	const int drawH = (int)((float)frame.h * drawScale);
	const int drawX = x - drawW / 2;
	const int drawY = y + (int)(physicsRadius_ + VISUAL_GROUND_OVERLAP) - drawH;
	const double angle = 0.0;
	SDL_FlipMode flip = (moveDirX_ < 0.0f) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

	Engine::GetInstance().render->DrawTexture(clip.texture, drawX, drawY, &frame,
		1.0f, angle, INT_MAX, INT_MAX, flip, drawScale);
}

bool Bouncer::CleanUp()
{
	LOG("Cleanup Bouncer");
	for (Clip& clip : clips_)
	{
		if (clip.texture != nullptr)
		{
			Engine::GetInstance().textures->UnLoad(clip.texture);
			clip.texture = nullptr;
		}
	}

	if (pbody != nullptr)
	{
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}

	return true;
}

void Bouncer::OnCollision(PhysBody* physA, PhysBody* physB)
{
	if (state_ == BouncerState::DEATH) return;
	Enemy::OnCollision(physA, physB);
}

void Bouncer::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	Enemy::OnCollisionEnd(physA, physB);
}

void Bouncer::TakeDamage(int damage)
{
	if (state_ == BouncerState::DEATH) return;

	health -= damage;
	LOG("Bouncer took %d damage -> health: %d/%d", damage, health, maxHealth);

	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();
	int bodyX, bodyY;
	pbody->GetPosition(bodyX, bodyY);
	float dirX = ((float)bodyX > playerPos.getX()) ? 1.0f : -1.0f;
	moveDirX_ = dirX;

	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity.x = dirX * std::max(MIN_HORIZONTAL_SPEED, std::abs(velocity.x));
	velocity.y = std::min(velocity.y, -NORMAL_JUMP_SPEED * 0.75f);
	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);

	if (health <= 0)
	{
		health = 0;
		TransitionTo(BouncerState::DEATH);
	}
	else
	{
		TransitionTo(BouncerState::HIT);
	}
}
