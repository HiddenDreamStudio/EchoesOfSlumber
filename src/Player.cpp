#include "Player.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Map.h"
#include "tracy/Tracy.hpp"

Player::Player() : Entity(EntityType::PLAYER),
texW(128), texH(128),
pickCoinFxId(-1),
pbody(nullptr),
damageFlashTimer_(0.0f)
{
	velocity.x = 0.0f;
	velocity.y = 0.0f;
	name = "Player";
}

Player::~Player() {

}

bool Player::Awake() {
	return true;
}

bool Player::Start() {

	std::unordered_map<int, std::string> aliases = {
		{0, "idle"},
		{14, "turnaround"},
		{28, "run"},
		{42, "jump"},
		{56, "hide"},
		{70, "damage"},
		{84, "death"}
	};
	anims.LoadFromTSX("assets/textures/animations/protagonistAnimation.xml", aliases);
	anims.SetCurrent("idle");

	anims.SetLoop("turnaround", false);
	anims.SetLoop("jump", false);
	anims.SetLoop("damage", false);
	anims.SetLoop("death", false);
	anims.SetLoop("hide", false); // Play once and freeze on last frame

	texture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/protagonistSpritesheet.png");

	wakeUpTexture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS Individual/SS_Despertar.png");
	for (int i = 0; i < 51; ++i) {
		SDL_Rect r = { i * 258, 0, 258, 258 };
		wakeUpAnim.AddFrame(r, 120);
	}
	wakeUpAnim.SetLoop(false);
	isWakingUp = true;

	climbAnims.LoadSequentialFromTSX("assets/textures/animations/protagonistClimbing.xml", "climb", 80);
	climbAnims.SetLoop("climb", false);
	climbTexture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS Individual/Spritesheet_climb.png");

	texW = 128;
	texH = 128;
	drawScale = 1.0f;

	pbody = Engine::GetInstance().physics->CreateCapsule((int)position.getX(), (int)position.getY(), 40, 100, bodyType::DYNAMIC, 0.0f);

	pbody->listener = this;
	pbody->ctype = ColliderType::PLAYER;

	pickCoinFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/coin-collision-sound-342335.wav");
	return true;
}

bool Player::Update(float dt)
{
	ZoneScoped;

	if (Engine::GetInstance().scene->isPaused_) {
		Draw(0.0f);
		return true;
	}

	GetPhysicsValues();

	// Tick hide cooldown
	if (hideCooldown_ > 0.0f) hideCooldown_ -= dt;

	if (!isDead_ && !isWakingUp && !isClimbing_)
	{
		if (!isShowingDamageAnim_) {
			// Hide must be evaluated first so it can block other actions
			Hide(dt);

			// All other actions are blocked while hiding
			if (!isHiding_) {
				Dash(dt);
				if (!isDashing_ && !isExitingHide_) Move();
				Jump();
				CheckLedge();
				Attack(dt);
				Teleport();
			}
		}
	}

	if (isClimbing_) UpdateClimb(dt);
	if (!isClimbing_) ApplyPhysics();

	if (damageFlashTimer_ > 0.0f) damageFlashTimer_ -= dt;
	if (iFrameTimer_ > 0.0f)      iFrameTimer_ -= dt;
	if (iFrameTimer_ <= 0.0f)     isInvincible_ = false;

	// Advance hide alpha pulse timer for visual feedback
	if (isHiding_ || isExitingHide_) hideAlphaTime_ += dt;
	else           hideAlphaTime_ = 0.0f;

	Draw(dt);

	return true;
}

void Player::Teleport() {
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_T) == KEY_DOWN) {
		pbody->SetPosition(96, 96);
	}
}

void Player::GetPhysicsValues() {
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity.x = 0.0f;
}

