#include "Engine.h"
#include "Window.h"
#include "Render.h"
#include "Textures.h"
#include "Log.h"
#include <cmath>
#include <vector>
#include <algorithm>

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

Render::~Render() {}

bool Render::Awake()
{
	LOG("Create SDL rendering context");
	bool ret = true;

	int scale = Engine::GetInstance().window->GetScale();
	SDL_Window* window = Engine::GetInstance().window->window;

	// NOTE: SDL_RENDER_SCALE_QUALITY is SDL2-only and has no effect in SDL3.
	// Per-texture filtering is set via SDL_SetTextureScaleMode instead.

	// Try to create a Vulkan renderer first
	renderer = SDL_CreateRenderer(window, "vulkan");

	// Fallback to default if Vulkan fails
	if (renderer == NULL) {
		LOG("Vulkan renderer failed: %s. Falling back to default GPU renderer.", SDL_GetError());
	renderer = SDL_CreateRenderer(window, nullptr);
	}

	if (renderer == NULL)
	{
		LOG("Could not create the renderer! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}
	else
	{
		LOG("Using render driver: %s", SDL_GetRendererName(renderer));

		// Render at 1280x720 logical resolution, auto-scale to fill display with letterbox
		SDL_SetRenderLogicalPresentation(renderer, 1280, 720, SDL_LOGICAL_PRESENTATION_LETTERBOX);

		if (configParameters.child("vsync").attribute("value").as_bool())
			SDL_SetRenderVSync(renderer, 1);
		else
			SDL_SetRenderVSync(renderer, 0);

		camera.w = Engine::GetInstance().window->width * scale;
		camera.h = Engine::GetInstance().window->height * scale;
		camera.x = 0;
		camera.y = 0;

        // Ensure eyelid effect is reset
        eyelidActive_ = false;
        eyelidElapsed_ = 0.0f;
	}

	TTF_Init();
	font = TTF_OpenFont("Assets/Fonts/arial.ttf", 25);
	menuFont = TTF_OpenFont("assets/fonts/Tiraroum.ttf", 60);

	return ret;
}

bool Render::Start()
{
	LOG("render start");
	if (!SDL_GetRenderViewport(renderer, &viewport))
	{
		LOG("SDL_GetRenderViewport failed: %s", SDL_GetError());
	}
	return true;
}

bool Render::PreUpdate()
{
    // Set clear color BEFORE clearing the screen
    SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, background.a);
	SDL_RenderClear(renderer);
	return true;
}

bool Render::Update(float dt)
{
	totalTime_ += dt / 1000.0f;
	UpdateFade(dt);

    if (eyelidActive_) {
        eyelidElapsed_ += dt;
        if (eyelidElapsed_ >= eyelidDuration_) {
            eyelidActive_ = false;
        }
    }

	return true;
}

bool Render::PostUpdate()
{
	DrawFade();
    DrawEyelidEffect();
	SDL_RenderPresent(renderer);
	return true;
}

void Render::OnWindowResize(int newWidth, int newHeight) {
    float scaleX = (float)newWidth / 1280.0f;
    float scaleY = (float)newHeight / 720.0f;
    this->renderScale = std::min(scaleX, scaleY);
}

void Render::DrawEyelidEffect()
{
    if (!eyelidActive_) return;

    int winW, winH;
    Engine::GetInstance().window->GetWindowSize(winW, winH);

    float progress = eyelidElapsed_ / eyelidDuration_;
    int barH = winH / 2;
    int topY = (int)(-progress * barH);
    int bottomY = (int)(winH / 2 + progress * barH);

    SDL_FRect topBar = { 0.0f, (float)topY, (float)winW, (float)barH };
    SDL_FRect bottomBar = { 0.0f, (float)bottomY, (float)winW, (float)barH };

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &topBar);
    SDL_RenderFillRect(renderer, &bottomBar);
}

void Render::StartEyelidEffect(float duration)
{
    eyelidActive_ = true;
    eyelidElapsed_ = 0.0f;
    eyelidDuration_ = duration;
}

