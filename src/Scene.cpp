#include "Engine.h"
#include "Input.h"
#include "Textures.h"
#include "Audio.h"
#include "Render.h"
#include "Window.h"
#include "Scene.h"
#include "Cinematics.h"
#include "Log.h"
#include "Entity.h"
#include "EntityManager.h"
#include "Player.h"
#include "Map.h"
#include "Item.h"
#include "Enemy.h"
#include "UIManager.h"
#include "SaveSystem.h"
#include <cstdlib>
#include <cmath>

// ────────────────────────────────────────────────────────────────────────────
// Button IDs  (main menu)
// ────────────────────────────────────────────────────────────────────────────
static constexpr int BTN_PLAY = 1;
static constexpr int BTN_SETTINGS = 2;
static constexpr int BTN_EXIT = 3;
// Settings panel
static constexpr int BTN_SETTINGS_BACK = 10;
static constexpr int BTN_MUSIC_UP = 11;
static constexpr int BTN_MUSIC_DOWN = 12;
static constexpr int BTN_SFX_UP = 13;
static constexpr int BTN_SFX_DOWN = 14;

Scene::Scene() : Module()
{
	name = "scene";
}

Scene::~Scene() {}

bool Scene::Awake()
{
	LOG("Loading Scene");
	// Defer initial scene load – renderer is not ready during Awake()
	hasPendingSceneChange = true;
	pendingScene = currentScene;
	return true;
}

bool Scene::Start() { return true; }
bool Scene::PreUpdate() { return true; }

bool Scene::Update(float dt)
{
	// Handle fade-driven scene transitions
	if (waitingForFade_ && Engine::GetInstance().render->IsFadeComplete()) {
		waitingForFade_ = false;
		ChangeScene(fadeTargetScene_);
		Engine::GetInstance().render->StartFade(FadeDirection::FADE_IN, 800.0f);
	}

	if (hasPendingSceneChange) {
		hasPendingSceneChange = false;
		ChangeScene(pendingScene);
	}

	switch (currentScene)
	{
	case SceneID::INTRO:
		UpdateIntro(dt);
		break;
	case SceneID::MAIN_MENU:
		PostUpdateMainMenu();
		UpdateMainMenu(dt);
		break;
	case SceneID::INTRO_CINEMATIC:
		UpdateIntroCinematic(dt);
		break;
	case SceneID::GAMEPLAY:
		UpdateGameplay(dt);
		break;
	}
	return true;
}

bool Scene::PostUpdate()
{
	bool ret = true;

	switch (currentScene)
	{
	case SceneID::GAMEPLAY:
		PostUpdateGameplay();
		break;
	default:
		break;
	}

	return ret;
}

bool Scene::OnUIMouseClickEvent(UIElement* uiElement)
{
	switch (currentScene)
	{
	case SceneID::MAIN_MENU:
		HandleMainMenuUIEvents(uiElement);
		break;
	case SceneID::GAMEPLAY:
		HandlePauseMenuUIEvents(uiElement);
		break;
	default:
		break;
	}
	return true;
}

bool Scene::CleanUp()
{
	LOG("Freeing scene");
	UnloadCurrentScene();
	return true;
}

Vector2D Scene::GetPlayerPosition()
{
	if (player) return player->GetPosition();
	return Vector2D(0, 0);
}

void Scene::SetPlayerPosition(Vector2D pos)
{
	if (player) player->SetPosition(pos);
}

// ============================================================================
//  Scene routing
// ============================================================================

void Scene::LoadScene(SceneID s)
{
	switch (s)
	{
	case SceneID::INTRO:           LoadIntro();           break;
	case SceneID::MAIN_MENU:       LoadMainMenu();       break;
	case SceneID::INTRO_CINEMATIC: LoadIntroCinematic(); break;
	case SceneID::GAMEPLAY:        LoadGameplay();        break;
	}
}

void Scene::ChangeScene(SceneID s)
{
	UnloadCurrentScene();
	currentScene = s;
	LoadScene(currentScene);
}

void Scene::UnloadCurrentScene()
{
	switch (currentScene)
	{
	case SceneID::INTRO:           UnloadIntro();           break;
	case SceneID::MAIN_MENU:       UnloadMainMenu();       break;
	case SceneID::INTRO_CINEMATIC: UnloadIntroCinematic(); break;
	case SceneID::GAMEPLAY:        UnloadGameplay();        break;
	}
}

// ============================================================================
//  MAIN MENU
// ============================================================================

void Scene::LoadMainMenu()
{
	showSettings_ = false;
	settingsCooldown_ = 0;
	musicVolume_ = 0.8f;
	sfxVolume_ = 0.8f;

	// Load menu textures
	SDL_Texture* rawLogo = Engine::GetInstance().textures->Load("assets/textures/menu/EchoesOfSlumber.png");
	// Recolor logo from blue to #D4DAEA (212, 218, 234)
	texMenuLogo_ = Engine::GetInstance().render->RecolorTexture(rawLogo, 212, 218, 234);
	Engine::GetInstance().textures->UnLoad(rawLogo);
	texMenuChild_ = Engine::GetInstance().textures->Load("assets/textures/menu/IL_NenFront_01.png");
	texMenuButton_ = Engine::GetInstance().textures->Load("assets/textures/menu/UI_Pause_Menu_button_white.png");

	// Load fragment textures
	const char* fragPaths[NUM_FRAGMENTS] = {
		"assets/textures/Menu/UI_Fragment1.png",
		"assets/textures/Menu/UI_Fragment2.png",
		"assets/textures/Menu/UI_Fragment3.png",
		"assets/textures/Menu/UI_Fragment4.png",
		"assets/textures/Menu/UI_Fragment5.png"
	};
	for (int i = 0; i < NUM_FRAGMENTS; i++)
		fragments_[i].tex = Engine::GetInstance().textures->Load(fragPaths[i]);
	fragmentsInited_ = false;
	fragmentTime_ = 0.0f;

	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	// Left half layout: logo + buttons centered horizontally in left half
	const int leftHalf = winW / 2;

	// Logo dimensions (used to position buttons below)
	const int logoW = 385;
	const int logoH = static_cast<int>(static_cast<float>(logoW) * (569.0f / 1559.0f)); // ~117px
	const int logoX = (leftHalf - logoW) / 2;
	const int logoY = winH / 4 - logoH / 2;  // centered at 1/4 vertical

	// Buttons centered in left half, below the logo
	// Button texture is 456x130 — keep proportional
	const int btnW = 315;
	const int btnH = static_cast<int>(static_cast<float>(btnW) * (130.0f / 456.0f)); // ~90px
	const int btnX = (leftHalf - btnW) / 2;
	const int startY = logoY + logoH + 50;
	const int gap = btnH + 15;

	menuAnimState_ = MenuAnimState::LOGO_FADE_IN;
	menuAnimTimer_ = 0.0f;

	SDL_Rect playPos     = { btnX, startY,           btnW, btnH };
	SDL_Rect settingsPos = { btnX, startY + gap,     btnW, btnH };
	SDL_Rect exitPos     = { btnX, startY + gap * 2, btnW, btnH };

	btnPlay_ = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PLAY, "play", playPos, this);
	btnSettings_ = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_SETTINGS, "options", settingsPos, this);
	btnExit_ = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_EXIT, "exit", exitPos, this);

    // Initial cinematic state: Hide buttons initially
	if (btnPlay_) { btnPlay_->alphaMod = 0.0f; btnPlay_->isVisible = false; }
	if (btnSettings_) { btnSettings_->alphaMod = 0.0f; btnSettings_->isVisible = false; }
	if (btnExit_) { btnExit_->alphaMod = 0.0f; btnExit_->isVisible = false; }

	// Set stone button texture
	if (texMenuButton_) {
		btnPlay_->SetTexture(texMenuButton_);
		btnSettings_->SetTexture(texMenuButton_);
		btnExit_->SetTexture(texMenuButton_);
	}

	// Settings panel — placed in the TOP half so BACK never overlaps main buttons
	const int panelW = 340;
	const int panelX = winW / 2 - panelW / 2;
	const int panelY = 60;
	const int smallBtnW = 40;
	const int smallBtnH = 34;
	const int rowH = 52;

	SDL_Rect musicUpPos   = { panelX + panelW - smallBtnW - 12, panelY + 60,              smallBtnW, smallBtnH };
	SDL_Rect musicDownPos = { panelX + 12,                       panelY + 60,              smallBtnW, smallBtnH };
	SDL_Rect sfxUpPos     = { panelX + panelW - smallBtnW - 12, panelY + 60 + rowH,       smallBtnW, smallBtnH };
	SDL_Rect sfxDownPos   = { panelX + 12,                       panelY + 60 + rowH,       smallBtnW, smallBtnH };
	SDL_Rect backPos      = { panelX + panelW / 2 - 60,          panelY + 60 + rowH * 2 + 10, 120, btnH };

	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_MUSIC_UP, "+", musicUpPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_MUSIC_DOWN, "-", musicDownPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_SFX_UP, "+", sfxUpPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_SFX_DOWN, "-", sfxDownPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_SETTINGS_BACK, "BACK", backPos, this);

	SetSettingsPanelVisible(false);
}

