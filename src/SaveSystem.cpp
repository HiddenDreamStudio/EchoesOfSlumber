#include "SaveSystem.h"
#include "Engine.h"
#include "Scene.h"
#include "Player.h"
#include "EntityManager.h"
#include "Map.h"
#include "Log.h"
#include "pugixml.hpp"
#include <chrono>
#include <filesystem>

SaveSystem::SaveSystem() : Module()
{
	name = "saveSystem";
}

SaveSystem::~SaveSystem()
{
}

bool SaveSystem::Awake()
{
	LOG("SaveSystem: Awake");

	// Create save folder if it doesn't exist
	if (!std::filesystem::exists(saveFolderPath_))
	{
		std::filesystem::create_directories(saveFolderPath_);
		LOG("SaveSystem: Created save folder at %s", saveFolderPath_.c_str());
	}

	return true;
}

bool SaveSystem::Start()
{
	LOG("SaveSystem: Start");
	return true;
}

bool SaveSystem::Update(float dt)
{
	// Handle deferred load (to avoid issues during update loop)
	if (pendingLoad_)
	{
		pendingLoad_ = false;
		ApplyGameState();
	}

	if (pendingSave_)
	{
		pendingSave_ = false;
		CollectGameState();
		WriteXML(quickSavePath_);
	}

	return true;
}

bool SaveSystem::CleanUp()
{
	LOG("SaveSystem: CleanUp");
	return true;
}

bool SaveSystem::SaveGame(const std::string& filename)
{
	LOG("SaveSystem: Saving game to %s", filename.c_str());

	CollectGameState();
	return WriteXML(saveFolderPath_ + filename);
}

bool SaveSystem::LoadGame(const std::string& filename)
{
	LOG("SaveSystem: Loading game from %s", filename.c_str());

	if (!ReadXML(saveFolderPath_ + filename))
	{
		LOG("SaveSystem: Failed to read save file");
		return false;
	}

	// Defer apply to next frame to avoid issues during update
	pendingLoad_ = true;
	return true;
}

bool SaveSystem::QuickSave()
{
	LOG("SaveSystem: QuickSave (F6)");
	pendingSave_ = true;
	return true;
}

bool SaveSystem::QuickLoad()
{
	LOG("SaveSystem: QuickLoad (F5)");

	if (!SaveFileExists(quickSavePath_))
	{
		LOG("SaveSystem: No quicksave file found");
		return false;
	}

	if (!ReadXML(quickSavePath_))
	{
		LOG("SaveSystem: Failed to read quicksave");
		return false;
	}

	pendingLoad_ = true;
	return true;
}

bool SaveSystem::SaveFileExists(const std::string& filename) const
{
	return std::filesystem::exists(filename);
}

void SaveSystem::CollectGameState()
{
	LOG("SaveSystem: Collecting game state");

	// Set timestamp
	auto now = std::chrono::system_clock::now();
	gameState_.saveTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
		now.time_since_epoch()).count();

	CollectSceneState();
	CollectPlayerState();
	CollectEntityStates();
}

void SaveSystem::ApplyGameState()
{
	LOG("SaveSystem: Applying game state");

	ApplySceneState();
	ApplyPlayerState();
	ApplyEntityStates();

	LOG("SaveSystem: Game state applied successfully");
}

void SaveSystem::CollectPlayerState()
{
	auto& scene = Engine::GetInstance().scene;
	Vector2D playerPos = scene->GetPlayerPosition();

	gameState_.playerPosX = playerPos.getX();
	gameState_.playerPosY = playerPos.getY();

	// Get player reference for additional state
	// Note: We access through Scene since Player is managed by EntityManager
	// For now, we save position. Speed and jumpForce are defaults.
	gameState_.playerSpeed = 4.0f;
	gameState_.playerJumpForce = 2.5f;
	gameState_.playerIsJumping = false;

	LOG("SaveSystem: Player position saved (%.1f, %.1f)", 
		gameState_.playerPosX, gameState_.playerPosY);
}

void SaveSystem::CollectSceneState()
{
	auto& scene = Engine::GetInstance().scene;
	auto& map = Engine::GetInstance().map;

	// Get current scene ID
	gameState_.currentSceneId = static_cast<int>(scene->GetCurrentScene());

	// Get current map path (use public members)
	gameState_.currentMapPath = map->mapPath + map->mapFileName;

	LOG("SaveSystem: Scene state saved (scene=%d, map=%s)", 
		gameState_.currentSceneId, gameState_.currentMapPath.c_str());
}