void Player::Move() {
	if (isWakingUp || isShowingDamageAnim_ || isHiding_ || isExitingHide_) return;

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) {
		velocity.x = -speed;
		if (!facingRight) {
			facingRight = true;
			if (!isJumping) anims.SetCurrent("turnaround");
		}

		if (!isJumping) {
			if (anims.GetCurrentName() != "turnaround" || anims.HasFinishedOnce("turnaround")) {
				if (anims.Has("run")) anims.SetCurrent("run");
				else anims.SetCurrent("idle");
			}
		}
	}
	else if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) {
		velocity.x = speed;
		if (facingRight) {
			facingRight = false;
			if (!isJumping) anims.SetCurrent("turnaround");
		}

		if (!isJumping) {
			if (anims.GetCurrentName() != "turnaround" || anims.HasFinishedOnce("turnaround")) {
				if (anims.Has("run")) anims.SetCurrent("run");
				else anims.SetCurrent("idle");
			}
		}
	}
	else if (!isJumping) {
		if (anims.GetCurrentName() == "jump" && !anims.HasFinishedOnce("jump")) {
			// let landing animation finish
		}
		else {
			anims.SetCurrent("idle");
		}
	}
}

void Player::Jump() {
	if (isWakingUp || isDashing_ || isShowingDamageAnim_ || isHiding_ || isExitingHide_) return;

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN) {
		if (!isJumping) {
			Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, 0.0f, -jumpForce, true);
			if (anims.Has("jump")) anims.SetCurrent("jump");
			isJumping = true;
			canDoubleJump = true;
			hasDoubleJumped = false;

			// Spawn jump dust
			Engine::GetInstance().entityManager->SpawnVFX(
				Vector2D(position.getX() + (float)texW / 2.0f, position.getY() + (float)texH / 2.0f + 50.0f),
				"assets/textures/spritesheets/SS_Pols_01.png",
				47, 202, 202, 0.015f
			);
		}
		else if (canDoubleJump && !hasDoubleJumped) {
			Engine::GetInstance().physics->SetYVelocity(pbody, 0.0f);
			Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, 0.0f, -doubleJumpForce, true);
			if (anims.Has("jump")) {
				anims.ResetCurrent();
			}
			hasDoubleJumped = true;

			// Spawn double jump dust
			Engine::GetInstance().entityManager->SpawnVFX(
				Vector2D(position.getX() + (float)texW / 2.0f, position.getY() + (float)texH / 2.0f + 30.0f),
				"assets/textures/spritesheets/SS_Pols_01.png",
				47, 202, 202, 0.015f
			);
		}
	}

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_UP && isJumping) {
		float vy = Engine::GetInstance().physics->GetYVelocity(pbody);
		if (vy < 0.0f) {
			Engine::GetInstance().physics->SetYVelocity(pbody, vy * 0.5f);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Hide mechanic
//   • Pressing H toggles the hiding state (subject to 15-second cooldown).
//   • While hiding the player stands still and plays the "hide" animation once,
//     then freezes on the last frame.
//   • Enemies query IsHiding() and skip pathfinding entirely.
//   • Cooldown of HIDE_COOLDOWN ms starts when the player exits hiding.
// ─────────────────────────────────────────────────────────────────────────────
void Player::Hide(float dt)
{
	if (isWakingUp || isClimbing_ || isDashing_ || isShowingDamageAnim_ || isDead_) return;

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_H) == KEY_DOWN)
	{
		if (!isHiding_ && !isExitingHide_)
		{
			// Blocked while on cooldown
			if (hideCooldown_ > 0.0f)
			{
				LOG("Hide on cooldown: %.0f ms remaining", hideCooldown_);
				return;
			}

			isHiding = true;
			velocity.x = 0.0f;
			Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
			anims.SetCurrent("hide");
			anims.ResetCurrent();
			LOG("Player hiding");
		}
		else if (isHiding_)
		{
			isHiding = false;
			isExitingHide_ = true;
			hideCooldown_ = HIDE_COOLDOWN; // cooldown starts on exit
			LOG("Player exiting hide — cooldown started (%.0f ms)", HIDE_COOLDOWN);
		}
	}

	if (isHiding_)
	{
		velocity.x = 0.0f;
		if (!anims.HasFinishedOnce("hide"))
			anims.Update(dt);
	}

	if (isExitingHide_)
	{
		velocity.x = 0.0f;
		anims.UpdateBackwards(dt);

		// Si vuelve al frame 0, ya está completamente visible y levantado
		if (anims.GetCurrentFrameIndex() == 0)
		{
			isExitingHide_ = false;
			anims.SetCurrent("idle");
			LOG("Player stopped hiding");
		}
	}
}

