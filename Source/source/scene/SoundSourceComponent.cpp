#include <scene/SoundSourceComponent.h>
#include <rendering/Material.h>
#include <iostream>
#include <math/Transform.h>
#include <audio/AudioFile.h>

#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIButtonActor.h>

namespace GEE
{
	namespace Audio
	{
		SoundSourceComponent::SoundSourceComponent(Actor& actor, Component* parentComp, const std::string& name, SoundBuffer sndBuffer, const Transform& transform) :
			Component(actor, parentComp, name, transform), ALIndex(0), SndBuffer(sndBuffer)
		{
			GenerateAL();
			LoadSound(sndBuffer);
		}

		SoundSourceComponent::SoundSourceComponent(Actor& actor, Component* parentComp, const std::string& name, const std::string& bufferPath, const Transform& transform):
			SoundSourceComponent(actor, parentComp, name, SoundBuffer(), transform)
		{
			Audio::Loader::LoadSoundFromFile(bufferPath, *this);
		}

		void SoundSourceComponent::LoadSound(const SoundBuffer& buffer)
		{
			SndBuffer = buffer;

			if (!SndBuffer.IsValid())
				return;

			alSourcei(ALIndex, AL_BUFFER, SndBuffer.ALIndex);
		}

		void SoundSourceComponent::SetLoop(bool loop)
		{
			if (ALIndex != 0)
				alSourcei(ALIndex, AL_LOOPING, (loop) ? (AL_TRUE) : (AL_FALSE));
		}

		bool SoundSourceComponent::IsPlaying()
		{
			if (ALIndex == 0)
				return false;

			ALint state;
			alGetSourcei(ALIndex, AL_SOURCE_STATE, &state);
			if (state == AL_PLAYING)
				return true;

			return false;
		}

		void SoundSourceComponent::Play()
		{
			if (!SndBuffer.IsValid())
			{
				std::cerr << "ERROR! No valid AL buffer is assigned to a sound source; Can't play the sound.\n";
				return;
			}

			if (IsPlaying())
				return;

			std::cout << "Gram dzwiek " << Name << '\n';
			GameHandle->GetAudioEngineHandle()->CheckError();

			alSourcePlay(ALIndex);
		}

		void SoundSourceComponent::Pause()
		{
			if (ALIndex != 0)
				alSourcePause(ALIndex);
		}

		void SoundSourceComponent::Stop()
		{
			if (ALIndex != 0)
				alSourceStop(ALIndex);
		}

		void SoundSourceComponent::Update(float deltaTime)
		{
			if (ALIndex != 0)
				alSourcefv(ALIndex, AL_POSITION, Math::GetDataPtr(ComponentTransform.GetWorldTransform().GetPos()));
		}

		MaterialInstance SoundSourceComponent::LoadDebugMatInst(EditorIconState state)
		{
			LoadDebugRenderMaterial("GEE_Mat_Default_Debug_SoundSourceComponent", "EditorAssets/soundsourcecomponent_debug.png");
			return Component::LoadDebugMatInst(state);
		}

		void SoundSourceComponent::GetEditorDescription(EditorDescriptionBuilder descBuilder)
		{
			Component::GetEditorDescription(descBuilder);

			descBuilder.AddField("Path").GetTemplates().PathInput([this](const std::string& path) {GameHandle->GetAudioEngineHandle()->CheckError(); Audio::Loader::LoadSoundFromFile(path, *this); GameHandle->GetAudioEngineHandle()->CheckError(); }, [this]()->std::string { return SndBuffer.Path;; }, { "*.wav" });
			descBuilder.AddField("Play").CreateChild<UIButtonActor>("PlayButton", "Play", [this]() { Play(); });
		}

		void SoundSourceComponent::Dispose()
		{
			if (IsPlaying())
				alSourceStop(ALIndex);

			if (ALIndex != 0)
				alDeleteSources(1, &ALIndex);
		}

		SoundSourceComponent::~SoundSourceComponent()
		{
			Dispose();
		}

