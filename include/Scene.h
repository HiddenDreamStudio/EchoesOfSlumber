#pragma once

#include "Module.h"
#include "Player.h"
#include "UIButton.h"

struct SDL_Texture;

enum class SceneID
{
	MAIN_MENU,
	INTRO_CINEMATIC,
	GAMEPLAY
};


class Scene : public Module
{
public:

	Scene();

	// Destructor
	virtual ~Scene();

	// Called before render is available
	bool Awake();

	// Called before the first frame
	bool Start();

	// Called before all Updates
	bool PreUpdate();

	// Called each loop iteration
	bool Update(float dt);

	// Called before all Updates
	bool PostUpdate();

	// Called before quitting
	bool CleanUp();

	// Return the player position
	Vector2D GetPlayerPosition();

	// Get tilePosDebug value
	std::string GetTilePosDebug() {
		return tilePosDebug;
	}

	// Handles multiple Gui Event methods
	bool OnUIMouseClickEvent(UIElement* uiElement);

	void ChangeScene(SceneID newScene);
	void UnloadCurrentScene();
	void LoadScene(SceneID newScene);

private:

	void LoadMainMenu();
	void UnloadMainMenu();
	void UpdateMainMenu(float dt);
	void HandleMainMenuUIEvents(UIElement* uiElement);

	void LoadIntroCinematic();
	void UnloadIntroCinematic();
	void UpdateIntroCinematic(float dt);

	void LoadGameplay();
	void UnloadGameplay();
	void UpdateGameplay(float dt);
	void PostUpdateGameplay();

private:

	std::shared_ptr<Player> player;
	SDL_Texture* mouseTileTex = nullptr;
	std::string tilePosDebug = "[0,0]";
	bool once = false;

	std::shared_ptr<UIButton> uiBt;
	float volume = 1.0;

	SceneID currentScene = SceneID::MAIN_MENU;

	// Deferred scene change to avoid use-after-free when called from UI callbacks
	bool hasPendingSceneChange = false;
	SceneID pendingScene = SceneID::MAIN_MENU;
};