void Render::FollowTarget(float dt)
{
    float viewW = GetWorldViewportWidth();
    float viewH = GetWorldViewportHeight();

    // Snap camera on first call instead of lerping from (0,0)
    if (!cameraInitialized_) {
        camera.x = static_cast<int>(-(cameraTargetX_ - viewW / 2.0f));
        camera.y = static_cast<int>(-(cameraTargetY_ - viewH / 2.0f));
        cameraInitialized_ = true;
        return;
    }

    float currentX = -camera.x + viewW / 2.0f;
    float currentY = -camera.y + viewH / 2.0f;

    float targetX = cameraTargetX_;
    float targetY = cameraTargetY_;

    // Camera Sway (Dynamic scenario feel)
    if (cameraSwayActive_) {
        targetX += std::sin(totalTime_ * 1.5f) * 15.0f;
        targetY += std::cos(totalTime_ * 1.0f) * 8.0f;
    }

    float deltaX = targetX - currentX;
    float deltaY = targetY - currentY;

    if (std::abs(deltaX) <= deadZoneWidth_) targetX = currentX;
    else targetX = currentX + (deltaX > 0 ? deltaX - deadZoneWidth_ : deltaX + deadZoneWidth_);

    if (std::abs(deltaY) <= deadZoneHeight_) targetY = currentY;
    else targetY = currentY + (deltaY > 0 ? deltaY - deadZoneHeight_ : deltaY + deadZoneHeight_);

    float lerpFactor = 1.0f - std::exp(-cameraSmoothSpeed_ * dt / 1000.0f);
    float newX = currentX + (targetX - currentX) * lerpFactor;
    float newY = currentY + (targetY - currentY) * lerpFactor;

    camera.x = static_cast<int>(-(newX - viewW / 2.0f));
    camera.y = static_cast<int>(-(newY - viewH / 2.0f));
}

bool Render::CleanUp()
{
	LOG("Destroying SDL render");
	if (font) TTF_CloseFont(font);
	if (menuFont) TTF_CloseFont(menuFont);
	if (gpuDevice) {
		// Release the window before destroying the GPU device
		SDL_Window* window = Engine::GetInstance().window->window;
		if (window) SDL_ReleaseWindowFromGPUDevice(gpuDevice, window);
		SDL_DestroyGPUDevice(gpuDevice);
	}
	if (renderer) SDL_DestroyRenderer(renderer);
	return true;
}

void Render::SetBackgroundColor(SDL_Color color) { background = color; }
void Render::SetViewPort(const SDL_Rect& rect) { SDL_SetRenderViewport(renderer, &rect); }
void Render::ResetViewPort() { SDL_SetRenderViewport(renderer, &viewport); }

bool Render::DrawTexture(SDL_Texture* texture, int x, int y, const SDL_Rect* section, float speed, double angle, int pivotX, int pivotY, SDL_FlipMode flip, float drawScale) const
{
	bool ret = true;
	int scale = Engine::GetInstance().window->GetScale();
	SDL_FRect rect;
	rect.x = (float)((int)(camera.x * speed) + x * scale);
	rect.y = (float)((int)(camera.y * speed) + y * scale);

	if (section != NULL) {
		rect.w = (float)(section->w * scale * drawScale);
		rect.h = (float)(section->h * scale * drawScale);
	} else {
		int tw, th;
		Engine::GetInstance().textures->GetSize(texture, tw, th);
		rect.w = (float)tw * scale;
		rect.h = (float)th * scale;
	}

	SDL_FRect srcRect;
	const SDL_FRect* src = NULL;
	if (section != NULL) {
		srcRect = { (float)section->x, (float)section->y, (float)section->w, (float)section->h };
		src = &srcRect;
	}

	SDL_FPoint pivot;
	SDL_FPoint* p = NULL;
	if (pivotX != INT_MAX && pivotY != INT_MAX) {
		pivot = { (float)pivotX, (float)pivotY };
		p = &pivot;
	}

	if (!SDL_RenderTextureRotated(renderer, texture, src, &rect, angle, p, flip)) {
		LOG("Cannot blit to screen. SDL_RenderTextureRotated error: %s", SDL_GetError());
		ret = false;
	}
	return ret;
}

