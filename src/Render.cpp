#include "Engine.h"
#include "Window.h"
#include "Render.h"
#include "Log.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Render::Render() : Module()
{
	name = "render";
	background.r = 0;
	background.g = 0;
	background.b = 0;
	background.a = 0;
}

// Destructor
Render::~Render()
{
}

// Called before render is available
bool Render::Awake()
{
	LOG("Create SDL rendering context");
	bool ret = true;

	int scale = Engine::GetInstance().window->GetScale();
	SDL_Window* window = Engine::GetInstance().window->window;

	// SDL3: no flags; create default renderer and set vsync separately
	renderer = SDL_CreateRenderer(window, nullptr);

	if (renderer == NULL)
	{
		LOG("Could not create the renderer! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}
	else
	{
		if (configParameters.child("vsync").attribute("value").as_bool())
		{
			if (!SDL_SetRenderVSync(renderer, 1))
			{
				LOG("Warning: could not enable vsync: %s", SDL_GetError());
			}
			else
			{
				LOG("Using vsync");
			}
		}

		camera.w = Engine::GetInstance().window->width * scale;
		camera.h = Engine::GetInstance().window->height * scale;
		camera.x = 0;
		camera.y = 0;
	}

	//initialise the SDL_ttf library
	TTF_Init();

	//load a font into memory
	font = TTF_OpenFont("Assets/Fonts/arial.ttf", 25);

	return ret;
}

// Called before the first frame
bool Render::Start()
{
	LOG("render start");
	// back background
	if (!SDL_GetRenderViewport(renderer, &viewport))
	{
		LOG("SDL_GetRenderViewport failed: %s", SDL_GetError());
	}
	return true;
}

// Called each loop iteration
bool Render::PreUpdate()
{
	SDL_RenderClear(renderer);
	return true;
}

bool Render::Update(float dt)
{
	return true;
}

bool Render::PostUpdate()
{
	SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, background.a);
	SDL_RenderPresent(renderer);
	return true;
}

// Called before quitting
bool Render::CleanUp()
{
	LOG("Destroying SDL render");
	SDL_DestroyRenderer(renderer);
	return true;
}

void Render::SetBackgroundColor(SDL_Color color)
{
	background = color;
}

void Render::SetViewPort(const SDL_Rect& rect)
{
	SDL_SetRenderViewport(renderer, &rect);
}

void Render::ResetViewPort()
{
	SDL_SetRenderViewport(renderer, &viewport);
}

// Blit to screen
bool Render::DrawTexture(SDL_Texture* texture, int x, int y, const SDL_Rect* section, float speed, double angle, int pivotX, int pivotY) const
{
	bool ret = true;
	int scale = Engine::GetInstance().window->GetScale();

	// SDL3 uses float rects for rendering
	SDL_FRect rect;
	rect.x = (float)((int)(camera.x) + x * scale);
	rect.y = (float)((int)(camera.y) + y * scale);

	if (section != NULL)
	{
		rect.w = (float)(section->w * scale * speed);
		rect.h = (float)(section->h * scale * speed);
	}
	else
	{
		float tw = 0.0f, th = 0.0f;
		if (!SDL_GetTextureSize(texture, &tw, &th))
		{
			LOG("SDL_GetTextureSize failed: %s", SDL_GetError());
			return false;
		}
		rect.w = tw * scale;
		rect.h = th * scale;
	}

	const SDL_FRect* src = NULL;
	SDL_FRect srcRect;
	if (section != NULL)
	{
		srcRect.x = (float)section->x;
		srcRect.y = (float)section->y;
		srcRect.w = (float)section->w;
		srcRect.h = (float)section->h;
		src = &srcRect;
	}

	SDL_FPoint* p = NULL;
	SDL_FPoint pivot;
	if (pivotX != INT_MAX && pivotY != INT_MAX)
	{
		pivot.x = (float)pivotX;
		pivot.y = (float)pivotY;
		p = &pivot;
	}

	// SDL3: returns bool; map to int-style check
	int rc = SDL_RenderTextureRotated(renderer, texture, src, &rect, angle, p, SDL_FLIP_NONE) ? 0 : -1;
	if (rc != 0)
	{
		LOG("Cannot blit to screen. SDL_RenderTextureRotated error: %s", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Render::DrawRectangle(const SDL_Rect& rect, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool filled, bool use_camera) const
{
	bool ret = true;
	int scale = Engine::GetInstance().window->GetScale();

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);

	SDL_FRect rec;
	if (use_camera)
	{
		rec.x = (float)((int)(camera.x + rect.x * scale));
		rec.y = (float)((int)(camera.y + rect.y * scale));
		rec.w = (float)(rect.w * scale);
		rec.h = (float)(rect.h * scale);
	}
	else
	{
		rec.x = (float)(rect.x * scale);
		rec.y = (float)(rect.y * scale);
		rec.w = (float)(rect.w * scale);
		rec.h = (float)(rect.h * scale);
	}

	int result = (filled ? SDL_RenderFillRect(renderer, &rec) : SDL_RenderRect(renderer, &rec)) ? 0 : -1;

	if (result != 0)
	{
		LOG("Cannot draw quad to screen. SDL_RenderFillRect/SDL_RenderRect error: %s", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Render::DrawLine(int x1, int y1, int x2, int y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool use_camera) const
{
	bool ret = true;
	int scale = Engine::GetInstance().window->GetScale();

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);

	float X1, Y1, X2, Y2;

	if (use_camera)
	{
		X1 = (float)(camera.x + x1 * scale);
		Y1 = (float)(camera.y + y1 * scale);
		X2 = (float)(camera.x + x2 * scale);
		Y2 = (float)(camera.y + y2 * scale);
	}
	else
	{
		X1 = (float)(x1 * scale);
		Y1 = (float)(y1 * scale);
		X2 = (float)(x2 * scale);
		Y2 = (float)(y2 * scale);
	}

	int result = SDL_RenderLine(renderer, X1, Y1, X2, Y2) ? 0 : -1;

	if (result != 0)
	{
		LOG("Cannot draw quad to screen. SDL_RenderLine error: %s", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Render::DrawCircle(int x, int y, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool use_camera) const
{
	bool ret = true;
	int scale = Engine::GetInstance().window->GetScale();

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);

	int result = -1;
	SDL_FPoint points[360];

	float factor = (float)M_PI / 180.0f;

	float cx = (float)((use_camera ? camera.x : 0) + x * scale);
	float cy = (float)((use_camera ? camera.y : 0) + y * scale);

	for (int i = 0; i < 360; ++i)
	{
		points[i].x = cx + (float)(radius * cos(i * factor));
		points[i].y = cy + (float)(radius * sin(i * factor));
	}

	result = SDL_RenderPoints(renderer, points, 360) ? 0 : -1;

	if (result != 0)
	{
		LOG("Cannot draw quad to screen. SDL_RenderPoints error: %s", SDL_GetError());
		ret = false;
	}

	return ret;
}

// Draw text on screen using SDL3_ttf
bool Render::DrawText(const char* text, int x, int y, int w, int h, SDL_Color color) const
{
	if (!font || !renderer || !text) {
		LOG("DrawText: invalid font/renderer/text");
		return false;
	}

	// Render the text to a surface
	// SDL3_ttf: length can be 0 for null-terminated strings
	SDL_Surface* surface = TTF_RenderText_Solid(font, text, 0, color);
	if (!surface) {
		LOG("DrawText: TTF_RenderText_Solid failed: %s", SDL_GetError());
		return false;
	}

	// Create a texture from the surface
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture) {
		LOG("DrawText: SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
		SDL_DestroySurface(surface);
		return false;
	}

	// Optional but often needed when using alpha/text
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

	// If w/h are 0, use the text’s natural size
	float fw = (w > 0) ? (float)w : (float)surface->w;
	float fh = (h > 0) ? (float)h : (float)surface->h;

	SDL_FRect dstrect = { (float)x, (float)y, fw, fh };

	// Render the texture to the current render target
	if (!SDL_RenderTexture(renderer, texture, nullptr, &dstrect)) {
		LOG("DrawText: SDL_RenderTexture failed: %s", SDL_GetError());
	}

	// Cleanup
	SDL_DestroyTexture(texture);
	SDL_DestroySurface(surface);

	return true;
}

// Camera frustum culling: returns true if the world-space rect is visible on screen
bool Render::IsOnScreenWorldRect(float x, float y, float w, float h, int margin) const
{
	float camX = -camera.x;
	float camY = -camera.y;
	float screenW = static_cast<float>(camera.w);
	float screenH = static_cast<float>(camera.h);

	// Check if rect is outside the camera viewport (with margin)
	if (x + w + margin < camX) return false;
	if (x - margin > camX + screenW) return false;
	if (y + h + margin < camY) return false;
	if (y - margin > camY + screenH) return false;

	return true;
}

// Camera System: Set target position for camera to follow
void Render::SetCameraTarget(float x, float y)
{
	cameraTargetX_ = x;
	cameraTargetY_ = y;

	// On first call, snap camera to target immediately (no lerp)
	if (!cameraInitialized_)
	{
		float viewW = GetWorldViewportWidth();
		float viewH = GetWorldViewportHeight();
		camera.x = static_cast<int>(-x + viewW / 2);
		camera.y = static_cast<int>(-y + viewH / 2);
		cameraInitialized_ = true;
	}
}

// Smooth follow using lerp with dead zones
void Render::FollowTarget(float dt)
{
	// Get world-space viewport dimensions (accounts for window scale)
	float viewW = GetWorldViewportWidth();
	float viewH = GetWorldViewportHeight();

	// Current camera center in world space
	float currentX = -camera.x + viewW / 2.0f;
	float currentY = -camera.y + viewH / 2.0f;

	// Target position (center of screen should be here)
	float targetX = cameraTargetX_;
	float targetY = cameraTargetY_;

	// Dead zone check - only move camera if target is outside dead zone
	float deltaX = targetX - currentX;
	float deltaY = targetY - currentY;

	// Apply dead zone - if within dead zone, don't move
	if (std::abs(deltaX) <= deadZoneWidth_)
	{
		targetX = currentX;
	}
	else
	{
		// Reduce delta by dead zone size (camera follows outside dead zone)
		targetX = currentX + (deltaX > 0 ? deltaX - deadZoneWidth_ : deltaX + deadZoneWidth_);
	}

	if (std::abs(deltaY) <= deadZoneHeight_)
	{
		targetY = currentY;
	}
	else
	{
		targetY = currentY + (deltaY > 0 ? deltaY - deadZoneHeight_ : deltaY + deadZoneHeight_);
	}

	// Lerp towards target position
	float lerpFactor = 1.0f - std::exp(-cameraSmoothSpeed_ * dt / 1000.0f);
	float newX = currentX + (targetX - currentX) * lerpFactor;
	float newY = currentY + (targetY - currentY) * lerpFactor;

	// Convert back to camera coordinates (negative because camera offset is inverted)
	camera.x = static_cast<int>(-(newX - viewW / 2.0f));
	camera.y = static_cast<int>(-(newY - viewH / 2.0f));
}

// Camera System - Issue #21: Set camera position directly (for cutscenes, etc.)
void Render::SetCameraPosition(float x, float y)
{
	float viewW = GetWorldViewportWidth();
	float viewH = GetWorldViewportHeight();
	camera.x = static_cast<int>(-x + viewW / 2);
	camera.y = static_cast<int>(-y + viewH / 2);

	// Keep internal camera follow state in sync with the direct position
	cameraTargetX_ = x;
	cameraTargetY_ = y;
	cameraInitialized_ = true;
}

// Camera System - Issue #21: Clamp camera to map boundaries
void Render::ClampCameraToMapBounds(float mapWidth, float mapHeight)
{
	// Get world-space viewport dimensions (accounts for window scale)
	float viewW = GetWorldViewportWidth();
	float viewH = GetWorldViewportHeight();

	// Camera X bounds
	if (camera.x > 0)
	{
		camera.x = 0;  // Left edge
	}
	if (camera.x < -(mapWidth - viewW))
	{
		camera.x = static_cast<int>(-(mapWidth - viewW));  // Right edge
	}

	// Camera Y bounds
	if (camera.y > 0)
	{
		camera.y = 0;  // Top edge
	}
	if (camera.y < -(mapHeight - viewH))
	{
		camera.y = static_cast<int>(-(mapHeight - viewH));  // Bottom edge
	}

	// Handle edge case where map is smaller than camera
	if (mapWidth < viewW)
	{
		camera.x = static_cast<int>((viewW - mapWidth) / 2.0f);
	}
	if (mapHeight < viewH)
	{
		camera.y = static_cast<int>((viewH - mapHeight) / 2.0f);
	}
}

// Camera System - Issue #21: Set dead zone size
void Render::SetDeadZone(float width, float height)
{
	// Clamp dead zone dimensions to non-negative values
	deadZoneWidth_ = std::fmax(0.0f, width);
	deadZoneHeight_ = std::fmax(0.0f, height);
}

// Camera System - Issue #21: Set camera follow speed
void Render::SetCameraSmoothSpeed(float speed)
{
	// Clamp camera smooth speed to non-negative values
	cameraSmoothSpeed_ = std::fmax(0.0f, speed);
}

// Camera System - Issue #21: Get current camera center position in world space
Vector2D Render::GetCameraPosition() const
{
	float viewW = GetWorldViewportWidth();
	float viewH = GetWorldViewportHeight();
	return Vector2D(
		static_cast<float>(-camera.x + viewW / 2),
		static_cast<float>(-camera.y + viewH / 2)
	);
}

// Camera System: Get world-space viewport width (accounts for window scale)
float Render::GetWorldViewportWidth() const
{
	int scale = Engine::GetInstance().window->GetScale();
	return static_cast<float>(camera.w) / scale;
}

// Camera System: Get world-space viewport height (accounts for window scale)
float Render::GetWorldViewportHeight() const
{
	int scale = Engine::GetInstance().window->GetScale();
	return static_cast<float>(camera.h) / scale;
}


