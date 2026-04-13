#include "UIButton.h"
#include "Render.h"
#include "Engine.h"
#include "Audio.h"

static constexpr int FONT_H = 25;

UIButton::UIButton(int id, SDL_Rect bounds, const char* text) : UIElement(UIElementType::BUTTON, id)
{
	this->bounds = bounds;
	this->text = text;
	canClick = true;
	drawBasic = false;
}

UIButton::~UIButton() {}

bool UIButton::Update(float dt)
{
	if (state == UIElementState::DISABLED) return false;

	Vector2D mousePos = Engine::GetInstance().input->GetMousePosition();

	bool hovered = (mousePos.getX() > bounds.x &&
		mousePos.getX() < bounds.x + bounds.w &&
		mousePos.getY() > bounds.y &&
		mousePos.getY() < bounds.y + bounds.h);

	if (hovered)
	{
		state = UIElementState::FOCUSED;
		if (Engine::GetInstance().input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_REPEAT)
			state = UIElementState::PRESSED;
		if (Engine::GetInstance().input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_UP)
			NotifyObserver();
	}
	else
	{
		state = UIElementState::NORMAL;
	}

	// Vertical centre: font is 25px tall
	int textY = bounds.y + (bounds.h - FONT_H) / 2;
	int textX = bounds.x + 16;

	SDL_Rect shadow = { bounds.x + 3, bounds.y + 3, bounds.w, bounds.h };

	switch (state)
	{
	case UIElementState::NORMAL:
		Engine::GetInstance().render->DrawRectangle(shadow, 0, 0, 0, 80, true, false);
		Engine::GetInstance().render->DrawRectangle(bounds, 15, 15, 25, 220, true, false);
		{
			SDL_Rect accent = { bounds.x, bounds.y, 3, bounds.h };
			Engine::GetInstance().render->DrawRectangle(accent, 80, 140, 200, 255, true, false);
		}
		Engine::GetInstance().render->DrawRectangle(bounds, 60, 80, 120, 180, false, false);
		Engine::GetInstance().render->DrawText(text.c_str(), textX, textY, 0, 0, { 180, 200, 220, 255 });
		break;

	case UIElementState::FOCUSED:
		Engine::GetInstance().render->DrawRectangle(shadow, 0, 0, 0, 60, true, false);
		Engine::GetInstance().render->DrawRectangle(bounds, 25, 35, 60, 240, true, false);
		{
			SDL_Rect accent = { bounds.x, bounds.y, 4, bounds.h };
			Engine::GetInstance().render->DrawRectangle(accent, 100, 180, 255, 255, true, false);
		}
		Engine::GetInstance().render->DrawRectangle(bounds, 80, 120, 180, 220, false, false);
		Engine::GetInstance().render->DrawText(text.c_str(), textX, textY, 0, 0, { 220, 235, 255, 255 });
		break;

	case UIElementState::PRESSED:
		Engine::GetInstance().render->DrawRectangle(bounds, 10, 60, 120, 255, true, false);
		{
			SDL_Rect accent = { bounds.x, bounds.y, 4, bounds.h };
			Engine::GetInstance().render->DrawRectangle(accent, 140, 220, 255, 255, true, false);
		}
		Engine::GetInstance().render->DrawRectangle(bounds, 100, 160, 220, 255, false, false);
		Engine::GetInstance().render->DrawText(text.c_str(), textX + 2, textY + 1, 0, 0, { 255, 255, 255, 255 });
		break;

	default:
		Engine::GetInstance().render->DrawRectangle(bounds, 40, 40, 40, 160, true, false);
		Engine::GetInstance().render->DrawText(text.c_str(), textX, textY, 0, 0, { 100, 100, 100, 200 });
		break;
	}

	return false;
}

bool UIButton::CleanUp()
{
	pendingToDelete = true;
	return true;
}