void Player::ApplyPhysics() {
	if (isJumping == true) {
		velocity.y = Engine::GetInstance().physics->GetYVelocity(pbody);
	}

	if (isDashing_) {
		velocity.x = dashDirX_ * DASH_SPEED;
		velocity.y = 0.0f;
	}

	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}

void Player::Draw(float dt) {

	SDL_Texture* activeTex = texture;
	const SDL_Rect* animFrame = nullptr;

	if (isDead_)
	{
		if (anims.GetCurrentName() != "death") anims.SetCurrent("death");
		anims.Update(dt);
		animFrame = &anims.GetCurrentFrame();
	}
	else if (isShowingDamageAnim_)
	{
		if (anims.GetCurrentName() != "damage") anims.SetCurrent("damage");
		anims.Update(dt);
		animFrame = &anims.GetCurrentFrame();
		if (anims.HasFinishedOnce("damage")) {
			isShowingDamageAnim_ = false;
			anims.SetCurrent("idle");
		}
	}
	else if (isHiding_ || isExitingHide_)
	{
		// Hide animation is advanced inside Hide() — just grab the current frame
		animFrame = &anims.GetCurrentFrame();
	}
	else if (!isClimbing_)
	{
		bool shouldUpdate = true;
		if (isJumping && anims.GetCurrentName() == "jump") {
			// Freeze on peak frame (tile 49 is the middle-ish frame)
			if (anims.GetCurrentFrameIndex() >= 7) {
				shouldUpdate = false;
			}
		}
		if (shouldUpdate) anims.Update(dt);
		animFrame = &anims.GetCurrentFrame();
	}

	int xInt, yInt;
	pbody->GetPosition(xInt, yInt);
	position.setX(static_cast<float>(xInt));
	position.setY(static_cast<float>(yInt));

	auto& render = Engine::GetInstance().render;
	Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();

	if (!isDead_)
	{
		render->SetCameraTarget(position.getX(), position.getY());
		render->FollowTarget(dt);
		render->ClampCameraToMapBounds(mapSize.getX(), mapSize.getY());
	}

	float currentDrawScale = drawScale;
	if (anims.GetCurrentName() == "jump") {
		currentDrawScale *= 1.25f;
	}

	int drawX = static_cast<int>(position.getX() - (static_cast<float>(texW) * currentDrawScale) / 2.0f);
	int drawY = static_cast<int>(position.getY() - (static_cast<float>(texH) * currentDrawScale) / 2.0f);

	bool spriteNativeRight = false;
	const std::string& animName = anims.GetCurrentName();
	if (animName == "jump" || animName == "turnaround") {
		spriteNativeRight = true;
	}

	SDL_FlipMode flip;
	if (spriteNativeRight) {
		flip = facingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	}
	else {
		flip = facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
	}

	if (isWakingUp) {
		wakeUpAnim.Update(dt);
		if (wakeUpAnim.HasFinishedOnce()) {
			isWakingUp = false;
		}
		else {
			const SDL_Rect& wuFrame = wakeUpAnim.GetCurrentFrame();
			float wakeScale = 0.65f;
			int wakeWidth = (int)(258.0f * wakeScale);
			int wakeHeight = (int)(258.0f * wakeScale);
			int wakeDrawX = xInt - (wakeWidth / 2) - 60;
			int wakeDrawY = yInt + 50 - wakeHeight + 15;
			render->ApplyAmbientTint(wakeUpTexture);
			render->DrawTexture(wakeUpTexture, wakeDrawX, wakeDrawY, &wuFrame, 1.0f, 0, INT_MAX, INT_MAX, flip, wakeScale);
			render->ResetAmbientTint(wakeUpTexture);
			return;
		}
	}

	if (isClimbing_) {
		const SDL_Rect& climbFrame = climbAnims.GetCurrentFrame();
		int x, y;
		pbody->GetPosition(x, y);
		int climbDrawX = x - (int)(256.0f * CLIMB_DRAW_SCALE) / 2;
		int climbDrawY = y - (int)(256.0f * CLIMB_DRAW_SCALE) / 2;
		SDL_FlipMode climbFlip = facingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
		Engine::GetInstance().render->DrawTexture(climbTexture, climbDrawX, climbDrawY, &climbFrame, 1.0f, 0, INT_MAX, INT_MAX, climbFlip, CLIMB_DRAW_SCALE);
		return;
	}

	// ── I-frame flicker (not during hide, dash, or death) ────────────────────
	bool skipDraw = !isDead_ && !isDashing_ && !isHiding_ && !isExitingHide_ && isInvincible_ &&
		(static_cast<int>(iFrameTimer_ / 100.0f) % 2 == 0);

	if (!skipDraw) {
		render->ApplyAmbientTint(activeTex);

		if (damageFlashTimer_ > 0.0f) {
			SDL_SetTextureColorMod(activeTex, 255, 100, 100);
		}

		// While hiding: gentle alpha pulse (80-180) to signal stealth
		Uint8 drawAlpha = 255;
		if (isHiding_ || isExitingHide_) {
			float pulse = 0.5f + 0.3f * sinf(hideAlphaTime_ * 0.004f);
			drawAlpha = static_cast<Uint8>(80.0f + 100.0f * pulse);
			SDL_SetTextureAlphaMod(activeTex, drawAlpha);
			SDL_SetTextureBlendMode(activeTex, SDL_BLENDMODE_BLEND);
		}

		render->DrawTexture(activeTex, drawX, drawY, animFrame, 1.0f, 0, INT_MAX, INT_MAX, flip, currentDrawScale);

		// Restore alpha and color modulation
		if (isHiding_ || isExitingHide_) {
			SDL_SetTextureAlphaMod(activeTex, 255);
		}
		if (damageFlashTimer_ > 0.0f) {
			SDL_SetTextureColorMod(activeTex, 255, 255, 255);
		}
		render->ResetAmbientTint(activeTex);
	}
}

