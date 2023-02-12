#pragma once
#include <scene/Component.h>
#include <AL/al.h>
namespace GEE
{
	namespace Audio
	{
		struct SoundBuffer
		{
			bool IsValid() const
			{
				return ALIndex != 0;
			}
	//	private:
			unsigned int ALIndex;
			String Path;	//used for optimization; we don't load a buffer from file if this file was already read before (there will never be two buffers that share the same path)
			Time Duration;
		};

		class SoundSourceComponent : public Component
		{
		public:

			SoundSourceComponent(Actor&, Component* parentComp, const std::string& name, SoundBuffer = SoundBuffer(), const Transform& = Transform());
			SoundSourceComponent(Actor&, Component* parentComp, const std::string& name, const std::string& bufferPath, const Transform& = Transform());

			SoundBuffer GetCurrentSoundBuffer() const;
			void LoadSound(const SoundBuffer&);

			void SetLoop(bool);
			bool IsPlaying();

			void Play();
			void Pause();
			void Stop();

			void Update(Time dt) override;

			MaterialInstance GetDebugMatInst(ButtonMaterialType) override;

			void GetEditorDescription(ComponentDescriptionBuilder) override;
			template <typename Archive> void Save(Archive& archive) const;
			template <typename Archive> void Load(Archive& archive);

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
		template<typename Archive>
		void SoundSourceComponent::Save(Archive& archive) const
		{
			std::string emptyStr = "";
			archive(cereal::make_nvp("Path", SndBuffer.Path), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
		}
		template<typename Archive>
		void SoundSourceComponent::Load(Archive& archive)
		{
			std::cout << "Began loading sound\n";
			std::string path;
			archive(cereal::make_nvp("Path", path), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
			Audio::Loader::LoadSoundFromFile(path, *this);
			std::cout << "Finished loading sound\n";
		}
	}
}

GEE_POLYMORPHIC_SERIALIZABLE_COMPONENT(GEE::Component, GEE::Audio::SoundSourceComponent)