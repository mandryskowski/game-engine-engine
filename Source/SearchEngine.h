#pragma once
#include "RenderEngine.h"

class SearchEngine
{
	RenderEngine* RenderEngPtr;
	Component* RootComponentPtr;

public:
	SearchEngine();
	void Setup(RenderEngine*, Component*);
	ModelComponent* FindModel(std::string);
	Material* FindMaterial(std::string);
};