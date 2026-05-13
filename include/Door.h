#pragma once

#include "Entity.h"
#include <SDL3/SDL.h>

struct SDL_Texture;

class Door : public Entity
{
public:

	Door();
	virtual ~Door();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	// Method to handle the opening
	void Open();

public:

	bool isOpen = false;

private:

	SDL_Texture* texture;
	const char* texturePath;
	int texW, texH;

	// Pointer to the physics body blocking passage
	PhysBody* pbody;
};