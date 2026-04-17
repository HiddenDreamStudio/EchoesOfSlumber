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

	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	// Left half layout: logo + buttons centered horizontally in left half
	const int leftHalf = winW / 2;

	// Logo dimensions (used to position buttons below)
	const int logoW = 385;
	const int logoH = (int)(logoW * (569.0f / 1559.0f)); // ~117px
	const int logoX = (leftHalf - logoW) / 2;
	const int logoY = winH / 4 - logoH / 2;  // centered at 1/4 vertical

	// Buttons centered in left half, below the logo
	// Button texture is 456x130 — keep proportional
	const int btnW = 315;
	const int btnH = (int)(btnW * (130.0f / 456.0f)); // ~90px
	const int btnX = (leftHalf - btnW) / 2;
	const int startY = logoY + logoH + 50;
	const int gap = btnH + 15;

	SDL_Rect playPos     = { btnX, startY,           btnW, btnH };
	SDL_Rect settingsPos = { btnX, startY + gap,     btnW, btnH };
	SDL_Rect exitPos     = { btnX, startY + gap * 2, btnW, btnH };

	auto playBtn = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PLAY, "play", playPos, this);
	auto settingsBtn = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_SETTINGS, "options", settingsPos, this);
	auto exitBtn = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_EXIT, "exit", exitPos, this);

	// Set stone button texture
	if (texMenuButton_) {
		playBtn->SetTexture(texMenuButton_);
		settingsBtn->SetTexture(texMenuButton_);
		exitBtn->SetTexture(texMenuButton_);
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
}

void Scene::UpdateMainMenu(float dt)
{
	// Decrement cooldown each frame
	if (settingsCooldown_ > 0) settingsCooldown_--;

	// ESC closes settings
	if (showSettings_ && Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN)
	{
		showSettings_ = false;
		SetSettingsPanelVisible(false);
	}
}

void Scene::PostUpdateMainMenu()
{
	auto& render = *Engine::GetInstance().render;
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	// ── Background #151F20 ───────────────────────────────────────────────────────
	SDL_Rect bg = { 0, 0, winW, winH };
	render.DrawRectangle(bg, 21, 31, 32, 255, true, false);

	// ── Character (right half, fill from near top to bottom) ─────────────────
	if (texMenuChild_) {
		// IL_NenFront_01.png is 1421x913
		// Target: character fills the right half of the screen, small margin above head
		int childH = winH - 20;  // fill height with 20px top margin
		int childW = (int)(childH * (1421.0f / 913.0f));
		int childX = winW - childW;
		int childY = 20;  // small space above head
		render.DrawTextureAlpha(texMenuChild_, childX, childY, childW, childH, 255);
	}

	// ── Game logo (centered in left half at 1/4 vertical, #D4DAEA) ─────────
	if (texMenuLogo_) {
		const int leftHalf = winW / 2;
		int logoW = 385;
		int logoH = (int)(logoW * (569.0f / 1559.0f));
		int logoX = (leftHalf - logoW) / 2;
		int logoY = winH / 4 - logoH / 2;
		render.DrawTextureAlpha(texMenuLogo_, logoX, logoY, logoW, logoH, 255);
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
	render.DrawText("MUSIC", panelX + 60, panelY + 66, 0, 0, { 150, 180, 210, 220 });

	// Volume bar background
	SDL_Rect barBg = { panelX + 60, panelY + 86, panelW - 120, 8 };
	render.DrawRectangle(barBg, 30, 40, 60, 200, true, false);
	// Volume bar fill
	int musicFill = (int)((panelW - 120) * musicVolume_);
	SDL_Rect barFill = { panelX + 60, panelY + 86, musicFill, 8 };
	render.DrawRectangle(barFill, 80, 160, 220, 255, true, false);

	// Value text
	char volText[8];
	snprintf(volText, sizeof(volText), "%d%%", (int)(musicVolume_ * 100));
	render.DrawText(volText, panelX + panelW / 2 - 15, panelY + 60, 0, 0,
		{ 200, 220, 240, 255 });

	// ── SFX volume row ───────────────────────────────────────────────────────
	render.DrawText("SFX", panelX + 60, panelY + 66 + rowH, 0, 0, { 150, 180, 210, 220 });

	SDL_Rect sfxBarBg = { panelX + 60, panelY + 86 + rowH, panelW - 120, 8 };
	render.DrawRectangle(sfxBarBg, 30, 40, 60, 200, true, false);
	int sfxFill = (int)((panelW - 120) * sfxVolume_);
	SDL_Rect sfxBarFill = { panelX + 60, panelY + 86 + rowH, sfxFill, 8 };
	render.DrawRectangle(sfxBarFill, 80, 160, 220, 255, true, false);

	snprintf(volText, sizeof(volText), "%d%%", (int)(sfxVolume_ * 100));
	render.DrawText(volText, panelX + panelW / 2 - 15, panelY + 60 + rowH, 0, 0,
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

		if (isSettingsBtn)
			el->state = visible ? UIElementState::NORMAL : UIElementState::DISABLED;

		if (isMainBtn)
			el->state = visible ? UIElementState::DISABLED : UIElementState::NORMAL;
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
		float zoom = 1.0f + 0.05f * zoomProgress;

		float tw = 0, th = 0;
		SDL_GetTextureSize(logo, &tw, &th);

		// Scale logo to fit nicely on screen
		float targetW = winW * 0.45f;
		float logoScale = targetW / tw;
		if (logoScale > 3.0f) logoScale = 3.0f;

		int drawW = (int)(tw * logoScale * zoom);
		int drawH = (int)(th * logoScale * zoom);
		int drawX = (winW - drawW) / 2;
		int drawY = (winH - drawH) / 2;

		render.DrawTextureAlpha(logo, drawX, drawY, drawW, drawH, alpha);
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
		player->position = Vector2D(96, 672);
		player->Start();
	}

	// Create pause menu buttons (disabled until paused)
	LoadPauseMenuButtons();
}

void Scene::UpdateGameplay(float dt)
{
	// Toggle pause with ESC
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN)
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
}

