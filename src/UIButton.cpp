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
		auto& render = *Engine::GetInstance().render;

		bool isHovered = (state == UIElementState::FOCUSED || state == UIElementState::PRESSED);

		float targetT = 0.0f;
		switch (state) {
		case UIElementState::FOCUSED: targetT = 1.0f;  break;
		case UIElementState::PRESSED: targetT = 0.5f;  break;
		default:                      targetT = 0.0f;  break;
		}

		float lerpAmount = 0.02f * dt;
		if (lerpAmount > 1.0f) lerpAmount = 1.0f;
		animT += (targetT - animT) * lerpAmount;

		float scaleAnim = 1.0f + (0.06f * animT);

		// ── Determine Texture & Measurements ─────────────────────────────────
		SDL_Texture* activeTex = (isHovered && hoverTexture) ? hoverTexture : texture;

		float tw, th, baseW, baseH;
		SDL_GetTextureSize(activeTex, &tw, &th);
		SDL_GetTextureSize(texture, &baseW, &baseH);

		// Calculate UI scale based on the original texture vs logical bounds
		float uiScaleX = (float)bounds.w / baseW;
		float uiScaleY = (float)bounds.h / baseH;

		SDL_Rect renderBounds;
		renderBounds.w = (int)(tw * uiScaleX * scaleAnim);
		renderBounds.h = (int)(th * uiScaleY * scaleAnim);
		renderBounds.x = bounds.x + bounds.w / 2 - renderBounds.w / 2;
		renderBounds.y = bounds.y + bounds.h / 2 - renderBounds.h / 2;

		Uint8 alpha = (state == UIElementState::DISABLED)
			? (Uint8)(120 * alphaMod)
			: (Uint8)((isHovered ? 255 : 230) * alphaMod);

		// ── Restore Original Functionality (Color Animation) ─────────────────
		// Transition: Dark (Normal) -> Pure White (Hover)
		SDL_Color textColor;
		textColor.r = (Uint8)(40 + (255 - 40) * animT);
		textColor.g = (Uint8)(35 + (255 - 35) * animT);
		textColor.b = (Uint8)(30 + (255 - 30) * animT);
		textColor.a = (Uint8)(255 * alphaMod);

		// Transition: Pure White (Normal) -> Near Black (Hover)
		Uint8 texR = (Uint8)(255 + (25 - 255) * animT);
		Uint8 texG = (Uint8)(255 + (22 - 255) * animT);
		Uint8 texB = (Uint8)(255 + (20 - 255) * animT);

		SDL_SetTextureColorMod(activeTex, texR, texG, texB);
		render.DrawTextureAlpha(activeTex,
			renderBounds.x, renderBounds.y,
			renderBounds.w, renderBounds.h,
			alpha);
		SDL_SetTextureColorMod(activeTex, 255, 255, 255); // Reset

		// Draw text centered on the LOGICAL area (to keep it stable)
		// but matching the animation scale
		SDL_Rect textBounds = renderBounds;
		textBounds.y += (int)(5 * scaleAnim);
		render.DrawMenuTextCentered(text.c_str(), textBounds, textColor);
	}
	else {
		// ── Renderizado por defecto con rectángulos (sin textura) ──────────────
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
