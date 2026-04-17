#pragma once

#include "Module.h"
#include "Vector2D.h"
#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"

enum class FadeDirection { FADE_IN, FADE_OUT };

class Render : public Module
{
public:

	Render();

	// Destructor
	virtual ~Render();

	// Called before render is available
	bool Awake();

	// Called before the first frame
	bool Start();

	// Called each loop iteration
	bool PreUpdate();
	bool Update(float dt);
	bool PostUpdate();

	// Called before quitting
	bool CleanUp();

	void SetViewPort(const SDL_Rect& rect);
	void ResetViewPort();

	// Drawing
	bool DrawTexture(SDL_Texture* texture, int x, int y, const SDL_Rect* section = NULL, float speed = 1.0f, double angle = 0, int pivotX = INT_MAX, int pivotY = INT_MAX, SDL_FlipMode flip = SDL_FLIP_NONE, float drawScale = 1.0f) const;
	bool DrawRectangle(const SDL_Rect& rect, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255, bool filled = true, bool useCamera = true) const;
	bool DrawLine(int x1, int y1, int x2, int y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255, bool useCamera = true) const;
	bool DrawCircle(int x1, int y1, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255, bool useCamera = true) const;
	bool DrawText(const char* text, int x, int y, int w, int h, SDL_Color color) const;
	bool DrawTextureAlpha(SDL_Texture* texture, int x, int y, int w, int h, Uint8 alpha = 255) const;
	bool DrawTextureAlphaF(SDL_Texture* texture, float x, float y, float w, float h, Uint8 alpha = 255) const;
	bool DrawMenuText(const char* text, int x, int y, int w, int h, SDL_Color color) const;
	bool DrawMenuTextCentered(const char* text, SDL_Rect area, SDL_Color color) const;
	SDL_Texture* CreateMenuTextTexture(const char* text, SDL_Color color) const;
	SDL_Texture* RecolorTexture(SDL_Texture* src, Uint8 r, Uint8 g, Uint8 b) const;

	// Fade overlay system
	void StartFade(FadeDirection dir, float durationMs);
	void UpdateFade(float dt);
	void DrawFade();
	bool IsFadeComplete() const;
	Uint8 GetFadeAlpha() const;

	// Set background color
	void SetBackgroundColor(SDL_Color color);

	bool IsOnScreenWorldRect(float x, float y, float w, float h, int margin = 0) const;

	// Camera System
	void SetCameraTarget(float x, float y);
	void FollowTarget(float dt);
	void SetCameraPosition(float x, float y);
	void ClampCameraToMapBounds(float mapWidth, float mapHeight);
	void SetDeadZone(float width, float height);
	void SetCameraSmoothSpeed(float speed);
	Vector2D GetCameraPosition() const;

public:

	SDL_Renderer* renderer;
	SDL_Rect camera;
	SDL_Rect viewport;
	SDL_Color background;

private:
	bool vsync = false;
	TTF_Font* font;
	TTF_Font* menuFont = nullptr;

	// Camera System - Issue #21
	float cameraTargetX_ = 0.0f;
	float cameraTargetY_ = 0.0f;
	float cameraSmoothSpeed_ = 5.0f;  // Lerp speed (higher = faster follow)
	float deadZoneWidth_ = 64.0f;     // Dead zone half-width (world pixels)
	float deadZoneHeight_ = 32.0f;    // Dead zone half-height (world pixels)
	bool cameraInitialized_ = false;

	// Helper to get world-space camera viewport dimensions (accounts for window scale)
	float GetWorldViewportWidth() const;
	float GetWorldViewportHeight() const;

	// Fade overlay
	bool fadeActive_ = false;
	FadeDirection fadeDir_ = FadeDirection::FADE_IN;
	float fadeDurationMs_ = 500.0f;
	float fadeElapsedMs_ = 0.0f;
	Uint8 fadeAlpha_ = 0;
};
