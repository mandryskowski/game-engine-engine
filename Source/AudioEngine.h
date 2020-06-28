#pragma once
#include <vector>
#include "SoundSourceComponent.h"
#include <AL/alc.h>
#include "AudioFile.h"

class AudioEngine: public AudioEngineManager
{
	ALCdevice* Device;
	ALCcontext* Context;
	std::vector <SoundBuffer*> ALBuffers;

	std::vector <SoundSourceComponent*> Sources;
	Transform* ListenerTransformPtr;

	GameManager* GameHandle;

private:
	SoundBuffer* LoadALBuffer(ALenum, ALvoid*, ALsizei, ALsizei, std::string = "");
	SoundBuffer* SearchForAlreadyLoadedBuffer(std::string);
public:
	AudioEngine(GameManager*);
	void Init();

	void SetListenerTransformPtr(Transform*);
	virtual SoundSourceComponent* AddSource(SoundSourceComponent*) override;
	virtual SoundSourceComponent* AddSource(std::string path, std::string name) override;
	SoundSourceComponent* FindSource(std::string name);

	void Update();

	SoundBuffer* LoadBuffer(std::string path);
	SoundBuffer* LoadBufferFromWav(std::string path);
	SoundBuffer* LoadBufferFromOgg(std::string path);

	~AudioEngine();
};