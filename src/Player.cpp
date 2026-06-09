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
#include "Platform.h"
#include <algorithm>

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

	wakeUpTexture = Engine::GetInstance().textures->Load("assets/textures/spritesheets/Spritesheets Prota i Oso color/Aixecant-se/ss_Aixecant-se_definitiu.png");

	// Track the grid dimensions for the new 2560x2560 stacked texture
	int columns = 10;

	for (int i = 0; i < 93; ++i) {
		// Calculate row and column indices for a 10x10 layout
		int col = i % columns;
		int row = i / columns;

		SDL_Rect r = { col * 256, row * 256, 256, 256 };
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
	// Phase 1 (charge): row 3 — 12 frames, plays once
	// Phase 2 (hold):   row 0 — 6 frames, loops while player keeps holding
	slingshotShootTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS Individual/SS_Tiraxines.png");
	for (int col = 0; col < 12; ++col) {
		SDL_Rect r = { col * 256, 3 * 256, 256, 256 }; // row 3
		slingshotAnim_.AddFrame(r, 80);
	}
	slingshotAnim_.SetLoop(false);
	for (int col = 0; col < 6; ++col) {
		SDL_Rect r = { col * 256, 0 * 256, 256, 256 }; // row 0
		slingshotHoldAnim_.AddFrame(r, 80);
	}
	slingshotHoldAnim_.SetLoop(true);

	// Load Bear Animations from XML
	bearAppearTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/Spritesheets Prota i Oso color/SS_oso_apareixent-Pintado.png");
	bearAppearAnims_.LoadFromTSX("assets/textures/animations/bearAppear.xml", { {0, "appear"} });
	bearAppearAnims_.SetCurrent("appear");
	bearAppearAnims_.SetLoop("appear", false);

	bearIdleTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/Spritesheets Prota i Oso color/SS_oso_idle-Pintado.png");
	bearIdleAnims_.LoadFromTSX("assets/textures/animations/bearIdle.xml", { {0, "idle"} });
	bearIdleAnims_.SetCurrent("idle");
	bearIdleAnims_.SetLoop("idle", true);

	bearWalkTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/Spritesheets Prota i Oso color/SS_oso_caminant-Pintado.png");
	bearWalkAnims_.LoadFromTSX("assets/textures/animations/bearWalk.xml", { {0, "walk"} });
	bearWalkAnims_.SetCurrent("walk");
	bearWalkAnims_.SetLoop("walk", true);

	bearAttackTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/Spritesheets Prota i Oso color/SS_oso_atacant-Pintado.png");
	bearAttackAnims_.LoadFromTSX("assets/textures/animations/bearAttack.xml", { {0, "attack"} });
	bearAttackAnims_.SetCurrent("attack");
	bearAttackAnims_.SetLoop("attack", false);

	bearDeathTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/Spritesheets Prota i Oso color/SS_oso_palmant-Pintado.png");
	bearDeathAnims_.LoadFromTSX("assets/textures/animations/bearDeath.xml", { {0, "death"} });
	bearDeathAnims_.SetCurrent("death");
	bearDeathAnims_.SetLoop("death", false);

	throwBearTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/Spritesheets Prota i Oso color/SS_llencar_oso.png");
	if (throwBearTexture_) {
		SDL_SetTextureScaleMode(throwBearTexture_, SDL_SCALEMODE_NEAREST);
	}
	throwBearAnims_.LoadFromTSX("assets/textures/animations/protagonistThrowBear.xml", { {24, "ThrowStart"}, {0, "BearActiveIdle"}, {72, "FallAsleep"}, {48, "SleepIdle"} });
	LOG("throwBearAnims_ loaded. Has ThrowStart: %d, BearActiveIdle: %d, FallAsleep: %d, SleepIdle: %d",
		throwBearAnims_.Has("ThrowStart"), throwBearAnims_.Has("BearActiveIdle"),
		throwBearAnims_.Has("FallAsleep"), throwBearAnims_.Has("SleepIdle"));
	throwBearAnims_.SetCurrent("ThrowStart");
	throwBearAnims_.SetLoop("ThrowStart", false);
	throwBearAnims_.SetLoop("BearActiveIdle", false);
	throwBearAnims_.SetLoop("FallAsleep", false);
	throwBearAnims_.SetLoop("SleepIdle", true);

	// Load yoyo trap animation (played when caught by Stitchling)
	yoyoTrapTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS Individual/SS_dany_yoyo.png");
	yoyoTrapAnims_.LoadFromTSX("assets/textures/animations/protagonistYoyo.xml", { {0, "yoyo"} });
	yoyoTrapAnims_.SetCurrent("yoyo");
	yoyoTrapAnims_.SetLoop("yoyo", true);

	// Load Drop Doll minigame animation (player squirms while grabbed — 2048x2048 = 8x8 grid, 256x256 frames)
	dollGrabbedTexture_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_prota_dropdoll.png");
	for (int i = 0; i < 64; ++i) {
		SDL_Rect r = { (i % 8) * 256, (i / 8) * 256, 256, 256 };
		dollGrabbedAnim_.AddFrame(r, 80);
	}
	dollGrabbedAnim_.SetLoop(true);

	hasStuffedAnimal_ = false;
	equippedItem_ = EquippedItem::NONE;

	pbody = Engine::GetInstance().physics->CreateCapsule((int)position.getX(), (int)position.getY(), 40, 100, bodyType::DYNAMIC, 0.0f);

	pbody->listener = this;
	pbody->ctype = ColliderType::PLAYER;

	pickCoinFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/coin-collision-sound-342335.wav");
	jumpFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/jump2.wav");
	landFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/land.wav");
	/*stepsFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/steps.wav");*/
	gameOverFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/game-over.wav");
	slingshotFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/tirachinas.wav");
	capeFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/capa.wav");
	// Comprobar en qué nivel estamos y cargar su sonido de pasos correspondiente
	int nivelActual = Engine::GetInstance().scene->GetCurrentLevelIndex();

	if (nivelActual == 0) {
		stepsFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/pasos_level1_1.wav");
	}
	else if (nivelActual == 1) {
		stepsFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/pasos_level2_2.wav");
	}
	else if (nivelActual == 2) {
		stepsFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/pasos_level3_3.wav");
	}
	else if (nivelActual == 3) {
		stepsFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/pasos_level4_4.wav");
	}
	return true;
}

