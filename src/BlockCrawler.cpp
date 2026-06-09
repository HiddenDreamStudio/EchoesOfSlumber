#include "BlockCrawler.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Log.h"
#include "Scene.h"
#include "Player.h"
#include "Physics.h"

namespace {
    struct FrameBox {
        int w, h, bottomGap;
    };
    const FrameBox moveBoxes[15] = {
        { 49, 43, 0 }, { 39, 34, 0 }, { 43, 37, 0 }, { 48, 40, 0 }, { 50, 41, 0 },
        { 49, 41, 0 }, { 42, 39, 0 }, { 36, 36, 0 }, { 41, 40, 0 }, { 47, 43, 0 },
        { 49, 44, 0 }, { 43, 41, 0 }, { 39, 37, 2 }, { 38, 36, 2 }, { 45, 40, 0 }
    };
    const FrameBox wallBoxes[11] = {
        { 45, 33, 0 }, { 45, 33, 0 }, { 45, 34, 0 }, { 46, 40, 0 }, { 46, 57, 0 },
        { 47, 94, 0 }, { 48, 130, 0 }, { 49, 147, 0 }, { 49, 153, 0 }, { 49, 155, 0 }, { 49, 155, 0 }
    };
    const FrameBox hitBoxes[7] = {
        { 49, 155, 0 }, { 49, 154, 0 }, { 49, 154, 0 }, { 48, 154, 0 }, { 48, 154, 0 }, { 48, 154, 0 }, { 49, 155, 0 }
    };
    const FrameBox dieBoxes[29] = {
        { 49, 155, 0 }, { 48, 134, 0 }, { 47, 114, 0 }, { 47, 94, 0 }, { 46, 77, 0 },
        { 46, 66, 0 }, { 46, 60, 0 }, { 46, 54, 0 }, { 47, 49, 0 }, { 45, 43, 0 },
        { 45, 38, 0 }, { 45, 35, 0 }, { 45, 33, 0 }, { 45, 33, 0 }, { 45, 33, 0 },
        { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 },
        { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 },
        { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 }
    };
}


BlockCrawler::BlockCrawler() : Entity(EntityType::BLOCK_CRAWLER)
{
	name = "BlockCrawler";
	health = 5;
	maxHealth = 5;
}

BlockCrawler::~BlockCrawler()
{
}

bool BlockCrawler::Awake()
{
	return true;
}

bool BlockCrawler::Start()
{
	// Load Textures
	moveTexture_ = Engine::GetInstance().textures.get()->Load("assets/textures/spritesheets/SS_Enemics_Level1/SP_BlocCrawler_move.png");
	wallTexture_ = Engine::GetInstance().textures.get()->Load("assets/textures/spritesheets/SS_Enemics_Level1/SP_BlocCrawler_wall.png");
	hitTexture_  = Engine::GetInstance().textures.get()->Load("assets/textures/spritesheets/SS_Enemics_Level1/SP_BlocCrawler_hit.png");
	dieTexture_  = Engine::GetInstance().textures.get()->Load("assets/textures/spritesheets/SS_Enemics_Level1/SP_BlocCrawler_die.png");

	// Load Animations
	moveAnims_.LoadFromTSX("assets/textures/animations/BlockCrawlerMove.xml", { {0, "move"} });
	wallAnims_.LoadFromTSX("assets/textures/animations/BlockCrawlerWall.xml", { {0, "wall"} });
	hitAnims_.LoadFromTSX("assets/textures/animations/BlockCrawlerHit.xml", { {0, "hit"} });
	dieAnims_.LoadFromTSX("assets/textures/animations/BlockCrawlerDie.xml", { {0, "die"} });

	// Don't loop transition animations
	wallAnims_.SetLoop("wall", false);
	hitAnims_.SetLoop("hit", false);
	dieAnims_.SetLoop("die", false);

	// Create initial physics body
	currentFrameIndex_ = -1;
	UpdatePhysicsForCurrentFrame();

	EnterState(State::MOVE);

	return true;
}

