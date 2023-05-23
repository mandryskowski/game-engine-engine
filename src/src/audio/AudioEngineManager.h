#pragma once
#include <utility/Utility.h>
namespace GEE
{
	namespace Audio
	{
		class SoundSourceComponent;
		struct SoundBuffer;

		class AudioEngineManager
		{
		public:
			virtual void CheckError() = 0;
			virtual SoundBuffer FindBuffer(const String& path) = 0;
			virtual void AddBuffer(const SoundBuffer&) = 0;

			virtual ~AudioEngineManager() = default;
		};
	}
}