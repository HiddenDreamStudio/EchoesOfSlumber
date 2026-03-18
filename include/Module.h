#pragma once

#include <string>
#include "pugixml.hpp"

class UIElement;

class Module
{
public:

	Module() : active(false)
	{}

	void Init()
	{
		active = true;
	}

	// Called before render is available
	virtual bool Awake()
	{
		return true;
	}

	// Called before the first frame
	virtual bool Start()
	{
		return true;
	}

	// Called each loop iteration
	virtual bool PreUpdate()
	{
		return true;
	}

	// Called each loop iteration
	virtual bool Update(float dt)
	{
		return true;
	}

	// Called each loop iteration
	virtual bool PostUpdate()
	{
		return true;
	}

	// Called before quitting
	virtual bool CleanUp()
	{
		return true;
	}
	
	virtual bool LoadParameters(pugi::xml_node parameters)
	{
		configParameters = parameters;
		return true;
	}

	virtual bool OnUIMouseClickEvent(UIElement* uiElement)
	{
		return true;
	}

public:

	std::string name;
	bool active;
	pugi::xml_node configParameters;

};