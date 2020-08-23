#pragma once
#include <string>

class RenderEngine;
class Actor;
class ModelComponent;
class Material;
class Shader;
class AudioEngine;
class SoundSourceComponent;

class SearchEngine
{
	RenderEngine* RenderEngPtr;
	AudioEngine* AudioEngPtr;
	Actor* RootActorPtr;

public:
	SearchEngine();
	void Setup(RenderEngine*, AudioEngine*, Actor*);

	Actor* FindActor(std::string);
	ModelComponent* FindModel(std::string);
	Material* FindMaterial(std::string);
	Shader* FindShader(std::string);
	SoundSourceComponent* FindSoundSource(std::string);
};