#pragma once
#include <vector>
#include <scene/SoundSourceComponent.h>
#include <AL/alc.h>

class AudioEngine: public AudioEngineManager
{
	ALCdevice* Device;
	ALCcontext* Context;
	std::vector <SoundBuffer*> ALBuffers;

	Transform* ListenerTransformPtr;

	GameManager* GameHandle;

private:
	SoundBuffer* LoadALBuffer(ALenum, ALvoid*, ALsizei, ALsizei, std::string = "");
	SoundBuffer* SearchForAlreadyLoadedBuffer(std::string);
public:
	AudioEngine(GameManager*);
	void Init();

	void SetListenerTransformPtr(Transform*);
	Transform* GetListenerTransformPtr();

	void Update();

	virtual SoundBuffer* LoadBufferFromFile(std::string path) override;
	SoundBuffer* LoadBufferFromWav(std::string path);
	SoundBuffer* LoadBufferFromOgg(std::string path);

	virtual void CheckError() override;

	~AudioEngine();
};