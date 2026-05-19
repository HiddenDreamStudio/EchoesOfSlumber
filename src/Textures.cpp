#include "Engine.h"
#include "Render.h"
#include "Textures.h"
#include "Log.h"

Textures::Textures() : Module()
{
	name = "textures";
}

// Destructor
Textures::~Textures()
{
}

// Called before render is available
bool Textures::Awake()
{
	LOG("Init Image library");
	bool ret = true;

	return ret;
}

// Called before the first frame
bool Textures::Start()
{
	LOG("start textures");
	bool ret = true;
	return ret;
}

// Called before quitting
bool Textures::CleanUp()
{
	LOG("Freeing textures and Image library");
	for (const auto& texture : textures) {
		SDL_DestroyTexture(texture);
	}
	textures.clear();

	return true;
}

// Load new texture from file path
SDL_Texture* const Textures::Load(const char* path)
{
	SDL_Texture* texture = NULL;
	SDL_Surface* surface = IMG_Load(path);

	if (surface == NULL)
	{
		LOG("Could not load surface with path: %s. IMG_Load: %s", path, SDL_GetError());
	}
	else
	{
		texture = LoadSurface(surface);
		SDL_DestroySurface(surface); // SDL3: free with SDL_DestroySurface
	}

	return texture;
}

// Unload texture
bool Textures::UnLoad(SDL_Texture* texture)
{
	for (auto it = textures.begin(); it != textures.end(); ++it) {
		if (*it == texture) {
			textureInfo.erase(texture);
			SDL_DestroyTexture(texture);
			textures.erase(it);
			return true;
		}
	}
	return false;
}

// Translate a surface into a texture
SDL_Texture* const Textures::LoadSurface(SDL_Surface* surface)
{
	SDL_Texture* texture = SDL_CreateTextureFromSurface(Engine::GetInstance().render->renderer, surface);

	if (texture == NULL)
	{
		LOG("Unable to create texture from surface! SDL Error: %s\n", SDL_GetError());
	}
	else
	{
		textures.push_back(texture);
		textureInfo[texture] = { surface->w, surface->h };
		SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
	}

	return texture;
}

// Retrieve size of a texture
void Textures::GetSize(const SDL_Texture* texture, int& width, int& height) const
{
	auto it = textureInfo.find(texture);
	if (it != textureInfo.end()) {
		width = it->second.w;
		height = it->second.h;
		return;
	}

	float tw = 0.0f;
	float th = 0.0f;
	if (!SDL_GetTextureSize((SDL_Texture*)texture, &tw, &th))
	{
		LOG("SDL_GetTextureSize failed: %s", SDL_GetError());
		width = 0;
		height = 0;
	}
	else
	{
		width = (int)tw;
		height = (int)th;
		textureInfo[texture] = { width, height };
	}
}
