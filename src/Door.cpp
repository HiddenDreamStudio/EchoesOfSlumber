#include "Door.h"
#include "Engine.h"    
#include "Textures.h"
#include "Physics.h"
#include "Render.h"
#include "Log.h"

Door::Door() : Entity(EntityType::DOOR), texW(0), texH(0), texturePath(nullptr)
{
	name = "door";
	pbody = nullptr;
	texture = nullptr;
}

Door::~Door() {}

bool Door::Awake()
{
	return true;
}

bool Door::Start()
{
	texturePath = "assets/textures/spritesheets/SS_porta_trencantse_lvl1.jpg";

	texW = 64;
	texH = 96;

	// Load texture
	texture = Engine::GetInstance().textures->Load(texturePath);

<<<<<<< Updated upstream
=======
	for (int i = 0; i < 4; i++) {
		SDL_Rect frame = { i * texW, 0, texW, texH };
		openAnim.AddFrame(frame, 150); // 150ms per frame
	}
	// Play only once
	openAnim.SetLoop(false);

	// Create static collider
>>>>>>> Stashed changes
	pbody = Engine::GetInstance().physics->CreateRectangle((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, texW, texH, bodyType::STATIC);
	pbody->ctype = ColliderType::DOOR;
	pbody->listener = this;

	return true;
}

bool Door::Update(float dt)
{
	if (pbody != nullptr) {
		int x, y;
		pbody->GetPosition(x, y);
		position.setX((float)x);
		position.setY((float)y);
	}

<<<<<<< Updated upstream
		if (!isOpen) {
			Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2);
		}
	}
=======
	if (texture != nullptr) {
		SDL_Rect currentFrame;

		if (isOpen) {
			openAnim.Update(dt);
			currentFrame = openAnim.GetCurrentFrame();
		}
		else {
			currentFrame = { 0, 0, texW, texH };
		}

		if (!isOpen || !openAnim.HasFinishedOnce()) {
			Engine::GetInstance().render->DrawTexture(texture, (int)position.getX() - texW / 2, (int)position.getY() - texH / 2, &currentFrame);
		}
	}
	else {
		// Draw placeholder if texture fails
		if (!isOpen) {
			SDL_Rect rect = { (int)position.getX() - texW / 2, (int)position.getY() - texH / 2, texW, texH };
			Engine::GetInstance().render->DrawRectangle(rect, 139, 69, 19, 255, true, true);
		}
	}
>>>>>>> Stashed changes

	return true;
}

bool Door::CleanUp()
{
	// Free texture
	if (texture != nullptr) {
		Engine::GetInstance().textures->UnLoad(texture);
		texture = nullptr;
	}
	// Free collider
	if (pbody != nullptr) {
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}
	return true;
}

void Door::Open()
{
	if (!isOpen)
	{
		isOpen = true;
		LOG("Door opened");

<<<<<<< Updated upstream
=======
		// Start breaking animation
		openAnim.Reset();

		// Remove collider so player can pass
>>>>>>> Stashed changes
		if (pbody != nullptr) {
			Engine::GetInstance().physics->DeletePhysBody(pbody);
			pbody = nullptr;
		}
	}
}