void Scene::UnloadMainMenu()
{
	Engine::GetInstance().uiManager->CleanUp();
	if (texMenuLogo_) { SDL_DestroyTexture(texMenuLogo_); texMenuLogo_ = nullptr; }
	if (texMenuChild_) { Engine::GetInstance().textures->UnLoad(texMenuChild_); texMenuChild_ = nullptr; }
	if (texMenuButton_) { Engine::GetInstance().textures->UnLoad(texMenuButton_); texMenuButton_ = nullptr; }
	for (int i = 0; i < NUM_FRAGMENTS; i++) {
		if (fragments_[i].tex) { Engine::GetInstance().textures->UnLoad(fragments_[i].tex); fragments_[i].tex = nullptr; }
	}
	fragmentsInited_ = false;
}

void Scene::UpdateMainMenu(float dt)
{
	// Decrement cooldown each frame
	if (settingsCooldown_ > 0) settingsCooldown_--;

	// Accumulate time for fragment floating animations
	fragmentTime_ += dt;

	// Cinematic logic
	if (menuAnimState_ != MenuAnimState::IDLE) {
		menuAnimTimer_ += dt;
		
		auto finishCinematic = [&]() {
			menuAnimState_ = MenuAnimState::IDLE;
			if (btnPlay_) { btnPlay_->alphaMod = 1.0f; btnPlay_->isVisible = true; }
			if (btnSettings_) { btnSettings_->alphaMod = 1.0f; btnSettings_->isVisible = true; }
			if (btnExit_) { btnExit_->alphaMod = 1.0f; btnExit_->isVisible = true; }
		};

		// Allow skipping entire cinematic
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN || 
			Engine::GetInstance().input->GetKey(SDL_SCANCODE_RETURN) == KEY_DOWN || 
			Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN) 
		{
			finishCinematic();
			return;
		}

		if (menuAnimState_ == MenuAnimState::LOGO_FADE_IN && menuAnimTimer_ > 1500.0f) {
			menuAnimState_ = MenuAnimState::LOGO_HOLD;
			menuAnimTimer_ = 0.0f;
		}
		else if (menuAnimState_ == MenuAnimState::LOGO_HOLD && menuAnimTimer_ > 1000.0f) {
			menuAnimState_ = MenuAnimState::SLIDE_LOGO;
			menuAnimTimer_ = 0.0f;
		}
		else if (menuAnimState_ == MenuAnimState::SLIDE_LOGO && menuAnimTimer_ > 1500.0f) {
			menuAnimState_ = MenuAnimState::SLIDE_CHILD;
			menuAnimTimer_ = 0.0f;
		}
		else if (menuAnimState_ == MenuAnimState::SLIDE_CHILD && menuAnimTimer_ > 1500.0f) {
			menuAnimState_ = MenuAnimState::FADE_FRAGS_BTNS;
			menuAnimTimer_ = 0.0f;
		}
		else if (menuAnimState_ == MenuAnimState::FADE_FRAGS_BTNS && menuAnimTimer_ > 3500.0f) {
			finishCinematic();
		}
	} 
	else 
	{
		// ESC closes settings only if IDLE
		if (showSettings_ && Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN)
		{
			showSettings_ = false;
			SetSettingsPanelVisible(false);
		}
	}
}

