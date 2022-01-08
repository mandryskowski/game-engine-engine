#include <audio/AudioEngine.h>

namespace GEE
{
	namespace Audio
	{
		AudioEngine::AudioEngine(GameManager* gameHandle) :
			Device(nullptr), Context(nullptr), ListenerTransformPtr(nullptr), GameHandle(gameHandle)
		{
			Init();
		}

		void AudioEngine::Init()
		{
			std::cout << "Opening audio device.\n";
			Device = alcOpenDevice(nullptr);
		
			if (!Device)
			{
				std::cerr << "ERROR! Couldn't configure audio device.\n";
				return;
			}

			Context = alcCreateContext(Device, nullptr);
			if (!Context)
			{
				std::cerr << "ERROR! Couldn't create an OpenAL context.\n";
				return;
			}

			ALCboolean current = alcMakeContextCurrent(Context);
			if (current != ALC_TRUE)
			{
				std::cerr << "ERROR! Couldn't make an OpenAL context current.\n";
				return;
			}
			std::cout << "Succesfully initialised audio engine\n";

			alListenerfv(AL_POSITION, Math::GetDataPtr(Vec3f(0.0f)));
			alListenerfv(AL_VELOCITY, Math::GetDataPtr(Vec3f(0.0f)));
		}


		///////////////////////////////////////////
		//////////////////PUBLIC///////////////////
		///////////////////////////////////////////

		void AudioEngine::SetListenerTransformPtr(Transform* transformPtr)
		{
			ListenerTransformPtr = transformPtr;
		}

		Transform* AudioEngine::GetListenerTransformPtr()
		{
			return ListenerTransformPtr;
		}

		void AudioEngine::Update()
		{
			////////////////// Update listener position & orientation
			if (!ListenerTransformPtr)
				return;

			alListenerfv(AL_POSITION, Math::GetDataPtr(ListenerTransformPtr->GetWorldTransform().GetPos()));

			Vec3f orientationVecs[2] = { ListenerTransformPtr->GetWorldTransform().GetFrontVec(), Vec3f(0.0f, 1.0f, 0.0f) };
			alListenerfv(AL_ORIENTATION, &orientationVecs[0].x);
		}

		void AudioEngine::AddBuffer(const SoundBuffer& buffer)
		{
			ALBuffers.push_back(buffer);
		}

		SoundBuffer AudioEngine::FindBuffer(const std::string& path)
		{
			auto found = std::find_if(ALBuffers.begin(), ALBuffers.end(), [&](const SoundBuffer& buffer) { return path == buffer.Path; });
			if (found == ALBuffers.end())
				return SoundBuffer();

			return *found;
		}

		void AudioEngine::CheckError()
		{
			ALenum error = alGetError();
			std::cout << "AL error check...\n";
			if (error == AL_INVALID_NAME)
				std::cout << "Invalid name\n";
			else if (error == AL_INVALID_ENUM)
				std::cout << "Invalid enum\n";
			else if (error == AL_INVALID_VALUE)
				std::cout << "Invalid value\n";
			else if (error == AL_INVALID_OPERATION)
				std::cout << "Invalid operation\n";
			else if (error == AL_OUT_OF_MEMORY)
				std::cout << "Out of memory\n";
			else if (error == AL_NO_ERROR)
				std::cout << "No error\n";
			else
				std::cout << "Unknown error\n";
		}

		AudioEngine::~AudioEngine()
		{
			if (Device)
				alcCloseDevice(Device);

			if (Context)
				alcDestroyContext(Context);
		}
	}
}