#pragma once

#include "UIElement.h"
#include "Vector2D.h"

class UIButton : public UIElement
{

public:

	SDL_Texture* hoverTexture = nullptr;
	void SetHoverTexture(SDL_Texture* tex) { hoverTexture = tex; }
	UIButton(int id, SDL_Rect bounds, const char* text);
	virtual ~UIButton();

	// Called each loop iteration
	bool Update(float dt);

	bool CleanUp() override;

private:

	bool canClick = true;
	bool drawBasic = false;
	float animT = 0.0f;
};