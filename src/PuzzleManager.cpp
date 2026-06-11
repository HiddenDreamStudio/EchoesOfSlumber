#include "PuzzleManager.h"
#include "Engine.h"
#include "Render.h"
#include "Window.h"
#include "Log.h"
#include <SDL3/SDL.h>
#include <cmath>

static const float VIGNETTE_INNER_RADIUS = 220.0f;

PuzzleManager::PuzzleManager() {}
PuzzleManager::~PuzzleManager() {}

void PuzzleManager::Init(SDL_Renderer* renderer) {
    renderer_ = renderer;
    Reset();
}

void PuzzleManager::LoadFiguresFromObjects(const std::vector<PuzzleFigure>& figures, SDL_FRect exitPortalRect) {
    figures_ = figures;
    exitPortalRect_ = exitPortalRect;
    collectedCount_ = 0;
    LOG("PUZZLE: Figura recogida %d, timerStarted=%d", collectedCount_, (int)timerStarted_);

    timerStarted_ = false;
    timer_ = 45.0f;
    portalOpen_ = false;
    puzzleActive_ = true;
    blinking_ = false;
    introTimer_ = 0.0f;
    currentRadius_ = 2000.0f;
}

void PuzzleManager::Reset() {
    collectedCount_ = 0;
    timerStarted_ = false;
    timer_ = 45.0f;
    portalOpen_ = false;
    puzzleActive_ = true;
    blinking_ = false;
    blinkTimer_ = 0.0f;
    vignetteAlpha_ = 210.0f;
    for (auto& f : figures_) f.collected = false;
    timedOut_ = false;
    introTimer_ = 0.0f;
    currentRadius_ = 2000.0f;
}

void PuzzleManager::Update(float dt, SDL_FRect playerRect, float playerScreenX, float playerScreenY) {
    playerScrX_ = playerScreenX;
    playerScrY_ = playerScreenY;
    playerWorldX_ = playerRect.x;
    playerWorldY_ = playerRect.y;

    if (!puzzleActive_ || portalOpen_) return;

    introTimer_ += dt * 0.001f;
    if (introTimer_ <= 1.0f) {
        currentRadius_ = 2000.0f;
    } else if (introTimer_ <= 3.0f) {
        float t = (introTimer_ - 1.0f) / 2.0f; // 0 to 1 over 2 seconds
        // Use ease-in-out or simple linear. Linear is fine.
        // Easing: t = t * t * (3.0f - 2.0f * t) makes it smoother
        t = t * t * (3.0f - 2.0f * t);
        currentRadius_ = 2000.0f - t * (2000.0f - 220.0f);
    } else {
        currentRadius_ = 220.0f;
    }

    for (auto& fig : figures_) {
        if (fig.collected) continue;

        bool hit = playerRect.x < fig.worldRect.x + fig.worldRect.w &&
            playerRect.x + playerRect.w > fig.worldRect.x &&
            playerRect.y < fig.worldRect.y + fig.worldRect.h &&
            playerRect.y + playerRect.h > fig.worldRect.y;

        if (hit) {
            fig.collected = true;
            collectedCount_++;
            if (!timerStarted_) {
                timerStarted_ = true;
                blinking_ = true;
            }
        }
    }

    if (!timerStarted_) {
        vignetteAlpha_ = 255.0f;  
    }
    if (timerStarted_) {
        timer_ -= dt * 0.001f; 

       
        float urgency = 1.0f - (timer_ / 45.0f);  
        float blinkSpeed = 0.002f + urgency * 0.012f;     
        blinkTimer_ += dt * blinkSpeed;
        float blink = sinf(blinkTimer_);
        vignetteAlpha_ = 210.0f + blink * 45.0f;        

        if (collectedCount_ >= 5) {
            portalOpen_ = true;
            puzzleActive_ = false;
            blinking_ = false;
            vignetteAlpha_ = 0.0f;  
            return;
        }

        if (timer_ <= 0.0f) {
            timedOut_ = true;
            puzzleActive_ = false;
        }
    }
}