void Scene::PostUpdateMainMenu()
{
	auto& render = *Engine::GetInstance().render;
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	// Compute target character rect
	int childH = winH - 20;
	int childW = static_cast<int>(static_cast<float>(childH) * (1421.0f / 913.0f));
	int childDestX = winW - childW;

	// Compute target Logo dimensions
	const int leftHalf = winW / 2;
	int logoW = 385;
	int logoH = static_cast<int>(static_cast<float>(logoW) * (569.0f / 1559.0f));
	int logoDestX = (leftHalf - logoW) / 2;
	int logoDestY = winH / 4 - logoH / 2;

	// Lazy-init fragments on first frame
	if (!fragmentsInited_) {
		InitFragments(winW, winH, childDestX, childW);
		fragmentsInited_ = true;
	}

	// Dynamic Cinematic State Variables
	int renderChildX = childDestX;
	int renderLogoX = logoDestX;
	int renderLogoY = logoDestY;
	int bgR = 21, bgG = 31, bgB = 32;

	if (menuAnimState_ != MenuAnimState::IDLE) {
		float t = 0.0f;
		
		switch (menuAnimState_) {
		case MenuAnimState::LOGO_FADE_IN:
			t = menuAnimTimer_ / 1500.0f;
			if (t > 1.0f) t = 1.0f;
			bgR = static_cast<int>(21.0f * t);
			bgG = static_cast<int>(31.0f * t);
			bgB = static_cast<int>(32.0f * t);
			renderLogoX = winW / 2 - logoW / 2;
			renderLogoY = winH / 2 - logoH / 2;
			renderChildX = winW; // Hidden fully on the right
			break;

		case MenuAnimState::LOGO_HOLD:
			renderLogoX = winW / 2 - logoW / 2;
			renderLogoY = winH / 2 - logoH / 2;
			renderChildX = winW;
			break;

		case MenuAnimState::SLIDE_LOGO:
			t = menuAnimTimer_ / 1500.0f;
			if (t > 1.0f) t = 1.0f;
			t = 1.0f - (float)pow(1.0f - t, 3.0f);
			renderLogoX = static_cast<int>((static_cast<float>(winW) / 2.0f - static_cast<float>(logoW) / 2.0f) + (static_cast<float>(logoDestX) - (static_cast<float>(winW) / 2.0f - static_cast<float>(logoW) / 2.0f)) * t);
			renderLogoY = static_cast<int>((static_cast<float>(winH) / 2.0f - static_cast<float>(logoH) / 2.0f) + (static_cast<float>(logoDestY) - (static_cast<float>(winH) / 2.0f - static_cast<float>(logoH) / 2.0f)) * t);
			renderChildX = winW;
			break;

		case MenuAnimState::SLIDE_CHILD:
			t = menuAnimTimer_ / 1500.0f;
			if (t > 1.0f) t = 1.0f;
			t = 1.0f - (float)pow(1.0f - t, 3.0f);
			renderChildX = (int)(winW + (childDestX - winW) * t);
			renderLogoX = logoDestX;
			renderLogoY = logoDestY;
			break;

		case MenuAnimState::FADE_FRAGS_BTNS:
			renderChildX = childDestX;
			renderLogoX = logoDestX;
			renderLogoY = logoDestY;

			if (btnPlay_) {
				float f0 = (menuAnimTimer_ - 0.0f) / 1000.0f;
				if (f0 < 0.0f) f0 = 0.0f; if (f0 > 1.0f) f0 = 1.0f;
				btnPlay_->alphaMod = f0;
				btnPlay_->isVisible = (f0 > 0.0f);
			}
			if (btnSettings_) {
				float f1 = (menuAnimTimer_ - 800.0f) / 1000.0f;
				if (f1 < 0.0f) f1 = 0.0f; if (f1 > 1.0f) f1 = 1.0f;
				btnSettings_->alphaMod = f1;
				btnSettings_->isVisible = (f1 > 0.0f);
			}
			if (btnExit_) {
				float f2 = (menuAnimTimer_ - 1600.0f) / 1000.0f;
				if (f2 < 0.0f) f2 = 0.0f; if (f2 > 1.0f) f2 = 1.0f;
				btnExit_->alphaMod = f2;
				btnExit_->isVisible = (f2 > 0.0f);
			}
			break;
		}

		// Enforce fragment fading based on time globally
		if (menuAnimState_ < MenuAnimState::FADE_FRAGS_BTNS) {
			for (int i = 0; i < NUM_FRAGMENTS; i++) fragments_[i].alpha = 0;
		} else if (menuAnimState_ == MenuAnimState::FADE_FRAGS_BTNS) {
			for (int i = 0; i < NUM_FRAGMENTS; i++) {
				float f = (menuAnimTimer_ - (i * 400.0f)) / 1000.0f;
				if (f < 0.0f) f = 0.0f; if (f > 1.0f) f = 1.0f;
				fragments_[i].alpha = (Uint8)(255 * f);
			}
		}
	} else {
		// IDLE state
		for (int i = 0; i < NUM_FRAGMENTS; i++) {
			fragments_[i].alpha = 255;
		}
	}

	// ── Background #151F20 ───────────────────────────────────────────────────────
	SDL_Rect bg = { 0, 0, winW, winH };
	render.DrawRectangle(bg, bgR, bgG, bgB, 255, true, false);

	// ── Fragments BEHIND the character ──────────────────────────────────────
	DrawFragments(false, winW, winH);

	// ── Character (right half, fill from near top to bottom) ─────────────────
	if (texMenuChild_) {
		int childY = 20;
		render.DrawTextureAlpha(texMenuChild_, renderChildX, childY, childW, childH, 255);
	}

	// ── Fragments IN FRONT of the character ─────────────────────────────────
	DrawFragments(true, winW, winH);

	// ── Game logo (centered in left half at 1/4 vertical, #D4DAEA) ─────────
	if (texMenuLogo_) {
		render.DrawTextureAlpha(texMenuLogo_, renderLogoX, renderLogoY, logoW, logoH, 255);
	}

	// ── Settings Panel (drawn before UI buttons so buttons render on top) ───
	if (showSettings_)
	{
		DrawSettingsPanel(winW, winH);
	}
}

void Scene::DrawSettingsPanel(int winW, int winH)
{
	auto& render = *Engine::GetInstance().render;

	const int panelW = 340;
	const int panelH = 240;
	const int panelX = winW / 2 - panelW / 2;
	const int panelY = 60;          // matches LoadMainMenu button positions
	const int rowH = 52;

	// Dim overlay
	SDL_Rect overlay = { 0, 0, winW, winH };
	render.DrawRectangle(overlay, 0, 0, 0, 160, true, false);

	// Panel background
	SDL_Rect panel = { panelX, panelY, panelW, panelH };
	render.DrawRectangle(panel, 10, 14, 28, 245, true, false);
	// Panel border
	render.DrawRectangle(panel, 80, 120, 180, 200, false, false);
	// Top accent
	SDL_Rect topBar = { panelX, panelY, panelW, 4 };
	render.DrawRectangle(topBar, 80, 140, 200, 255, true, false);

	// Title
	render.DrawText("SETTINGS", panelX + panelW / 2 - 44, panelY + 14, 0, 0,
		{ 180, 210, 240, 255 });

	// ── Music volume row ─────────────────────────────────────────────────────
	render.DrawText("MUSIC", panelX + 60, panelY + 56, 0, 0, { 150, 180, 210, 220 });

	// Volume bar background
	SDL_Rect barBg = { panelX + 60, panelY + 86, panelW - 120, 8 };
	render.DrawRectangle(barBg, 30, 40, 60, 200, true, false);
	// Volume bar fill
	int musicFill = static_cast<int>(static_cast<float>(panelW - 120) * musicVolume_);
	SDL_Rect barFill = { panelX + 60, panelY + 86, musicFill, 8 };
	render.DrawRectangle(barFill, 80, 160, 220, 255, true, false);

	// Value text
	char volText[8];
	snprintf(volText, sizeof(volText), "%d%%", static_cast<int>(musicVolume_ * 100.0f));
	render.DrawText(volText, panelX + panelW / 2 - 15, panelY + 56, 0, 0,
		{ 200, 220, 240, 255 });

	// ── SFX volume row ───────────────────────────────────────────────────────
	render.DrawText("SFX", panelX + 60, panelY + 56 + rowH, 0, 0, { 150, 180, 210, 220 });

	SDL_Rect sfxBarBg = { panelX + 60, panelY + 86 + rowH, panelW - 120, 8 };
	render.DrawRectangle(sfxBarBg, 30, 40, 60, 200, true, false);
	int sfxFill = static_cast<int>(static_cast<float>(panelW - 120) * sfxVolume_);
	SDL_Rect sfxBarFill = { panelX + 60, panelY + 86 + rowH, sfxFill, 8 };
	render.DrawRectangle(sfxBarFill, 80, 160, 220, 255, true, false);

	snprintf(volText, sizeof(volText), "%d%%", static_cast<int>(sfxVolume_ * 100.0f));
	render.DrawText(volText, panelX + panelW / 2 - 15, panelY + 56 + rowH, 0, 0,
		{ 200, 220, 240, 255 });
}

void Scene::HandleMainMenuUIEvents(UIElement* uiElement)
{
	// Ignore events during fade or cooldown
	if (waitingForFade_) return;
	if (settingsCooldown_ > 0) return;

	switch (uiElement->id)
	{
		// ── Main buttons ─────────────────────────────────────────────────────────
	case BTN_PLAY:
		LOG("Main Menu: Play");
		waitingForFade_ = true;
		fadeTargetScene_ = SceneID::INTRO_CINEMATIC;
		Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 1000.0f);
		break;

	case BTN_SETTINGS:
		showSettings_ = !showSettings_;
		SetSettingsPanelVisible(showSettings_);
		settingsCooldown_ = 4;   // ignore events for 4 frames after opening
		break;

	case BTN_EXIT:
		LOG("Main Menu: Exit");
		// Signal engine to quit via window event
		SDL_Event quitEvent;
		quitEvent.type = SDL_EVENT_QUIT;
		SDL_PushEvent(&quitEvent);
		break;

		// ── Settings buttons ─────────────────────────────────────────────────────
	case BTN_MUSIC_UP:
		musicVolume_ = std::min(1.0f, musicVolume_ + 0.1f);
		Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
		break;

	case BTN_MUSIC_DOWN:
		musicVolume_ = std::max(0.0f, musicVolume_ - 0.1f);
		Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
		break;

	case BTN_SFX_UP:
		sfxVolume_ = std::min(1.0f, sfxVolume_ + 0.1f);
		Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);
		break;

	case BTN_SFX_DOWN:
		sfxVolume_ = std::max(0.0f, sfxVolume_ - 0.1f);
		Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);
		break;

	case BTN_SETTINGS_BACK:
		showSettings_ = false;
		SetSettingsPanelVisible(false);
		break;

	default:
		break;
	}
}

