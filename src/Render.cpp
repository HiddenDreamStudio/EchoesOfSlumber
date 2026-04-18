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

	menuFont = TTF_OpenFont("assets/fonts/Tiraroum.ttf", 60);
	if (!menuFont) {
		LOG("Warning: could not load menu font Tiraroum.ttf: %s", SDL_GetError());
	}

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
	UpdateFade(dt);
	return true;
}

bool Render::PostUpdate()
{
	DrawFade();
	SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, background.a);
	SDL_RenderPresent(renderer);
	return true;
}

// Called before quitting
bool Render::CleanUp()
{
	LOG("Destroying SDL render");
	if (menuFont) {
		TTF_CloseFont(menuFont);
		menuFont = nullptr;
	}
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
bool Render::DrawTexture(SDL_Texture* texture, int x, int y, const SDL_Rect* section, float speed, double angle, int pivotX, int pivotY, SDL_FlipMode flip, float drawScale) const
{
	bool ret = true;
	int scale = Engine::GetInstance().window->GetScale();

	// SDL3 uses float rects for rendering
	SDL_FRect rect;
	rect.x = (float)((int)(camera.x) + x * scale);
	rect.y = (float)((int)(camera.y) + y * scale);

	if (section != NULL)
	{
		rect.w = (float)(section->w * scale * drawScale);
		rect.h = (float)(section->h * scale * drawScale);
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

	// SDL3: returns bool; use the flip parameter
	int rc = SDL_RenderTextureRotated(renderer, texture, src, &rect, angle, p, flip) ? 0 : -1;
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
// FIX: applies window scale so text aligns correctly with DrawRectangle elements
bool Render::DrawText(const char* text, int x, int y, int w, int h, SDL_Color color) const
{
	if (!font || !renderer || !text) {
		LOG("DrawText: invalid font/renderer/text");
		return false;
	}

	int scale = Engine::GetInstance().window->GetScale();

	SDL_Surface* surface = TTF_RenderText_Solid(font, text, 0, color);
	if (!surface) {
		LOG("DrawText: TTF_RenderText_Solid failed: %s", SDL_GetError());
		return false;
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture) {
		LOG("DrawText: SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
		SDL_DestroySurface(surface);
		return false;
	}

	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(texture, color.a);

	// Apply scale to position and size — same convention as DrawRectangle (use_camera=false)
	float fx = (float)(x * scale);
	float fy = (float)(y * scale);
	// If caller passes explicit size, scale it; otherwise use natural surface size (sharp)
	float fw = (w > 0) ? (float)(w * scale) : (float)(surface->w);
	float fh = (h > 0) ? (float)(h * scale) : (float)(surface->h);

	SDL_FRect dstrect = { fx, fy, fw, fh };

	if (!SDL_RenderTexture(renderer, texture, nullptr, &dstrect)) {
		LOG("DrawText: SDL_RenderTexture failed: %s", SDL_GetError());
	}

	SDL_SetTextureAlphaMod(texture, 255);
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
	float viewW = GetWorldViewportWidth();
	float viewH = GetWorldViewportHeight();

	float currentX = -camera.x + viewW / 2.0f;
	float currentY = -camera.y + viewH / 2.0f;

	float targetX = cameraTargetX_;
	float targetY = cameraTargetY_;

	float deltaX = targetX - currentX;
	float deltaY = targetY - currentY;

	if (std::abs(deltaX) <= deadZoneWidth_)
		targetX = currentX;
	else
		targetX = currentX + (deltaX > 0 ? deltaX - deadZoneWidth_ : deltaX + deadZoneWidth_);

	if (std::abs(deltaY) <= deadZoneHeight_)
		targetY = currentY;
	else
		targetY = currentY + (deltaY > 0 ? deltaY - deadZoneHeight_ : deltaY + deadZoneHeight_);

	float lerpFactor = 1.0f - std::exp(-cameraSmoothSpeed_ * dt / 1000.0f);
	float newX = currentX + (targetX - currentX) * lerpFactor;
	float newY = currentY + (targetY - currentY) * lerpFactor;

	camera.x = static_cast<int>(-(newX - viewW / 2.0f));
	camera.y = static_cast<int>(-(newY - viewH / 2.0f));
}

void Render::SetCameraPosition(float x, float y)
{
	float viewW = GetWorldViewportWidth();
	float viewH = GetWorldViewportHeight();
	camera.x = static_cast<int>(-x + viewW / 2);
	camera.y = static_cast<int>(-y + viewH / 2);
	cameraTargetX_ = x;
	cameraTargetY_ = y;
	cameraInitialized_ = true;
}

void Render::ClampCameraToMapBounds(float mapWidth, float mapHeight)
{
	float viewW = GetWorldViewportWidth();
	float viewH = GetWorldViewportHeight();

	if (camera.x > 0) camera.x = 0;
	if (camera.x < -(mapWidth - viewW))
		camera.x = static_cast<int>(-(mapWidth - viewW));

	if (camera.y > 0) camera.y = 0;
	if (camera.y < -(mapHeight - viewH))
		camera.y = static_cast<int>(-(mapHeight - viewH));

	if (mapWidth < viewW)
		camera.x = static_cast<int>((viewW - mapWidth) / 2.0f);
	if (mapHeight < viewH)
		camera.y = static_cast<int>((viewH - mapHeight) / 2.0f);
}

void Render::SetDeadZone(float width, float height)
{
	deadZoneWidth_ = std::fmax(0.0f, width);
	deadZoneHeight_ = std::fmax(0.0f, height);
}

void Render::SetCameraSmoothSpeed(float speed)
{
	cameraSmoothSpeed_ = std::fmax(0.0f, speed);
}

Vector2D Render::GetCameraPosition() const
{
	float viewW = GetWorldViewportWidth();
	float viewH = GetWorldViewportHeight();
	return Vector2D(
		static_cast<float>(-camera.x + viewW / 2),
		static_cast<float>(-camera.y + viewH / 2)
	);
}

float Render::GetWorldViewportWidth() const
{
	int scale = Engine::GetInstance().window->GetScale();
	return static_cast<float>(camera.w) / scale;
}

float Render::GetWorldViewportHeight() const
{
	int scale = Engine::GetInstance().window->GetScale();
	return static_cast<float>(camera.h) / scale;
}

// ── Texture drawing with alpha (screen-space, no camera) ──────────────────────

bool Render::DrawTextureAlpha(SDL_Texture* texture, int x, int y, int w, int h, Uint8 alpha) const
{
	if (!texture) return false;
	int scale = Engine::GetInstance().window->GetScale();

	SDL_FRect dst;
	dst.x = (float)(x * scale);
	dst.y = (float)(y * scale);
	dst.w = (float)(w * scale);
	dst.h = (float)(h * scale);

	SDL_SetTextureAlphaMod(texture, alpha);
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	bool ok = SDL_RenderTexture(renderer, texture, nullptr, &dst);
	SDL_SetTextureAlphaMod(texture, 255);

	return ok;
}

bool Render::DrawTextureAlphaF(SDL_Texture* texture, float x, float y, float w, float h, Uint8 alpha) const
{
	if (!texture) return false;
	int scale = Engine::GetInstance().window->GetScale();

	SDL_FRect dst;
	dst.x = x * scale;
	dst.y = y * scale;
	dst.w = w * scale;
	dst.h = h * scale;

	SDL_SetTextureAlphaMod(texture, alpha);
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	bool ok = SDL_RenderTexture(renderer, texture, nullptr, &dst);
	SDL_SetTextureAlphaMod(texture, 255);

	return ok;
}

// ── Text drawing with menu font (screen-space) ───────────────────────────────

bool Render::DrawMenuText(const char* text, int x, int y, int w, int h, SDL_Color color) const
{
	if (!menuFont || !renderer || !text) return false;

	int scale = Engine::GetInstance().window->GetScale();

	SDL_Surface* surface = TTF_RenderText_Blended(menuFont, text, 0, color);
	if (!surface) return false;

	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
	if (!tex) {
		SDL_DestroySurface(surface);
		return false;
	}

	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(tex, color.a);

	float fx = (float)(x * scale);
	float fy = (float)(y * scale);
	float fw = (w > 0) ? (float)(w * scale) : (float)(surface->w);
	float fh = (h > 0) ? (float)(h * scale) : (float)(surface->h);

	SDL_FRect dstrect = { fx, fy, fw, fh };
	SDL_RenderTexture(renderer, tex, nullptr, &dstrect);

	SDL_SetTextureAlphaMod(tex, 255);
	SDL_DestroyTexture(tex);
	SDL_DestroySurface(surface);

	return true;
}

// ── Centered text with menu font ─────────────────────────────────────────────

bool Render::DrawMenuTextCentered(const char* text, SDL_Rect area, SDL_Color color) const
{
	if (!menuFont || !renderer || !text) return false;

	int scale = Engine::GetInstance().window->GetScale();

	SDL_Surface* surface = TTF_RenderText_Blended(menuFont, text, 0, color);
	if (!surface) return false;

	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
	if (!tex) {
		SDL_DestroySurface(surface);
		return false;
	}

	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(tex, color.a);

	float textW = (float)surface->w;
	float textH = (float)surface->h;
	float areaX = (float)(area.x * scale);
	float areaY = (float)(area.y * scale);
	float areaW = (float)(area.w * scale);
	float areaH = (float)(area.h * scale);

	SDL_FRect dstrect;
	dstrect.w = textW;
	dstrect.h = textH;
	dstrect.x = areaX + (areaW - textW) / 2.0f;
	dstrect.y = areaY + (areaH - textH) / 2.0f;

	SDL_RenderTexture(renderer, tex, nullptr, &dstrect);

	SDL_SetTextureAlphaMod(tex, 255);
	SDL_DestroyTexture(tex);
	SDL_DestroySurface(surface);

	return true;
}

// ── Create texture from menu-font text ───────────────────────────────────────

SDL_Texture* Render::CreateMenuTextTexture(const char* text, SDL_Color color) const
{
	if (!menuFont || !renderer || !text) return nullptr;

	SDL_Surface* surface = TTF_RenderText_Blended(menuFont, text, 0, color);
	if (!surface) return nullptr;

	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_DestroySurface(surface);

	if (tex) {
		SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
	}

	return tex;
}

// ── Recolor texture: replace all pixel colors with target RGB, keep alpha ────

SDL_Texture* Render::RecolorTexture(SDL_Texture* src, Uint8 r, Uint8 g, Uint8 b) const
{
	if (!src || !renderer) return nullptr;

	float tw = 0, th = 0;
	SDL_GetTextureSize(src, &tw, &th);
	int w = (int)tw, h = (int)th;

	// Render source texture to a surface via a temporary target texture
	SDL_Texture* target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_TARGET, w, h);
	if (!target) return nullptr;

	SDL_SetRenderTarget(renderer, target);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, src, nullptr, nullptr);
	SDL_SetRenderTarget(renderer, nullptr);

	// Read pixels from the target texture
	SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
	// We need to read from the target, so set it again
	SDL_SetRenderTarget(renderer, target);
	SDL_Surface* surf = SDL_RenderReadPixels(renderer, nullptr);
	SDL_SetRenderTarget(renderer, nullptr);

	if (!surf) {
		SDL_DestroyTexture(target);
		if (surface) SDL_DestroySurface(surface);
		return nullptr;
	}
	if (surface) SDL_DestroySurface(surface);

	// Recolor all pixels: replace RGB with target color, keep alpha
	SDL_Surface* converted = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
	SDL_DestroySurface(surf);
	if (!converted) {
		SDL_DestroyTexture(target);
		return nullptr;
	}

	SDL_LockSurface(converted);
	Uint32* pixels = (Uint32*)converted->pixels;
	int pixelCount = converted->w * converted->h;
	for (int i = 0; i < pixelCount; i++) {
		Uint8 pr, pg, pb, pa;
		SDL_GetRGBA(pixels[i], SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32), nullptr, &pr, &pg, &pb, &pa);
		if (pa > 0) {
			pixels[i] = SDL_MapRGBA(SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32), nullptr, r, g, b, pa);
		}
	}
	SDL_UnlockSurface(converted);

	SDL_Texture* result = SDL_CreateTextureFromSurface(renderer, converted);
	SDL_DestroySurface(converted);
	SDL_DestroyTexture(target);

	if (result) {
		SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND);
	}

	return result;
}

