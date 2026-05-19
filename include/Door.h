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

<<<<<<< Updated upstream
	// Method to handle the opening
	void Open();

public:

	bool isOpen = false;

private:

=======
	// Open the door
	void Open();

public:
	// Door state
	bool isOpen = false;

private:
	// Texture data
>>>>>>> Stashed changes
	SDL_Texture* texture;
	const char* texturePath;
	int texW, texH;

<<<<<<< Updated upstream
	// Pointer to the physics body blocking passage
=======
	// Collision body
>>>>>>> Stashed changes
	PhysBody* pbody;

	// Breaking animation
	Animation openAnim;
};