void Scene::SetSettingsPanelVisible(bool visible)
{
	// Disable/enable settings-panel buttons and main buttons
	auto& list = Engine::GetInstance().uiManager->UIElementsList;

	for (auto& el : list)
	{
		bool isSettingsBtn = (el->id >= BTN_SETTINGS_BACK && el->id <= BTN_SFX_DOWN)
			|| el->id == BTN_SETTINGS_BACK;
		bool isMainBtn = (el->id == BTN_PLAY || el->id == BTN_SETTINGS || el->id == BTN_EXIT);

		if (isSettingsBtn) {
			el->state = visible ? UIElementState::NORMAL : UIElementState::DISABLED;
			el->isVisible = visible;
		}

		if (isMainBtn) {
			el->state = visible ? UIElementState::DISABLED : UIElementState::NORMAL;
			el->isVisible = !visible;
		}
	}
}

// ============================================================================
//  INTRO (Splash Logos)
// ============================================================================

static constexpr float INTRO_FADE_MS  = 800.0f;
static constexpr float INTRO_HOLD_MS  = 1500.0f;
static constexpr float INTRO_TOTAL_MS = INTRO_FADE_MS * 2.0f + INTRO_HOLD_MS;

void Scene::LoadIntro()
{
	LOG("Loading Intro splash screen");
	introPhase_ = IntroPhase::CITM_FADEIN;
	introTimer_ = 0.0f;

	// Fade in from black at game boot
	Engine::GetInstance().render->StartFade(FadeDirection::FADE_IN, 800.0f);

	texCitmLogo_ = Engine::GetInstance().textures->Load("assets/textures/icons/logo-citm.png");
	texStudioPlaceholder_ = Engine::GetInstance().render->CreateMenuTextTexture(
		"HIDDEN DREAM STUDIO", { 255, 255, 255, 255 });
}

void Scene::UnloadIntro()
{
	if (texCitmLogo_) { Engine::GetInstance().textures->UnLoad(texCitmLogo_); texCitmLogo_ = nullptr; }
	if (texStudioPlaceholder_) { SDL_DestroyTexture(texStudioPlaceholder_); texStudioPlaceholder_ = nullptr; }
}

void Scene::UpdateIntro(float dt)
{
	// Skip with Space
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN) {
		if (!waitingForFade_) {
			waitingForFade_ = true;
			fadeTargetScene_ = SceneID::MAIN_MENU;
			Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 300.0f);
		}
	}

	if (waitingForFade_) { DrawIntro(); return; }

	introTimer_ += dt;

	switch (introPhase_) {
	case IntroPhase::CITM_FADEIN:
		if (introTimer_ >= INTRO_FADE_MS) { introTimer_ = 0; introPhase_ = IntroPhase::CITM_HOLD; }
		break;
	case IntroPhase::CITM_HOLD:
		if (introTimer_ >= INTRO_HOLD_MS) { introTimer_ = 0; introPhase_ = IntroPhase::CITM_FADEOUT; }
		break;
	case IntroPhase::CITM_FADEOUT:
		if (introTimer_ >= INTRO_FADE_MS) { introTimer_ = 0; introPhase_ = IntroPhase::STUDIO_FADEIN; }
		break;
	case IntroPhase::STUDIO_FADEIN:
		if (introTimer_ >= INTRO_FADE_MS) { introTimer_ = 0; introPhase_ = IntroPhase::STUDIO_HOLD; }
		break;
	case IntroPhase::STUDIO_HOLD:
		if (introTimer_ >= INTRO_HOLD_MS) { introTimer_ = 0; introPhase_ = IntroPhase::STUDIO_FADEOUT; }
		break;
	case IntroPhase::STUDIO_FADEOUT:
		if (introTimer_ >= INTRO_FADE_MS) {
			introPhase_ = IntroPhase::DONE;
			waitingForFade_ = true;
			fadeTargetScene_ = SceneID::MAIN_MENU;
			Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 500.0f);
		}
		break;
	case IntroPhase::DONE:
		break;
	}

	DrawIntro();
}

void Scene::DrawIntro()
{
	auto& render = *Engine::GetInstance().render;
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	// Black background
	SDL_Rect bg = { 0, 0, winW, winH };
	render.DrawRectangle(bg, 0, 0, 0, 255, true, false);

	SDL_Texture* logo = nullptr;
	Uint8 alpha = 0;
	float cumTime = 0.0f;

	bool isCitm = (introPhase_ == IntroPhase::CITM_FADEIN ||
	               introPhase_ == IntroPhase::CITM_HOLD ||
	               introPhase_ == IntroPhase::CITM_FADEOUT);
	bool isStudio = (introPhase_ == IntroPhase::STUDIO_FADEIN ||
	                 introPhase_ == IntroPhase::STUDIO_HOLD ||
	                 introPhase_ == IntroPhase::STUDIO_FADEOUT);

	if (isCitm) {
		logo = texCitmLogo_;
		if (introPhase_ == IntroPhase::CITM_FADEIN) {
			alpha = (Uint8)(255.0f * (introTimer_ / INTRO_FADE_MS));
			cumTime = introTimer_;
		} else if (introPhase_ == IntroPhase::CITM_HOLD) {
			alpha = 255;
			cumTime = INTRO_FADE_MS + introTimer_;
		} else {
			alpha = (Uint8)(255.0f * (1.0f - introTimer_ / INTRO_FADE_MS));
			cumTime = INTRO_FADE_MS + INTRO_HOLD_MS + introTimer_;
		}
	}
	else if (isStudio) {
		logo = texStudioPlaceholder_;
		if (introPhase_ == IntroPhase::STUDIO_FADEIN) {
			alpha = (Uint8)(255.0f * (introTimer_ / INTRO_FADE_MS));
			cumTime = introTimer_;
		} else if (introPhase_ == IntroPhase::STUDIO_HOLD) {
			alpha = 255;
			cumTime = INTRO_FADE_MS + introTimer_;
		} else {
			alpha = (Uint8)(255.0f * (1.0f - introTimer_ / INTRO_FADE_MS));
			cumTime = INTRO_FADE_MS + INTRO_HOLD_MS + introTimer_;
		}
	}

	if (logo && alpha > 0) {
		float zoomProgress = cumTime / INTRO_TOTAL_MS;
		if (zoomProgress > 1.0f) zoomProgress = 1.0f;
		
		// Ease-out quadratic for smoother zoom deceleration
		float easeT = zoomProgress * (2.0f - zoomProgress);
		float zoom = 1.0f + 0.05f * easeT;

		float tw = 0, th = 0;
		SDL_GetTextureSize(logo, &tw, &th);

		// Scale logo to fit nicely on screen
		float targetW = winW * 0.45f;
		float logoScale = targetW / tw;
		if (logoScale > 3.0f) logoScale = 3.0f;

		float drawW = tw * logoScale * zoom;
		float drawH = th * logoScale * zoom;
		float drawX = (winW - drawW) / 2.0f;
		float drawY = (winH - drawH) / 2.0f;

		render.DrawTextureAlphaF(logo, drawX, drawY, drawW, drawH, alpha);
	}
}

// ============================================================================
//  INTRO CINEMATIC
// ============================================================================

