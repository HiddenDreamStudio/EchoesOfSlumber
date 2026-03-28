#pragma once

#include "Module.h"
#include <string>
#include <memory>

// Forward declarations
class Player;
class Entity;

// Game state structure for serialization
struct GameState
{
	// Scene state
	int currentSceneId = 0;
	std::string currentMapPath;

	// Player state
	float playerPosX = 0.0f;
	float playerPosY = 0.0f;
	float playerSpeed = 4.0f;
	float playerJumpForce = 2.5f;
	bool playerIsJumping = false;

	// Timestamp
	long long saveTimestamp = 0;
};

class SaveSystem : public Module
{
public:
	SaveSystem();
	virtual ~SaveSystem();

	// Module lifecycle
	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	// Save/Load API
	bool SaveGame(const std::string& filename);
	bool LoadGame(const std::string& filename);
	bool QuickSave();
	bool QuickLoad();

	// Check if save file exists
	bool SaveFileExists(const std::string& filename) const;

	// Get/Set current game state
	GameState& GetGameState() { return gameState_; }
	void SetGameState(const GameState& state) { gameState_ = state; }

	// Collect current game state from all modules
	void CollectGameState();

	// Apply loaded game state to all modules
	void ApplyGameState();

private:
	// Internal helpers
	bool WriteXML(const std::string& filename);
	bool ReadXML(const std::string& filename);

	// Collect state from individual sources
	void CollectPlayerState();
	void CollectSceneState();
	void CollectEntityStates();

	// Apply state to individual targets
	void ApplyPlayerState();
	void ApplySceneState();
	void ApplyEntityStates();

private:
	GameState gameState_;
	std::string quickSavePath_ = "assets/saves/savedata.xml";
	std::string saveFolderPath_ = "assets/saves/";
	bool pendingLoad_ = false;
	bool pendingSave_ = false;
};
