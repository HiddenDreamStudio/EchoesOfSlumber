#pragma once

#include "Input.h"
#include "Render.h"

enum class EntityType
{
	PLAYER,
	ITEM,
	ENEMY,
	ENEMY_B,
	ENEMY_C,
	PROJECTILE,
	CHECKPOINT,
	BOX,
	PUSH_ROCK,
	VFX,
	PLATFORM,
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