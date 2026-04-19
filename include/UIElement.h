#pragma once

#include "Input.h"
#include "Render.h"
#include "Module.h"
#include "Vector2D.h"

enum class UIElementType
{
	BUTTON,
	TOGGLE,
	CHECKBOX,
	SLIDER,
	SLIDERBAR,
	COMBOBOX,
	DROPDOWNBOX,
	INPUTBOX,
	VALUEBOX,
	SPINNER
};

enum class UIElementState
{
	DISABLED,
	NORMAL,
	FOCUSED,
	PRESSED,
	SELECTED
};

class UIElement : public std::enable_shared_from_this<UIElement>
{
public:

	UIElement() {}

	// Constructor
	UIElement(UIElementType type, int id) : type(type), id(id), state(UIElementState::NORMAL) {}

	// Constructor
	UIElement(UIElementType type, SDL_Rect bounds, const char* text) :
		type(type),
		state(UIElementState::NORMAL),
		bounds(bounds),
		text(text)
	{
		color.r = 255; color.g = 255; color.b = 255;
		texture = NULL;
	}

	// Called each loop iteration
	virtual bool Update(float dt)
	{
		return true;
	}

	// 
	void SetTexture(SDL_Texture* tex)
	{
		texture = tex;
		section = { 0, 0, 0, 0 };
	}

	// 
	void SetObserver(Module* module)
	{
		observer = module;
	}

	// 
	void NotifyObserver()
	{
		observer->OnUIMouseClickEvent(this);
	}

	virtual bool CleanUp()
	{
		return true;
	}

	virtual bool Destroy()
	{
		return true;
	}

public:

	int id;
	UIElementType type;
	UIElementState state;

	std::string text;       // UIElement text (if required)
	SDL_Rect bounds;        // Position and size
	SDL_Color color;        // Tint color

	SDL_Texture* texture;   // Texture atlas reference
	SDL_Rect section;       // Texture atlas base section

	Module* observer;        // Observer 

	bool pendingToDelete = false;
	
	float alphaMod = 1.0f;  // Global alpha multiplier (0.0 to 1.0)
};