void Scene::LoadIntroCinematic()
{
	LOG("Playing intro cinematic...");
	Engine::GetInstance().cinematics->PlayVideo("assets/video/test.mp4");
}

void Scene::UnloadIntroCinematic()
{
	Engine::GetInstance().cinematics->StopVideo();
}

void Scene::UpdateIntroCinematic(float dt)
{
	if (waitingForFade_) return;

	bool skipRequested = Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN || 
						 Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN;

	if (skipRequested) {
		// Smoothly fade out the ongoing video
		waitingForFade_ = true;
		fadeTargetScene_ = SceneID::GAMEPLAY;
		Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 1000.0f);
	} 
	else if (!Engine::GetInstance().cinematics->IsPlaying()) {
		// Video finished naturally. It has already vanished, so transition immediately!
		waitingForFade_ = true;
		fadeTargetScene_ = SceneID::GAMEPLAY;
		Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 0.0f);
	}
}

// ============================================================================
//  GAMEPLAY
// ============================================================================

void Scene::LoadGameplay()
{
	isPaused_ = false;
	showPauseOptions_ = false;

	Engine::GetInstance().map->Load("assets/maps/", "MapTemplate.tmx");
	Engine::GetInstance().map->LoadEntities(player);

	if (player == nullptr) {
		player = std::dynamic_pointer_cast<Player>(
			Engine::GetInstance().entityManager->CreateEntity(EntityType::PLAYER));
		player->position = Vector2D(96.0f, 672.0f);
		player->Start();
	}

	// Create pause menu buttons (disabled until paused)
	LoadPauseMenuButtons();

	// Set ambient lighting tint for cave environment (GPU color modulation)
	// Derived from the background palette: dark blue-grey cave (#2B3545 → tint ~60%)
	Engine::GetInstance().render->SetAmbientTint(140, 155, 190);
<<<<<<< HEAD


	auto setupAnimFromTSX = [](const char* tsxPath, Animation& anim, SDL_Texture*& tex) {
		anim = Animation();
		pugi::xml_document doc;
		if (!doc.load_file(tsxPath)) {
			LOG("Error: Failed to load TSX: %s", tsxPath);
			return;
		}

		pugi::xml_node tileset = doc.child("tileset");
		int tw = tileset.attribute("tilewidth").as_int();
		int th = tileset.attribute("tileheight").as_int();
		int count = tileset.attribute("tilecount").as_int();
		int columns = tileset.attribute("columns").as_int();
		std::string imgPath = tileset.child("image").attribute("source").as_string();
		
		std::string finalPath = "assets/textures/spritesheets/SS Healthbar/";
		size_t lastSlash = imgPath.find_last_of('/');
		if (lastSlash != std::string::npos) {
			finalPath += imgPath.substr(lastSlash + 1);
		} else {
			finalPath += imgPath;
		}

		tex = Engine::GetInstance().textures->Load(finalPath.c_str());
		if (!tex) LOG("Error: Failed to load texture from TSX: %s", finalPath.c_str());

		LOG("HUD TSX loaded: %s, %dx%d, %d frames", tsxPath, tw, th, count);

		for (int i = 0; i < count; i++) {
			SDL_Rect r;
			r.x = (i % columns) * tw;
			r.y = (i / columns) * th;
			r.w = tw;
			r.h = th;
			anim.AddFrame(r, 100);
		}
		anim.SetLoop(false);
	};

	setupAnimFromTSX("assets/textures/animations/HealthAnimations/SS_Healthbar-1.tsx", animHealth1_, texHealth1_);
	setupAnimFromTSX("assets/textures/animations/HealthAnimations/SS_Healthbar-2.tsx", animHealth2_, texHealth2_);
	setupAnimFromTSX("assets/textures/animations/HealthAnimations/SS_Healthbar-3.tsx", animHealth3_, texHealth3_);

	currentHealthUI_ = 3;
	activeHealthAnim_ = 0;
	isGameOver_ = false;
	texGameOver_ = Engine::GetInstance().render->CreateMenuTextTexture("GAME OVER, YOU DIED", { 255, 0, 0, 255 });

	// Game Over Button
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);
	
	// Load Game Over textures
	texGameOverBg_ = nullptr;
	texGameOverBtn_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Pause_Menu_button_white.png");
	texGameOverKid_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Kid_Fragmented2.png");
	texGameOverFrag1_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Fragment1.png");
	texGameOverFrag3_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Fragment3.png");
	texGameOverFrag4_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Fragment4.png");
	texGameOverFrag5_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Fragment5.png");

	SDL_Rect contBtnPos = { winW / 2 - 230, winH / 2 - 20, 460, 84 };
	SDL_Rect mainBtnPos = { winW / 2 - 230, winH / 2 + 84, 460, 84 };
	
	auto contBtn = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_GAMEOVER_CONTINUE, "CONTINUE", contBtnPos, this);
	auto goBtn = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_GAMEOVER_MAINMENU, "MAIN MENU", mainBtnPos, this);

	// Hide Game Over buttons by default
	contBtn->isVisible = false;
	goBtn->isVisible = false;

	if (texGameOverBtn_) {
		contBtn->SetTexture(texGameOverBtn_);
		goBtn->SetTexture(texGameOverBtn_);
	}
	if (goBtn) {
		goBtn->state = UIElementState::DISABLED;
	}
}

void Scene::UpdateGameplay(float dt)
{
	// Toggle pause with ESC
	if (!isGameOver_ && Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN)
	{
		if (showPauseOptions_) {
			showPauseOptions_ = false;
			SetPauseOptionsPanelVisible(false);
		}
		else {
			isPaused_ = !isPaused_;
			SetPauseMenuVisible(isPaused_);
		}
	}

	// Draw pause overlay BEFORE UIManager::Update so buttons render on top
	if (isPaused_) DrawPauseMenu();

	if (isPaused_) return;

	if (player) {
		// Fall death check
		Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
		if (player->position.getY() > mapSize.getY() + 100.0f && player->health > 0) {
			player->TakeDamage(player->health);
			currentHealthUI_ = 0;
			activeHealthAnim_ = 0;
			isGameOver_ = true;
		}

		// HUD Logic
		if (player->health < currentHealthUI_) {
			if (currentHealthUI_ == 3) {
				activeHealthAnim_ = 1;
				animHealth1_.Reset();
			} else if (currentHealthUI_ == 2) {
				activeHealthAnim_ = 2;
				animHealth2_.Reset();
			} else if (currentHealthUI_ == 1) {
				activeHealthAnim_ = 3;
				animHealth3_.Reset();
			}
			currentHealthUI_ = player->health;
		}

		if (activeHealthAnim_ == 1) {
			animHealth1_.Update(dt);
			if (animHealth1_.HasFinishedOnce()) activeHealthAnim_ = 0;
		} else if (activeHealthAnim_ == 2) {
			animHealth2_.Update(dt);
			if (animHealth2_.HasFinishedOnce()) activeHealthAnim_ = 0;
		} else if (activeHealthAnim_ == 3) {
			animHealth3_.Update(dt);
			if (animHealth3_.HasFinishedOnce()) activeHealthAnim_ = 0;
		}

		if (player->health <= 0 && activeHealthAnim_ == 0 && !isGameOver_) {
			isGameOver_ = true;
			// Enable Game Over buttons
			auto& list = Engine::GetInstance().uiManager->UIElementsList;
			for (auto& el : list) {
				if (el->id == BTN_GAMEOVER_MAINMENU || el->id == BTN_GAMEOVER_CONTINUE) {
					el->isVisible = true;
					el->state = UIElementState::NORMAL;
				}
				
				if (el->id == BTN_GAMEOVER_CONTINUE) {
					if (!Engine::GetInstance().saveSystem->HasValidSave())
						el->state = UIElementState::DISABLED;
				}
			}
		}
	}
}

