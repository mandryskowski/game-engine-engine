#pragma once
#include "SearchEngine.h"
#include "RenderEngine.h"
#include "AudioEngine.h"
#include "Actor.h"

SearchEngine::SearchEngine()
{
	RenderEngPtr = nullptr;
	AudioEngPtr = nullptr;
	RootActorPtr = nullptr;
}

void SearchEngine::Setup(RenderEngine* renderEngPtr, AudioEngine* audioEngPtr, Actor* rootActorPtr)
{
	RenderEngPtr = renderEngPtr;
	AudioEngPtr = audioEngPtr;
	RootActorPtr = rootActorPtr;
}

Actor* SearchEngine::FindActor(std::string name)
{
	return RootActorPtr->SearchForActor(name);
}

ModelComponent* SearchEngine::FindModel(std::string name)
{
	return RenderEngPtr->FindModel(name);
}

Material* SearchEngine::FindMaterial(std::string name)
{
	return RenderEngPtr->FindMaterial(name);
}

Shader* SearchEngine::FindShader(std::string name)
{
	return RenderEngPtr->FindShader(name);
}

SoundSourceComponent* SearchEngine::FindSoundSource(std::string name)
{
	return AudioEngPtr->FindSource(name);
}