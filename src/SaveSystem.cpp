#include "SaveSystem.h"
#include "Engine.h"
#include "Scene.h"
#include "Player.h"
#include "EntityManager.h"
#include "Map.h"
#include "Physics.h"
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

	// Create save folder if it doesn't exist (use error_code to avoid exceptions)
	std::error_code ec;
	if (!std::filesystem::exists(saveFolderPath_, ec))
	{
		if (!std::filesystem::create_directories(saveFolderPath_, ec) && ec)
		{
			LOG("SaveSystem: Failed to create save folder at %s (error: %s)",
				saveFolderPath_.c_str(), ec.message().c_str());
			return false;
		}
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
		bool saveOk = WriteXML(quickSavePath_);
		if (!saveOk)
		{
			LOG("SaveSystem: Quick save failed for %s", quickSavePath_.c_str());
		}
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
	// Note: filename should be a base name (e.g., "slot1.xml"), not a full path
	std::string fullPath = saveFolderPath_ + filename;
	LOG("SaveSystem: Saving game to %s", fullPath.c_str());

	CollectGameState();
	return WriteXML(fullPath);
}

bool SaveSystem::LoadGame(const std::string& filename)
{
	// Note: filename should be a base name (e.g., "slot1.xml"), not a full path
	std::string fullPath = saveFolderPath_ + filename;
	LOG("SaveSystem: Loading game from %s", fullPath.c_str());

	if (!ReadXML(fullPath))
	{
		LOG("SaveSystem: Failed to read save file %s", fullPath.c_str());
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

bool SaveSystem::QuickSaveAt(float playerPosX, float playerPosY, const std::string& checkpointId)
{
	LOG("SaveSystem: Checkpoint quicksave at %.1f, %.1f (%s)", playerPosX, playerPosY, checkpointId.c_str());
	pendingCheckpointPositionOverride_ = true;
	pendingCheckpointPosX_ = playerPosX;
	pendingCheckpointPosY_ = playerPosY;
	pendingCheckpointId_ = checkpointId;
	gameState_.activeCheckpointId = checkpointId;
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

bool SaveSystem::QuickLoadImmediate()
{
	LOG("SaveSystem: QuickLoadImmediate");

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

	ApplyGameState();
	return true;
}

bool SaveSystem::HasValidSave() const
{
	if (!SaveFileExists(quickSavePath_)) return false;

	pugi::xml_document doc;
	if (!doc.load_file(quickSavePath_.c_str())) return false;

	pugi::xml_node root = doc.child("savegame");
	if (!root) return false;

	pugi::xml_node playerNode = root.child("player");
	if (playerNode)
	{
		pugi::xml_node stateNode = playerNode.child("state");
		if (stateNode)
		{
			int health = stateNode.attribute("health").as_int(0);
			return health > 0;
		}
	}

	return false;
}

bool SaveSystem::HasCheckpointSave() const
{
	if (!gameState_.activeCheckpointId.empty() && gameState_.playerHealth > 0)
		return true;

	if (!SaveFileExists(quickSavePath_)) return false;

	pugi::xml_document doc;
	if (!doc.load_file(quickSavePath_.c_str())) return false;

	pugi::xml_node root = doc.child("savegame");
	if (!root) return false;

	pugi::xml_node sceneNode = root.child("scene");
	std::string checkpointId = sceneNode ? sceneNode.attribute("activeCheckpointId").as_string("") : "";
	if (checkpointId.empty()) return false;

	pugi::xml_node playerNode = root.child("player");
	pugi::xml_node stateNode = playerNode ? playerNode.child("state") : pugi::xml_node();
	return stateNode && stateNode.attribute("health").as_int(0) > 0;
}

bool SaveSystem::SaveFileExists(const std::string& filename) const
{
	// Use error_code overload to avoid exceptions on filesystem errors
	std::error_code ec;
	bool exists = std::filesystem::exists(filename, ec);
	if (ec)
	{
		LOG("SaveSystem: Error checking file existence for %s: %s",
			filename.c_str(), ec.message().c_str());
		return false;
	}
	return exists;
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

	// NOTE: Currently we only persist the player's position.
	// Additional runtime state such as speed, jump force, and jump status
	// should be read from Player (via Scene or another API) once exposed.
	// These are placeholder values until Player exposes getters for them.
	gameState_.playerSpeed = 4.0f;
	gameState_.playerJumpForce = 2.5f;
	gameState_.playerIsJumping = false;
	gameState_.playerHealth = scene->player ? scene->player->health : 3;
	gameState_.playerHasBlanket = scene->player ? scene->player->HasBlanket() : false;
	gameState_.playerHasSlingshot = scene->player ? scene->player->HasSlingshot() : false;

	if (pendingCheckpointPositionOverride_)
	{
		gameState_.playerPosX = pendingCheckpointPosX_;
		gameState_.playerPosY = pendingCheckpointPosY_;
		gameState_.activeCheckpointId = pendingCheckpointId_;
		pendingCheckpointPositionOverride_ = false;
		pendingCheckpointId_.clear();
	}

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
	gameState_.activeCheckpointId = scene->GetActiveCheckpointId();

	// Serialize visited cells: "mapName:cx,cy;cx,cy|mapName2:cx,cy"
	std::string cellsStr;
	for (auto const& [mapName, cells] : scene->visitedCells_) {
		if (!cellsStr.empty()) cellsStr += "|";
		cellsStr += mapName + ":";
		bool first = true;
		for (auto const& [cx, cy] : cells) {
			if (!first) cellsStr += ";";
			cellsStr += std::to_string(cx) + "," + std::to_string(cy);
			first = false;
		}
	}
	gameState_.visitedRoomsStr = cellsStr;

	// Serialize map purchase flags: "mapName1|mapName2"
	std::string purchasedStr;
	for (auto const& [mapName, revealed] : scene->mapRevealed_) {
		if (revealed) {
			if (!purchasedStr.empty()) purchasedStr += "|";
			purchasedStr += mapName;
		}
	}
	gameState_.mapPurchasedStr = purchasedStr;

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

	// Set player position and health
	Vector2D newPos(gameState_.playerPosX, gameState_.playerPosY);
	scene->SetPlayerPosition(newPos);

	// Reset Game Over state and snap camera to player
	scene->SetGameOverVisible(false);
	Engine::GetInstance().render->SetCameraPosition(newPos.getX(), newPos.getY());

	if (scene->player) {
		scene->player->position = Vector2D(
			gameState_.playerPosX + (float)scene->player->texW * 0.5f,
			gameState_.playerPosY + (float)scene->player->texH * 0.5f);
		scene->player->health = gameState_.playerHealth;
		scene->player->SetHasBlanket(gameState_.playerHasBlanket);
		scene->player->SetHasSlingshot(gameState_.playerHasSlingshot);
		scene->player->Revive();
		scene->ResetHealthUI(scene->player->health);
	}

	scene->capaCollected_ = gameState_.playerHasBlanket;
	scene->slingshotCollected_ = gameState_.playerHasSlingshot;

	LOG("SaveSystem: Player position restored (%.1f, %.1f)",
		gameState_.playerPosX, gameState_.playerPosY);
}

void SaveSystem::ApplySceneState()
{
	auto& scene = Engine::GetInstance().scene;
	auto& map = Engine::GetInstance().map;

	scene->visitedCells_.clear();
	scene->mapRevealed_.clear();
	scene->SetActiveCheckpointId(gameState_.activeCheckpointId);

	if (!gameState_.currentMapPath.empty())
	{
		size_t slash = gameState_.currentMapPath.find_last_of("/\\");
		std::string savedPath = (slash == std::string::npos) ? "" : gameState_.currentMapPath.substr(0, slash + 1);
		std::string savedFile = (slash == std::string::npos) ? gameState_.currentMapPath : gameState_.currentMapPath.substr(slash + 1);
		if (savedPath.empty()) savedPath = "assets/maps/";

		scene->player.reset();
		Engine::GetInstance().entityManager->CleanUp();
		Engine::GetInstance().physics->FlushPendingDeletes();
		map->CleanUp();
		Engine::GetInstance().physics->FlushPendingDeletes();

		// Prevent Box2D crashes by clearing leftover physical references
		if (scene->puzzleManager2) {
			scene->puzzleManager2->Reset();
		}
		scene->activeBoss_.reset();

		scene->currentMapFile_ = savedFile;
		for (int i = 0; i < (int)scene->levels_.size(); ++i)
		{
			if (scene->levels_[i].file == savedFile)
			{
				scene->currentLevelIndex_ = i;
				break;
			}
		}

		map->Load(savedPath, savedFile);
		map->LoadEntities(scene->player);

		if (scene->player == nullptr)
		{
			scene->player = std::dynamic_pointer_cast<Player>(
				Engine::GetInstance().entityManager->CreateEntity(EntityType::PLAYER));
			scene->player->position = Vector2D(gameState_.playerPosX, gameState_.playerPosY);
			scene->player->Start();
		}

		LOG("SaveSystem: Loaded saved map %s before applying player state", gameState_.currentMapPath.c_str());
	}

	// Deserialize visited cells
	std::string cellsStr = gameState_.visitedRoomsStr;
	size_t pos = 0;
	while (pos < cellsStr.size()) {
		size_t pipe = cellsStr.find('|', pos);
		std::string token = (pipe == std::string::npos) ? cellsStr.substr(pos) : cellsStr.substr(pos, pipe - pos);
		if (!token.empty()) {
			size_t colon = token.find(':');
			if (colon != std::string::npos) {
				std::string mapName = token.substr(0, colon);
				std::string coordsStr = token.substr(colon + 1);
				size_t cpos = 0;
				while (cpos < coordsStr.size()) {
					size_t semi = coordsStr.find(';', cpos);
					std::string coordToken = (semi == std::string::npos) ? coordsStr.substr(cpos) : coordsStr.substr(cpos, semi - cpos);
					if (!coordToken.empty()) {
						size_t comma = coordToken.find(',');
						if (comma != std::string::npos) {
							int cx = std::stoi(coordToken.substr(0, comma));
							int cy = std::stoi(coordToken.substr(comma + 1));
							scene->visitedCells_[mapName].insert({ cx, cy });
						}
					}
					if (semi == std::string::npos) break;
					cpos = semi + 1;
				}
			}
		}
		if (pipe == std::string::npos) break;
		pos = pipe + 1;
	}

	// Deserialize map reveals
	std::string purchasedStr = gameState_.mapPurchasedStr;
	pos = 0;
	while (pos < purchasedStr.size()) {
		size_t pipe = purchasedStr.find('|', pos);
		std::string token = (pipe == std::string::npos) ? purchasedStr.substr(pos) : purchasedStr.substr(pos, pipe - pos);
		if (!token.empty()) {
			scene->mapRevealed_[token] = true;
		}
		if (pipe == std::string::npos) break;
		pos = pipe + 1;
	}

	scene->SyncCheckpointEntities();
	LOG("SaveSystem: Scene state restored (saved scene=%d)", gameState_.currentSceneId);
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
	sceneNode.append_attribute("activeCheckpointId") = gameState_.activeCheckpointId.c_str();
	sceneNode.append_attribute("visitedRooms") = gameState_.visitedRoomsStr.c_str();
	sceneNode.append_attribute("mapPurchased") = gameState_.mapPurchasedStr.c_str();

	// Player state
	pugi::xml_node playerNode = root.append_child("player");

	pugi::xml_node posNode = playerNode.append_child("position");
	posNode.append_attribute("x") = gameState_.playerPosX;
	posNode.append_attribute("y") = gameState_.playerPosY;

	pugi::xml_node stateNode = playerNode.append_child("state");
	stateNode.append_attribute("speed") = gameState_.playerSpeed;
	stateNode.append_attribute("jumpForce") = gameState_.playerJumpForce;
	stateNode.append_attribute("isJumping") = gameState_.playerIsJumping;
	stateNode.append_attribute("health") = gameState_.playerHealth;
	stateNode.append_attribute("hasBlanket") = gameState_.playerHasBlanket;
	stateNode.append_attribute("hasSlingshot") = gameState_.playerHasSlingshot;

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
		gameState_.activeCheckpointId = sceneNode.attribute("activeCheckpointId").as_string("");
		gameState_.visitedRoomsStr = sceneNode.attribute("visitedRooms").as_string("");
		gameState_.mapPurchasedStr = sceneNode.attribute("mapPurchased").as_string("");
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
			gameState_.playerHealth = stateNode.attribute("health").as_int(3);
			gameState_.playerHasBlanket = stateNode.attribute("hasBlanket").as_bool(false);
			gameState_.playerHasSlingshot = stateNode.attribute("hasSlingshot").as_bool(false);
		}
	}

	// Read entities (placeholder)
	// pugi::xml_node entitiesNode = root.child("entities");
	// TODO: Parse entity states

	LOG("SaveSystem: Save file loaded from %s", filename.c_str());
	return true;
}