#include "Item.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"

Item::Item() : Entity(EntityType::ITEM), texture(nullptr), texturePath(nullptr), texW(0), texH(0), pbody(nullptr)
{
	name = "item";
}

Item::~Item() {}

bool Item::Awake() { return true; }

bool Item::Start() {
	// Load texture
	texture = Engine::GetInstance().textures->Load("Assets/Textures/goldCoin.png");

	// Set size
	if (texture != nullptr) {
		Engine::GetInstance().textures.get()->GetSize(texture, texW, texH);
	}
	else {
		texW = 32;
		texH = 32;
	}

	// Create dynamic collider
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, texW / 2, bodyType::DYNAMIC);
	pbody->ctype = ColliderType::ITEM;
	pbody->listener = this;

	return true;
}

bool Item::Update(float dt)
{
	if (!active) return true;

	int x, y;
	pbody->GetPosition(x, y);

	if (!Engine::GetInstance().scene->isPaused_) {
		position.setX((float)x);
		position.setY((float)y);
	}

	// Draw item
	if (texture != nullptr) {
		Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2);
	}
	else {
		// Draw placeholder
		SDL_Rect rect = { x - texW / 2, y - texH / 2, texW, texH };
		Engine::GetInstance().render->DrawRectangle(rect, 255, 255, 0, 255, true, true);
	}

	return true;
}

bool Item::CleanUp()
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

bool Item::Destroy()
{
	LOG("Destroying item");
	active = false;
	pendingToDelete = true;
	return true;
}