void BlockCrawler::UpdatePhysicsForCurrentFrame()
{
	int frameIdx = 0;
	const FrameBox* box = nullptr;

	switch (currentState_) {
		case State::MOVE: frameIdx = moveAnims_.GetCurrentFrameIndex(); if(frameIdx >= 0 && frameIdx < 15) box = &moveBoxes[frameIdx]; break;
		case State::WALL: frameIdx = wallAnims_.GetCurrentFrameIndex(); if(frameIdx >= 0 && frameIdx < 11) box = &wallBoxes[frameIdx]; break;
		case State::HIT:  frameIdx = hitAnims_.GetCurrentFrameIndex();  if(frameIdx >= 0 && frameIdx < 7)  box = &hitBoxes[frameIdx];  break;
		case State::DEATH:frameIdx = dieAnims_.GetCurrentFrameIndex();  if(frameIdx >= 0 && frameIdx < 29) box = &dieBoxes[frameIdx];  break;
	}

	if (!box) return;

	if (frameIdx == currentFrameIndex_) return;
	currentFrameIndex_ = frameIdx;

	float renderScale = 1.5f;
	int newPhysW = (int)(box->w * renderScale * 0.8f);
	int newPhysH = (int)(box->h * renderScale * 0.9f);
	if (newPhysW <= 0) newPhysW = 10;
	if (newPhysH <= 0) newPhysH = 10;

	int bx = (int)position.getX();
	int by = (int)position.getY();

	bodyType bType = (currentState_ == State::MOVE) ? DYNAMIC : STATIC;

	if (pbody != nullptr) {
		pbody->GetPosition(bx, by);
		int floorY = by + currentPhysHalfH_;
		by = floorY - (newPhysH / 2);
		
		b2Vec2 vel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = Engine::GetInstance().physics->CreateRectangle(bx, by, newPhysW, newPhysH, bType);
		b2Body_SetFixedRotation(pbody->body, true);
		Engine::GetInstance().physics->SetLinearVelocity(pbody, vel);
	} else {
		pbody = Engine::GetInstance().physics->CreateRectangle(bx, by, newPhysW, newPhysH, bType);
		b2Body_SetFixedRotation(pbody->body, true);
	}

	currentPhysHalfH_ = newPhysH / 2;
	pbody->listener = this;
	pbody->ctype = (currentState_ == State::DEATH) ? ColliderType::UNKNOWN : ColliderType::ENEMY;
}

bool BlockCrawler::Update(float dt)
{
	if (!active || pendingToDelete) return true;

	if (Engine::GetInstance().scene->isPaused_ || Engine::GetInstance().scene->isGameOver_) {
		return true;
	}

	// Sync position
	if (pbody != nullptr) {
		int x, y;
		pbody->GetPosition(x, y);
		position.setX((float)x);
		position.setY((float)y);
		// Wake body
		b2Body_SetAwake(pbody->body, true);
	}

	UpdateFSM(dt);

	// Check distance fallback to prevent sticky contact
	if (isContactWithPlayer_ && playerListener_ != nullptr) {
		int enemyX, enemyY;
		if (pbody) {
			pbody->GetPosition(enemyX, enemyY);
			int playerX, playerY;
			auto pl = Engine::GetInstance().scene->player;
			if (pl && pl->pbody) {
				pl->pbody->GetPosition(playerX, playerY);
				float dx = (float)enemyX - (float)playerX;
				float dy = (float)enemyY - (float)playerY;
				if (dx * dx + dy * dy > 150.0f * 150.0f) {
					isContactWithPlayer_ = false;
					playerListener_ = nullptr;
				}
			} else {
				isContactWithPlayer_ = false;
				playerListener_ = nullptr;
			}
		}
	}

	// Apply Contact Damage if player is touching (ONLY IN MOVE STATE)
	if (currentState_ == State::MOVE && isContactWithPlayer_ && playerListener_ != nullptr) {
		if (contactDamageCooldown_ <= 0.0f) {
			playerListener_->TakeDamage(1);
			contactDamageCooldown_ = CONTACT_DAMAGE_INTERVAL;
		}
	}

	if (contactDamageCooldown_ > 0.0f) {
		contactDamageCooldown_ -= dt;
	}

	Draw();

	return true;
}

void BlockCrawler::UpdateFSM(float dt)
{
	stateTimer_ += dt;

	switch (currentState_) {
		case State::MOVE: {
			moveAnims_.Update(dt);
			// Patrol logic
			if (movingRight_) {
				b2Vec2 vel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
				vel.x = speed_;
				Engine::GetInstance().physics->SetLinearVelocity(pbody, vel);
				facingRight_ = true;
				if (position.getX() >= patrolRight_) movingRight_ = false;
			} else {
				b2Vec2 vel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
				vel.x = -speed_;
				Engine::GetInstance().physics->SetLinearVelocity(pbody, vel);
				facingRight_ = false;
				if (position.getX() <= patrolLeft_) movingRight_ = true;
			}

			if (stateTimer_ >= MOVE_DURATION) {
				EnterState(State::WALL);
			}
			break;
		}
		case State::WALL: {
			wallAnims_.Update(dt);
			b2Vec2 vel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
			vel.x = 0.0f;
			Engine::GetInstance().physics->SetLinearVelocity(pbody, vel);

			if (wallAnims_.HasFinishedOnce("wall") && stateTimer_ >= WALL_DURATION) {
				EnterState(State::MOVE);
			}
			break;
		}
		case State::HIT: {
			hitAnims_.Update(dt);
			b2Vec2 vel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
			vel.x = 0.0f;
			Engine::GetInstance().physics->SetLinearVelocity(pbody, vel);
			
			if (hitAnims_.HasFinishedOnce("hit")) {
				EnterState(State::WALL);
			}
			break;
		}
		case State::DEATH: {
			dieAnims_.Update(dt);
			b2Vec2 vel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
			vel.x = 0.0f;
			Engine::GetInstance().physics->SetLinearVelocity(pbody, vel);
			
			if (dieAnims_.HasFinishedOnce("die")) {
				active = false;
				pendingToDelete = true;
			}
			break;
		}
	}
	UpdatePhysicsForCurrentFrame();
}

