#pragma once
#include "Component.h"
#include <AL/al.h>

struct SoundBuffer
{
	unsigned int ALIndex;
	std::string Path;	//used for optimization; we don't load a buffer from file if this file was already read before (there will never be two buffers that share the same path)
};

class SoundSourceComponent : public Component
{
	unsigned int ALIndex;
	SoundBuffer* BufferPtr;

public:

	SoundSourceComponent(GameManager*, std::string, SoundBuffer* = nullptr, Transform = Transform());
	void GenAL(SoundBuffer* = nullptr);	//pass a pointer here only if you want to update it in the class

	void SetLoop(bool);
	bool IsPlaying();

	void Play();
	void Pause();
	void Stop();

	virtual void Update(float) override;

	void Dispose();

};