// ── Fade overlay system ──────────────────────────────────────────────────────

void Render::StartFade(FadeDirection dir, float durationMs)
{
	fadeActive_ = true;
	fadeDir_ = dir;
	fadeDurationMs_ = durationMs;
	fadeElapsedMs_ = 0.0f;
	fadeAlpha_ = (dir == FadeDirection::FADE_IN) ? 255 : 0;
}

void Render::UpdateFade(float dt)
{
	if (!fadeActive_) return;

	fadeElapsedMs_ += dt;
	float t = fadeElapsedMs_ / fadeDurationMs_;
	if (t > 1.0f) t = 1.0f;

	if (fadeDir_ == FadeDirection::FADE_IN) {
		fadeAlpha_ = (Uint8)(255.0f * (1.0f - t));
	} else {
		fadeAlpha_ = (Uint8)(255.0f * t);
	}

	if (fadeElapsedMs_ >= fadeDurationMs_) {
		fadeActive_ = false;
	}
}

void Render::DrawFade()
{
	if (fadeAlpha_ == 0) return;

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, fadeAlpha_);
	SDL_FRect fullscreen = { 0, 0, (float)camera.w, (float)camera.h };
	SDL_RenderFillRect(renderer, &fullscreen);
}

bool Render::IsFadeComplete() const
{
	return !fadeActive_;
}

Uint8 Render::GetFadeAlpha() const
{
	return fadeAlpha_;
}

// ── Ambient Tint System ─────────────────────────────────────────────────────
// Uses SDL_SetTextureColorMod which maps to a GPU fragment shader multiply.
// Each pixel's RGB is multiplied by (tint / 255), giving environment-aware
// color grading that runs entirely on the GPU.

void Render::SetAmbientTint(Uint8 r, Uint8 g, Uint8 b)
{
	ambientTint_ = { r, g, b, 255 };
}

void Render::SetAmbientTint(SDL_Color c)
{
	ambientTint_ = c;
}

SDL_Color Render::GetAmbientTint() const
{
	return ambientTint_;
}

void Render::ApplyAmbientTint(SDL_Texture* tex) const
{
	if (!tex) return;
	SDL_SetTextureColorMod(tex, ambientTint_.r, ambientTint_.g, ambientTint_.b);
}

void Render::ResetAmbientTint(SDL_Texture* tex) const
{
	if (!tex) return;
	SDL_SetTextureColorMod(tex, 255, 255, 255);
}