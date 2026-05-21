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
	background.a = 255; // Opaque background to ensure additive glow visibility in post-processing
}

Render::~Render() {}

bool Render::Awake()
{
	LOG("Create SDL rendering context");
	bool ret = true;

	int scale = Engine::GetInstance().window->GetScale();
	SDL_Window* window = Engine::GetInstance().window->window;

	// ── VULKAN ARCHITECTURE PLAN: STEP 1 ────────────────────────────────
	// Try to create a renderer using the new SDL3 GPU API backend.
	// This ensures we have a Vulkan-capable device that we can also use for custom shaders.
	renderer = SDL_CreateRenderer(window, "gpu");

	if (renderer == NULL) {
		LOG("SDL_GPU renderer failed: %s. Falling back to explicit Vulkan renderer.", SDL_GetError());
		renderer = SDL_CreateRenderer(window, "vulkan");
	}

	// Fallback to default if Vulkan fails
	if (renderer == NULL) {
		LOG("Vulkan renderer failed: %s. Falling back to default renderer.", SDL_GetError());
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

		// Extract the shared GPUDevice from the renderer properties.
		// This avoids the VK_ERROR_NATIVE_WINDOW_IN_USE_KHR conflict by sharing the same Vulkan surface.
		gpuDevice = (SDL_GPUDevice*)SDL_GetPointerProperty(SDL_GetRendererProperties(renderer), SDL_PROP_RENDERER_GPU_DEVICE_POINTER, NULL);
		
		if (gpuDevice) {
			LOG("Vulkan SDL_GPU device shared successfully via Renderer.");
		} else {
			LOG("Warning: Could not extract SDL_GPU device from renderer. Advanced shaders may fail.");
		}
		// ───────────────────────────────────────────────────────────────────

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

		// Create scene target texture for post processing (1280x720 logical size)
		sceneTargetTex_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 1280, 720);
		if (!sceneTargetTex_) {
			LOG("Error: sceneTargetTex_ could not be created! SDL_Error: %s", SDL_GetError());
		}

		// Create radial glow textures
		SDL_Color goldColor = { 255, 230, 180, 255 };
		glowTex_ = CreateRadialGlowTexture(300, goldColor);
		if (glowTex_) LOG("Gold radial glow texture created.");

		SDL_Color whiteColor = { 255, 255, 255, 255 };
		whiteGlowTex_ = CreateRadialGlowTexture(300, whiteColor);
		if (whiteGlowTex_) LOG("White radial glow texture created.");
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
	if (sceneTargetTex_) {
		SDL_SetRenderTarget(renderer, sceneTargetTex_);
	}
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
	if (sceneTargetTex_) {
		// Reset target to default screen backbuffer
		SDL_SetRenderTarget(renderer, nullptr);

		// Clear screen backbuffer to background color
		SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, background.a);
		SDL_RenderClear(renderer);

		float blurVal = blurIntensity;

		if (blurVal > 0.0f) {
			// Draw base scene
			SDL_SetTextureBlendMode(sceneTargetTex_, SDL_BLENDMODE_BLEND);
			SDL_SetTextureAlphaMod(sceneTargetTex_, 255);
			SDL_RenderTexture(renderer, sceneTargetTex_, nullptr, nullptr);

			// Enhanced Soft Focus (8-tap star pattern) for a deeper cinematic feel
			float offset = 5.0f * blurVal;
			SDL_FRect dOffsets[8] = {
				{ -offset, -offset, 1280.0f, 720.0f }, {  offset, -offset, 1280.0f, 720.0f },
				{ -offset,  offset, 1280.0f, 720.0f }, {  offset,  offset, 1280.0f, 720.0f },
				{ -offset * 1.5f, 0, 1280.0f, 720.0f }, { offset * 1.5f, 0, 1280.0f, 720.0f },
				{ 0, -offset * 1.5f, 1280.0f, 720.0f }, { 0, offset * 1.5f, 1280.0f, 720.0f }
			};

			SDL_SetTextureAlphaMod(sceneTargetTex_, (Uint8)(55)); // Increased opacity for stronger effect
			for (int i = 0; i < 8; ++i) {
				SDL_RenderTexture(renderer, sceneTargetTex_, nullptr, &dOffsets[i]);
			}
		}
		else {
			// Draw native 1:1 scene
			SDL_SetTextureBlendMode(sceneTargetTex_, SDL_BLENDMODE_BLEND);
			SDL_SetTextureAlphaMod(sceneTargetTex_, 255);
			SDL_RenderTexture(renderer, sceneTargetTex_, nullptr, nullptr);
		}
	}

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
	if (!cameraMovementActive_) return; // Skip movement if disabled (e.g. during cinematic intro)

	float viewW = GetWorldViewportWidth();
    float viewH = GetWorldViewportHeight();

    // Snap camera on first call instead of lerping from (0,0)
    if (!cameraInitialized_) {
        camera.x = static_cast<int>(-(cameraTargetX_ - viewW / 2.0f));
        camera.y = static_cast<int>(-(cameraTargetY_ - viewH * 0.75f)); // RAISED: Player at 75% of screen height
        cameraInitialized_ = true;
        return;
    }

    float currentX = -camera.x + viewW / 2.0f;
    float currentY = -camera.y + viewH * 0.75f; // RAISED anchor point

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
    camera.y = static_cast<int>(-(newY - viewH * 0.75f)); // RAISED
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
	if (sceneTargetTex_) {
		SDL_DestroyTexture(sceneTargetTex_);
		sceneTargetTex_ = nullptr;
	}
	if (glowTex_) {
		SDL_DestroyTexture(glowTex_);
		glowTex_ = nullptr;
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
	float s = cameraZoom;
	if (speed == 0.0f) s = 1.0f; // UI elements don't zoom

	SDL_FRect rect;
	// Apply world-space zoom centered at the dynamic zoomCenter
	rect.x = (float)((zoomCenterX + s * ((float)camera.x * speed + (float)x - zoomCenterX)) * scale);
	rect.y = (float)((zoomCenterY + s * ((float)camera.y * speed + (float)y - zoomCenterY)) * scale);

	if (section != NULL) {
		rect.w = (float)(section->w * scale * drawScale * s);
		rect.h = (float)(section->h * scale * drawScale * s);
	} else {
		int tw, th;
		Engine::GetInstance().textures->GetSize(texture, tw, th);
		rect.w = (float)tw * scale * s;
		rect.h = (float)th * scale * s;
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
		pivot = { (float)pivotX * s, (float)pivotY * s }; // Scale pivot for rotated textures
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
	float s = cameraZoom;
	if (!use_camera) s = 1.0f;

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);

	SDL_FRect rec;
	float camX = use_camera ? (float)camera.x : 0.0f;
	float camY = use_camera ? (float)camera.y : 0.0f;
	
	// Apply world-space zoom centered at the dynamic zoomCenter
	rec.x = (float)((zoomCenterX + s * (camX + (float)rect.x - zoomCenterX)) * scale);
	rec.y = (float)((zoomCenterY + s * (camY + (float)rect.y - zoomCenterY)) * scale);
	rec.w = (float)rect.w * scale * s;
	rec.h = (float)rect.h * scale * s;

	bool result = (filled ? SDL_RenderFillRect(renderer, &rec) : SDL_RenderRect(renderer, &rec));
	if (!result) LOG("DrawRectangle error: %s", SDL_GetError());
	return result;
}

