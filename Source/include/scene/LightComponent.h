#pragma once
#include <scene/Component.h>
#include <rendering/RenderableVolume.h>
enum LightType
{
	DIRECTIONAL,
	POINT,
	SPOT
};

class LightComponent: public Component
{
public:
	LightComponent(Actor&, Component* parentComp, std::string name = "undefinedLight", LightType = LightType::POINT, unsigned int index = 0, unsigned int shadowMapNr = 0, float = 10.0f, glm::mat4 = glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 10.0f), glm::vec3 = glm::vec3(0.0f), glm::vec3 = glm::vec3(1.0f), glm::vec3 = glm::vec3(0.5f), glm::vec3 = glm::vec3(1.0f, 0.0f, 0.0f));
	LightComponent(LightComponent&&);

	LightType GetType() const;
	float GetFar() const;
	unsigned int GetLightIndex() const;
	unsigned int GetShadowMapNr() const;
	glm::mat4 GetProjection() const;
	EngineBasicShape GetVolumeType() const;
	Shader* GetRenderShader(const RenderToolboxCollection& renderCol) const;

	bool HasValidShadowMap() const;
	void MarkValidShadowMap();

	void InvalidateCache();



	void CalculateLightRadius();	//calculates ProjectionMat and Far
	void SetAdditionalData(glm::vec3);
	void SetAttenuation(float);
	void SetType(LightType);
	void SetIndex(unsigned int);
	void UpdateUBOData(UniformBuffer*, size_t = -1);
	glm::vec3& operator[](unsigned int);

	virtual	MaterialInstance GetDebugMatInst(EditorIconState) override;
	virtual void GetEditorDescription(EditorDescriptionBuilder) override;

	virtual ~LightComponent();

private:
	LightType Type;

	glm::vec3 Ambient;
	glm::vec3 Diffuse;
	glm::vec3 Specular;

	float Attenuation;
	float CutOff;			//"additional data"
	float OuterCutOff;		//cutoff angles are defined as: glm::cos( glm::radians( angle in degrees ) )
	//
	float OuterCutOffBeforeCos;	//for optimisation purposes; it allows to calculate a cone's radius more easily

	unsigned int LightIndex;
	unsigned int ShadowMapNr;
	float Far;
	glm::mat4 Projection;

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