void Player::Dash(float dt)
{
	auto& input = Engine::GetInstance().input;

	if (dashCooldown_ > 0.0f) dashCooldown_ -= dt;

	if (!isDashing_ && dashCooldown_ <= 0.0f &&
		input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_DOWN)
	{
		isDashing_ = true;
		dashTimer_ = DASH_DURATION;
		dashDirX_ = facingRight ? -1.0f : 1.0f;
		isInvincible_ = true;
		iFrameTimer_ = DASH_DURATION;
		LOG("Player dash started");
	}

	if (isDashing_)
	{
		dashTimer_ -= dt;
		if (dashTimer_ <= 0.0f)
		{
			isDashing_ = false;
			dashCooldown_ = DASH_COOLDOWN;
			LOG("Player dash ended");
		}
	}
}

void Player::Attack(float dt)
{
	if (isHiding_ || isExitingHide_) return;

	auto& input = Engine::GetInstance().input;
	auto& physics = Engine::GetInstance().physics;

	if (attackCooldown_ > 0.0f) attackCooldown_ -= dt;

	if (input->GetKey(SDL_SCANCODE_J) == KEY_DOWN && !isAttacking_ && attackCooldown_ <= 0.0f)
	{
		isAttacking_ = true;
		attackTimer_ = ATTACK_DURATION;
		attackCooldown_ = ATTACK_COOLDOWN;

		int bodyX, bodyY;
		pbody->GetPosition(bodyX, bodyY);

		int hitboxX = facingRight ? bodyX - (int)HITBOX_OFFSET : bodyX + (int)HITBOX_OFFSET;
		attackHitbox_ = physics->CreateRectangleSensor(hitboxX, bodyY, (int)HITBOX_W, (int)HITBOX_H, bodyType::STATIC);
		attackHitbox_->listener = this;
		attackHitbox_->ctype = ColliderType::ATTACK;
		LOG("Player attack started");
	}

	if (isAttacking_)
	{
		attackTimer_ -= dt;

		if (attackHitbox_ != nullptr)
		{
			int bodyX, bodyY;
			pbody->GetPosition(bodyX, bodyY);
			int hitboxX = facingRight ? bodyX - (int)HITBOX_OFFSET : bodyX + (int)HITBOX_OFFSET;
			attackHitbox_->SetPosition(hitboxX, bodyY);
		}

		if (attackTimer_ <= 0.0f)
		{
			isAttacking_ = false;
			if (attackHitbox_ != nullptr)
			{
				physics->DeletePhysBody(attackHitbox_);
				attackHitbox_ = nullptr;
			}
			LOG("Player attack ended");
		}
	}
}