void PuzzleManager::Render(SDL_Renderer* renderer, float cameraX, float cameraY) {
    if (!puzzleActive_ && !portalOpen_) return;

    DrawVignette(renderer, playerScrX_, playerScrY_);
    DrawFigures(renderer, cameraX, cameraY);

    DrawBlockedPortal(renderer, cameraX, cameraY);


    if (timerStarted_ && !portalOpen_) {
        DrawTimer(renderer);
    }
}

void PuzzleManager::DrawVignette(SDL_Renderer* renderer, float cx, float cy) {
    int screenW = 0, screenH = 0;
    SDL_GetCurrentRenderOutputSize(renderer, &screenW, &screenH);

    float radius = currentRadius_;  

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int y = 0; y < screenH; y++) {
        float dy = (float)y - cy;
        float dxSq = radius * radius - dy * dy;

        int clearLeft = (dxSq > 0) ? (int)(cx - sqrtf(dxSq)) : (int)cx;
        int clearRight = (dxSq > 0) ? (int)(cx + sqrtf(dxSq)) : (int)cx;

        clearLeft = std::max(0, std::min(clearLeft, screenW));
        clearRight = std::max(0, std::min(clearRight, screenW));

        Uint8 alpha = (Uint8)vignetteAlpha_;

        if (clearLeft > 0) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
            SDL_FRect left = { 0.0f, (float)y, (float)clearLeft, 1.0f };
            SDL_RenderFillRect(renderer, &left);
        }

        if (clearRight < screenW) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
            SDL_FRect right = { (float)clearRight, (float)y, (float)(screenW - clearRight), 1.0f };
            SDL_RenderFillRect(renderer, &right);
        }
    }
}

void PuzzleManager::DrawFigures(SDL_Renderer* renderer, float cameraX, float cameraY) {
    for (const auto& fig : figures_) {
        if (fig.collected) continue;

        float sx = fig.worldRect.x + cameraX;
        float sy = fig.worldRect.y + cameraY;
        float w = fig.worldRect.w;
        float h = fig.worldRect.h;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        if (fig.name == "puzzle_square") {
            SDL_SetRenderDrawColor(renderer, 100, 200, 255, 220);
            SDL_FRect r = { sx, sy, w, h };
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderRect(renderer, &r);
        }
        else if (fig.name == "puzzle_circle") {
            SDL_SetRenderDrawColor(renderer, 200, 100, 255, 220);
            float cx2 = sx + w / 2, cy2 = sy + h / 2, rad = w / 2;
            int segs = 32;
            for (int i = 0; i < segs; i++) {
                float a1 = (float)i / segs * 6.2831f;
                float a2 = (float)(i + 1) / segs * 6.2831f;
                SDL_RenderLine(renderer,
                    cx2 + cosf(a1) * rad, cy2 + sinf(a1) * rad,
                    cx2 + cosf(a2) * rad, cy2 + sinf(a2) * rad);
            }
        }
        else if (fig.name == "puzzle_triangle") {
            SDL_SetRenderDrawColor(renderer, 100, 255, 150, 220);
            SDL_RenderLine(renderer, sx, sy + h, sx + w, sy + h);
            SDL_RenderLine(renderer, sx, sy + h, sx + w / 2, sy);
            SDL_RenderLine(renderer, sx + w, sy + h, sx + w / 2, sy);
        }
        else if (fig.name == "puzzle_diamond") {
            SDL_SetRenderDrawColor(renderer, 255, 220, 50, 220);
            float mx = sx + w / 2, my = sy + h / 2;
            SDL_RenderLine(renderer, mx, sy, sx + w, my);
            SDL_RenderLine(renderer, sx + w, my, mx, sy + h);
            SDL_RenderLine(renderer, mx, sy + h, sx, my);
            SDL_RenderLine(renderer, sx, my, mx, sy);
        }
        else if (fig.name == "puzzle_star") {
            SDL_SetRenderDrawColor(renderer, 255, 100, 100, 220);
            float mx = sx + w / 2, my = sy + h / 2;
            float outer = w / 2, inner = w / 4;
            int points = 5;
            for (int i = 0; i < points; i++) {
                float a1 = -1.5708f + (float)i * 1.2566f;
                float a2 = a1 + 0.6283f;
                float nextA = a1 + 1.2566f;
                SDL_RenderLine(renderer,
                    mx + cosf(a1) * outer, my + sinf(a1) * outer,
                    mx + cosf(a2) * inner, my + sinf(a2) * inner);
                SDL_RenderLine(renderer,
                    mx + cosf(a2) * inner, my + sinf(a2) * inner,
                    mx + cosf(nextA) * outer, my + sinf(nextA) * outer);
            }
        }
    }
}

