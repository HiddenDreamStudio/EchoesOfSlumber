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

	// Opens the door
	void Open();

public:
	bool isOpen = false;

private:
	SDL_Texture* texture;
	const char* texturePath;
	int texW, texH;

	// Physics body blocking passage
	PhysBody* pbody;
};