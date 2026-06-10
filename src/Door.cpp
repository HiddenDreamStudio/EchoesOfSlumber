#include "Door.h"
#include "Engine.h"    
#include "Textures.h"
#include "Physics.h"
#include "Render.h"
#include "Log.h"

// Init vars to prevent C26495 warnings
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
	texturePath = "assets/textures/door.png";
	texW = 32;
	texH = 64;

	texture = Engine::GetInstance().textures->Load(texturePath);

	// Create static body to block player
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

		// Draw only if closed
		if (!isOpen) {
			Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2);
		}
	}
	return true;
}

bool Door::CleanUp()
{
	if (texture != nullptr) {
		Engine::GetInstance().textures->UnLoad(texture);
		texture = nullptr;
	}
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

		// Remove collision so player can pass
		if (pbody != nullptr) {
			Engine::GetInstance().physics->DeletePhysBody(pbody);
			pbody = nullptr;
		}
	}
}
