#pragma once

#include "Entity.h"
#include "Animation.h"
#include <SDL3/SDL.h>

struct SDL_Texture;

// Door entity class
class Door : public Entity
{
public:
	Door();
	virtual ~Door();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	// Open the door
	void Open();

public:
	// Door state
	bool isOpen = false;

private:
	// Texture data
	SDL_Texture* texture;
	const char* texturePath;
	int texW, texH; // Individual frame dimensions (256x256)

	// Collision body
	PhysBody* pbody;

	// Breaking animation
	Animation openAnim;
};