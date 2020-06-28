#include "SoundSourceComponent.h"

SoundSourceComponent::SoundSourceComponent(GameManager* gameHandle, std::string name, SoundBuffer* buffer, Transform transform):
	Component(gameHandle, name, transform), ALIndex(0), BufferPtr(buffer)
{
	if (buffer)
		GenAL(buffer);
}

void SoundSourceComponent::GenAL(SoundBuffer* buffer)
{
	if (buffer)
		BufferPtr = buffer;
	if (!BufferPtr)
	{
		std::cerr << "ERROR! No AL buffer is assigned to a sound source; Can't generate AL source.\n";
		return;
	}

	alGenSources(1, &ALIndex);
	alSourcefv(ALIndex, AL_POSITION, glm::value_ptr(glm::vec3(0.0f)));
	alSourcefv(ALIndex, AL_VELOCITY, glm::value_ptr(glm::vec3(0.0f)));
	alSourcei(ALIndex, AL_BUFFER, BufferPtr->ALIndex);
}

void SoundSourceComponent::SetLoop(bool loop)
{
	alSourcei(ALIndex, AL_LOOPING, (loop) ? (AL_TRUE) : (AL_FALSE));
}

bool SoundSourceComponent::IsPlaying()
{
	ALint state;
	alGetSourcei(ALIndex, AL_SOURCE_STATE, &state);
	if (state == AL_PLAYING)
		return true;

	return false;
}

void SoundSourceComponent::Play()
{
	if (!BufferPtr)
	{
		std::cerr << "ERROR! No AL buffer is assigned to a sound source; Can't play the sound.\n";
		return;
	}

	if (IsPlaying())
		return;

	std::cerr << "NUTA\n";

	alSourcePlay(ALIndex);
}

void SoundSourceComponent::Pause()
{
	alSourcePause(ALIndex);
}

void SoundSourceComponent::Stop()
{
	alSourceStop(ALIndex);
}

void SoundSourceComponent::Update(float deltaTime)
{
	alSourcefv(ALIndex, AL_POSITION, glm::value_ptr(ComponentTransform.GetWorldTransform().PositionRef));
}

void SoundSourceComponent::Dispose()
{
	if (IsPlaying())
		alSourceStop(ALIndex);

	alDeleteSources(1, &ALIndex);
}