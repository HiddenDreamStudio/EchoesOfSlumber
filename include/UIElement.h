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

	UIElement() : type(UIElementType::BUTTON), id(-1), state(UIElementState::NORMAL), texture(nullptr), observer(nullptr), alphaMod(1.0f) {
		bounds = { 0, 0, 0, 0 };
		section = { 0, 0, 0, 0 };
		color = { 255, 255, 255, 255 };
	}

	// Constructor
	UIElement(UIElementType type, int id) : type(type), id(id), state(UIElementState::NORMAL), texture(nullptr), observer(nullptr), alphaMod(1.0f) {
		bounds = { 0, 0, 0, 0 };
		section = { 0, 0, 0, 0 };
		color = { 255, 255, 255, 255 };
	}

	// Constructor
	UIElement(UIElementType type, SDL_Rect bounds, const char* text) :
		type(type),
		state(UIElementState::NORMAL),
		bounds(bounds),
		text(text),
		texture(nullptr),
		observer(nullptr),
		alphaMod(1.0f),
		id(-1)
	{
		color = { 255, 255, 255, 255 };
		section = { 0, 0, 0, 0 };
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

	int id = -1;
	UIElementType type = UIElementType::BUTTON;
	UIElementState state = UIElementState::NORMAL;

	std::string text;       // UIElement text (if required)
	SDL_Rect bounds = { 0, 0, 0, 0 };        // Position and size
	SDL_Color color = { 255, 255, 255, 255 };        // Tint color

	SDL_Texture* texture = nullptr;   // Texture atlas reference
	SDL_Rect section = { 0, 0, 0, 0 };       // Texture atlas base section

	Module* observer = nullptr;        // Observer 

	bool pendingToDelete = false;
	bool isVisible = true;
	float alphaMod = 1.0f;  // Global alpha multiplier (0.0 to 1.0)
};