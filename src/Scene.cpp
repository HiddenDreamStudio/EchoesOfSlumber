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

Scene::Scene() : Module()
{
	name = "scene";
}

Scene::~Scene()
{}

bool Scene::Awake()
{
	LOG("Loading Scene");
	LoadScene(currentScene);
	return true;
}

bool Scene::Start()
{
	return true;
}

bool Scene::PreUpdate()
{
	return true;
}

bool Scene::Update(float dt)
{
	// Apply deferred scene change (safe here, outside UI callbacks)
	if (hasPendingSceneChange) {
		hasPendingSceneChange = false;
		ChangeScene(pendingScene);
	}

	switch (currentScene)
	{
	case SceneID::MAIN_MENU:
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
	case SceneID::MAIN_MENU:
		break;
	case SceneID::INTRO_CINEMATIC:
		break;
	case SceneID::GAMEPLAY:
		PostUpdateGameplay();
		break;
	}

	if(Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN)
		ret = false;

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
	else return Vector2D(0,0);
}

void Scene::SetPlayerPosition(Vector2D pos)
{
	if (player) player->SetPosition(pos);
}

// *********************************************
// Scene change functions
// *********************************************

void Scene::LoadScene(SceneID newScene)
{
	switch (newScene)
	{
	case SceneID::MAIN_MENU:
		LoadMainMenu();
		break;
	case SceneID::INTRO_CINEMATIC:
		LoadIntroCinematic();
		break;
	case SceneID::GAMEPLAY:
		LoadGameplay();
		break;
	}
}

void Scene::ChangeScene(SceneID newScene)
{
	UnloadCurrentScene();
	currentScene = newScene;
	LoadScene(currentScene);
}

void Scene::UnloadCurrentScene() {

	switch (currentScene)
	{
	case SceneID::MAIN_MENU:
		UnloadMainMenu();
		break;
	case SceneID::INTRO_CINEMATIC:
		UnloadIntroCinematic();
		break;
	case SceneID::GAMEPLAY:
		UnloadGameplay();
		break;
	}
}

// *********************************************
// MAIN MENU
// *********************************************

void Scene::LoadMainMenu() {

	// Create a "Play" button centered on screen
	SDL_Rect btPos = { 580, 350, 120, 30 };
	std::dynamic_pointer_cast<UIButton>(Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 1, "Play", btPos, this));
}

void Scene::UnloadMainMenu() {
	Engine::GetInstance().uiManager->CleanUp();	
}

void Scene::UpdateMainMenu(float dt) {}

void Scene::HandleMainMenuUIEvents(UIElement* uiElement)
{
	switch (uiElement->id)
	{
	case 1: // Play button
		LOG("Main Menu: Play clicked!");
		hasPendingSceneChange = true;
		pendingScene = SceneID::INTRO_CINEMATIC;
		break;
	default:
		break;
	}
}

// *********************************************
// INTRO CINEMATIC
// *********************************************

void Scene::LoadIntroCinematic() {
	LOG("Playing intro cinematic...");
	Engine::GetInstance().cinematics->PlayVideo("assets/video/test.mp4");
}

void Scene::UnloadIntroCinematic() {
	Engine::GetInstance().cinematics->StopVideo();
}

void Scene::UpdateIntroCinematic(float dt) {
	// When the video finishes (or is skipped), transition to gameplay
	if (!Engine::GetInstance().cinematics->IsPlaying()) {
		ChangeScene(SceneID::GAMEPLAY);
	}
}

// *********************************************
// GAMEPLAY (blank template — add your game here)
// *********************************************

void Scene::LoadGameplay() {

	// Load the map
	Engine::GetInstance().map->Load("assets/maps/", "MapTemplate.tmx");

	// Load entities from map (Player spawn point, etc.)
	Engine::GetInstance().map->LoadEntities(player);

	// If the map didn't contain an Entities object group, create the player manually
	if (player == nullptr) {
		player = std::dynamic_pointer_cast<Player>(Engine::GetInstance().entityManager->CreateEntity(EntityType::PLAYER));
		player->position = Vector2D(128, 128);
		player->Start();
	}

	// ---- Add your game entities here ----
	// Example:
	// auto item = std::dynamic_pointer_cast<Item>(Engine::GetInstance().entityManager->CreateEntity(EntityType::ITEM));
	// item->position = Vector2D(200, 672);
	// item->Start();
}

void Scene::UpdateGameplay(float dt) {
	// ---- Add gameplay logic here ----
}

void Scene::UnloadGameplay() {

	auto& uiManager = Engine::GetInstance().uiManager;
	uiManager->CleanUp();

	player.reset();

	Engine::GetInstance().map->CleanUp();
	Engine::GetInstance().entityManager->CleanUp();
}

void Scene::PostUpdateGameplay() {

	// Save/Load game state
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F5) == KEY_DOWN) {
		Engine::GetInstance().saveSystem->QuickLoad();
	}

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F6) == KEY_DOWN) {
		Engine::GetInstance().saveSystem->QuickSave();
	}
}