void Scene::UnloadGameplay()
{
	Engine::GetInstance().uiManager->CleanUp();
	player.reset();
	Engine::GetInstance().map->CleanUp();
	Engine::GetInstance().entityManager->CleanUp();
	isPaused_ = false;
	showPauseOptions_ = false;

	if (texHealth1_) { Engine::GetInstance().textures->UnLoad(texHealth1_); texHealth1_ = nullptr; }
	if (texHealth2_) { Engine::GetInstance().textures->UnLoad(texHealth2_); texHealth2_ = nullptr; }
	if (texHealth3_) { Engine::GetInstance().textures->UnLoad(texHealth3_); texHealth3_ = nullptr; }
	if (texGameOver_) { SDL_DestroyTexture(texGameOver_); texGameOver_ = nullptr; }
	
	if (texGameOverBg_) { Engine::GetInstance().textures->UnLoad(texGameOverBg_); texGameOverBg_ = nullptr; }
	if (texGameOverBtn_) { Engine::GetInstance().textures->UnLoad(texGameOverBtn_); texGameOverBtn_ = nullptr; }
	if (texGameOverKid_) { Engine::GetInstance().textures->UnLoad(texGameOverKid_); texGameOverKid_ = nullptr; }
	if (texGameOverFrag1_) { Engine::GetInstance().textures->UnLoad(texGameOverFrag1_); texGameOverFrag1_ = nullptr; }
	if (texGameOverFrag3_) { Engine::GetInstance().textures->UnLoad(texGameOverFrag3_); texGameOverFrag3_ = nullptr; }
	if (texGameOverFrag5_) { Engine::GetInstance().textures->UnLoad(texGameOverFrag5_); texGameOverFrag5_ = nullptr; }
}

void Scene::PostUpdateGameplay()
{
	// Quick save/load shortcuts (only when not paused)
	if (!isPaused_ && !isGameOver_) {
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F5) == KEY_DOWN)
			Engine::GetInstance().saveSystem->QuickLoad();
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F6) == KEY_DOWN)
			Engine::GetInstance().saveSystem->QuickSave();
	}

	// --- Draw Health HUD ---
	SDL_Rect r;
	const SDL_Rect* frame = nullptr;
	SDL_Texture* texToDraw = nullptr;

	if (activeHealthAnim_ == 1) {
		texToDraw = texHealth1_;
		frame = &animHealth1_.GetCurrentFrame();
	} else if (activeHealthAnim_ == 2) {
		texToDraw = texHealth2_;
		frame = &animHealth2_.GetCurrentFrame();
	} else if (activeHealthAnim_ == 3) {
		texToDraw = texHealth3_;
		frame = &animHealth3_.GetCurrentFrame();
	} else {
		// Draw static frame based on health
		if (currentHealthUI_ == 3) {
			texToDraw = texHealth1_;
			if (texHealth1_) {
				r = animHealth1_.GetCurrentFrame(); // Use frame 0 (Reset ensures it's at 0)
				frame = &r;
			}
		} else if (currentHealthUI_ == 2) {
			texToDraw = texHealth2_;
			if (texHealth2_) {
				r = animHealth2_.GetCurrentFrame();
				frame = &r;
			}
		} else if (currentHealthUI_ == 1) {
			texToDraw = texHealth3_;
			if (texHealth3_) {
				r = animHealth3_.GetCurrentFrame();
				frame = &r;
			}
		} else if (currentHealthUI_ <= 0) {
			texToDraw = texHealth3_;
			if (texHealth3_) {
				r = animHealth3_.GetCurrentFrame();
				frame = &r;
			}
		}
	}

	if (texToDraw && frame) {
		// Draw HUD at top left. Using speed = 0.0f makes it static to camera
		// Scale reduced to 0.5f as requested
		Engine::GetInstance().render->DrawTexture(texToDraw, 40, 40, frame, 0.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, 0.5f);
	}

	// --- Game Over Screen ---
	if (isGameOver_) {
		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);

		// Dark overlay removed as requested

		// "GAME OVER" Text
		if (texGameOver_) {
			float tw, th;
			SDL_GetTextureSize(texGameOver_, &tw, &th);
			float drawX = ((float)winW - tw) / 2.0f;
			float drawY = ((float)winH - th) / 2.0f - 200.0f; 
			
			float scale = 1.5f;
			float sw = tw * scale;
			float sh = th * scale;
			float sx = ((float)winW - sw) / 2.0f;
			float sy = drawY - (sh - th) / 2.0f;

			// Enable linear filtering for smoother scaling
			SDL_SetTextureScaleMode(texGameOver_, SDL_SCALEMODE_LINEAR);

			// Directional Shadow: Draw once with a fixed offset and low alpha
			float shadowOffset = 6.0f;
			SDL_SetTextureColorMod(texGameOver_, 255, 100, 100); // Shadow color (light red)
			Engine::GetInstance().render->DrawTextureAlphaF(texGameOver_, sx + shadowOffset, sy + shadowOffset, sw, sh, (Uint8)130);
			
			SDL_SetTextureColorMod(texGameOver_, 255, 255, 255); // Reset color mod

			// Main text draw
			Engine::GetInstance().render->DrawTextureAlphaF(texGameOver_, sx, sy, sw, sh, (Uint8)255);
		}

		// Draw Fragments and Kid
		if (texGameOverKid_) {
			float kw, kh;
			SDL_GetTextureSize(texGameOverKid_, &kw, &kh);
			float kScale = 0.22f; // Reduced size as requested
			Engine::GetInstance().render->DrawTextureAlphaF(texGameOverKid_, (float)winW - kw * kScale - 20.0f, (float)winH - kh * kScale - 20.0f, kw * kScale, kh * kScale, (Uint8)255);
		}
		
		// Distributed fragments
		if (texGameOverFrag1_) Engine::GetInstance().render->DrawTextureAlphaF(texGameOverFrag1_, 100.0f, 150.0f, 160.0f, 160.0f, 180);
		if (texGameOverFrag3_) Engine::GetInstance().render->DrawTextureAlphaF(texGameOverFrag3_, (float)winW - 350.0f, 80.0f, 200.0f, 200.0f, 150);
		if (texGameOverFrag5_) Engine::GetInstance().render->DrawTextureAlphaF(texGameOverFrag5_, 200.0f, (float)winH - 300.0f, 140.0f, 140.0f, 160);
		if (texGameOverFrag1_) Engine::GetInstance().render->DrawTextureAlphaF(texGameOverFrag1_, (float)winW / 2.0f + 250.0f, (float)winH / 2.0f + 120.0f, 120.0f, 120.0f, 130);
		
		// New Fragment 4 in bottom-left
		if (texGameOverFrag4_) Engine::GetInstance().render->DrawTextureAlphaF(texGameOverFrag4_, 50.0f, (float)winH - 220.0f, 180.0f, 180.0f, 200);
	}
}

// ── Pause menu helpers ────────────────────────────────────────────────────────