void Scene::UnloadGameplay()
{
	Engine::GetInstance().uiManager->CleanUp();
	player.reset();
	Engine::GetInstance().map->CleanUp();
	Engine::GetInstance().entityManager->CleanUp();
	isPaused_ = false;
	showPauseOptions_ = false;
}

void Scene::PostUpdateGameplay()
{
	// Quick save/load shortcuts (only when not paused)
	if (!isPaused_) {
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F5) == KEY_DOWN)
			Engine::GetInstance().saveSystem->QuickLoad();
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F6) == KEY_DOWN)
			Engine::GetInstance().saveSystem->QuickSave();
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
	for (auto& el : list)
	{
		bool isPauseMain = (el->id >= BTN_PAUSE_CONTINUE && el->id <= BTN_PAUSE_QUIT);
		if (isPauseMain)
			el->state = visible ? UIElementState::NORMAL : UIElementState::DISABLED;

		// Always hide options sub-panel when toggling main
		bool isOpt = (el->id >= BTN_PAUSE_OPT_MUSIC_UP && el->id <= BTN_PAUSE_OPT_BACK);
		if (isOpt)
			el->state = UIElementState::DISABLED;
	}
}

void Scene::SetPauseOptionsPanelVisible(bool visible)
{
	auto& list = Engine::GetInstance().uiManager->UIElementsList;
	for (auto& el : list)
	{
		bool isPauseMain = (el->id >= BTN_PAUSE_CONTINUE && el->id <= BTN_PAUSE_QUIT);
		bool isOpt = (el->id >= BTN_PAUSE_OPT_MUSIC_UP && el->id <= BTN_PAUSE_OPT_BACK);

		if (isPauseMain)
			el->state = visible ? UIElementState::DISABLED : UIElementState::NORMAL;
		if (isOpt)
			el->state = visible ? UIElementState::NORMAL : UIElementState::DISABLED;
	}
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
	render.DrawText("MUSIC", panelX + 60, panelY + 66, 0, 0, { 150, 180, 210, 220 });
	SDL_Rect mBarBg = { panelX + 60, panelY + 86, panelW - 120, 8 };
	render.DrawRectangle(mBarBg, 30, 40, 60, 200, true, false);
	int mFill = (int)((panelW - 120) * musicVolume_);
	SDL_Rect mBarFill = { panelX + 60, panelY + 86, mFill, 8 };
	render.DrawRectangle(mBarFill, 80, 160, 220, 255, true, false);
	char vol[8];
	snprintf(vol, sizeof(vol), "%d%%", (int)(musicVolume_ * 100));
	render.DrawText(vol, panelX + panelW / 2 - 15, panelY + 60, 0, 0, { 200, 220, 240, 255 });

	// SFX row
	render.DrawText("SFX", panelX + 60, panelY + 66 + rowH, 0, 0, { 150, 180, 210, 220 });
	SDL_Rect sBarBg = { panelX + 60, panelY + 86 + rowH, panelW - 120, 8 };
	render.DrawRectangle(sBarBg, 30, 40, 60, 200, true, false);
	int sFill = (int)((panelW - 120) * sfxVolume_);
	SDL_Rect sBarFill = { panelX + 60, panelY + 86 + rowH, sFill, 8 };
	render.DrawRectangle(sBarFill, 80, 160, 220, 255, true, false);
	snprintf(vol, sizeof(vol), "%d%%", (int)(sfxVolume_ * 100));
	render.DrawText(vol, panelX + panelW / 2 - 15, panelY + 60 + rowH, 0, 0, { 200, 220, 240, 255 });
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

	default: break;
	}
}