bool Render::DrawLine(int x1, int y1, int x2, int y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool use_camera) const
{
	int scale = Engine::GetInstance().window->GetScale();
	float s = cameraZoom;
	if (!use_camera) s = 1.0f;

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);
	float camX = use_camera ? (float)camera.x : 0.0f;
	float camY = use_camera ? (float)camera.y : 0.0f;

	float fx1 = (float)((zoomCenterX + s * (camX + (float)x1 - zoomCenterX)) * scale);
	float fy1 = (float)((zoomCenterY + s * (camY + (float)y1 - zoomCenterY)) * scale);
	float fx2 = (float)((zoomCenterX + s * (camX + (float)x2 - zoomCenterX)) * scale);
	float fy2 = (float)((zoomCenterY + s * (camY + (float)y2 - zoomCenterY)) * scale);

	bool result = SDL_RenderLine(renderer, fx1, fy1, fx2, fy2);
	if (!result) LOG("DrawLine error: %s", SDL_GetError());
	return result;
}

bool Render::DrawCircle(int x, int y, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool use_camera) const
{
	int scale = Engine::GetInstance().window->GetScale();
	float s = cameraZoom;
	if (!use_camera) s = 1.0f;

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);
	float camX = use_camera ? (float)camera.x : 0.0f;
	float camY = use_camera ? (float)camera.y : 0.0f;

	float cx = (float)((zoomCenterX + s * (camX + (float)x - zoomCenterX)) * scale);
	float cy = (float)((zoomCenterY + s * (camY + (float)y - zoomCenterY)) * scale);
	
	SDL_FPoint points[361]; // Use stack array for performance
	float factor = (float)M_PI / 180.0f;
	for (int i = 0; i < 360; ++i) {
		points[i].x = cx + (float)(radius * cos(i * factor) * scale * s);
		points[i].y = cy + (float)(radius * sin(i * factor) * scale * s);
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
	float vW = GetWorldViewportWidth(), vH = GetWorldViewportHeight();
	return !(x + w + margin < camX || x - margin > camX + vW || y + h + margin < camY || y - margin > camY + vH);
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
	// LOGIC FIX: In Silksong, we want the player to stay at logical (640, 540) 
	// which is the center-X and 75%-Y of the 1280x720 screen.
	// This keeps Hornet in the lower third of the viewport.
	camera.x = (int)(-x + 640.0f);
	camera.y = (int)(-y + 540.0f); // RAISED: 720 * 0.75 = 540
	cameraTargetX_ = x;
	cameraTargetY_ = y;
	cameraInitialized_ = true;

	LOG("RENDER: SetCameraPosition to (%.2f, %.2f). Result camera.x/y: (%d, %d). Player Screen Pos: (%.2f, %.2f)", 
		x, y, camera.x, camera.y, (float)camera.x + x, (float)camera.y + y);
}

