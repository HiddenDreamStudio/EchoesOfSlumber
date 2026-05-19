#pragma once

#include "Module.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <list>
#include <unordered_map>
#include <string>

struct TextureData {
	int w, h;
};

class Textures : public Module
{
public:

	Textures();

	// Destructor
	virtual ~Textures();

	// Called before render is available
	bool Awake();

	// Called before the first frame
	bool Start();

	// Called before quitting
	bool CleanUp();

	// Load Texture
	SDL_Texture* const Load(const char* path);
	SDL_Texture* const LoadSurface(SDL_Surface* surface);
	bool UnLoad(SDL_Texture* texture);
	void GetSize(const SDL_Texture* texture, int& width, int& height) const;

public:
	std::list<SDL_Texture*> textures;
	mutable std::unordered_map<const SDL_Texture*, TextureData> textureInfo;

};