void Player::TakeDamage(int damage)
{
	// Cannot take damage while hiding
	if (isInvincible_ || isHiding_) return;

	health -= damage;
	LOG("Player took %d damage -> health: %d", damage, health);

	isInvincible_ = true;
	iFrameTimer_ = IFRAME_DURATION;
	damageFlashTimer_ = DAMAGE_FLASH_DURATION;

	int playerX, playerY;
	pbody->GetPosition(playerX, playerY);

	float closestDist = 999999.0f;
	float enemyDirX = 0.0f;
	for (const auto& entity : Engine::GetInstance().entityManager->entities) {
		if ((entity->type == EntityType::ENEMY || entity->type == EntityType::ENEMY_B || entity->type == EntityType::ENEMY_C) && entity->active) {
			float dx = entity->position.getX() - (float)playerX;
			float dy = entity->position.getY() - (float)playerY;
			float dist = dx * dx + dy * dy;
			if (dist < closestDist) {
				closestDist = dist;
				enemyDirX = dx;
			}
		}
	}

	if (enemyDirX < 0)      facingRight = true;
	else if (enemyDirX > 0) facingRight = false;

	float knockbackForce = 5.0f;
	float dir = facingRight ? 1.0f : -1.0f;
	Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, dir * knockbackForce, -2.0f, true);

	if (health <= 0)
	{
		health = 0;
		isDead_ = true;
		isHiding_ = false;
		anims.SetCurrent("death");
		LOG("Player is dead");
	}
	else
	{
		isHiding_ = false;
		isShowingDamageAnim_ = true;
		anims.SetCurrent("damage");
		anims.ResetCurrent();
	}
}

void Player::Revive()
{
	isDead_ = false;
	isHiding_ = false;
	isInvincible_ = false;
	isShowingDamageAnim_ = false;
	hideAlphaTime_ = 0.0f;
	hideCooldown_ = 0.0f; // reset cooldown on revive
	anims.SetCurrent("idle");
	damageFlashTimer_ = 0.0f;
}

bool Player::CleanUp()
{
	LOG("Cleanup player");
	Engine::GetInstance().textures->UnLoad(texture);
	if (wakeUpTexture) Engine::GetInstance().textures->UnLoad(wakeUpTexture);
	if (climbTexture)  Engine::GetInstance().textures->UnLoad(climbTexture);

	if (attackHitbox_ != nullptr)
	{
		Engine::GetInstance().physics->DeletePhysBody(attackHitbox_);
		attackHitbox_ = nullptr;
	}
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	return true;
}

SDL_Rect Player::GetCurrentAnimationRect() const
{
	if (isWakingUp) return wakeUpAnim.GetCurrentFrame();
	return anims.GetCurrentFrame();
}

bool Player::IsFacingRight() const
{
	return facingRight;
}

