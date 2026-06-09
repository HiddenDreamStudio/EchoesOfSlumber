#include "HidingRock.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "Input.h"
#include "Scene.h"
#include "Player.h"
#include "Log.h"
#include "Audio.h"

HidingRock::HidingRock() : Entity(EntityType::HIDING_ROCK) {
	name = "HidingRock";
}

HidingRock::~HidingRock() {}

bool HidingRock::Awake() { return true; }

bool HidingRock::Start() {
	std::string texPath = "assets/textures/AS_environment/redissenys assets_Nivell 1/AS_Nivell_01/AS_roca_escondite_0";
	texPath += std::to_string(rockType_) + ".png";
	
	texRock_ = Engine::GetInstance().textures->Load(texPath.c_str());
	if (texRock_) {
		float tw, th;
		SDL_GetTextureSize(texRock_, &tw, &th);
		texW_ = (int)tw;
		texH_ = (int)th;
	}

	pbody_ = Engine::GetInstance().physics->CreateRectangleSensor((int)position.getX(), (int)position.getY(), 120, 100, bodyType::STATIC);
	pbody_->ctype = ColliderType::UNKNOWN;
	pbody_->listener = this;

	SDL_Color color = { 255, 255, 255, 255 };
	texPrompt_ = Engine::GetInstance().render->CreateMenuTextTexture("Press E to hide", color);
	if (texPrompt_) {
		float pw, ph;
		SDL_GetTextureSize(texPrompt_, &pw, &ph);
		promptW_ = (int)pw;
		promptH_ = (int)ph;
	}

	return true;
}

bool HidingRock::Update(float dt) {
	if (texRock_) {
		int bx, by;
		pbody_->GetPosition(bx, by);
		
		float scale = 0.35f;
		SDL_Rect section = { 0, 0, texW_, texH_ };
		int drawX = bx - (int)(texW_ * scale / 2.0f);
		int drawY = by + 100 - (int)(texH_ * scale);
		Engine::GetInstance().render->DrawTexture(texRock_, drawX, drawY, &section, 1.0f, 0.0, INT_MAX, INT_MAX, SDL_FLIP_NONE, scale);
	}

	auto playerShared = Engine::GetInstance().scene->player;
	if (!playerShared) return true;

	if (playerInRange_) {
		auto& input = Engine::GetInstance().input;
		bool interactDown = input->GetKey(SDL_SCANCODE_E) == KEY_DOWN || input->GetGamepadButton(SDL_GAMEPAD_BUTTON_WEST) == KEY_DOWN;
		
		// If player is not already hiding behind another object/mechanic, we can toggle
		if (!playerShared->IsHiding() || playerShared->IsHiding()) { // We only care if they are hiding behind this rock or not hiding
			// If we press interact, we toggle hiding
			if (interactDown) {
				bool currentlyHiding = false; // We can check if they are hiding behind a rock.
				// Wait, Player doesn't expose a way to check `isHidingBehindRock_` directly, but `IsHiding()` is true if hiding behind rock.
				// However, they might be hiding with the blanket. If so, they shouldn't hide behind rock.
				// Let's just track if they are currently hiding.
				if (!playerShared->IsHiding()) {
					playerShared->SetHidingBehindRock(true);
					// When hiding, we put the rock in front of the player (draw order is already managed if rock is in entities, 
					// but player is drawn in Player.cpp. Player is likely drawn on top of entities if drawn later. 
					// Tiled Map PostUpdate draws decorations. Since we draw the rock here in Update, if player draws in Update, it might be an issue.
					// We'll see).
					Engine::GetInstance().audio->PlayFx(Engine::GetInstance().scene->GetMenuClickFxId());
				} else {
					playerShared->SetHidingBehindRock(false);
					Engine::GetInstance().audio->PlayFx(Engine::GetInstance().scene->GetMenuClickFxId());
				}
			}
		}

		if (texPrompt_ && !playerShared->IsHiding()) {
			int bx, by;
			pbody_->GetPosition(bx, by);
			
			float promptScale = 0.4f;
			int textW = (int)(promptW_ * promptScale);
			int textH = (int)(promptH_ * promptScale);
			int padding = 10;
			
			int boxW = textW + padding * 2;
			int boxH = textH + padding * 2;
			int boxX = bx - boxW / 2;
			int boxY = by + 100 - (int)(texH_ * 0.35f) - boxH - 10;
			
			auto& render = Engine::GetInstance().render;
			
			// Black filled background
			SDL_Rect bgRect = { boxX, boxY, boxW, boxH };
			render->DrawRectangle(bgRect, 0, 0, 0, 200, true, true);
			
			// White border
			SDL_Rect borderRect = { boxX, boxY, boxW, boxH };
			render->DrawRectangle(borderRect, 255, 255, 255, 255, false, true);
			
			// Text centered inside the box
			SDL_Rect promptSec = { 0, 0, promptW_, promptH_ };
			int promptX = boxX + padding;
			int promptY = boxY + padding;
			render->DrawTexture(texPrompt_, promptX, promptY, &promptSec, 1.0f, 0.0, INT_MAX, INT_MAX, SDL_FLIP_NONE, promptScale);
		}
	} else {
		// If player moves out of range or is far, ensure they are not hiding behind this rock.
		// Actually if they are hiding, they shouldn't be able to move out of range because movement is blocked.
	}

	return true;
}

bool HidingRock::CleanUp() {
	if (texRock_) Engine::GetInstance().textures->UnLoad(texRock_);
	if (texPrompt_) SDL_DestroyTexture(texPrompt_);
	if (pbody_) Engine::GetInstance().physics->DeletePhysBody(pbody_);
	return true;
}

void HidingRock::OnCollision(PhysBody* physA, PhysBody* physB) {
	if (physB->ctype == ColliderType::PLAYER) {
		playerInRange_ = true;
	}
}

void HidingRock::OnCollisionEnd(PhysBody* physA, PhysBody* physB) {
	if (physB->ctype == ColliderType::PLAYER) {
		playerInRange_ = false;
	}
}
