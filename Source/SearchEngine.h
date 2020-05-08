#pragma once
#include "RenderEngine.h"
#include "AudioEngine.h"

class SearchEngine
{
	RenderEngine* RenderEngPtr;
	AudioEngine* AudioEngPtr;
	Component* RootComponentPtr;

public:
	SearchEngine();
	void Setup(RenderEngine*, AudioEngine*, Component*);
	ModelComponent* FindModel(std::string);
	Material* FindMaterial(std::string);
	SoundSourceComponent* FindSoundSource(std::string);
};