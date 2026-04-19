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
	if (!isVisible) return false;

	if (state != UIElementState::DISABLED)
	{
		Vector2D mousePos = Engine::GetInstance().input->GetMousePosition();

		bool hovered = (mousePos.getX() > (float)bounds.x &&
			mousePos.getX() < (float)bounds.x + (float)bounds.w &&
			mousePos.getY() > (float)bounds.y &&
			mousePos.getY() < (float)bounds.y + (float)bounds.h);

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
	}

	// Vertical centre: font is 25px tall
	int textY = bounds.y + (bounds.h - FONT_H) / 2;
	int textX = bounds.x + 16;

	if (texture) {
		// Texture-backed button (stone buttons, etc.)
		auto& render = *Engine::GetInstance().render;
		Uint8 alpha = 230;

		float targetT = 0.0f;
		switch (state) {
		case UIElementState::FOCUSED:
			alpha = 255;
			targetT = 1.0f;
			break;
		case UIElementState::PRESSED:
			alpha = 255;
			targetT = 0.5f; // Pressed is halfway between normal and focused
			break;
		case UIElementState::DISABLED:
			alpha = 120;
			targetT = 0.0f;
			break;
		default:
			alpha = 230;
			targetT = 0.0f;
			break;
		}

		// Smooth transition
		float lerpAmount = 0.02f * dt;
		if (lerpAmount > 1.0f) lerpAmount = 1.0f;
		animT += (targetT - animT) * lerpAmount;

		// Expand button scale based on animT (0.0 = 1.0x scale, 1.0 = 1.06x scale)
		float scaleW = 1.0f + (0.06f * animT);
		float scaleH = 1.0f + (0.06f * animT);

		SDL_Rect renderBounds;
		renderBounds.w = (int)(bounds.w * scaleW);
		renderBounds.h = (int)(bounds.h * scaleH);
		renderBounds.x = bounds.x - (renderBounds.w - bounds.w) / 2;
		renderBounds.y = bounds.y - (renderBounds.h - bounds.h) / 2;

		// Interpolate Colors
		// TEXT COLOR: Normally { 50, 45, 40 } -> FOCUSED { 240, 240, 240 }
		SDL_Color textColor;
		textColor.r = (Uint8)(50 + (240 - 50) * animT);
		textColor.g = (Uint8)(45 + (240 - 45) * animT);
		textColor.b = (Uint8)(40 + (240 - 40) * animT);
		textColor.a = (Uint8)(255 * alphaMod);

		// TEXTURE COLOR: Normally { 255, 255, 255 } -> FOCUSED { 50, 45, 40 }
		Uint8 texR = (Uint8)(255 + (50 - 255) * animT);
		Uint8 texG = (Uint8)(255 + (45 - 255) * animT);
		Uint8 texB = (Uint8)(255 + (40 - 255) * animT);

		SDL_SetTextureColorMod(texture, texR, texG, texB);
		render.DrawTextureAlpha(texture, renderBounds.x, renderBounds.y, renderBounds.w, renderBounds.h, (Uint8)(alpha * alphaMod));
		SDL_SetTextureColorMod(texture, 255, 255, 255); // Reset

		// Shift text slightly downwards for visual centering on the stone texture
		SDL_Rect textBounds = renderBounds;
		textBounds.y += (int)(1 * scaleH);
		render.DrawMenuTextCentered(text.c_str(), textBounds, textColor);
	}
	else {
		// Original rectangle-based rendering
		SDL_Rect shadow = { bounds.x + 3, bounds.y + 3, bounds.w, bounds.h };

		switch (state)
		{
		case UIElementState::NORMAL:
			Engine::GetInstance().render->DrawRectangle(shadow, 0, 0, 0, (Uint8)(80 * alphaMod), true, false);
			Engine::GetInstance().render->DrawRectangle(bounds, 15, 15, 25, (Uint8)(220 * alphaMod), true, false);
			{
				SDL_Rect accent = { bounds.x, bounds.y, 3, bounds.h };
				Engine::GetInstance().render->DrawRectangle(accent, 80, 140, 200, (Uint8)(255 * alphaMod), true, false);
			}
			Engine::GetInstance().render->DrawRectangle(bounds, 60, 80, 120, (Uint8)(180 * alphaMod), false, false);
			Engine::GetInstance().render->DrawText(text.c_str(), textX, textY, 0, 0, { 180, 200, 220, (Uint8)(255 * alphaMod) });
			break;

		case UIElementState::FOCUSED:
			Engine::GetInstance().render->DrawRectangle(shadow, 0, 0, 0, (Uint8)(60 * alphaMod), true, false);
			Engine::GetInstance().render->DrawRectangle(bounds, 25, 35, 60, (Uint8)(240 * alphaMod), true, false);
			{
				SDL_Rect accent = { bounds.x, bounds.y, 4, bounds.h };
				Engine::GetInstance().render->DrawRectangle(accent, 100, 180, 255, (Uint8)(255 * alphaMod), true, false);
			}
			Engine::GetInstance().render->DrawRectangle(bounds, 80, 120, 180, (Uint8)(220 * alphaMod), false, false);
			Engine::GetInstance().render->DrawText(text.c_str(), textX, textY, 0, 0, { 220, 235, 255, (Uint8)(255 * alphaMod) });
			break;

		case UIElementState::PRESSED:
			Engine::GetInstance().render->DrawRectangle(bounds, 10, 60, 120, (Uint8)(255 * alphaMod), true, false);
			{
				SDL_Rect accent = { bounds.x, bounds.y, 4, bounds.h };
				Engine::GetInstance().render->DrawRectangle(accent, 140, 220, 255, (Uint8)(255 * alphaMod), true, false);
			}
			Engine::GetInstance().render->DrawRectangle(bounds, 100, 160, 220, (Uint8)(255 * alphaMod), false, false);
			Engine::GetInstance().render->DrawText(text.c_str(), textX + 2, textY + 1, 0, 0, { 255, 255, 255, (Uint8)(255 * alphaMod) });
			break;

		default:
			Engine::GetInstance().render->DrawRectangle(bounds, 40, 40, 40, (Uint8)(160 * alphaMod), true, false);
			Engine::GetInstance().render->DrawText(text.c_str(), textX, textY, 0, 0, { 100, 100, 100, (Uint8)(200 * alphaMod) });
			break;
		}
	}

	return false;
}

bool UIButton::CleanUp()
{
	pendingToDelete = true;
	return true;
}