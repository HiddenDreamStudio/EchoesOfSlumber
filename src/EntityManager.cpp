#include "EntityManager.h"
#include "Player.h"
#include "Engine.h"
#include "Textures.h"
#include "Scene.h"
#include "Log.h"
#include "Item.h"
#include "EnemyCarmel.h"
#include "EnemyB.h"
#include "EnemyC.h"
#include "Projectile.h"
#include "Checkpoint.h"
#include "Box.h"
#include "PushRock.h"
#include "VFX.h"
#include "SlingshotProjectile.h"
#include "DropDoll.h"
#include "Boss2.h"
#include "Boss1.h"
#include "RopedRock.h"
#include "tracy/Tracy.hpp"
#include "Platform.h"
#include "Door.h"

EntityManager::EntityManager() : Module()
{
	name = "entitymanager";
}

// Destructor
EntityManager::~EntityManager()
{}

// Called before render is available
bool EntityManager::Awake()
{
	LOG("Loading Entity Manager");
	bool ret = true;

	//Iterates over the entities and calls the Awake
	for(const auto entity : entities)
	{
		if (entity->active == false) continue;
		ret = entity->Awake();
	}

	return ret;

}

bool EntityManager::Start() {

	bool ret = true; 

	//Iterates over the entities and calls Start
	for(const auto entity : entities)
	{
		if (entity->active == false) continue;
		ret = entity->Start();
	}

	return ret;
}

// Called before quitting
bool EntityManager::CleanUp()
{
	bool ret = true;

	for (const auto entity : entities)
	{
		ret = entity->CleanUp();
	}
	entities.clear();

	return ret;
}

std::shared_ptr<Entity> EntityManager::CreateEntity(EntityType type)
{
	std::shared_ptr<Entity> entity = std::make_shared<Entity>();
	switch (type)
	{
	case EntityType::PLAYER:
		entity = std::make_shared<Player>();
		break;
	case EntityType::ITEM:
		entity = std::make_shared<Item>();
		break;
	case EntityType::ENEMY:
		entity = std::make_shared<EnemyCarmel>();
		break;
	case EntityType::ENEMY_B:
		entity = std::make_shared<EnemyB>();
		break;
	case EntityType::ENEMY_C:
		entity = std::make_shared<EnemyC>();
		break;
	case EntityType::PROJECTILE:
		entity = std::make_shared<Projectile>();
		break;
	case EntityType::CHECKPOINT:
		entity = std::make_shared<Checkpoint>();
		break;
	case EntityType::BOX:
		entity = std::make_shared<Box>();
		break;
	case EntityType::PLATFORM:
		entity = std::make_shared<Platform>();
		break;
	case EntityType::PUSH_ROCK:
		entity = std::make_shared<PushRock>();
		break;
	case EntityType::VFX:
		entity = std::make_shared<VFX>();
		break;
	case EntityType::DOOR:
		entity = std::make_shared<Door>();
		break;
	case EntityType::SLINGSHOT_PROJECTILE:
		entity = std::make_shared<SlingshotProjectile>();
		break;
	case EntityType::DROP_DOLL:
		entity = std::make_shared<DropDoll>();
		break;
	case EntityType::BOSS_2:
		entity = std::make_shared<Boss2>();
		break;
	case EntityType::BOSS_1:
		entity = std::make_shared<Boss1>();
		break;
	case EntityType::ROPE_ROCK:
		entity = std::make_shared<RopedRock>();
		break;
	default:
		break;
	}

	entities.push_back(entity);

	return entity;
}

void EntityManager::DestroyEntity(std::shared_ptr<Entity> entity)
{
	entity->CleanUp();
	entities.remove(entity);
}

void EntityManager::AddEntity(std::shared_ptr<Entity> entity)
{
	if ( entity != nullptr) entities.push_back(entity);
}

std::shared_ptr<Entity> EntityManager::SpawnVFX(Vector2D pos, const char* path, int frames, int w, int h, float speed, float angle, float scale)
{
	auto vfx = std::dynamic_pointer_cast<VFX>(CreateEntity(EntityType::VFX));
	vfx->position = pos;
	vfx->SetTexture(path, frames, w, h, speed, angle, scale);
	return vfx;
}

bool EntityManager::Update(float dt)
{
	ZoneScoped;

	bool ret = true;

	//List to store entities pending deletion
	std::list<std::shared_ptr<Entity>> pendingDelete;
	
	//Iterates over the entities and calls Update
	for(const auto entity : entities)
	{
		//If the entity is marked for deletion, add it to the pendingDelete list
		if (entity->pendingToDelete)
		{
			pendingDelete.push_back(entity);
		}
		//If the entity is not active, skip it
		if (entity->active == false) continue;
		ret = entity->Update(dt);
	}

	//Now iterates over the pendingDelete list and destroys the entities
	for (const auto entity : pendingDelete)
	{
		DestroyEntity(entity);
	}

	return ret;
}

bool EntityManager::PostUpdate() {
	ZoneScoped;
	bool ret = true;

	for (const auto entity : entities)
	{
		if (entity->active == false) continue;
		ret = entity->PostUpdate();
		if (!ret) break;
	}

	return ret;
}