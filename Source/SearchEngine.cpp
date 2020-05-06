#pragma once
#include "SearchEngine.h"

SearchEngine::SearchEngine()
{
	RenderEngPtr = nullptr;
	RootComponentPtr = nullptr;
}

void SearchEngine::Setup(RenderEngine* renderEngPtr, Component* rootCompPtr)
{
	RenderEngPtr = renderEngPtr;
	RootComponentPtr = rootCompPtr;
}

ModelComponent* SearchEngine::FindModel(std::string name)
{
	return RenderEngPtr->FindModel(name);
}

Material* SearchEngine::FindMaterial(std::string name)
{
	return RenderEngPtr->FindMaterial(name);
}