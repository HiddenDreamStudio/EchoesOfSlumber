#include "Door.h"
#include "Engine.h"    
#include "Textures.h"
#include "Physics.h"
#include "Render.h"
#include "Log.h"

Door::Door() : Entity(EntityType::DOOR)
{
	name = "door";
	pbody = nullptr;
	texture = nullptr;
}

Door::~Door() {}

bool Door::Awake()
{
	return true;
}

bool Door::Start()
{
	texturePath = "assets/textures/door.png";
	
	texW = 32; 
	texH = 64;

	return true;
}

bool Door::Update(float dt)
{
	if (!isOpen)
	{

	}
	else 
	{

	}

	return true;
}

bool Door::CleanUp()
{
	return true;
}

void Door::Open()
{
	if (!isOpen)
	{
		isOpen = true;
	}
}