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
#include "SlingshotProjectile.h"
#include "tracy/Tracy.hpp"
#include "Door.h"

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
		{35, "turnaround"},
		{70, "run"},
		{88, "stoprun"},
		{105, "jump"},
		{140, "damage"},
		{175, "death"},
		{209, "hide"}
	};
	anims.LoadFromTSX("assets/textures/animations/protagonistSpritesheetNew.xml", aliases);
	anims.SetCurrent("idle");

	anims.SetLoop("turnaround", false);
	anims.SetLoop("jump", false);
	anims.SetLoop("damage", false);
	anims.SetLoop("death", false);
	anims.SetLoop("stoprun", false);
	anims.SetLoop("hide", false); // Play once and freeze on last frame

	texture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/protagonistSpritesheetNew.png");

	wakeUpTexture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS Individual/SS_Despertar.png");
	for (int i = 0; i < 51; ++i) {
		SDL_Rect r = { i * 258, 0, 258, 258 };
		wakeUpAnim.AddFrame(r, 120);
	}
	wakeUpAnim.SetLoop(false);
	isWakingUp = true;

	texW = 128;
	texH = 128;
	drawScale = 0.5f;

	// Load push animation spritesheet (256x256 tiles, 5 columns, 20 frames)
	pushTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS Individual/spritesheetempujarcaja.png");
	for (int i = 0; i < 20; ++i) {
		SDL_Rect r = { (i % 5) * 256, (i / 5) * 256, 256, 256 };
		pushAnim_.AddFrame(r, 120);  // slow frame rate for effortful feel
	}
	pushAnim_.SetLoop(true);

	// Load slingshot shoot animation spritesheet (3072x1024 = 12 columns x 4 rows, 256x256 frames)
	slingshotShootTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS Individual/SS_Tiraxines.png");
	for (int i = 0; i < 48; ++i) {
		SDL_Rect r = { (i % 12) * 256, (i / 12) * 256, 256, 256 };
		slingshotAnim_.AddFrame(r, 80);
	}
	slingshotAnim_.SetLoop(false);

	pbody = Engine::GetInstance().physics->CreateCapsule((int)position.getX(), (int)position.getY(), 40, 100, bodyType::DYNAMIC, 0.0f);

	pbody->listener = this;
	pbody->ctype = ColliderType::PLAYER;

	pickCoinFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/coin-collision-sound-342335.wav");
	jumpFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/jump.wav");
	stepsFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/steps.wav");
	gameOverFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/game-over.wav");

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

	if (knockbackTimer_ > 0.0f) {
		knockbackTimer_ -= dt;
		velocity.x = knockbackX_;
	}

	// Tick hide cooldown
	if (hideCooldown_ > 0.0f) hideCooldown_ -= dt;
	if (slingshotCooldown_ > 0.0f) slingshotCooldown_ -= dt;

	if (!isDead_ && !isWakingUp)
	{
		if (!isShowingDamageAnim_) {
			// Hide must be evaluated first so it can block other actions
			Hide(dt);

			// All other actions are blocked while hiding
			if (!isHiding_) {
				if (!isExitingHide_) Move();
				Jump();
				if (!isPushing_) Attack(dt);
				if (!isPushing_ && !isAttacking_) Slingshot(dt);
				Teleport();
			}
		}
	}

	ApplyPhysics();

	if (velocity.x != 0.0f && !isJumping && !isDead_ && !isShowingDamageAnim_ && !isHiding_ && !isExitingHide_ && !isWakingUp) {
		stepTimer_ -= dt;

		
		if (stepTimer_ <= 0.0f) {
			Engine::GetInstance().audio->PlayFx(stepsFxId);
			stepTimer_ = STEP_COOLDOWN;
		}
	}
	else {
		
		stepTimer_ = 0.0f;
	}

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

	auto& input = Engine::GetInstance().input;
	bool moveLeft = input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT || input->GetLeftStickX() < -0.2f;
	bool moveRight = input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT || input->GetLeftStickX() > 0.2f;

	// Determine effective speed (slower while pushing)
	float effectiveSpeed = speed;
	if (isPushing_) {
		effectiveSpeed = speed * PUSH_SPEED_FACTOR;
	}

	if (moveLeft) {
		velocity.x = -effectiveSpeed;
		if (!facingRight) {
			facingRight = true;
			if (!isJumping && !isPushing_) anims.SetCurrent("turnaround");
		}

		if (!isJumping && !isPushing_) {
			if (anims.GetCurrentName() != "turnaround" || anims.HasFinishedOnce("turnaround")) {
				if (anims.Has("run")) anims.SetCurrent("run");
				else anims.SetCurrent("idle");
			}
		}
	}
	else if (moveRight) {
		velocity.x = effectiveSpeed;
		if (facingRight) {
			facingRight = false;
			if (!isJumping && !isPushing_) anims.SetCurrent("turnaround");
		}

		if (!isJumping && !isPushing_) {
			if (anims.GetCurrentName() != "turnaround" || anims.HasFinishedOnce("turnaround")) {
				if (anims.Has("run")) anims.SetCurrent("run");
				else anims.SetCurrent("idle");
			}
		}
	}
	else {
		// Player released keys — not pushing anymore
		if (isPushing_) {
			// Keep isPushing_ true only while actively pressing toward the rock
			// The collision contact may still exist but we're not pushing
		}
		if (!isJumping) {
			if (anims.GetCurrentName() == "jump" && !anims.HasFinishedOnce("jump")) {
				// let landing animation finish
			}
			else if (anims.GetCurrentName() == "run") {
				anims.SetCurrent("stoprun");
				anims.ResetCurrent();
			}
			else if (anims.GetCurrentName() == "stoprun") {
				if (anims.HasFinishedOnce("stoprun")) {
					anims.SetCurrent("idle");
				}
			}
			else if (anims.GetCurrentName() != "turnaround" || anims.HasFinishedOnce("turnaround")) {
				anims.SetCurrent("idle");
			}
		}
	}
}