void SaveSystem::CollectEntityStates()
{
	// For now, entity states will be collected when we have proper entity tracking
	// TODO: Iterate entityManager->entities and collect isPicked/active states
	LOG("SaveSystem: Entity states collection - TODO");
}

void SaveSystem::ApplyPlayerState()
{
	auto& scene = Engine::GetInstance().scene;

	// Set player position
	Vector2D newPos(gameState_.playerPosX, gameState_.playerPosY);
	scene->SetPlayerPosition(newPos);

	LOG("SaveSystem: Player position restored (%.1f, %.1f)", 
		gameState_.playerPosX, gameState_.playerPosY);
}

void SaveSystem::ApplySceneState()
{
	// For now, we assume the scene is already loaded
	// Full scene transitions would require more complex handling
	LOG("SaveSystem: Scene state applied (scene=%d)", gameState_.currentSceneId);
}

void SaveSystem::ApplyEntityStates()
{
	// TODO: Restore entity states (items picked, enemies killed)
	LOG("SaveSystem: Entity states restoration - TODO");
}

bool SaveSystem::WriteXML(const std::string& filename)
{
	pugi::xml_document doc;

	// Root node
	pugi::xml_node root = doc.append_child("savegame");
	root.append_attribute("version") = "1.0";

	// Timestamp
	pugi::xml_node timestampNode = root.append_child("timestamp");
	timestampNode.append_attribute("value") = gameState_.saveTimestamp;

	// Scene state
	pugi::xml_node sceneNode = root.append_child("scene");
	sceneNode.append_attribute("id") = gameState_.currentSceneId;
	sceneNode.append_attribute("map") = gameState_.currentMapPath.c_str();

	// Player state
	pugi::xml_node playerNode = root.append_child("player");

	pugi::xml_node posNode = playerNode.append_child("position");
	posNode.append_attribute("x") = gameState_.playerPosX;
	posNode.append_attribute("y") = gameState_.playerPosY;

	pugi::xml_node stateNode = playerNode.append_child("state");
	stateNode.append_attribute("speed") = gameState_.playerSpeed;
	stateNode.append_attribute("jumpForce") = gameState_.playerJumpForce;
	stateNode.append_attribute("isJumping") = gameState_.playerIsJumping;

	// Entities node (placeholder for future)
	pugi::xml_node entitiesNode = root.append_child("entities");
	// TODO: Add entity states here

	// Save to file
	if (!doc.save_file(filename.c_str()))
	{
		LOG("SaveSystem: Failed to write save file %s", filename.c_str());
		return false;
	}

	LOG("SaveSystem: Game saved to %s", filename.c_str());
	return true;
}

bool SaveSystem::ReadXML(const std::string& filename)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename.c_str());

	if (!result)
	{
		LOG("SaveSystem: Failed to parse save file %s: %s", 
			filename.c_str(), result.description());
		return false;
	}

	pugi::xml_node root = doc.child("savegame");
	if (!root)
	{
		LOG("SaveSystem: Invalid save file format (missing savegame root)");
		return false;
	}

	// Read timestamp
	pugi::xml_node timestampNode = root.child("timestamp");
	if (timestampNode)
	{
		gameState_.saveTimestamp = timestampNode.attribute("value").as_llong();
	}

	// Read scene state
	pugi::xml_node sceneNode = root.child("scene");
	if (sceneNode)
	{
		gameState_.currentSceneId = sceneNode.attribute("id").as_int();
		gameState_.currentMapPath = sceneNode.attribute("map").as_string();
	}

	// Read player state
	pugi::xml_node playerNode = root.child("player");
	if (playerNode)
	{
		pugi::xml_node posNode = playerNode.child("position");
		if (posNode)
		{
			gameState_.playerPosX = posNode.attribute("x").as_float();
			gameState_.playerPosY = posNode.attribute("y").as_float();
		}

		pugi::xml_node stateNode = playerNode.child("state");
		if (stateNode)
		{
			gameState_.playerSpeed = stateNode.attribute("speed").as_float(4.0f);
			gameState_.playerJumpForce = stateNode.attribute("jumpForce").as_float(2.5f);
			gameState_.playerIsJumping = stateNode.attribute("isJumping").as_bool(false);
		}
	}

	// Read entities (placeholder)
	// pugi::xml_node entitiesNode = root.child("entities");
	// TODO: Parse entity states

	LOG("SaveSystem: Save file loaded from %s", filename.c_str());
	return true;
}