		void SoundSourceComponent::GenerateAL()
		{
			if (ALIndex != 0)
				alDeleteSources(1, &ALIndex);

			alGenSources(1, &ALIndex);
			alSourcefv(ALIndex, AL_POSITION, Math::GetDataPtr(Vec3f(0.0f)));
			alSourcefv(ALIndex, AL_VELOCITY, Math::GetDataPtr(Vec3f(0.0f)));
			alSourcei(ALIndex, AL_BUFFER, SndBuffer.ALIndex);
		}

		namespace Loader
		{
			void LoadSoundFromFile(const std::string& path, SoundSourceComponent& soundComp)
			{
				////////////////// Check our current buffers; if one was loaded from the same path as passed to this function just return its address and don't waste time
				SoundBuffer buffer = soundComp.GetScene().GetGameHandle()->GetAudioEngineHandle()->FindBuffer(path);
				if (buffer.IsValid())
				{
					soundComp.LoadSound(buffer);
					return;
				}


				////////////////// If we have to load, call the appropriate method based on path's file format
				std::string format = path.substr(path.find('.') + 1);

				if (format == "wav")
					buffer = Impl::LoadBufferFromWav(path);
				else if (format == "ogg")
					buffer = Impl::LoadBufferFromOgg(path);
				else
				{
					std::cerr << "ERROR! Unrecognized audio format " << format << " of file " << path << '\n';
					return;
				}

				if (!buffer.IsValid())
					return;

				soundComp.GetScene().GetGameHandle()->GetAudioEngineHandle()->AddBuffer(buffer);

				soundComp.LoadSound(buffer);
			}

			namespace Impl
			{
				SoundBuffer LoadBufferFromWav(const std::string& path)
				{
					///////////////// Load the file
					AudioFile<float> file;

					if (!file.load(path))
					{
						std::cerr << "ERROR! Can't load WAV file " << path << "!\n";
						return SoundBuffer();
					}

					///////////////// Assign a specific OpenAL format (and give a warning if the loaded type is stereo)
					bool stereo = file.isStereo();
					ALenum format = AL_FORMAT_MONO8;

					if (stereo)
						std::cerr << "INFO: Audio file " << path << " is stereo - it won't work with a 3D sound source.\n";

					if (file.getBitDepth() == 8)
						format = (stereo) ? (AL_FORMAT_STEREO8) : (AL_FORMAT_MONO8);
					else if (file.getBitDepth() == 16)
						format = (stereo) ? (AL_FORMAT_STEREO16) : (AL_FORMAT_MONO16);
					else
					{
						std::cerr << "ERROR! Unsupported bit depth of " << path << ". The bit depth count is: " << file.getBitDepth() << ".\n";
						return SoundBuffer();
					}

					//////////////// Create an OpenAL buffer using our loaded data.
					std::vector <char> charSamples;
					charSamples.reserve(file.samples[0].size());

					std::transform(file.samples[0].begin(), file.samples[0].end(), std::back_inserter(charSamples), [path](const float& sample) { return static_cast<char>(sample * 128.0f); });

					std::cout << path << ": " << charSamples.size() << ",  samplerate: " << file.getSampleRate() / 2 << ",  bitdepth: " << file.getBitDepth() << ",  length[s]: " << file.getLengthInSeconds() << '\n';
					return LoadALBuffer(format, &charSamples[0], (ALsizei)charSamples.size(), (ALsizei)file.getSampleRate() / 2, path);
				}

				SoundBuffer LoadBufferFromOgg(const std::string& path)
				{
					std::cerr << "Ogg Vorbis format is not currently supported.\n";
					return SoundBuffer();
				}

				SoundBuffer LoadALBuffer(ALenum format, ALvoid* data, ALsizei size, ALsizei freq, const std::string& path)
				{
					SoundBuffer buffer;

					alGenBuffers(1, &buffer.ALIndex);
					alBufferData(buffer.ALIndex, format, data, size - size % 4, freq);

					buffer.Path = path;

					return buffer;
				}
			}
		}
	}
}