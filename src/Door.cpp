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

bool Door::Awake() { return true; }

bool Door::Start()
{
	texturePath = "assets/textures/spritesheets/SS_portes/SS_porta_trencantse_lvl1.png";

	texW = 256;
	texH = 256;

	texture = Engine::GetInstance().textures->Load(texturePath);

	for (int i = 0; i < 24; i++) {
		SDL_Rect frame = { i * texW, 0, texW, texH };
		openAnim.AddFrame(frame, 50);
	}
	openAnim.SetLoop(false);

	// Create static collider
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

	if (texture != nullptr) {
		SDL_Rect currentFrame;

		if (isOpen) {
			// Update breaking animation
			openAnim.Update(dt);
			currentFrame = openAnim.GetCurrentFrame();
		}
		else {
			// First frame if closed
			currentFrame = { 0, 0, texW, texH };
		}

		// Draw texture until animation finishes
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

		// Start breaking animation
		openAnim.Reset();

		// Remove collider
		if (pbody != nullptr) {
			Engine::GetInstance().physics->DeletePhysBody(pbody);
			pbody = nullptr;
		}
	}
}