bool Render::DrawRectangle(const SDL_Rect& rect, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool filled, bool use_camera) const
{
	int scale = Engine::GetInstance().window->GetScale();
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);

	SDL_FRect rec;
	float camX = use_camera ? (float)camera.x : 0.0f;
	float camY = use_camera ? (float)camera.y : 0.0f;
	rec = { camX + rect.x * scale, camY + rect.y * scale, (float)rect.w * scale, (float)rect.h * scale };

	bool result = (filled ? SDL_RenderFillRect(renderer, &rec) : SDL_RenderRect(renderer, &rec));
	if (!result) LOG("DrawRectangle error: %s", SDL_GetError());
	return result;
}

bool Render::DrawLine(int x1, int y1, int x2, int y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool use_camera) const
{
	int scale = Engine::GetInstance().window->GetScale();
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);
	float camX = use_camera ? (float)camera.x : 0.0f;
	float camY = use_camera ? (float)camera.y : 0.0f;
	bool result = SDL_RenderLine(renderer, camX + (float)x1 * scale, camY + (float)y1 * scale, camX + (float)x2 * scale, camY + (float)y2 * scale);
	if (!result) LOG("DrawLine error: %s", SDL_GetError());
	return result;
}

bool Render::DrawCircle(int x, int y, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool use_camera) const
{
	int scale = Engine::GetInstance().window->GetScale();
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);
	float cx = (float)((use_camera ? camera.x : 0) + x * scale);
	float cy = (float)((use_camera ? camera.y : 0) + y * scale);
	
	SDL_FPoint points[361]; // Use stack array for performance
	float factor = (float)M_PI / 180.0f;
	for (int i = 0; i < 360; ++i) {
		points[i].x = cx + (float)(radius * cos(i * factor) * scale);
		points[i].y = cy + (float)(radius * sin(i * factor) * scale);
	}
	points[360] = points[0];
	bool result = SDL_RenderLines(renderer, points, 361);
	if (!result) LOG("DrawCircle error: %s", SDL_GetError());
	return result;
}

bool Render::DrawText(const char* text, int x, int y, int w, int h, SDL_Color color) const
{
	if (!font || !renderer || !text) return false;
	SDL_Surface* surface = TTF_RenderText_Solid(font, text, 0, color);
	if (!surface) return false;
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture) { SDL_DestroySurface(surface); return false; }
	int scale = Engine::GetInstance().window->GetScale();
	SDL_FRect dstrect = { (float)x * scale, (float)y * scale, (w > 0) ? (float)w * scale : (float)surface->w, (h > 0) ? (float)h * scale : (float)surface->h };
	SDL_RenderTexture(renderer, texture, nullptr, &dstrect);
	SDL_DestroyTexture(texture);
	SDL_DestroySurface(surface);
	return true;
}

bool Render::IsOnScreenWorldRect(float x, float y, float w, float h, int margin) const
{
	float camX = -(float)camera.x, camY = -(float)camera.y;
	float sw = (float)camera.w, sh = (float)camera.h;
	return !(x + w + margin < camX || x - margin > camX + sw || y + h + margin < camY || y - margin > camY + sh);
}

void Render::SetCameraTarget(float x, float y)
{
	bool wasInitialized = cameraInitialized_;
	cameraTargetX_ = x;
	cameraTargetY_ = y;
	if (!wasInitialized) {
		// Snap camera to target on first call to avoid lerping from (0,0)
		float vW = GetWorldViewportWidth(), vH = GetWorldViewportHeight();
		camera.x = (int)(-x + vW / 2);
		camera.y = (int)(-y + vH / 2);
	}
	cameraInitialized_ = true;
}
void Render::SetCameraPosition(float x, float y)
{
	float vW = GetWorldViewportWidth();
	float vH = GetWorldViewportHeight();
	camera.x = (int)(-x + vW / 2);
	camera.y = (int)(-y + vH / 2);
	cameraTargetX_ = x;
	cameraTargetY_ = y;
	cameraInitialized_ = true;
}

void Render::ClampCameraToMapBounds(float mapWidth, float mapHeight)
{
	float vW = GetWorldViewportWidth();
	float vH = GetWorldViewportHeight();
	if (camera.x > 0) camera.x = 0;
	if (camera.x < -(mapWidth - vW)) camera.x = (int)-(mapWidth - vW);
	if (camera.y > 0) camera.y = 0;
	if (camera.y < -(mapHeight - vH)) camera.y = (int)-(mapHeight - vH);
}

