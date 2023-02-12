#pragma once
#include <vector>
#include <scene/SoundSourceComponent.h>
#include <AL/alc.h>

namespace GEE
{
	namespace Audio
	{
		class AudioEngine : public AudioEngineManager
		{
			ALCdevice* Device;
			ALCcontext* Context;
			std::vector <SoundBuffer> ALBuffers;

			Transform* ListenerTransformPtr;

			GameManager* GameHandle;

		public:
			AudioEngine(GameManager*);
			void Init();

			void SetListenerTransformPtr(Transform*);
			Transform* GetListenerTransformPtr();

			void Update();

			virtual void AddBuffer(const SoundBuffer&) override;
			virtual SoundBuffer FindBuffer(const std::string&) override;

			virtual void CheckError() override;

			virtual ~AudioEngine() override;
		};
	}
}