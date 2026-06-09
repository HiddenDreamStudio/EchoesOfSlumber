#pragma once

#include "Entity.h"
#include <SDL3/SDL.h>

class PhysBody;

class HidingRock : public Entity
{
public:
	HidingRock();
	virtual ~HidingRock();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OnCollision(PhysBody* physA, PhysBody* physB) override;
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB) override;

	void SetRockType(int type) { rockType_ = type; }

private:
	int rockType_ = 1;
	SDL_Texture* texRock_ = nullptr;
	PhysBody* pbody_ = nullptr;

	bool playerInRange_ = false;

	int texW_ = 0;
	int texH_ = 0;
	
	SDL_Texture* texPrompt_ = nullptr;
	int promptW_ = 0;
	int promptH_ = 0;
};