void Scene::LoadPauseMenuButtons()
{
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	const int btnW = 240;
	const int btnH = 42;
	const int btnX = winW / 2 - btnW / 2;
	const int startY = winH / 2 - 115;
	const int gap = 54;

	// Main pause buttons
	SDL_Rect contPos = { btnX, startY,          btnW, btnH };
	SDL_Rect optPos = { btnX, startY + gap,     btnW, btnH };
	SDL_Rect savePos = { btnX, startY + gap * 2, btnW, btnH };
	SDL_Rect menuPos = { btnX, startY + gap * 3, btnW, btnH };
	SDL_Rect quitPos = { btnX, startY + gap * 4, btnW, btnH };

	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_CONTINUE, "CONTINUE", contPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_OPTIONS, "OPTIONS", optPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_SAVE, "SAVE GAME", savePos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_MAINMENU, "MAIN MENU", menuPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_QUIT, "QUIT GAME", quitPos, this);

	// Options sub-panel buttons
	const int panelW = 340;
	const int panelX = winW / 2 - panelW / 2;
	const int panelY = winH / 2 - 100;
	const int smallBtnW = 40;
	const int smallBtnH = 34;
	const int rowH = 52;

	SDL_Rect mUpPos = { panelX + panelW - smallBtnW - 12, panelY + 60,         smallBtnW, smallBtnH };
	SDL_Rect mDownPos = { panelX + 12,                       panelY + 60,         smallBtnW, smallBtnH };
	SDL_Rect sUpPos = { panelX + panelW - smallBtnW - 12, panelY + 60 + rowH,  smallBtnW, smallBtnH };
	SDL_Rect sDownPos = { panelX + 12,                       panelY + 60 + rowH,  smallBtnW, smallBtnH };
	SDL_Rect backPos = { panelX + panelW / 2 - 60,          panelY + 60 + rowH * 2 + 10, 120, btnH };

	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_OPT_MUSIC_UP, "+", mUpPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_OPT_MUSIC_DOWN, "-", mDownPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_OPT_SFX_UP, "+", sUpPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_OPT_SFX_DOWN, "-", sDownPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_OPT_BACK, "BACK", backPos, this);

	// Everything disabled until paused
	SetPauseMenuVisible(false);
}

void Scene::SetPauseMenuVisible(bool visible)
{
	auto& list = Engine::GetInstance().uiManager->UIElementsList;
	for (auto& el : list) {
		if (el->id == BTN_PAUSE_CONTINUE || el->id == BTN_PAUSE_OPTIONS || el->id == BTN_PAUSE_SAVE || el->id == BTN_PAUSE_MAINMENU || el->id == BTN_PAUSE_QUIT) {
			el->isVisible = visible;
			el->state = visible ? UIElementState::NORMAL : UIElementState::DISABLED;
		}
		// Always hide options sub-panel when toggling main
		bool isOpt = (el->id >= BTN_PAUSE_OPT_MUSIC_UP && el->id <= BTN_PAUSE_OPT_BACK);
		if (isOpt) {
			el->state = UIElementState::DISABLED;
			el->isVisible = false;
		}
	}
}

void Scene::SetPauseOptionsPanelVisible(bool visible)
{
	auto& list = Engine::GetInstance().uiManager->UIElementsList;
	for (auto& el : list)
	{
		bool isPauseMain = (el->id >= BTN_PAUSE_CONTINUE && el->id <= BTN_PAUSE_QUIT);
		bool isOpt = (el->id >= BTN_PAUSE_OPT_MUSIC_UP && el->id <= BTN_PAUSE_OPT_BACK);

		if (isPauseMain) {
			el->state = visible ? UIElementState::DISABLED : UIElementState::NORMAL;
			el->isVisible = !visible;
		}
		if (isOpt) {
			el->state = visible ? UIElementState::NORMAL : UIElementState::DISABLED;
			el->isVisible = visible;
		}
	}
}

void Scene::SetGameOverVisible(bool visible)
{
	isGameOver_ = visible;
	auto& list = Engine::GetInstance().uiManager->UIElementsList;
	for (auto& el : list) {
		if (el->id == BTN_GAMEOVER_CONTINUE || el->id == BTN_GAMEOVER_MAINMENU) {
			el->isVisible = visible;
		}
	}
}

void Scene::ResetHealthUI(int health)
{
	currentHealthUI_ = health;
	activeHealthAnim_ = 0;
	animHealth1_.Reset();
	animHealth2_.Reset();
	animHealth3_.Reset();
}

void Scene::DrawPauseMenu()
{
	auto& render = *Engine::GetInstance().render;
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	// ── Dark overlay ─────────────────────────────────────────────────────────
	SDL_Rect overlay = { 0, 0, winW, winH };
	render.DrawRectangle(overlay, 0, 0, 0, 180, true, false);

	if (showPauseOptions_)
	{
		DrawPauseOptionsPanel(winW, winH);
		return;
	}

	// ── Panel background ─────────────────────────────────────────────────────
	const int panelW = 300;
	const int panelH = 340;
	const int panelX = winW / 2 - panelW / 2;
	const int panelY = winH / 2 - 180;

	SDL_Rect panelBg = { panelX, panelY, panelW, panelH };
	SDL_Rect panelSh = { panelX + 4, panelY + 4, panelW, panelH };
	render.DrawRectangle(panelSh, 0, 0, 0, 100, true, false);
	render.DrawRectangle(panelBg, 8, 12, 22, 250, true, false);
	render.DrawRectangle(panelBg, 60, 90, 150, 200, false, false);

	// Top accent bar
	SDL_Rect topBar = { panelX, panelY, panelW, 4 };
	render.DrawRectangle(topBar, 80, 140, 200, 255, true, false);

	// ── PAUSED title ─────────────────────────────────────────────────────────
	render.DrawText("PAUSED", panelX + panelW / 2 - 37, panelY + 14, 0, 0,
		{ 180, 210, 240, 255 });

	// Thin separator
	render.DrawLine(panelX + 20, panelY + 46, panelX + panelW - 20, panelY + 46,
		60, 100, 160, 150, false);
}

void Scene::DrawPauseOptionsPanel(int winW, int winH)
{
	auto& render = *Engine::GetInstance().render;

	const int panelW = 340;
	const int panelH = 240;
	const int panelX = winW / 2 - panelW / 2;
	const int panelY = winH / 2 - 100;
	const int rowH = 52;

	SDL_Rect panel = { panelX, panelY, panelW, panelH };
	render.DrawRectangle(panel, 8, 12, 22, 250, true, false);
	render.DrawRectangle(panel, 60, 90, 150, 200, false, false);

	SDL_Rect topBar = { panelX, panelY, panelW, 4 };
	render.DrawRectangle(topBar, 80, 140, 200, 255, true, false);

	render.DrawText("OPTIONS", panelX + panelW / 2 - 42, panelY + 14, 0, 0,
		{ 180, 210, 240, 255 });

	// Music row
	render.DrawText("MUSIC", panelX + 60, panelY + 56, 0, 0, { 150, 180, 210, 220 });
	SDL_Rect mBarBg = { panelX + 60, panelY + 86, panelW - 120, 8 };
	render.DrawRectangle(mBarBg, 30, 40, 60, 200, true, false);
	int mFill = static_cast<int>(static_cast<float>(panelW - 120) * musicVolume_);
	SDL_Rect mBarFill = { panelX + 60, panelY + 86, mFill, 8 };
	render.DrawRectangle(mBarFill, 80, 160, 220, 255, true, false);
	char vol[8];
	snprintf(vol, sizeof(vol), "%d%%", static_cast<int>(musicVolume_ * 100.0f));
	render.DrawText(vol, panelX + panelW / 2 - 15, panelY + 56, 0, 0, { 200, 220, 240, 255 });

	// SFX row
	render.DrawText("SFX", panelX + 60, panelY + 56 + rowH, 0, 0, { 150, 180, 210, 220 });
	SDL_Rect sBarBg = { panelX + 60, panelY + 86 + rowH, panelW - 120, 8 };
	render.DrawRectangle(sBarBg, 30, 40, 60, 200, true, false);
	int sFill = static_cast<int>(static_cast<float>(panelW - 120) * sfxVolume_);
	SDL_Rect sBarFill = { panelX + 60, panelY + 86 + rowH, sFill, 8 };
	render.DrawRectangle(sBarFill, 80, 160, 220, 255, true, false);
	snprintf(vol, sizeof(vol), "%d%%", static_cast<int>(sfxVolume_ * 100.0f));
	render.DrawText(vol, panelX + panelW / 2 - 15, panelY + 56 + rowH, 0, 0, { 200, 220, 240, 255 });
}

