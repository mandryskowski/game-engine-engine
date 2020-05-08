#pragma once
#include "SearchEngine.h"

SearchEngine::SearchEngine()
{
	RenderEngPtr = nullptr;
	AudioEngPtr = nullptr;
	RootComponentPtr = nullptr;
}

void SearchEngine::Setup(RenderEngine* renderEngPtr, AudioEngine* audioEngPtr, Component* rootCompPtr)
{
	RenderEngPtr = renderEngPtr;
	AudioEngPtr = audioEngPtr;
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

SoundSourceComponent* SearchEngine::FindSoundSource(std::string name)
{
	return AudioEngPtr->FindSource(name);
}