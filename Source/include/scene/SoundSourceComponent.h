#pragma once
#include <scene/Component.h>
#include <AL/al.h>
namespace GEE
{
	namespace Audio
	{
		struct SoundBuffer
		{
			unsigned int ALIndex;
			std::string Path;	//used for optimization; we don't load a buffer from file if this file was already read before (there will never be two buffers that share the same path)
			bool IsValid() const
			{
				return ALIndex != 0;
			}
		};

		class SoundSourceComponent : public Component
		{
		public:

			SoundSourceComponent(Actor&, Component* parentComp, const std::string& name, SoundBuffer = SoundBuffer(), const Transform& = Transform());
			SoundSourceComponent(Actor&, Component* parentComp, const std::string& name, const std::string& bufferPath, const Transform& = Transform());

			void LoadSound(const SoundBuffer&);

			void SetLoop(bool);
			bool IsPlaying();

			void Play();
			void Pause();
			void Stop();

			virtual void Update(float) override;

			virtual	MaterialInstance GetDebugMatInst(EditorIconState) override;

			virtual void GetEditorDescription(EditorDescriptionBuilder) override;
			template <typename Archive> void Save(Archive& archive) const
			{
				std::string emptyStr = "";
				archive(cereal::make_nvp("Path", SndBuffer.Path), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
			}
			template <typename Archive> void Load(Archive& archive)
			{
				std::cout << "Began loading sound\n";
				std::string path;
				archive(cereal::make_nvp("Path", path), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
				Audio::Loader::LoadSoundFromFile(path, *this);
				std::cout << "Finished loading sound\n";
			}

			void Dispose();
			~SoundSourceComponent();

		private:
			void GenerateAL();
		private:
			unsigned int ALIndex;
			SoundBuffer SndBuffer;

		};

		namespace Loader
		{
			void LoadSoundFromFile(const std::string&, SoundSourceComponent&);
			namespace Impl
			{
				SoundBuffer LoadBufferFromWav(const std::string& path);
				SoundBuffer LoadBufferFromOgg(const std::string& path);

				SoundBuffer LoadALBuffer(ALenum, ALvoid*, ALsizei, ALsizei, const std::string&);
			}
		}
	}
}

CEREAL_REGISTER_TYPE(GEE::Audio::SoundSourceComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GEE::Component, GEE::Audio::SoundSourceComponent)