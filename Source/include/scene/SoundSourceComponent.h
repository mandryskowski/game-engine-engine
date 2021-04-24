#pragma once
#include <scene/Component.h>
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

	SoundSourceComponent(Actor&, Component* parentComp, std::string, SoundBuffer* = nullptr, Transform = Transform());

	void GenAL(SoundBuffer* = nullptr);	//pass a pointer here only if you want to update it in the class

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
		if (BufferPtr)
			archive(cereal::make_nvp("Path", BufferPtr->Path), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
		else
			archive(cereal::make_nvp("Path", emptyStr), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
	}
	template <typename Archive> void Load(Archive& archive)
	{
		std::cout << "Began loading sound\n";
		std::string path;
		archive(cereal::make_nvp("Path", path), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
		GenAL(GameHandle->GetAudioEngineHandle()->LoadBufferFromFile(path));
		std::cout << "Finished loading sound\n";
	}

	void Dispose();
	~SoundSourceComponent();
};