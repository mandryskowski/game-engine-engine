#pragma once
#include <scene/Component.h>
#include <rendering/Texture.h>
#include <rendering/LightProbe.h>

namespace GEE
{
	class LightProbeComponent : public Component
	{
	public:
		LightProbeComponent(Actor&, Component* parentComp, const std::string& name, EngineBasicShape = EngineBasicShape::CUBE);
		LightProbeComponent(LightProbeComponent&&);
		EngineBasicShape GetShape() const;
		Texture GetEnvironmentMap() const;
		unsigned int GetProbeIndex() const;
		float GetProbeIntensity() const;
		Shader* GetRenderShader(const RenderToolboxCollection& renderCol) const;

		bool IsGlobalProbe() const;

		void SetProbeIndex(unsigned int);

		virtual	MaterialInstance GetDebugMatInst(EditorIconState) override;
		virtual void GetEditorDescription(EditorDescriptionBuilder) override;
		template <typename Archive> void Save(Archive& archive) const
		{
			archive(CEREAL_NVP(Shape), CEREAL_NVP(OptionalFilepath), CEREAL_NVP(ProbeIntensity), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			archive(CEREAL_NVP(Shape), CEREAL_NVP(OptionalFilepath), CEREAL_NVP(ProbeIntensity), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
			if (!OptionalFilepath.empty())
				LightProbeLoader::LoadLightProbeFromFile(*this, OptionalFilepath);
		}
		~LightProbeComponent();
	private:
		friend class LightProbeLoader;
		EngineBasicShape Shape;
		unsigned int ProbeIndex;
		Texture EnvironmentMap;
		std::string OptionalFilepath;

		float ProbeIntensity;
	};

}

CEREAL_REGISTER_TYPE(GEE::LightProbeComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GEE::Component, GEE::LightProbeComponent)