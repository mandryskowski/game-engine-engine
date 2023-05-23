#pragma once
#include <scene/Component.h>
#include <rendering/RenderableVolume.h>

namespace GEE
{
	class RenderToolboxCollection;
	class Shader;

	enum class LightType
	{
		DIRECTIONAL,
		POINT,
		SPOT
	};

	class LightComponent : public Component
	{
	public:
		LightComponent(Actor&, Component* parentComp, std::string name = "undefinedLight", LightType = LightType::POINT, unsigned int index = 0, unsigned int shadowMapNr = 0, float = 10.0f, Mat4f = glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 10.0f), Vec3f = Vec3f(0.0f), Vec3f = Vec3f(1.0f), Vec3f = Vec3f(0.5f), Vec3f = Vec3f(1.0f, 0.0f, 0.0f));
		LightComponent(LightComponent&&);

	protected:
		friend class Hierarchy::Node<LightComponent>;
		LightComponent& operator=(const LightComponent&);
	public:
		LightComponent& operator=(LightComponent&&) = delete;	//TODO: de-delete this, it should be written but i am too lazy


		LightType GetType() const;
		float GetFar() const;
		float GetShadowBias() const;
		unsigned int GetLightIndex() const;
		unsigned int GetShadowMapNr() const;
		Mat4f GetProjection() const;
		EngineBasicShape GetVolumeType() const;
		Shader* GetRenderShader(const RenderToolboxCollection& renderCol) const;

		bool HasValidShadowMap() const;
		void MarkValidShadowMap();
		bool ShouldCullFrontsForShadowMap() const;

		void InvalidateCache(bool includingShadowMap = true);



		void CalculateLightRadius();	//calculates ProjectionMat and Far
		void SetAdditionalData(Vec3f);
		void SetAttenuation(float);
		void SetCutOff(float);
		void SetOuterCutOff(float);
		void SetShadowBias(float);
		void SetType(LightType);
		void SetIndex(unsigned int);
		void SetDiffuse(const Vec3f& diffuse);
		void UpdateUBOData(UniformBuffer*, size_t = -1);
		Vec3f& operator[](unsigned int);

		virtual	MaterialInstance GetDebugMatInst(ButtonMaterialType) override;
		virtual void GetEditorDescription(ComponentDescriptionBuilder) override;
		virtual void DebugRender(SceneMatrixInfo info, Shader& shader, const Vec3f& debugIconScale = Vec3f(0.05f)) const override;

		template <typename Archive> void Save(Archive& archive) const
		{
			archive(cereal::make_nvp("Lighttype", static_cast<int>(Type)), CEREAL_NVP(Ambient), CEREAL_NVP(Diffuse), CEREAL_NVP(Specular), CEREAL_NVP(Attenuation), CEREAL_NVP(CutOff), CEREAL_NVP(OuterCutOff), CEREAL_NVP(Far), CEREAL_NVP(ShadowBias), CEREAL_NVP(bShadowMapCullFronts), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			LightType type;
			Vec3f ambient, diffuse, specular;
			float attenuation, cutOff, outerCutOff, far, shadowBias;
			bool cullFronts;

			archive(cereal::make_nvp("Lighttype", type), cereal::make_nvp("Ambient", ambient), cereal::make_nvp("Diffuse", diffuse), cereal::make_nvp("Specular", specular),
					cereal::make_nvp("Attenuation", attenuation), cereal::make_nvp("CutOff", cutOff), cereal::make_nvp("OuterCutOff", outerCutOff), cereal::make_nvp("Far", far),
					cereal::make_nvp("ShadowBias", shadowBias), cereal::make_nvp("bShadowMapCullFronts", cullFronts),
					cereal::base_class<Component>(this));
			SetType(type);
			Ambient = ambient;
			Diffuse = diffuse;
			Specular = specular;

			Far = far;
			SetAttenuation(attenuation);
			SetCutOff(cutOff);
			SetOuterCutOff(outerCutOff);
			SetShadowBias(shadowBias);
			bShadowMapCullFronts = cullFronts;

			InvalidateCache();	//make sure that everything is invalidated; e.g. loading a point light won't invalidate its shadowmaps because the light type we set is identical
		}

		virtual ~LightComponent();

	private:
		LightType Type;

		Vec3f Ambient;
		Vec3f Diffuse;
		Vec3f Specular;

		float Attenuation;
		float CutOff;			//"additional data"
		float OuterCutOff;		//cutoff angles are defined as: glm::cos( glm::radians( angle in degrees ) )

		float ShadowBias;
		bool bShadowMapCullFronts;
		

		unsigned int LightIndex;
		unsigned int ShadowMapNr;
		float Far;
		Mat4f Projection;

		bool DirtyFlag;	//for optimisation purposes; it should be set to on if the light's property has changed
		bool bHasValidShadowMap;
		unsigned int TransformDirtyFlagIndex; //again for optimisation; the ComponentTransform's flag of this index should be on if the light's position/direction has changed
	};

	class LightVolume : public RenderableVolume
	{
		const LightComponent* LightCompPtr;
	public:
		LightVolume(const LightComponent&);
		virtual EngineBasicShape GetShape() const override;
		virtual Transform GetRenderTransform() const override;
		virtual Shader* GetRenderShader(const RenderToolboxCollection& renderCol) const override;
		virtual void SetupRenderUniforms(const Shader& shader) const override;
	};


	LightType toLightType(std::string);
}

GEE_POLYMORPHIC_SERIALIZABLE_COMPONENT(GEE::Component, GEE::LightComponent)