void Render::SetDeadZone(float w, float h)
{
	deadZoneWidth_ = std::fmax(0.0f, w);
	deadZoneHeight_ = std::fmax(0.0f, h);
}

void Render::SetCameraSmoothSpeed(float s)
{
	cameraSmoothSpeed_ = std::fmax(0.0f, s);
}

Vector2D Render::GetCameraPosition() const
{
	float vW = GetWorldViewportWidth();
	float vH = GetWorldViewportHeight();
	return Vector2D((float)(-camera.x + vW / 2), (float)(-camera.y + vH / 2));
}

float Render::GetWorldViewportWidth() const
{
	return (float)camera.w / Engine::GetInstance().window->GetScale();
}

float Render::GetWorldViewportHeight() const
{
	return (float)camera.h / Engine::GetInstance().window->GetScale();
}

bool Render::DrawTextureAlpha(SDL_Texture* t, int x, int y, int w, int h, Uint8 a) const
{
	if (!t) return false;
	int s = Engine::GetInstance().window->GetScale();
	SDL_FRect d = { (float)x * s, (float)y * s, (float)w * s, (float)h * s };

	SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(t, a);
	bool ok = SDL_RenderTexture(renderer, t, nullptr, &d);
	SDL_SetTextureAlphaMod(t, 255);
	return ok;
}

bool Render::DrawTextureAlphaF(SDL_Texture* t, float x, float y, float w, float h, Uint8 a) const
{
	if (!t) return false;
	int s = Engine::GetInstance().window->GetScale();
	SDL_FRect d = { x * s, y * s, w * s, h * s };

	SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(t, a);
	bool ok = SDL_RenderTexture(renderer, t, nullptr, &d);
	SDL_SetTextureAlphaMod(t, 255);
	return ok;
}

bool Render::DrawMenuText(const char* t, int x, int y, int w, int h, SDL_Color c) const
{
	if (!menuFont || !t) return false;
	SDL_Surface* s = TTF_RenderText_Blended(menuFont, t, 0, c);
	if (!s) return false;

	SDL_Texture* tx = SDL_CreateTextureFromSurface(renderer, s);
	if (!tx) {
		LOG("SDL_CreateTextureFromSurface error: %s", SDL_GetError());
		SDL_DestroySurface(s);
		return false;
	}

	SDL_SetTextureBlendMode(tx, SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(tx, c.a);

	SDL_FRect d = {
		(float)x * Engine::GetInstance().window->GetScale(),
		(float)y * Engine::GetInstance().window->GetScale(),
		(w > 0) ? (float)w * Engine::GetInstance().window->GetScale() : (float)s->w,
		(h > 0) ? (float)h * Engine::GetInstance().window->GetScale() : (float)s->h
	};

	bool ok = SDL_RenderTexture(renderer, tx, nullptr, &d);

	SDL_DestroyTexture(tx);
	SDL_DestroySurface(s);
	return ok;
}

bool Render::DrawMenuTextCentered(const char* t, SDL_Rect a, SDL_Color c) const
{
	return DrawMenuTextCentered(t, a, c, 1.0f);
}

bool Render::DrawMenuTextCentered(const char* t, SDL_Rect a, SDL_Color c, float ts) const
{
	if (!menuFont || !t) return false;
	SDL_Surface* s = TTF_RenderText_Blended(menuFont, t, 0, c);
	if (!s) return false;

	SDL_Texture* tx = SDL_CreateTextureFromSurface(renderer, s);
	if (!tx) {
		LOG("SDL_CreateTextureFromSurface error: %s", SDL_GetError());
		SDL_DestroySurface(s);
		return false;
	}

	SDL_SetTextureBlendMode(tx, SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(tx, c.a);

	float tw = (float)s->w * ts, th = (float)s->h * ts, sc = (float)Engine::GetInstance().window->GetScale();
	SDL_FRect d = { (a.x * sc) + (a.w * sc - tw) / 2.0f, (a.y * sc) + (a.h * sc - th) / 2.0f, tw, th };

	bool ok = SDL_RenderTexture(renderer, tx, nullptr, &d);

	SDL_DestroyTexture(tx);
	SDL_DestroySurface(s);
	return ok;
}

SDL_Texture* Render::CreateMenuTextTexture(const char* t, SDL_Color c) const
{
	if (!menuFont || !t) return nullptr;
	SDL_Surface* s = TTF_RenderText_Blended(menuFont, t, 0, c);
	if (!s) return nullptr;

	SDL_Texture* tx = SDL_CreateTextureFromSurface(renderer, s);
	SDL_DestroySurface(s);

	if (tx) {
		SDL_SetTextureBlendMode(tx, SDL_BLENDMODE_BLEND);
	}
	return tx;
}

SDL_Texture* Render::RecolorTexture(SDL_Texture* src, Uint8 r, Uint8 g, Uint8 b) const
{
	if (!src || !renderer) return nullptr;

	float tw = 0, th = 0;
	SDL_GetTextureSize(src, &tw, &th);
	int w = (int)tw, h = (int)th;

	// Render source texture to a target texture
	SDL_Texture* target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, w, h);
	if (!target) return nullptr;

	SDL_SetRenderTarget(renderer, target);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, src, nullptr, nullptr);
	
	// Read pixels back to a surface
	SDL_Surface* surf = SDL_RenderReadPixels(renderer, nullptr);
	SDL_SetRenderTarget(renderer, nullptr);

	if (!surf) {
		SDL_DestroyTexture(target);
		return nullptr;
	}

	// Recolor all pixels: replace RGB with target color, keep alpha
	SDL_Surface* converted = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
	SDL_DestroySurface(surf);
	if (!converted) {
		SDL_DestroyTexture(target);
		return nullptr;
	}

	Uint32* pixels = (Uint32*)converted->pixels;
	int pixelCount = converted->w * converted->h;
	for (int i = 0; i < pixelCount; i++) {
		Uint8 pr, pg, pb, pa;
		SDL_GetRGBA(pixels[i], SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32), nullptr, &pr, &pg, &pb, &pa);
		if (pa > 0) {
			pixels[i] = SDL_MapRGBA(SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32), nullptr, r, g, b, pa);
		}
	}

	SDL_Texture* result = SDL_CreateTextureFromSurface(renderer, converted);
	SDL_DestroySurface(converted);
	SDL_DestroyTexture(target);

	if (result) {
		SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND);
		SDL_SetTextureScaleMode(result, SDL_SCALEMODE_LINEAR);
        // Also cache size for the new texture
        Engine::GetInstance().textures->textureInfo[result] = { w, h };
	}

	return result;
}