void Player::Jump() {
	if (isWakingUp || isShowingDamageAnim_ || isHiding_ || isExitingHide_) return;

	auto& input = Engine::GetInstance().input;
	bool jumpDown = input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN ||
	                input->GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_DOWN;
	bool jumpUp   = input->GetKey(SDL_SCANCODE_SPACE) == KEY_UP ||
	                input->GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_UP;

	if (jumpDown) {
		if (!isJumping) {
			Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, 0.0f, -jumpForce, true);
			if (anims.Has("jump")) anims.SetCurrent("jump");
			isJumping = true;

			Engine::GetInstance().audio->PlayFx(jumpFxId);

			// Spawn jump dust (Centered at bottom of 100px capsule)
			Engine::GetInstance().entityManager->SpawnVFX(
				Vector2D(position.getX(), position.getY() + 50.0f),
				"assets/textures/spritesheets/SS_Pols_01.png",
				12, 794, 202, 0.015f, 0.0f, 0.2f
			);
		}
	}

	if (jumpUp && isJumping) {
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
	if (isWakingUp || isShowingDamageAnim_ || isDead_) return;

	auto& input = Engine::GetInstance().input;
	bool hideDown = input->GetKey(SDL_SCANCODE_H) == KEY_DOWN ||
	                input->GetGamepadButton(SDL_GAMEPAD_BUTTON_NORTH) == KEY_DOWN;

	// Cannot hide without the blanket (cape collectible)
	if (!hasBlanket_) {
		if (hideDown) {
			Engine::GetInstance().scene->ShowNoCapeNotification();
		}
		return;
	}

	if (hideDown)
	{
		if (!isHiding_ && !isExitingHide_)
		{
			// Blocked while on cooldown
			if (hideCooldown_ > 0.0f)
			{
				LOG("Hide on cooldown: %.0f ms remaining", hideCooldown_);
				return;
			}

			isHiding_ = true;
			velocity.x = 0.0f;
			Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
			anims.SetCurrent("hide");
			anims.ResetCurrent();
			LOG("Player hiding");
		}
		else if (isHiding_)
		{
			isHiding_ = false;
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

// ─────────────────────────────────────────────────────────────────────────────
// Slingshot mechanic
//   • Left mouse button press: begin charging
//   • Hold: accumulate charge, compute aim angle from player→mouse
//   • Release: fire projectile with charge-scaled speed
//   • Visual: trajectory preview dots + charge bar + slingshot animation
// ─────────────────────────────────────────────────────────────────────────────
void Player::Slingshot(float dt)
{
	if (!hasSlingshot_) return;

	if (isWakingUp || isShowingDamageAnim_ || isDead_ || isHiding_ || isExitingHide_)
	{
		isAiming_ = false;
		chargeTimer_ = 0.0f;
		return;
	}

	auto& input = Engine::GetInstance().input;
	auto& render = Engine::GetInstance().render;

	KeyState lmb = input->GetMouseButtonDown(SDL_BUTTON_LEFT);

	if (lmb == KEY_DOWN && !isAiming_ && slingshotCooldown_ <= 0.0f)
	{
		isAiming_ = true;
		chargeTimer_ = 0.0f;
		slingshotAnim_.Reset();
		LOG("Slingshot aiming started");
	}

	if (isAiming_)
	{
		chargeTimer_ += dt;
		if (chargeTimer_ > MAX_CHARGE_TIME) chargeTimer_ = MAX_CHARGE_TIME;

		// Calculate aim angle from player center to mouse world position
		int playerX, playerY;
		pbody->GetPosition(playerX, playerY);

		Vector2D mouseScreen = input->GetMousePosition();
		// Convert screen mouse pos to world coords
		float mouseWorldX = mouseScreen.getX() - (float)render->camera.x;
		float mouseWorldY = mouseScreen.getY() - (float)render->camera.y;

		float dx = mouseWorldX - (float)playerX;
		float dy = mouseWorldY - (float)playerY;
		aimAngle_ = std::atan2(dy, dx);

		// Update facing direction based on aim
		if (dx > 0) facingRight = false;
		else if (dx < 0) facingRight = true;

		// Advance slingshot animation based on charge (map charge ratio to frame)
		slingshotAnim_.Update(dt);

		// Draw trajectory preview dots (parabolic prediction)
		float chargeRatio = chargeTimer_ / MAX_CHARGE_TIME;
		float launchSpeed = MIN_LAUNCH_SPEED + (MAX_LAUNCH_SPEED - MIN_LAUNCH_SPEED) * chargeRatio;

		float dirX = std::cos(aimAngle_);
		float dirY = std::sin(aimAngle_);

		// Simulate trajectory (in meters, then convert to pixels)
		float simVx = dirX * launchSpeed;
		float simVy = dirY * launchSpeed;
		float gravity = 10.0f; // matches GRAVITY_Y magnitude

		for (int i = 1; i <= 5; i++)
		{
			float t = (float)i * 0.12f; // time steps
			float px = (float)playerX + METERS_TO_PIXELS(simVx * t);
			float py = (float)playerY + METERS_TO_PIXELS(simVy * t + 0.5f * gravity * t * t);

			Uint8 dotAlpha = (Uint8)(120 - i * 16);
			if (dotAlpha < 30) dotAlpha = 30;

			render->DrawCircle((int)px, (int)py, 3, 200, 180, 140, dotAlpha, true);
		}

		// Draw subtle charge bar near player feet
		int barW = 30;
		int barH = 3;
		int barX = playerX - barW / 2;
		int barY = playerY + 55;
		int fillW = (int)((float)barW * chargeRatio);

		// Background bar
		SDL_Rect bgBar = { barX, barY, barW, barH };
		render->DrawRectangle(bgBar, 40, 35, 25, 80, true, true);

		// Fill bar (amber tone)
		if (fillW > 0) {
			SDL_Rect fillBar = { barX, barY, fillW, barH };
			Uint8 fillAlpha = (Uint8)(60.0f + 140.0f * chargeRatio);
			render->DrawRectangle(fillBar, 200, 170, 100, fillAlpha, true, true);
		}

		// Stop movement while aiming
		velocity.x = 0.0f;

		if (lmb == KEY_UP)
		{
			// Fire!
			isAiming_ = false;
			slingshotCooldown_ = SLINGSHOT_COOLDOWN;

			// Spawn projectile
			auto proj = std::dynamic_pointer_cast<SlingshotProjectile>(
				Engine::GetInstance().entityManager->CreateEntity(EntityType::SLINGSHOT_PROJECTILE));

			// Spawn fully outside the player's collision body for all aim directions.
			// The player body is much taller than the old 30px offset, so vertical and
			// diagonal shots could start inside the player and immediately self-collide.
			const float projectileSpawnClearance = 70.0f;
			float spawnOffsetX = std::cos(aimAngle_) * projectileSpawnClearance;
			float spawnOffsetY = std::sin(aimAngle_) * projectileSpawnClearance;
			proj->position = Vector2D((float)playerX + spawnOffsetX, (float)playerY - 10.0f + spawnOffsetY);

			proj->SetLaunch(dirX, dirY, launchSpeed);
			proj->Start();

			LOG("Slingshot fired! angle=%.1f speed=%.1f", aimAngle_ * RADTODEG, launchSpeed);
		}
	}
}
void Player::ApplyPhysics() {
	if (isJumping == true) {
		velocity.y = Engine::GetInstance().physics->GetYVelocity(pbody);
	}

	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);

	// Synchronize rock velocity to avoid jitter and clipping
	if (isPushing_ && pushedRockBody_ != nullptr) {
		// Only synchronize when pushing towards the rock or stopping
		if ((pushDir_ == 1.0f && velocity.x > 0.0f) || 
		    (pushDir_ == -1.0f && velocity.x < 0.0f) || 
		    velocity.x == 0.0f) {
			Engine::GetInstance().physics->SetXVelocity(pushedRockBody_, velocity.x);
		}
	}
}

void Player::Draw(float dt) {

	SDL_Texture* activeTex = texture;
	const SDL_Rect* animFrame = nullptr;
	float currentDrawScale = drawScale;

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
	else if (isPushing_ && velocity.x != 0.0f && !isJumping)
	{
		// Push animation — use dedicated spritesheet
		pushAnim_.Update(dt);
		activeTex = pushTexture_;
		animFrame = &pushAnim_.GetCurrentFrame();
		// The push frame is 256x256, we want it to appear the same size as
		// the normal 128x128 walk animation, so we halve the draw scale.
		currentDrawScale = 0.5f;
	}
	else if (isAiming_ && slingshotShootTexture_)
	{
		// Slingshot aiming animation — use dedicated spritesheet
		activeTex = slingshotShootTexture_;
		animFrame = &slingshotAnim_.GetCurrentFrame();
		// 256x256 frames -> same half scale as push
		currentDrawScale = 0.5f;
	}
	else 
	{
		bool shouldUpdate = true;
		if (isJumping && anims.GetCurrentName() == "jump") {
			// Freeze on peak frame (tile 118 is the peak frame in the new spritesheet)
			if (anims.GetCurrentFrameIndex() >= 13) {
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


	int currentFrameW = animFrame ? animFrame->w : texW;
	int currentFrameH = animFrame ? animFrame->h : texH;
	int drawX = static_cast<int>(position.getX() - (static_cast<float>(currentFrameW) * currentDrawScale) / 2.0f);
	int drawY = static_cast<int>(position.getY() - (static_cast<float>(currentFrameH) * currentDrawScale) / 2.0f);

	// Adjust drawing position when pushing so hands align with the edge of the rock visually
	if (isPushing_ && velocity.x != 0.0f && !isJumping) {
		drawX -= static_cast<int>(pushDir_ * 35.0f);
	}

	bool spriteNativeRight = false;
	const std::string& animName = anims.GetCurrentName();
	if (animName == "jump" || animName == "turnaround") {
		spriteNativeRight = true;
	}

	SDL_FlipMode flip;
	if (isPushing_ && velocity.x != 0.0f && !isJumping) {
		// Push spritesheet character faces LEFT natively
		// When pushing right (velocity.x > 0), flip horizontally
		flip = (velocity.x > 0.0f) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	}
	else if (spriteNativeRight) {
		flip = facingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	}
	else {
		flip = facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
	}

	if (isWakingUp) {
		if (wakeUpAnimStarted) {
			wakeUpAnim.Update(dt);
			if (wakeUpAnim.HasFinishedOnce()) {
				isWakingUp = false;
			}
		}
		if (isWakingUp) {
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

	// ── I-frame flicker (not during hide or death) ────────────────────
	bool skipDraw = !isDead_ && !isHiding_ && !isExitingHide_ && isInvincible_ &&
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

void Player::Attack(float dt)
{
	if (isHiding_ || isExitingHide_) return;

	auto& input = Engine::GetInstance().input;
	auto& physics = Engine::GetInstance().physics;

	if (attackCooldown_ > 0.0f) attackCooldown_ -= dt;

	bool attackDown = input->GetKey(SDL_SCANCODE_J) == KEY_DOWN ||
	                  input->GetGamepadButton(SDL_GAMEPAD_BUTTON_WEST) == KEY_DOWN;

	if (attackDown && !isAttacking_ && attackCooldown_ <= 0.0f)
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
		Engine::GetInstance().audio->PlayMusic(nullptr);
		Engine::GetInstance().audio->PlayFx(gameOverFxId);
		LOG("Player is dead");
	}
	else
	{
		isHiding_ = false;
		if (suppressDamageAnim_)
		{
			suppressDamageAnim_ = false;
		}
		else
		{
			isShowingDamageAnim_ = true;
			anims.SetCurrent("damage");
			anims.ResetCurrent();
		}
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
	isAiming_ = false;
	chargeTimer_ = 0.0f;
	slingshotCooldown_ = 0.0f;
	anims.SetCurrent("idle");
	damageFlashTimer_ = 0.0f;
}

bool Player::CleanUp()
{
	LOG("Cleanup player");
	Engine::GetInstance().textures->UnLoad(texture);
	if (wakeUpTexture) Engine::GetInstance().textures->UnLoad(wakeUpTexture);

	if (attackHitbox_ != nullptr)
	{
		Engine::GetInstance().physics->DeletePhysBody(attackHitbox_);
		attackHitbox_ = nullptr;
	}
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	if (pushTexture_) {
		Engine::GetInstance().textures->UnLoad(pushTexture_);
		pushTexture_ = nullptr;
	}
	if (slingshotShootTexture_) {
		Engine::GetInstance().textures->UnLoad(slingshotShootTexture_);
		slingshotShootTexture_ = nullptr;
	}
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

		// Check if the player lands on top of the platform
		if (playerY < platY) {
			if (isJumping) {
				// Spawn landing dust (Centered at bottom of capsule)
				Engine::GetInstance().entityManager->SpawnVFX(
					Vector2D(position.getX(), position.getY() + 50.0f),
					"assets/textures/spritesheets/SS_Pols_01.png",
					12, 794, 202, 0.03f, 0.0f, 0.2f
				);
				// Allow jump animation to play the landing frames (after frame 7)
				if (anims.GetCurrentName() == "jump") {
					// We don't reset to idle immediately to let landing frames show
				} else {
					anims.SetCurrent("idle");
				}
			}
			isJumping = false;
		}
		break;
	}
	case ColliderType::ITEM:
		Engine::GetInstance().audio->PlayFx(pickCoinFxId);
		physB->listener->Destroy();
		keys++;
		break;
	case ColliderType::DOOR:
	{
		Door* door = (Door*)physB->listener;
		if (door != nullptr && !door->isOpen && keys > 0) {
			door->Open();
			keys--;
		}
		break;
	}
	case ColliderType::PROJECTILE:
		TakeDamage(1);
		if (physB->listener != nullptr)
			physB->listener->Destroy();
		break;
	case ColliderType::UNKNOWN:
		break;
	case ColliderType::PUSH_ROCK:
	{
		// Detect lateral push — only if player is roughly beside the rock, not on top
		int playerX, playerY, rockX, rockY;
		physA->GetPosition(playerX, playerY);
		physB->GetPosition(rockX, rockY);

		int vertDiff = abs(playerY - rockY);
		// Only count as push if the player is at roughly the same height (not landing on top)
		if (vertDiff < 50) {
			pushContactCount_++;
			isPushing_ = true;
			pushDir_ = (playerX < rockX) ? 1.0f : -1.0f;
			pushAnim_.Reset();
			pushedRockBody_ = physB;
		}
		else {
			// Player is above the rock — treat as platform for landing
			if (playerY < rockY) {
				if (isJumping) {
					if (anims.GetCurrentName() != "jump") {
						anims.SetCurrent("idle");
					}
				}
				isJumping = false;
				
			}
		}
		break;
	}
	default:
		break;
	}
}

void Player::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	if (physB->ctype == ColliderType::PUSH_ROCK) {
		pushContactCount_--;
		if (pushContactCount_ <= 0) {
			pushContactCount_ = 0;
			isPushing_ = false;
			pushedRockBody_ = nullptr;
		}
	}
}

Vector2D Player::GetPosition() {
	int x, y;
	pbody->GetPosition(x, y);
	return Vector2D(static_cast<float>(x) - texW / 2.0f, static_cast<float>(y) - texH / 2.0f);
}

void Player::SetPosition(Vector2D pos) {
	pbody->SetPosition(static_cast<int>(pos.getX() + texW / 2.0f), static_cast<int>(pos.getY() + texH / 2.0f));
}

bool Player::Destroy()
{
	LOG("Destroying Player");
	active = false;
	pendingToDelete = true;
	return true;
}