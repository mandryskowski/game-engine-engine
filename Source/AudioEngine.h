#pragma once
#include <vector>
#include "SoundSourceComponent.h"
#include <AL/alc.h>
#include "AudioFile.h"

class AudioEngine
{
	ALCdevice* Device;
	ALCcontext* Context;
	std::vector <SoundBuffer*> ALBuffers;

	std::vector <SoundSourceComponent*> Sources;
	Transform* ListenerTransformPtr;

private:
	SoundBuffer* LoadALBuffer(ALenum, ALvoid*, ALsizei, ALsizei, std::string = "");
	SoundBuffer* SearchForAlreadyLoadedBuffer(std::string);
public:
	AudioEngine();
	void Init();

	void SetListenerTransformPtr(Transform*);
	SoundSourceComponent* AddSource(SoundSourceComponent*);
	SoundSourceComponent* AddSource(std::string path, std::string name);

	void Update();

	SoundBuffer* LoadBuffer(std::string path);
	SoundBuffer* LoadBufferFromWav(std::string path);
	SoundBuffer* LoadBufferFromOgg(std::string path);

	~AudioEngine();
};