void Render::StartFade(FadeDirection d, float du) { fadeActive_ = true; fadeDir_ = d; fadeDurationMs_ = du; fadeElapsedMs_ = 0.0f; fadeAlpha_ = (d == FadeDirection::FADE_IN) ? 255 : 0; }
void Render::UpdateFade(float dt) { if (!fadeActive_) return; fadeElapsedMs_ += dt; float t = std::min(1.0f, fadeElapsedMs_ / fadeDurationMs_); fadeAlpha_ = (fadeDir_ == FadeDirection::FADE_IN) ? (Uint8)(255.0f * (1.0f - t)) : (Uint8)(255.0f * t); if (fadeElapsedMs_ >= fadeDurationMs_) fadeActive_ = false; }
void Render::DrawFade() { if (fadeAlpha_ == 0) return; SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); SDL_SetRenderDrawColor(renderer, 0, 0, 0, fadeAlpha_); SDL_FRect f = { 0, 0, (float)camera.w, (float)camera.h }; SDL_RenderFillRect(renderer, &f); }
bool Render::IsFadeComplete() const { return !fadeActive_; }
Uint8 Render::GetFadeAlpha() const { return fadeAlpha_; }

void Render::SetAmbientTint(Uint8 r, Uint8 g, Uint8 b) { ambientTint_ = { r, g, b, 255 }; }
void Render::SetAmbientTint(SDL_Color c) { ambientTint_ = c; }
SDL_Color Render::GetAmbientTint() const { return ambientTint_; }
void Render::ApplyAmbientTint(SDL_Texture* t) const { if (t) SDL_SetTextureColorMod(t, ambientTint_.r, ambientTint_.g, ambientTint_.b); }
void Render::ResetAmbientTint(SDL_Texture* t) const { if (t) SDL_SetTextureColorMod(t, 255, 255, 255); }