void PuzzleManager::DrawBlockedPortal(SDL_Renderer* renderer, float cameraX, float cameraY) {
    float sx = exitPortalRect_.x + cameraX;
    float sy = exitPortalRect_.y + cameraY;
    float w = exitPortalRect_.w;
    float h = exitPortalRect_.h;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    bool open = portalOpen_;

    SDL_FRect r = { sx, sy, w, h };
    Uint8 cr = open ? 0 : 180;
    Uint8 cg = open ? 200 : 0;
    Uint8 cb = open ? 80 : 0;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < 18; i++) {
        float t = (float)i / 18.0f;
        Uint8 alpha = (Uint8)((1.0f - t) * (1.0f - t) * 60.0f);
        SDL_SetRenderDrawColor(renderer, cr, cg, cb, alpha);
        float sw = w / 2.0f * t, sh = h / 2.0f * t;
        SDL_FRect l = { sx,         sy,         sw, h };
        SDL_FRect r2 = { sx + w - sw, sy,         sw, h };
        SDL_FRect tp = { sx,         sy,          w, sh };
        SDL_FRect bo = { sx,         sy + h - sh, w, sh };
        SDL_RenderFillRect(renderer, &l);
        SDL_RenderFillRect(renderer, &r2);
        SDL_RenderFillRect(renderer, &tp);
        SDL_RenderFillRect(renderer, &bo);
    }

    float dx = (playerWorldX_ + 24.0f) - (exitPortalRect_.x + w / 2.0f);
    float dy = (playerWorldY_ + 40.0f) - (exitPortalRect_.y + h / 2.0f);
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < 250.0f) {
        auto& render = *Engine::GetInstance().render;
        int scale = Engine::GetInstance().window->GetScale();

        const char* label = portalOpen_ ? "DESBLOQUEADO" : "BLOQUEADO";
        SDL_Color color = portalOpen_
            ? SDL_Color{ 0, 255, 100, 255 }
        : SDL_Color{ 255, 80, 80, 255 };

        const int panelW = portalOpen_ ? 260 : 200;
        const int panelH = 40;

        float portalCenterScrX = exitPortalRect_.x + cameraX + w / 2.0f;
        float portalTopScrY = exitPortalRect_.y + cameraY;

        int panelX = (int)(portalCenterScrX - panelW / 2.0f);
        int panelY = (int)(portalTopScrY - 52.0f);

        SDL_Rect panel = { panelX, panelY, panelW, panelH };
        render.DrawRectangle(panel, 0, 0, 0, 200, true, false);

        SDL_Rect border = { panelX - 2, panelY - 2, panelW + 4, panelH + 4 };
        render.DrawRectangle(border, color.r, color.g, color.b, 220, false, false);

        render.DrawMenuTextCentered(label, panel, color, 0.32f);
    }
}

void PuzzleManager::DrawTimer(SDL_Renderer* renderer) {
    int screenW = 0, screenH = 0;
    SDL_GetRenderOutputSize(renderer, &screenW, &screenH);

    float pct = timer_ / 45.0f;
    if (pct < 0.0f) pct = 0.0f;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect bg = { 20, 20, (float)screenW - 40, 18 };
    SDL_RenderFillRect(renderer, &bg);

    float red = 255.0f * (1.0f - pct);
    float green = 255.0f * pct;
    SDL_SetRenderDrawColor(renderer,
        (Uint8)red,
        (Uint8)green,
        0,
        220);
    SDL_FRect bar = { 22, 22, ((float)screenW - 44) * pct, 14 };
    SDL_RenderFillRect(renderer, &bar);
}