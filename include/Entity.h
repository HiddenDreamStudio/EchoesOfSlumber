#pragma once

#include "Input.h"
#include "Render.h"
#include <memory>
#include <string>

enum class EntityType
{
	PLAYER,
	ITEM,
	ENEMY,
	ENEMY_B,
	ENEMY_C,
	ENEMY_PLUSH,
	ENEMY_STITCHLING,
	BOUNCER,
	PROJECTILE,
	CHECKPOINT,
	BOX,
	PLATFORM,
	PUSH_ROCK,
	VFX,
	DOOR,
	LEVER,
	SLINGSHOT_PROJECTILE,
	DROP_DOLL,
	BOSS_2,
	BOSS_1,
	ROPE_ROCK,
	ENEMY_WINDUP_SCURRY,
	UNKNOWN
};

class PhysBody;

class Entity : public std::enable_shared_from_this<Entity>
{
public:
	Entity() : type(EntityType::UNKNOWN) {}
	Entity(EntityType type) : type(type), active(true) {}

	virtual bool Awake()
	{
		return true;
	}

	virtual bool Start()
	{
		return true;
	}

	virtual bool Update(float dt)
	{
		return true;
	}

	virtual bool PostUpdate()
	{
		return true;
	}

	virtual bool CleanUp()
	{
		return true;
	}

	virtual bool Destroy()
	{
		return true;
	}

	void Enable()
	{
		if (!active)
		{
			active = true;
			Start();
		}
	}

	void Disable()
	{
		if (active)
		{
			active = false;
			CleanUp();
		}
	}

	virtual void OnCollision(PhysBody* physA, PhysBody* physB) {

	};

	virtual void OnCollisionEnd(PhysBody* physA, PhysBody* physB) {

	};

public:

	std::string name;
	EntityType type;
	bool active = true;
	bool pendingToDelete = false;

	// Possible properties, it depends on how generic we
	// want our Entity class, maybe it's not renderable...
	Vector2D position;
	bool renderable = true;

	// Health system
	int health = 3;
	int maxHealth = 3;

	virtual void TakeDamage(int damage) { health -= damage; }
};
