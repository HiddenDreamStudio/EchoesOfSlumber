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

Player::Player() : Entity(EntityType::PLAYER)
{
	name = "Player";
}

Player::~Player() {

}

bool Player::Awake() {
	return true;
}

bool Player::Start() {

	// Load animations from TSX.
	// testpersonaje.tsx has 250x250 tiles with only one animation at tile id 0.
	// We map id 0 to all three state names so SetCurrent() never fails.
	std::unordered_map<int, std::string> aliases = { {0,"idle"} };
	anims.LoadFromTSX("assets/textures/testpersonaje.tsx", aliases);
	anims.SetCurrent("idle");

	// Load the spritesheet texture (source frames are 250x250)
	texture = Engine::GetInstance().textures->Load("assets/textures/spritesheet damage color.png");

	// Desired in-game display size for the player sprite.
	// The source tile is 250x250 but we want the player to appear ~128x128 on screen.
	texW = 128;
	texH = 128;

	// Compute the draw scale: how much to shrink the 250px source frame to 128px display.
	drawScale = (float)texW / 250.0f;

	// Create a capsule collider to match the tall player sprite.
	// Width 40px (body width), height 100px (body height) - vertical capsule.
	pbody = Engine::GetInstance().physics->CreateCapsule((int)position.getX(), (int)position.getY(), 40, 100, bodyType::DYNAMIC);
	
	pbody->listener = this;
	pbody->ctype = ColliderType::PLAYER;

	//initialize audio effect
	pickCoinFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/coin-collision-sound-342335.wav");
	return true;
}

bool Player::Update(float dt)
{
	ZoneScoped;
	GetPhysicsValues();
	Move();
	Jump();
	Teleport();
	ApplyPhysics();
	Draw(dt);

	return true;
}

void Player::Teleport() {
	// Teleport the player to a specific position for testing purposes
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_T) == KEY_DOWN) {
		pbody->SetPosition(96, 96);
	}
}

void Player::GetPhysicsValues() {
	// Read current velocity
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y }; // Reset horizontal velocity by default, this way the player stops when no key is pressed
}

void Player::Move() {

	// Move left/right
	// Note: testpersonaje.tsx only has one animation ("idle").
	// When separate move/jump animations are added to the TSX, update these strings.
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) {
		velocity.x = -speed;
		facingRight = true;
		if (anims.Has("move")) anims.SetCurrent("move");
	}
	else if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) {
		velocity.x = speed;
		facingRight = false;
		if (anims.Has("move")) anims.SetCurrent("move");
	}
	else if (!isJumping) {
		anims.SetCurrent("idle");
	}
}

void Player::Jump() {

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN && isJumping == false) {
		Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, 0.0f, -jumpForce, true);
		if (anims.Has("jump")) anims.SetCurrent("jump");
		isJumping = true;
	}

	// Parametric jump: cut vertical velocity when space is released early
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_UP && isJumping) {
		float vy = Engine::GetInstance().physics->GetYVelocity(pbody);
		if (vy < 0) {
			Engine::GetInstance().physics->SetYVelocity(pbody, vy * 0.5f);
		}
	}
}

void Player::ApplyPhysics() {
	// Preserve vertical speed while jumping
	if (isJumping == true) {
		velocity.y = Engine::GetInstance().physics->GetYVelocity(pbody);
	}

	// Apply velocity via helper
	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}

void Player::Draw(float dt) {

	anims.Update(dt);
	const SDL_Rect& animFrame = anims.GetCurrentFrame();

	// Update render position using your PhysBody helper
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	// Camera System - Issue #21: Smooth camera follow with dead zones
	// NOTE: Camera update in Player::Draw causes 1-frame jitter with Map tiles.
	// TODO (#21): For proper fix, move world rendering to PostUpdate or add a Camera module
	// that updates after Physics but before Map (requires module order refactoring).
	auto& render = Engine::GetInstance().render;
	Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();  
	
	// Set camera target to player position
	render->SetCameraTarget(position.getX(), position.getY());
	
	// Apply smooth follow with lerp and dead zones
	render->FollowTarget(dt);
	
	// Clamp camera to map boundaries
	render->ClampCameraToMapBounds(mapSize.getX(), mapSize.getY());

	// Center the sprite on the physics body position.
	int drawX = x - texW / 2;
	int drawY = y - texH / 2;

	// Flip the sprite horizontally when facing left.
	// drawScale converts the 250px source frame to the desired 128px display size.
	SDL_FlipMode flip = facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
	Engine::GetInstance().render->DrawTexture(texture, drawX, drawY, &animFrame, 1.0f, 0, INT_MAX, INT_MAX, flip, drawScale);
}

bool Player::CleanUp()
{
	LOG("Cleanup player");
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	return true;
}

void Player::OnCollision(PhysBody* physA, PhysBody* physB) {
	switch (physB->ctype)
	{
	case ColliderType::PLATFORM:
		LOG("Collision PLATFORM");
		//reset the jump flag when touching the ground
		isJumping = false;
		anims.SetCurrent("idle");
		break;
	case ColliderType::ITEM:
		LOG("Collision ITEM");
		Engine::GetInstance().audio->PlayFx(pickCoinFxId);
		physB->listener->Destroy();
		break;
	case ColliderType::UNKNOWN:
		LOG("Collision UNKNOWN");
		break;
	default:
		break;
	}
}

void Player::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	switch (physB->ctype)
	{
	case ColliderType::PLATFORM:
		LOG("End Collision PLATFORM");
		break;
	case ColliderType::ITEM:
		LOG("End Collision ITEM");
		break;
	case ColliderType::UNKNOWN:
		LOG("End Collision UNKNOWN");
		break;
	default:
		break;
	}
}

Vector2D Player::GetPosition() {
	int x, y;
	pbody->GetPosition(x, y);
	// Adjust for center
	return Vector2D((float)x - texW / 2, (float)y - texH / 2);
}

void Player::SetPosition(Vector2D pos) {
	pbody->SetPosition((int)(pos.getX() + texW / 2), (int)(pos.getY() + texH / 2));
}

bool Player::Destroy()
{
	LOG("Destroying Player");
	active = false;
	pendingToDelete = true;
	return true;
}