void Scene::HandlePauseMenuUIEvents(UIElement* uiElement)
{
	if (waitingForFade_) return;

	switch (uiElement->id)
	{
	case BTN_PAUSE_CONTINUE:
		isPaused_ = false;
		SetPauseMenuVisible(false);
		break;

	case BTN_PAUSE_OPTIONS:
		showPauseOptions_ = true;
		SetPauseOptionsPanelVisible(true);
		break;

	case BTN_PAUSE_SAVE:
		Engine::GetInstance().saveSystem->QuickSave();
		// Brief visual feedback: show saved message (next frame)
		LOG("Game saved from pause menu");
		break;

	case BTN_PAUSE_MAINMENU:
		isPaused_ = false;
		waitingForFade_ = true;
		fadeTargetScene_ = SceneID::MAIN_MENU;
		Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 500.0f);
		break;

	case BTN_PAUSE_QUIT:
	{
		SDL_Event quitEvent;
		quitEvent.type = SDL_EVENT_QUIT;
		SDL_PushEvent(&quitEvent);
		break;
	}

	// ── Options sub-panel ────────────────────────────────────────────────────
	case BTN_PAUSE_OPT_MUSIC_UP:
		musicVolume_ = std::min(1.0f, musicVolume_ + 0.1f);
		Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
		break;
	case BTN_PAUSE_OPT_MUSIC_DOWN:
		musicVolume_ = std::max(0.0f, musicVolume_ - 0.1f);
		Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
		break;
	case BTN_PAUSE_OPT_SFX_UP:
		sfxVolume_ = std::min(1.0f, sfxVolume_ + 0.1f);
		Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);
		break;
	case BTN_PAUSE_OPT_SFX_DOWN:
		sfxVolume_ = std::max(0.0f, sfxVolume_ - 0.1f);
		Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);
		break;
	case BTN_PAUSE_OPT_BACK:
		showPauseOptions_ = false;
		SetPauseOptionsPanelVisible(false);
		break;
	
	case BTN_GAMEOVER_MAINMENU:
		isGameOver_ = false;
		waitingForFade_ = true;
		fadeTargetScene_ = SceneID::MAIN_MENU;
		Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 500.0f);
		break;
	
	case BTN_GAMEOVER_CONTINUE:
	{
		SetGameOverVisible(false);
		Engine::GetInstance().saveSystem->QuickLoad();
		break;
	}

	default:
		break;
	}
}

// ============================================================================
//  FLOATING FRAGMENTS  (Main Menu Decoration)
// ============================================================================

static float RandF(float lo, float hi) {
	return lo + (float)rand() / (float)RAND_MAX * (hi - lo);
}

void Scene::InitFragments(int winW, int winH, int childX, int childW)
{
	srand((unsigned)time(nullptr));

	// Character face exclusion zone — upper center of the child image
	float faceLeft   = childX + childW * 0.20f;
	float faceRight  = childX + childW * 0.80f;
	float faceTop    = 0.0f;
	float faceBottom = winH * 0.45f;

	// Screen quadrant boundaries
	float halfW = winW * 0.5f;
	float halfH = winH * 0.5f;

	for (int i = 0; i < NUM_FRAGMENTS; i++) {
		auto& f = fragments_[i];
		if (!f.tex) continue;

		// Query native texture size
		float tw = 0, th = 0;
		SDL_GetTextureSize(f.tex, &tw, &th);

		// Size fragments to ~25-30% of screen width as requested
		float scale = RandF(0.25f, 0.30f);
		f.w = winW * scale;
		f.h = f.w * (th / tw);

		// Decide front vs back: first 3 front, rest back
		f.inFront = (i < 3);

		// Distribute uniformly to surround the character in the right half of the screen.
		// Since they are HUGE, they have strict fixed positions so they frame the character and NEVER overlap.
		
		float padX = 10.0f;
		float padY = 15.0f;
		
		if (i == 0) { // FRONT 1 (Bottom-left of the right half)
			f.x = halfW + padX;
			f.y = winH - f.h - padY;
		}
		else if (i == 1) { // FRONT 2 (Bottom-center of the right half)
			f.x = halfW + (winW - halfW) / 2.0f - (f.w / 2.0f);
			// Ensured it stays 100% inside the screen instead of pushing it out
			f.y = winH - f.h - padY;
		}
		else if (i == 2) { // FRONT 3 (Bottom-right of the right half)
			f.x = winW - f.w - padX;
			f.y = winH - f.h - padY;
		}
		else if (i == 3) { // BACK 1 (Top-left of the right half - Fixed)
			f.x = halfW + padX;
			f.y = padY + 10.0f;
		}
		else if (i == 4) { // BACK 2 (Top-right of the right half - Fixed)
			f.x = winW - f.w - padX;
			f.y = padY + 10.0f;
		}

		// Animation parameters — unique per fragment
		f.floatSpeed     = RandF(0.4f, 0.9f);     // rad/s (slow, dreamy)
		f.floatAmplitude = RandF(8.0f, 22.0f);     // px vertical sway
		f.floatPhase     = RandF(0.0f, 6.2831f);   // random start phase
		f.driftX         = RandF(0.15f, 0.45f);    // subtle horizontal sway speed
		f.driftPhase     = RandF(0.0f, 6.2831f);
		f.rotSpeed       = RandF(-6.0f, 6.0f);     // degrees/sec - very gentle
		f.rotation       = RandF(0.0f, 360.0f);

		// Alpha: no blur, full opacity
		f.alpha = 255;
	}
}

void Scene::DrawFragments(bool front, int winW, int winH)
{
	auto& render = *Engine::GetInstance().render;

	// Time in seconds (fragmentTime_ is in ms from dt)
	float t = fragmentTime_ / 1000.0f;

	for (int i = 0; i < NUM_FRAGMENTS; i++) {
		auto& f = fragments_[i];
		if (!f.tex || f.inFront != front) continue;

		// Smooth sinusoidal floating
		float yOff = f.floatAmplitude * sinf(t * f.floatSpeed + f.floatPhase);
		float xOff = (f.floatAmplitude * 0.3f) * sinf(t * f.driftX + f.driftPhase);

		// Smooth rotation update
		float angle = f.rotation + f.rotSpeed * t;

		float drawX = f.x + xOff;
		float drawY = f.y + yOff;

		// Use DrawTextureAlphaF for sub-pixel smooth rendering
		// For the rotation, we need to use the full DrawTexture with angle
		// But DrawTextureAlphaF doesn't support rotation, so we'll use SDL directly
		SDL_SetTextureAlphaMod(f.tex, f.alpha);
		SDL_SetTextureBlendMode(f.tex, SDL_BLENDMODE_BLEND);

		int scale = Engine::GetInstance().window->GetScale();
		SDL_FRect dst;
		dst.x = drawX * static_cast<float>(scale);
		dst.y = drawY * static_cast<float>(scale);
		dst.w = f.w * static_cast<float>(scale);
		dst.h = f.h * static_cast<float>(scale);

		SDL_RenderTextureRotated(render.renderer, f.tex, nullptr, &dst,
			(double)angle, nullptr, SDL_FLIP_NONE);

		SDL_SetTextureAlphaMod(f.tex, 255);
	}
}