void Render::ClampCameraToMapBounds(float mapWidth, float mapHeight)
{
	if (!cameraClampingActive_) {
		// LOG("RENDER: ClampCameraToMapBounds SKIPPED (clamping inactive)");
		return;
	}

	float vW = GetWorldViewportWidth();
	float vH = GetWorldViewportHeight();
	int oldX = camera.x;
	int oldY = camera.y;

	if (camera.x > 0) camera.x = 0;
	if (camera.x < -(mapWidth - vW)) camera.x = (int)-(mapWidth - vW);
	if (camera.y > 0) camera.y = 0;
	if (camera.y < -(mapHeight - vH)) camera.y = (int)-(mapHeight - vH);

	if (oldX != camera.x || oldY != camera.y) {
		LOG("RENDER: Camera CLAMPED from (%d, %d) to (%d, %d). Map: %.2f x %.2f. Viewport: %.2f x %.2f", oldX, oldY, camera.x, camera.y, mapWidth, mapHeight, vW, vH);
	}
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
	// Return the logical world point the camera is looking at (center-X, 75%-Y)
	return Vector2D((float)(-camera.x + vW / 2.0f), (float)(-camera.y + vH * 0.75f));
}

float Render::GetWorldViewportWidth() const
{
	return 1280.0f / cameraZoom;
}

float Render::GetWorldViewportHeight() const
{
	return 720.0f / cameraZoom;
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

SDL_Texture* Render::CreateRadialGlowTexture(int radius, SDL_Color centerColor)
{
	int size = radius * 2;
	SDL_Texture* tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, size, size);
	if (!tex) return nullptr;

	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

	void* pixels;
	int pitch;
	if (SDL_LockTexture(tex, nullptr, &pixels, &pitch) == 0) {
		Uint8* bytes = (Uint8*)pixels;
		float maxDist = (float)radius;
		for (int y = 0; y < size; ++y) {
			float dy = (float)(y - radius);
			for (int x = 0; x < size; ++x) {
				float dx = (float)(x - radius);
				float dist = std::sqrt(dx * dx + dy * dy);
				float factor = 1.0f - (dist / maxDist);
				if (factor < 0.0f) factor = 0.0f;
				// Smoother falloff (squared) instead of cubed for more "body" in the glow
				factor = factor * factor;

				Uint8 r = (Uint8)(centerColor.r * factor);
				Uint8 g = (Uint8)(centerColor.g * factor);
				Uint8 b = (Uint8)(centerColor.b * factor);
				Uint8 a = (Uint8)(centerColor.a * factor);

				Uint8* pixel = bytes + y * pitch + x * 4;
				pixel[0] = r;
				pixel[1] = g;
				pixel[2] = b;
				pixel[3] = a;
			}
		}
		SDL_UnlockTexture(tex);
	}
	return tex;
}

void Render::DrawPlayerGlow(int worldX, int worldY, float radiusScale, Uint8 alpha)
{
	if (!glowTex_) return;
	int scale = Engine::GetInstance().window->GetScale();
	float s = cameraZoom;
	float radius = 300.0f * radiusScale;
	float size = radius * 2.0f;

	SDL_FRect dst;
	// Apply world-space zoom centered at the dynamic zoomCenter
	dst.x = (float)((zoomCenterX + s * ((float)camera.x + (float)worldX - radius - zoomCenterX)) * scale);
	dst.y = (float)((zoomCenterY + s * ((float)camera.y + (float)worldY - radius - zoomCenterY)) * scale);
	dst.w = size * scale * s;
	dst.h = size * scale * s;

	SDL_SetTextureBlendMode(glowTex_, SDL_BLENDMODE_ADD);
	SDL_SetTextureAlphaMod(glowTex_, alpha);
	SDL_RenderTexture(renderer, glowTex_, nullptr, &dst);
}

void Render::DrawWhiteGlow(int worldX, int worldY, float radiusScale, Uint8 alpha)
{
	if (!whiteGlowTex_) return;
	int scale = Engine::GetInstance().window->GetScale();
	float s = cameraZoom;
	float radius = 300.0f * radiusScale;
	float size = radius * 2.0f;

	SDL_FRect dst;
	dst.x = (float)((zoomCenterX + s * ((float)camera.x + (float)worldX - radius - zoomCenterX)) * scale);
	dst.y = (float)((zoomCenterY + s * ((float)camera.y + (float)worldY - radius - zoomCenterY)) * scale);
	dst.w = size * scale * s;
	dst.h = size * scale * s;

	SDL_SetTextureBlendMode(whiteGlowTex_, SDL_BLENDMODE_ADD);
	SDL_SetTextureAlphaMod(whiteGlowTex_, alpha);
	SDL_RenderTexture(renderer, whiteGlowTex_, nullptr, &dst);
}