bool Player::Update(float dt)
{
	ZoneScoped;

	if (Engine::GetInstance().scene->isPaused_) {
		Draw(0.0f);
		return true;
	}

	if (Engine::GetInstance().scene->IsCheckpointTransitionActive()) {
		Draw(0.0f);
		return true;
	}

	GetPhysicsValues();

	if (platformDropTimer_ > 0.0f) {
		platformDropTimer_ -= dt;
		if (platformDropTimer_ <= 0.0f) {
			platformBelow = nullptr;
		}
	}

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F10) == KEY_DOWN) {
		godMode_ = !godMode_;
		b2Body_SetGravityScale(pbody->body, godMode_ ? 0.0f : 1.0f);

		b2Filter filter = b2DefaultFilter();
		if (godMode_) {
			filter.maskBits = 0x00000000;
		}

		int shapeCount = b2Body_GetShapeCount(pbody->body);
		b2ShapeId shapes[10];
		b2Body_GetShapes(pbody->body, shapes, shapeCount);
		for (int i = 0; i < shapeCount; i++) {
			b2Shape_SetFilter(shapes[i], filter);
		}
		LOG("God Mode: %s", godMode_ ? "ON" : "OFF");

		if (godMode_) {
			isJumping = false;
		} else {
			// Exiting godmode: zero velocity so godspeed doesn't bleed into first normal frame
			Engine::GetInstance().physics->SetLinearVelocity(pbody, 0.0f, 0.0f);
			velocity.x = 0.0f;
			velocity.y = 0.0f;
			// Reset animation from "jump" (forced by godmode) to "idle" so the sprite
			// flip convention doesn't invert when Move() switches to "run"
			anims.SetCurrent("idle");
		}
	}

	if (godMode_) {
		auto& input = Engine::GetInstance().input;
		float godSpeed = 12.0f;
		velocity.x = 0.0f;
		velocity.y = 0.0f;

		if (input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT || input->GetKey(SDL_SCANCODE_UP) == KEY_REPEAT || input->GetLeftStickY() < -0.2f) {
			velocity.y = -godSpeed;
		}
		if (input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT || input->GetKey(SDL_SCANCODE_DOWN) == KEY_REPEAT || input->GetLeftStickY() > 0.2f) {
			velocity.y = godSpeed;
		}
		if (input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT || input->GetKey(SDL_SCANCODE_LEFT) == KEY_REPEAT || input->GetLeftStickX() < -0.2f) {
			velocity.x = -godSpeed;
			facingRight = true;  // facingRight=true means facing LEFT (matching Move() convention)
		}
		if (input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT || input->GetKey(SDL_SCANCODE_RIGHT) == KEY_REPEAT || input->GetLeftStickX() > 0.2f) {
			velocity.x = godSpeed;
			facingRight = false; // facingRight=false means facing RIGHT
		}

		Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity.x, velocity.y);
		anims.SetCurrent("jump");

		Draw(dt);
		return true;
	}

	if (knockbackTimer_ > 0.0f) {
		knockbackTimer_ -= dt;
		velocity.x = knockbackX_;
	}

	// Tick hide cooldown
	if (hideCooldown_ > 0.0f) hideCooldown_ -= dt;
	if (slingshotCooldown_ > 0.0f) slingshotCooldown_ -= dt;
	if (bearCooldownTimer_ > 0.0f) bearCooldownTimer_ -= dt;

	if (!isDead_ && !isWakingUp)
	{
		// Fast-swap equipped items during gameplay (blocked in bear mode)
		auto& input = *Engine::GetInstance().input;
		bool canSwap = !isBearMode_ && !isBearTransforming_ && !isThrowingBear_ && !isKidSleeping_;
		if (canSwap) {
			if (input.GetKey(SDL_SCANCODE_1) == KEY_DOWN && hasBlanket_) {
				equippedItem_ = EquippedItem::BLANKET;
				Engine::GetInstance().audio->PlayFx(Engine::GetInstance().scene->GetMenuClickFxId());
			}
			if (input.GetKey(SDL_SCANCODE_2) == KEY_DOWN && hasSlingshot_) {
				equippedItem_ = EquippedItem::SLINGSHOT;
				Engine::GetInstance().audio->PlayFx(Engine::GetInstance().scene->GetMenuClickFxId());
			}
			if (input.GetKey(SDL_SCANCODE_3) == KEY_DOWN && hasStuffedAnimal_) {
				equippedItem_ = EquippedItem::STUFFED_ANIMAL;
				Engine::GetInstance().audio->PlayFx(Engine::GetInstance().scene->GetMenuClickFxId());
			}
		}

		// Toggle bear mode (blocked during throw, transform, and sleep transitions)
		bool bearToggleDown = false;
		if (!isBearTransforming_ && !isThrowingBear_ && !isKidSleeping_) {
			bearToggleDown = input.GetKey(SDL_SCANCODE_O) == KEY_DOWN ||
			                 input.GetGamepadButton(SDL_GAMEPAD_BUTTON_EAST) == KEY_DOWN;
		}

		if (bearToggleDown) {
			if (!hasStuffedAnimal_) {
				Engine::GetInstance().scene->ShowNoBearNotification(false);
			}
			else if (equippedItem_ != EquippedItem::STUFFED_ANIMAL) {
				Engine::GetInstance().scene->ShowNoBearNotification(true);
			}
		}

		if (bearToggleDown && hasStuffedAnimal_ && equippedItem_ == EquippedItem::STUFFED_ANIMAL &&
			!isShowingDamageAnim_ && !isHiding_ && !isExitingHide_ && !isAiming_)
		{
			if (!isBearMode_ && !isBearTransforming_ && !isThrowingBear_ && !isKidSleeping_ && bearCooldownTimer_ <= 0.0f) {
				isThrowingBear_ = true;
				bearHealth_ = 3;
				throwBearAnims_.SetCurrent("ThrowStart");
				throwBearAnims_.ResetCurrent();
				velocity.x = 0.0f;
				Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);

				int bodyX, bodyY;
				pbody->GetPosition(bodyX, bodyY);
				bearSummonPosition_ = Vector2D((float)bodyX, (float)bodyY);
				bearSummonFacingRight_ = facingRight;

				Engine::GetInstance().audio->PlayFx(Engine::GetInstance().scene->GetMenuClickFxId());
				LOG("Player starting bear throw");
			}
			else if (isBearMode_ && !isBearTransforming_ && !isAttacking_) {
				// Exit bear mode: kid wakes up and bear goes away
				isBearMode_ = false;
				isBearTransforming_ = false;
				isKidSleeping_ = true;
				bearSleepTimer_ = 2000.0f; // 2 seconds sleep
				bearCooldownTimer_ = 5000.0f; // 5 seconds cooldown
				throwBearAnims_.SetCurrent("FallAsleep");
				throwBearAnims_.ResetCurrent();

				// Teleport player physics body back to kid position
				pbody->SetPosition((int)bearSummonPosition_.getX(), (int)bearSummonPosition_.getY());
				
				// Restore facing direction to when the bear was summoned
				facingRight = bearSummonFacingRight_;
				
				Engine::GetInstance().audio->PlayFx(Engine::GetInstance().scene->GetMenuClickFxId());
				LOG("Player exited bear mode, kid is sleeping at summon position");
			}
		}

		if (!isShowingDamageAnim_) {
			if (isThrowingBear_) {
				velocity.x = 0.0f;
				Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
				throwBearAnims_.Update(dt);
				if (throwBearAnims_.HasFinishedOnce("ThrowStart")) {
					isThrowingBear_ = false;
					isBearTransforming_ = true;
					bearAppearAnims_.ResetCurrent();

					// Teleport bear to a far position in the direction the player is facing
					float spawnOffset = 300.0f; // Distance from the player
					float targetX = bearSummonPosition_.getX();
					if (bearSummonFacingRight_) {
						// bearSummonFacingRight_ = true means facing left
						targetX -= spawnOffset;
					} else {
						// bearSummonFacingRight_ = false means facing right
						targetX += spawnOffset;
					}

					float hitX = 0.0f, hitY = 0.0f;
					if (Engine::GetInstance().physics->RayCastWorld(
						(int)bearSummonPosition_.getX(), (int)bearSummonPosition_.getY(),
						(int)targetX, (int)bearSummonPosition_.getY(),
						hitX, hitY))
					{
						// There's a wall or obstacle! Spawn slightly back from the hit point
						if (bearSummonFacingRight_) {
							targetX = hitX + 50.0f;
						} else {
							targetX = hitX - 50.0f;
						}
					}

					pbody->SetPosition((int)targetX, (int)bearSummonPosition_.getY());
					LOG("Player starting bear transformation at far position: %.1f", targetX);
				}
			}
			else if (isKidSleeping_) {
				velocity.x = 0.0f;
				Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
				throwBearAnims_.Update(dt);
				if (throwBearAnims_.GetCurrentName() == "FallAsleep" && throwBearAnims_.HasFinishedOnce("FallAsleep")) {
					throwBearAnims_.SetCurrent("SleepIdle");
				}
				bearSleepTimer_ -= dt;
				if (bearSleepTimer_ <= 0.0f) {
					isKidSleeping_ = false;
					anims.SetCurrent("idle");
				}
			}
			else if (isBearTransforming_) {
				// Transform state blocks all normal actions
				velocity.x = 0.0f;
				Engine::GetInstance().physics->SetXVelocity(pbody, 0.0f);
				bearAppearAnims_.Update(dt);
				if (bearAppearAnims_.HasFinishedOnce("appear")) {
					isBearTransforming_ = false;
					isBearMode_ = true;
					bearIdleAnims_.ResetCurrent();
					// Start the kid waving animation (plays once then transitions)
					throwBearAnims_.SetCurrent("BearActiveIdle");
					LOG("Player bear transformation complete! Now in bear mode.");
				}
				// Update the kid animation during transformation
				throwBearAnims_.Update(dt);
			}
			else if (isBearMode_) {
				// Giant Bear mode controls (only move left/right and attack)
				Move();
				Attack(dt);
				Teleport();
				// Kid animation: wave once -> fall asleep -> sleep loop
				throwBearAnims_.Update(dt);
				if (throwBearAnims_.GetCurrentName() == "BearActiveIdle" && throwBearAnims_.HasFinishedOnce("BearActiveIdle")) {
					throwBearAnims_.SetCurrent("FallAsleep");
					LOG("Kid finished waving, now falling asleep");
				}
				else if (throwBearAnims_.GetCurrentName() == "FallAsleep" && throwBearAnims_.HasFinishedOnce("FallAsleep")) {
					throwBearAnims_.SetCurrent("SleepIdle");
					LOG("Kid now sleeping idle");
				}
			}
			else {
				// Standard human controls
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
	}

	ApplyPhysics();

	if (velocity.x != 0.0f && !isJumping && !isDead_ && !isShowingDamageAnim_ && !isHiding_ && !isExitingHide_ && !isWakingUp && !isBearMode_) {
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
	if (isWakingUp || isShowingDamageAnim_ || isHiding_ || isExitingHide_ || isYoyoTrapped_ || isDollGrabbed_) return;

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
			if (!isJumping && !isPushing_ && !isBearMode_) anims.SetCurrent("turnaround");
		}

		if (!isBearMode_) {
			if (!isJumping && !isPushing_) {
				if (anims.GetCurrentName() != "turnaround" || anims.HasFinishedOnce("turnaround")) {
					if (anims.Has("run")) anims.SetCurrent("run");
					else anims.SetCurrent("idle");
				}
			}
		}
	}
	else if (moveRight) {
		velocity.x = effectiveSpeed;
		if (facingRight) {
			facingRight = false;
			if (!isJumping && !isPushing_ && !isBearMode_) anims.SetCurrent("turnaround");
		}

		if (!isBearMode_) {
			if (!isJumping && !isPushing_) {
				if (anims.GetCurrentName() != "turnaround" || anims.HasFinishedOnce("turnaround")) {
					if (anims.Has("run")) anims.SetCurrent("run");
					else anims.SetCurrent("idle");
				}
			}
		}
	}
	else {
		// Player released keys — not pushing anymore
		if (isPushing_) {
			// Keep isPushing_ true only while actively pressing toward the rock
			// The collision contact may still exist but we're not pushing
		}
		if (!isJumping && !isBearMode_) {
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
	if (isWakingUp || isShowingDamageAnim_ || isHiding_ || isExitingHide_ || isYoyoTrapped_ || isDollGrabbed_) return;

	auto& input = Engine::GetInstance().input;
	bool jumpDown = input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN ||
		input->GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_DOWN;
	bool jumpUp = input->GetKey(SDL_SCANCODE_SPACE) == KEY_UP ||
		input->GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_UP;

	if (jumpDown) {
		if (!isJumping) {
			// Reseteamos la Y para que la fuerza del salto se aplique siempre igual
			b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
			Engine::GetInstance().physics->SetLinearVelocity(pbody, currentVel.x, 0.0f);
			velocity.y = 0.0f;

			Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, 0.0f, -jumpForce, true);
			if (anims.Has("jump")) anims.SetCurrent("jump");
			isJumping = true;

			// Limpiar platformBelow al saltar para no arrastrar la velocidad
			platformBelow = nullptr;

			Engine::GetInstance().audio->PlayFx(jumpFxId);

			// Spawn jump dust
			Engine::GetInstance().entityManager->SpawnVFX(
				Vector2D(position.getX(), position.getY() + 50.0f),
				"assets/textures/spritesheets/SS_Pols_01.png",
				12, 794, 202, 0.015f, 0.0f, 0.2f
			);
		}
	}

	// Salto variable: corta la velocidad si sueltas el botón
	if (jumpUp && isJumping) {
		float vy = Engine::GetInstance().physics->GetYVelocity(pbody);
		if (vy < 0.0f) {
			Engine::GetInstance().physics->SetYVelocity(pbody, vy * 0.5f);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Hide mechanic
// ─────────────────────────────────────────────────────────────────────────────
void Player::Hide(float dt)
{
	if (isWakingUp || isShowingDamageAnim_ || isDead_ || isYoyoTrapped_ || isDollGrabbed_) return;

	auto& input = Engine::GetInstance().input;
	bool hideDown = input->GetKey(SDL_SCANCODE_H) == KEY_DOWN ||
		input->GetGamepadButton(SDL_GAMEPAD_BUTTON_NORTH) == KEY_DOWN;

	// Cannot hide without the blanket (cape collectible) or if it's not equipped
	if (!hasBlanket_ || equippedItem_ != EquippedItem::BLANKET) {
		if (hideDown && !hasBlanket_) {
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
			Engine::GetInstance().audio->PlayFx(capeFxId);
			LOG("Player hiding");
		}
		else if (isHiding_)
		{
			isHiding_ = false;
			isExitingHide_ = true;
			hideCooldown_ = HIDE_COOLDOWN; // cooldown starts on exit
			Engine::GetInstance().audio->PlayFx(capeFxId);
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
// ─────────────────────────────────────────────────────────────────────────────
void Player::Slingshot(float dt)
{
	if (!hasSlingshot_ || equippedItem_ != EquippedItem::SLINGSHOT) return;

	if (isWakingUp || isShowingDamageAnim_ || isDead_ || isHiding_ || isExitingHide_ || isYoyoTrapped_ || isDollGrabbed_)
	{
		isAiming_ = false;
		isAimingWithGamepad_ = false;
		chargeTimer_ = 0.0f;
		return;
	}

	auto& input = Engine::GetInstance().input;
	auto& render = Engine::GetInstance().render;

	KeyState lmb = input->GetMouseButtonDown(SDL_BUTTON_LEFT);
	bool triggerDown = input->GetRightTrigger() > 0.3f;
	bool triggerReleased = input->GetRightTrigger() < 0.1f;

	if (!isAiming_ && slingshotCooldown_ <= 0.0f)
	{
		if (lmb == KEY_DOWN || triggerDown)
		{
			isAiming_ = true;
			isAimingWithGamepad_ = triggerDown;
			chargeTimer_ = 0.0f;
			slingshotCharged_ = false;
			slingshotAnim_.Reset();
			slingshotHoldAnim_.Reset();
			LOG("Slingshot aiming started");
		}
	}

	if (isAiming_)
	{
		chargeTimer_ += dt;
		if (chargeTimer_ > MAX_CHARGE_TIME) chargeTimer_ = MAX_CHARGE_TIME;

		// Calculate aim angle
		int playerX, playerY;
		pbody->GetPosition(playerX, playerY);

		float dx = 0.0f;
		float dy = 0.0f;

		if (isAimingWithGamepad_)
		{
			// Calculate aim angle from right stick first
			float rx = input->GetRightStickX();
			float ry = input->GetRightStickY();
			if (rx * rx + ry * ry > 0.04f)
			{
				dx = rx;
				dy = ry;
				aimAngle_ = std::atan2(dy, dx);
			}
			else
			{
				// Default to left stick direction or facing direction
				float lx = input->GetLeftStickX();
				float ly = input->GetLeftStickY();
				if (lx * lx + ly * ly > 0.04f)
				{
					dx = lx;
					dy = ly;
					aimAngle_ = std::atan2(dy, dx);
				}
				else
				{
					// Default based on facing direction (native sprite faces LEFT on facingRight = true)
					dx = facingRight ? -1.0f : 1.0f;
					dy = 0.0f;
					aimAngle_ = facingRight ? 3.14159265f : 0.0f;
				}
			}
		}
		else
		{
			// Mouse aiming
			Vector2D mouseScreen = input->GetMousePosition();
			float mouseWorldX = mouseScreen.getX() - (float)render->camera.x;
			float mouseWorldY = mouseScreen.getY() - (float)render->camera.y;

			dx = mouseWorldX - (float)playerX;
			dy = mouseWorldY - (float)playerY;
			aimAngle_ = std::atan2(dy, dx);
		}

		// Update facing direction based on aim
		if (dx > 0) facingRight = false;
		else if (dx < 0) facingRight = true;

		// Advance slingshot animation: charge first, then hold loop
		if (!slingshotCharged_) {
			slingshotAnim_.Update(dt);
			if (slingshotAnim_.HasFinishedOnce()) {
				slingshotCharged_ = true;
			}
		} else {
			slingshotHoldAnim_.Update(dt);
		}

		// Draw trajectory preview dots (parabolic prediction)
		float chargeRatio = chargeTimer_ / MAX_CHARGE_TIME;
		float launchSpeed = MIN_LAUNCH_SPEED + (MAX_LAUNCH_SPEED - MIN_LAUNCH_SPEED) * chargeRatio;

		float dirX = std::cos(aimAngle_);
		float dirY = std::sin(aimAngle_);

		// Simulate trajectory (in meters, then convert to pixels)
		float simVx = dirX * launchSpeed;
		float simVy = dirY * launchSpeed;
		float gravity = 10.0f;

		for (int i = 1; i <= 5; i++)
		{
			float t = (float)i * 0.12f; // time steps
			float px = (float)playerX + METERS_TO_PIXELS(simVx * t);
			float py = (float)playerY + METERS_TO_PIXELS(simVy * t + 0.5f * gravity * t * t);

			Uint8 dotAlpha = (Uint8)(120 - i * 16);
			if (dotAlpha < 30) dotAlpha = 30;

			render->DrawCircle((int)px, (int)py, 3, 200, 180, 140, dotAlpha, true);
		}


		// Stop movement while aiming
		velocity.x = 0.0f;

		bool shouldFire = false;
		if (isAimingWithGamepad_)
		{
			if (triggerReleased) shouldFire = true;
		}
		else
		{
			if (lmb == KEY_UP) shouldFire = true;
		}

		if (shouldFire)
		{
			isAiming_ = false;
			isAimingWithGamepad_ = false;
			slingshotCooldown_ = SLINGSHOT_COOLDOWN;

			// Spawn projectile
			auto proj = std::dynamic_pointer_cast<SlingshotProjectile>(
				Engine::GetInstance().entityManager->CreateEntity(EntityType::SLINGSHOT_PROJECTILE));

			// Spawn fully outside the player's collision body for all aim directions.
			const float projectileSpawnClearance = 70.0f;
			float spawnOffsetX = std::cos(aimAngle_) * projectileSpawnClearance;
			float spawnOffsetY = std::sin(aimAngle_) * projectileSpawnClearance;
			proj->position = Vector2D((float)playerX + spawnOffsetX, (float)playerY - 10.0f + spawnOffsetY);

			proj->SetLaunch(dirX, dirY, launchSpeed);
			proj->Start();

			Engine::GetInstance().audio->PlayFx(slingshotFxId);

			LOG("Slingshot fired! angle=%.1f speed=%.1f", aimAngle_ * RADTODEG, launchSpeed);
		}
	}
}

void Player::ApplyPhysics() {
	if (isDollGrabbed_) {
		Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });
		return;
	}
	if (isJumping == true) {
		velocity.y = Engine::GetInstance().physics->GetYVelocity(pbody);
	}

	if (platformBelow != nullptr && !isJumping) {
		float platVelX = Engine::GetInstance().physics->GetXVelocity(platformBelow);
		float platVelY = Engine::GetInstance().physics->GetYVelocity(platformBelow);

		velocity.x += platVelX;

		if (platVelY > 0.0f) {
			velocity.y = platVelY + 1.5f;
		}
	}

	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);

	// Synchronize rock velocity to avoid jitter and clipping
	if (isPushing_ && pushedRockBody_ != nullptr) {
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

	bool drawingBear = isBearMode_ || isBearTransforming_;
	int bearOffsetDrawY = 0;

	if (isThrowingBear_)
	{
		activeTex = throwBearTexture_;
		animFrame = &throwBearAnims_.GetCurrentFrame();
		currentDrawScale = 0.5f;
	}
	else if (isKidSleeping_)
	{
		activeTex = throwBearTexture_;
		animFrame = &throwBearAnims_.GetCurrentFrame();
		currentDrawScale = 0.5f;
	}
	else if (isDead_ && isBearMode_)
	{
		bearDeathAnims_.Update(dt);
		activeTex = bearDeathTexture_;
		animFrame = &bearDeathAnims_.GetCurrentFrame();
		currentDrawScale = 0.5f;
		bearOffsetDrawY = 64;
	}
	else if (isBearTransforming_)
	{
		bearAppearAnims_.Update(dt);
		activeTex = bearAppearTexture_;
		animFrame = &bearAppearAnims_.GetCurrentFrame();
		currentDrawScale = 0.5f;
		bearOffsetDrawY = 64;
	}
	else if (isBearMode_)
	{
		if (isAttacking_)
		{
			bearAttackAnims_.Update(dt);
			activeTex = bearAttackTexture_;
			animFrame = &bearAttackAnims_.GetCurrentFrame();
			currentDrawScale = 0.256f; // 256 / 1000
			bearOffsetDrawY = 64;
		}
		else if (velocity.x != 0.0f)
		{
			bearWalkAnims_.Update(dt);
			activeTex = bearWalkTexture_;
			animFrame = &bearWalkAnims_.GetCurrentFrame();
			currentDrawScale = 0.5f;
			bearOffsetDrawY = 64;
		}
		else
		{
			bearIdleAnims_.Update(dt);
			activeTex = bearIdleTexture_;
			animFrame = &bearIdleAnims_.GetCurrentFrame();
			currentDrawScale = 0.5f;
			bearOffsetDrawY = 64;
		}
	}
	else if (isDead_)
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
	else if (isYoyoTrapped_ && yoyoTrapTexture_)
	{
		// Yoyo trap animation — player is caught by the Stitchling
		yoyoTrapAnims_.Update(dt);
		activeTex = yoyoTrapTexture_;
		animFrame = &yoyoTrapAnims_.GetCurrentFrame();
		// 857x480 frames → scale down to match ~128px character height
		currentDrawScale = 0.27f;
	}
	else if (isDollGrabbed_ && dollGrabbedTexture_)
	{
		// Drop Doll minigame — player squirms while the doll clings to their head
		dollGrabbedAnim_.Update(dt);
		activeTex = dollGrabbedTexture_;
		animFrame = &dollGrabbedAnim_.GetCurrentFrame();
		// 256x256 frames -> same half scale as push/slingshot
		currentDrawScale = 0.5f;
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
		// Slingshot aiming animation — charge phase or hold phase
		activeTex = slingshotShootTexture_;
		if (slingshotCharged_) {
			animFrame = &slingshotHoldAnim_.GetCurrentFrame();
		} else {
			animFrame = &slingshotAnim_.GetCurrentFrame();
		}
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

	if (drawingBear) {
		drawY -= bearOffsetDrawY;
	}

	// Adjust drawing position when pushing so hands align with the edge of the rock visually
	if (isPushing_ && velocity.x != 0.0f && !isJumping) {
		drawX -= static_cast<int>(pushDir_ * 35.0f);
	}

	// --- Fix: In Player::Draw() ---

    // 1. Move the flip calculation UP so the wake-up block can use it
    bool spriteNativeRight = false;
    if (!isBearMode_ && !isBearTransforming_ && !isThrowingBear_ && !isKidSleeping_) {
        const std::string& animName = anims.GetCurrentName();
        if (animName == "jump" || animName == "turnaround" || animName == "idle") {
            spriteNativeRight = true; // these sprites natively face RIGHT
        }
    }

    SDL_FlipMode flip;
    if (isAiming_ && slingshotShootTexture_) {
        // Slingshot sprite: invert to match facing direction
        flip = facingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    }
    else if (isPushing_ && velocity.x != 0.0f && !isJumping) {
        flip = (velocity.x > 0.0f) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    }
    else if (spriteNativeRight) {
        flip = facingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    }
    else {
        // This is what your standard animations fall back to
        flip = facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
    }

    if (isWakingUp) {
        facingRight = true; // Force facing left to match wake-up spritesheet
        if (wakeUpAnimStarted) {
            wakeUpAnim.Update(dt * wakeUpAnimSpeed_);
            if (wakeUpAnim.HasFinishedOnce()) {
                isWakingUp = false;
				wakeUpAnimSpeed_ = 1.0f;
            }
        }
        if (isWakingUp) {
            const SDL_Rect& wuFrame = wakeUpAnim.GetCurrentFrame();
            float wakeScale = 0.65f;
            int wakeWidth = (int)(256.0f * wakeScale);
            int wakeHeight = (int)(256.0f * wakeScale); 
            int wakeDrawX = xInt - (wakeWidth / 2) - 60;
            int wakeDrawY = yInt + 50 - wakeHeight + 15;
            
            render->ApplyAmbientTint(wakeUpTexture);
            render->DrawTexture(wakeUpTexture, wakeDrawX, wakeDrawY, &wuFrame, 1.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, wakeScale);
            render->ResetAmbientTint(wakeUpTexture);
            return;
        }
    }

	if (drawingBear && throwBearTexture_) {
		const SDL_Rect& kidFrame = throwBearAnims_.GetCurrentFrame();
		float kidScale = 0.5f;
		int kidDrawX = static_cast<int>(bearSummonPosition_.getX() - (static_cast<float>(kidFrame.w) * kidScale) / 2.0f);
		int kidDrawY = static_cast<int>(bearSummonPosition_.getY() - (static_cast<float>(kidFrame.h) * kidScale) / 2.0f);

		SDL_FlipMode kidFlip = bearSummonFacingRight_ ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

		render->ApplyAmbientTint(throwBearTexture_);
		render->DrawTexture(throwBearTexture_, kidDrawX, kidDrawY, &kidFrame, 1.0f, 0, INT_MAX, INT_MAX, kidFlip, kidScale);
		render->ResetAmbientTint(throwBearTexture_);
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

		// Add subtle permanent white circular glow to the player
		if (!isDead_ && !isWakingUp) {
			float radiusScale = 0.55f;
			Uint8 alpha = 255; // MAX ALPHA for troubleshooting
			render->DrawWhiteGlow(xInt, yInt, radiusScale, alpha);
		}

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
	if (isHiding_ || isExitingHide_ || isYoyoTrapped_ || isDollGrabbed_) return;

	auto& input = Engine::GetInstance().input;
	auto& physics = Engine::GetInstance().physics;

	if (attackCooldown_ > 0.0f) attackCooldown_ -= dt;

	bool attackDown = false;
	if (isBearMode_) {
		attackDown = input->GetMouseButtonDown(1) == KEY_DOWN ||
			input->GetGamepadButton(SDL_GAMEPAD_BUTTON_WEST) == KEY_DOWN;
	}
	else {
		attackDown = input->GetKey(SDL_SCANCODE_J) == KEY_DOWN ||
			input->GetGamepadButton(SDL_GAMEPAD_BUTTON_WEST) == KEY_DOWN;
	}

	if (attackDown && !isAttacking_ && attackCooldown_ <= 0.0f)
	{
		isAttacking_ = true;
		if (isBearMode_) {
			attackTimer_ = 480.0f; // 6 frames * 80ms
			bearAttackAnims_.ResetCurrent();
		}
		else {
			attackTimer_ = ATTACK_DURATION;
		}
		attackCooldown_ = ATTACK_COOLDOWN;

		int bodyX, bodyY;
		pbody->GetPosition(bodyX, bodyY);

		int hitboxW = isBearMode_ ? 100 : HITBOX_W;
		int hitboxH = isBearMode_ ? 120 : HITBOX_H;
		int offset = isBearMode_ ? 80 : HITBOX_OFFSET;

		int hitboxX = facingRight ? bodyX - hitboxW / 2 - offset : bodyX + hitboxW / 2 + offset;
		attackHitbox_ = physics->CreateRectangleSensor(hitboxX, bodyY, hitboxW, hitboxH, bodyType::STATIC);
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
			int hitboxW = isBearMode_ ? 100 : HITBOX_W;
			int offset = isBearMode_ ? 80 : HITBOX_OFFSET;
			int hitboxX = facingRight ? bodyX - hitboxW / 2 - offset : bodyX + hitboxW / 2 + offset;
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
	// Cannot take damage while hiding, sleeping, waking up, throwing the bear, or transforming
	if (isInvincible_ || isHiding_ || isKidSleeping_ || isWakingUp || isThrowingBear_ || isBearTransforming_) return;

	Engine::GetInstance().scene->TriggerScreenDamage();

	if (isBearMode_) {
		bearHealth_ -= damage;
		LOG("Bear took %d damage -> bearHealth: %d", damage, bearHealth_);

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

		if (bearHealth_ <= 0) {
			bearHealth_ = 0;
			isBearMode_ = false;
			isBearTransforming_ = false;
			isKidSleeping_ = true;
			bearSleepTimer_ = 2000.0f; // 2 seconds sleep
			bearCooldownTimer_ = 5000.0f; // 5 seconds cooldown
			throwBearAnims_.SetCurrent("FallAsleep");
			throwBearAnims_.ResetCurrent();

			// Teleport player physics body back to kid position
			pbody->SetPosition((int)bearSummonPosition_.getX(), (int)bearSummonPosition_.getY());
			
			// Restore facing direction to when the bear was summoned
			facingRight = bearSummonFacingRight_;
			
			Engine::GetInstance().audio->PlayFx(Engine::GetInstance().scene->GetMenuClickFxId());
			LOG("Bear health depleted! Exiting bear mode, returning to kid.");
		}
		return;
	}

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
		if ((entity->type == EntityType::ENEMY || entity->type == EntityType::ENEMY_B ||
			entity->type == EntityType::ENEMY_C || entity->type == EntityType::BOUNCER) && entity->active) {
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

	// Cancel slingshot aiming on damage so the player doesn't get stuck
	isAiming_ = false;
	isAimingWithGamepad_ = false;
	chargeTimer_ = 0.0f;
	slingshotCharged_ = false;

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
	isBearMode_ = false;
	isBearTransforming_ = false;
	isThrowingBear_ = false;
	isKidSleeping_ = false;
	bearSleepTimer_ = 0.0f;
	hideAlphaTime_ = 0.0f;
	hideCooldown_ = 0.0f; // reset cooldown on revive
	isAiming_ = false;
	chargeTimer_ = 0.0f;
	slingshotCooldown_ = 0.0f;
	isYoyoTrapped_ = false;
	knockbackX_ = 0.0f;
	knockbackTimer_ = 0.0f;
	velocity = { 0.0f, 0.0f };
	if (pbody) Engine::GetInstance().physics->SetLinearVelocity(pbody, 0.0f, 0.0f);
	anims.SetCurrent("idle");
	damageFlashTimer_ = 0.0f;
}

void Player::StartWakeUp(float speedMultiplier)
{
	Revive();
	isWakingUp = true;
	wakeUpAnimStarted = true;
	wakeUpAnimSpeed_ = std::max(0.1f, speedMultiplier);
	wakeUpAnim.Reset();
	facingRight = true; // Force facing left to match wake-up spritesheet
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
	if (bearAppearTexture_) {
		Engine::GetInstance().textures->UnLoad(bearAppearTexture_);
		bearAppearTexture_ = nullptr;
	}
	if (bearIdleTexture_) {
		Engine::GetInstance().textures->UnLoad(bearIdleTexture_);
		bearIdleTexture_ = nullptr;
	}
	if (bearWalkTexture_) {
		Engine::GetInstance().textures->UnLoad(bearWalkTexture_);
		bearWalkTexture_ = nullptr;
	}
	if (bearAttackTexture_) {
		Engine::GetInstance().textures->UnLoad(bearAttackTexture_);
		bearAttackTexture_ = nullptr;
	}
	if (bearDeathTexture_) {
		Engine::GetInstance().textures->UnLoad(bearDeathTexture_);
		bearDeathTexture_ = nullptr;
	}
	if (throwBearTexture_) {
		Engine::GetInstance().textures->UnLoad(throwBearTexture_);
		throwBearTexture_ = nullptr;
	}
	if (dollGrabbedTexture_) {
		Engine::GetInstance().textures->UnLoad(dollGrabbedTexture_);
		dollGrabbedTexture_ = nullptr;
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
			platformBelow = physB;
			platformDropTimer_ = 0.0f;

			float platVy = Engine::GetInstance().physics->GetYVelocity(physB);
			if (platVy > 0.0f) {
				b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
				if (currentVel.y > platVy) {
					Engine::GetInstance().physics->SetLinearVelocity(pbody, currentVel.x, platVy);
					velocity.y = platVy;
				}
			}

			if (isJumping) {
				Engine::GetInstance().audio->PlayFx(landFxId);

				Engine::GetInstance().entityManager->SpawnVFX(
					Vector2D(position.getX(), position.getY() + 50.0f),
					"assets/textures/spritesheets/SS_Pols_01.png",
					12, 794, 202, 0.03f, 0.0f, 0.2f
				);

				if (anims.GetCurrentName() == "jump") {
				}
				else {
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
					// Play landing sound effect when landing on a rock
					Engine::GetInstance().audio->PlayFx(landFxId);

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
	if (physB->ctype == ColliderType::PLATFORM) {
		if (isJumping) {
			platformBelow = nullptr;
			platformDropTimer_ = 0.0f;
		}
		else if (platformBelow == physB) {
			platformDropTimer_ = 100.0f;
		}
	}

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