void Player::OnCollision(PhysBody* physA, PhysBody* physB) {
	switch (physB->ctype)
	{
	case ColliderType::PLATFORM:
	{
		int playerX, playerY, platX, platY;
		physA->GetPosition(playerX, playerY);
		physB->GetPosition(platX, platY);
		if (playerY < platY) {
			if (isJumping) {
				// Spawn landing dust
				Engine::GetInstance().entityManager->SpawnVFX(
					Vector2D(position.getX() + (float)texW / 2.0f, position.getY() + (float)texH / 2.0f + 50.0f),
					"assets/textures/spritesheets/SS_Pols_01.png",
					47, 202, 202, 0.03f
				);
				// Allow jump animation to play the landing frames (after frame 7)
				if (anims.GetCurrentName() == "jump") {
					// We don't reset to idle immediately to let landing frames show
				} else {
					anims.SetCurrent("idle");
				}
			}
			isJumping = false;
			canDoubleJump = false;
			hasDoubleJumped = false;
		}
		break;
	}
	case ColliderType::ITEM:
		Engine::GetInstance().audio->PlayFx(pickCoinFxId);
		physB->listener->Destroy();
		break;
	case ColliderType::PROJECTILE:
		TakeDamage(1);
		if (physB->listener != nullptr)
			physB->listener->Destroy();
		break;
	case ColliderType::UNKNOWN:
		break;
	default:
		break;
	}
}

void Player::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
}

Vector2D Player::GetPosition() {
	int x, y;
	pbody->GetPosition(x, y);
	return Vector2D(static_cast<float>(x) - texW / 2.0f, static_cast<float>(y) - texH / 2.0f);
}

void Player::SetPosition(Vector2D pos) {
	pbody->SetPosition(static_cast<int>(pos.getX() + texW / 2.0f), static_cast<int>(pos.getY() + texH / 2.0f));
}

void Player::CheckLedge() {
	if (!isJumping || isClimbing_ || isHiding_) return;

	auto& physics = Engine::GetInstance().physics;
	int px, py;
	pbody->GetPosition(px, py);

	int dir = facingRight ? -1 : 1;

	int bodyRayStartX = px;
	int bodyRayStartY = py - LEDGE_RAY_MARGIN;
	int bodyRayEndX = px + dir * LEDGE_RAY_REACH;
	int bodyRayEndY = bodyRayStartY;

	int headRayStartX = px;
	int headRayStartY = py - LEDGE_HEAD_OFFSET;
	int headRayEndX = px + dir * LEDGE_RAY_REACH;
	int headRayEndY = headRayStartY;

	float bodyHitX, bodyHitY, headHitX, headHitY;
	bool bodyHit = physics->RayCastWorld(bodyRayStartX, bodyRayStartY, bodyRayEndX, bodyRayEndY, bodyHitX, bodyHitY);
	bool headHit = physics->RayCastWorld(headRayStartX, headRayStartY, headRayEndX, headRayEndY, headHitX, headHitY);

	if (bodyHit && !headHit) {
		isClimbing_ = true;

		physics->SetLinearVelocity(pbody, 0.0f, 0.0f);
		b2Body_SetGravityScale(pbody->body, 0.0f);

		climbTargetX_ = bodyHitX + dir * 30.0f;
		climbTargetY_ = (float)headRayStartY - 20.0f;

		climbAnims.SetCurrent("climb");
		climbAnims.ResetCurrent();

		LOG("Ledge detected via raycast!");
	}
}

void Player::UpdateClimb(float dt) {
	climbAnims.Update(dt);

	if (climbAnims.HasFinishedOnce("climb")) {
		pbody->SetPosition((int)climbTargetX_, (int)climbTargetY_);
		b2Body_SetGravityScale(pbody->body, 1.0f);

		isClimbing_ = false;
		isJumping = false;
		canDoubleJump = false;
		hasDoubleJumped = false;
		anims.SetCurrent("idle");

		LOG("Ledge climb completed");
	}
}

bool Player::Destroy()
{
	LOG("Destroying Player");
	active = false;
	pendingToDelete = true;
	return true;
}