void BlockCrawler::EnterState(State newState)
{
	currentState_ = newState;
	stateTimer_ = 0.0f;
	currentFrameIndex_ = -1;

	switch (currentState_) {
		case State::MOVE:
			moveAnims_.SetCurrent("move");
			break;
		case State::WALL: {
			wallAnims_.SetCurrent("wall");
			wallAnims_.ResetCurrent();
			auto pl = Engine::GetInstance().scene->player;
			if (pl && pl->pbody) {
				int px, py, ex, ey;
				pl->pbody->GetPosition(px, py);
				if (pbody) {
					pbody->GetPosition(ex, ey);
					facingRight_ = (px < ex);
				}
			}
			break;
		}
		case State::HIT:
			hitAnims_.SetCurrent("hit");
			hitAnims_.ResetCurrent();
			break;
		case State::DEATH:
			if (pbody != nullptr) {
				pbody->ctype = ColliderType::UNKNOWN;
			}
			dieAnims_.SetCurrent("die");
			dieAnims_.ResetCurrent();
			break;
	}
	UpdatePhysicsForCurrentFrame();
}

void BlockCrawler::TakeDamage(int damage)
{
	if (currentState_ == State::DEATH || currentState_ == State::HIT) return;
	
	// Invulnerable in move state
	if (currentState_ == State::MOVE) return;

	health -= damage;

	if (health <= 0) {
		EnterState(State::DEATH);
	} else {
		EnterState(State::HIT);
	}
}

void BlockCrawler::OnCollision(PhysBody* physA, PhysBody* physB)
{
	if (!active || pendingToDelete) return;

	if (physB->ctype == ColliderType::PLAYER) {
		isContactWithPlayer_ = true;
		playerListener_ = physB->listener;
	}
	else if (physB->ctype == ColliderType::ATTACK) {
		TakeDamage(1);
	}
	else if (physB->ctype == ColliderType::SLINGSHOT_PROJ) {
		TakeDamage(1);
		physB->listener->pendingToDelete = true;
	}
}

void BlockCrawler::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	if (physB->ctype == ColliderType::PLAYER) {
		isContactWithPlayer_ = false;
		playerListener_ = nullptr;
	}
}

void BlockCrawler::Draw()
{
	SDL_Rect rect;
	SDL_Texture* tex = nullptr;

	switch (currentState_) {
		case State::MOVE:
			rect = moveAnims_.GetCurrentFrame();
			tex = moveTexture_;
			break;
		case State::WALL:
			rect = wallAnims_.GetCurrentFrame();
			tex = wallTexture_;
			break;
		case State::HIT:
			rect = hitAnims_.GetCurrentFrame();
			tex = hitTexture_;
			break;
		case State::DEATH:
			rect = dieAnims_.GetCurrentFrame();
			tex = dieTexture_;
			break;
	}

	if (tex != nullptr) {
		float renderScale = 1.5f;
		float renderW = texW * renderScale;
		float renderH = texH * renderScale;
		
		int rx = (int)(position.getX() - renderW / 2.0f);
		
		// Align bottom of sprite to bottom of physics body
		int spriteBottomOffset = 15; // Visual tweak in case the sprite has transparent padding at the bottom
		
		int ry = (int)(position.getY() + currentPhysHalfH_ - renderH) + spriteBottomOffset;

		SDL_FlipMode flip = facingRight_ ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
		Engine::GetInstance().render->DrawTexture(tex, rx, ry, &rect, 1.0f, 0, INT_MAX, INT_MAX, flip, renderScale);
	}
}

bool BlockCrawler::CleanUp()
{
	if (moveTexture_) Engine::GetInstance().textures.get()->UnLoad(moveTexture_);
	if (wallTexture_) Engine::GetInstance().textures.get()->UnLoad(wallTexture_);
	if (hitTexture_)  Engine::GetInstance().textures.get()->UnLoad(hitTexture_);
	if (dieTexture_)  Engine::GetInstance().textures.get()->UnLoad(dieTexture_);

	if (pbody != nullptr) {
		Engine::GetInstance().physics.get()->DeletePhysBody(pbody);
		pbody = nullptr;
	}

	return true;
}
