#include <scene/LightComponent.h>
#include <game/GameScene.h>
#include <rendering/material/Material.h>
#include <scene/UIInputBoxActor.h>
#include <UI/UICanvasActor.h>

#include <UI/UICanvasField.h>

#include <rendering/Renderer.h>

#include <editor/EditorActions.h>

#include "rendering/RenderEngineManager.h"

namespace GEE
{
	LightComponent::LightComponent(Actor& actor, Component* parentComp, std::string name, LightType type, unsigned int index, unsigned int shadowNr, float far, Mat4f projection, Vec3f amb, Vec3f diff, Vec3f spec, Vec3f settings) :
		Component(actor, parentComp, name, Transform()),
		Type(type),
		Ambient(amb),
		Diffuse(diff),
		Specular(spec),
		Attenuation(settings.x),
		ShadowBias(0.001f),
		bShadowMapCullFronts(true),
		LightIndex(index),
		ShadowMapNr(shadowNr),
		Far(far),
		Projection(projection),
		DirtyFlag(true),
		bHasValidShadowMap(false)
	{
		CutOff = glm::cos(glm::radians(settings.y));
		OuterCutOff = glm::cos(glm::radians(settings.z));

		std::cout << "WAZNE! ASSIGNING LIGHT INDEX " << index << "\n";

		GetScene().GetRenderData()->AddLight(*this);

		TransformDirtyFlagIndex = GetTransform().AddWorldDirtyFlag();

		CalculateLightRadius();
	}

	LightComponent::LightComponent(LightComponent&& comp) :
		Component(std::move(comp)),
		Type(comp.Type),
		Ambient(comp.Ambient),
		Diffuse(comp.Diffuse),
		Specular(comp.Specular),
		Attenuation(comp.Attenuation),
		CutOff(comp.CutOff),
		OuterCutOff(comp.OuterCutOff),
		ShadowBias(comp.ShadowBias),
		bShadowMapCullFronts(comp.bShadowMapCullFronts),
		LightIndex(comp.LightIndex),
		ShadowMapNr(comp.ShadowMapNr),
		Far(comp.Far),
		Projection(comp.Projection),
		DirtyFlag(comp.DirtyFlag),
		bHasValidShadowMap(comp.bHasValidShadowMap)
	{
		TransformDirtyFlagIndex = GetTransform().AddWorldDirtyFlag();
		GetScene().GetRenderData()->AddLight(*this);
	}

	LightComponent& LightComponent::operator=(const LightComponent& light)
	{
		Component::operator=(light);

		Type = light.Type;
		Ambient = light.Ambient;
		Diffuse = light.Diffuse;
		Specular = light.Specular;
		Attenuation = light.Attenuation;
		ShadowBias = light.ShadowBias;
		bShadowMapCullFronts = light.bShadowMapCullFronts;
		CutOff = light.CutOff;
		OuterCutOff = light.OuterCutOff;
		LightIndex = light.LightIndex;
		Far = light.Far;
		Projection = light.Projection;
		DirtyFlag = light.DirtyFlag;
		ShadowMapNr = light.ShadowMapNr;
		bHasValidShadowMap = light.bHasValidShadowMap;

		return *this;
	}

	LightType LightComponent::GetType() const
	{
		return Type;
	}

	float LightComponent::GetFar() const
	{
		return Far;
	}

	float LightComponent::GetShadowBias() const
	{
		return ShadowBias;
	}

	unsigned int LightComponent::GetLightIndex() const
	{
		return LightIndex;
	}

	unsigned int LightComponent::GetShadowMapNr() const
	{
		return ShadowMapNr;
	}

	Mat4f LightComponent::GetProjection() const
	{
		return Projection;
	}

	EngineBasicShape LightComponent::GetVolumeType() const
	{
		switch (Type)
		{
		case LightType::SPOT:
			if (Ambient == Vec3f(0.0f))
				return EngineBasicShape::Cone;
		case LightType::POINT:
			return EngineBasicShape::Sphere;
		case LightType::DIRECTIONAL:
			return EngineBasicShape::Quad;
		default:
			std::cerr << "ERROR! Unknown light type: " << (int)Type << ".\n";
			return EngineBasicShape::Sphere;
		}
	}

	Shader* LightComponent::GetRenderShader(const RenderToolboxCollection& renderCol) const
	{
		return GetGameHandle()->GetRenderEngineHandle()->GetLightShader(renderCol, Type);
	}

	bool LightComponent::HasValidShadowMap() const
	{
		return bHasValidShadowMap;
	}

	void LightComponent::MarkValidShadowMap()
	{
		bHasValidShadowMap = true;
	}

	bool LightComponent::ShouldCullFrontsForShadowMap() const
	{
		return bShadowMapCullFronts;
	}

	void LightComponent::InvalidateCache(bool includingShadowMap)
	{
		if (includingShadowMap) std::cout << "Invalidating cache\n";
		DirtyFlag = true;
		if (includingShadowMap) bHasValidShadowMap = false;
		GetTransform().FlagMyDirtiness();
	}

	void LightComponent::CalculateLightRadius()
	{
		if (Type == LightType::DIRECTIONAL)
			return;

		DirtyFlag = true;

		///////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////

		float constant = 1.0f;
		float linear = 0.0f;
		float quadratic = Attenuation;
		float lightMax = glm::max(glm::max(Diffuse.r, Diffuse.g), Diffuse.b);

		float radius = 10.0f;
		if (Attenuation > 0.0f)
			radius = (-linear + glm::sqrt(linear * linear - 4.0f * quadratic * (constant - lightMax * (256.0f / 5.0f)))) / (2.0f * quadratic);

		radius = glm::min(radius, 50.0f);	// Limit radius to 50 units - the currently used projection matrix allows to render fragments up to 100 units away from the camera.

		Far = radius;
		Projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, Far);

		GetTransform().SetScale(Vec3f(radius));

		///////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////

		if (Type == LightType::SPOT && Ambient == Vec3f(0.0f))	//If we are a spotlight and we don't emit any ambient light, we can set the light volume to a cone instead of a sphere - more sweet FPS
		{
			float scale = radius * glm::tan(glm::acos(OuterCutOff));	//radius == height in our code
			GetTransform().SetScale(Vec3f(scale, scale, radius));
		}
	}

	void LightComponent::SetAdditionalData(Vec3f data)
	{
		float prevCutOff = CutOff, prevOuterCutOff = OuterCutOff;

		SetAttenuation(data.x);
		CutOff = glm::cos(glm::radians(data.y));
		OuterCutOff = glm::cos(glm::radians(data.z));

		DirtyFlag = true;
		if (prevCutOff != CutOff || prevOuterCutOff != OuterCutOff)
			bHasValidShadowMap = false;
	}

	void LightComponent::SetAttenuation(float attenuation)
	{
		if (Attenuation == attenuation)
			return;

		Attenuation = attenuation;
		DirtyFlag = true;
		CalculateLightRadius();
	}

	void LightComponent::SetCutOff(float cutoff)
	{
		CutOff = cutoff;
		DirtyFlag = true;
	}

	void LightComponent::SetOuterCutOff(float outercutoff)
	{
		OuterCutOff = outercutoff;
		CalculateLightRadius();
	}

	void LightComponent::SetShadowBias(float bias)
	{
		ShadowBias = bias;
		DirtyFlag = true;
	}

	void LightComponent::SetType(LightType type)
	{
		if (Type != type)
			bHasValidShadowMap = false;

		Type = type;
		DirtyFlag = true;
	}

	void LightComponent::SetIndex(unsigned int index)
	{
		if (ShadowMapNr != index)
			bHasValidShadowMap = false;
		LightIndex = index;
		ShadowMapNr = index;
		DirtyFlag = true;
	}

	void LightComponent::SetDiffuse(const Vec3f& diffuse)
	{
		Diffuse = diffuse;
		DirtyFlag = true;
		CalculateLightRadius();
	}

	void LightComponent::UpdateUBOData(UniformBuffer* lightsUBO, size_t offset)
	{
		if (offset != -1)
			lightsUBO->offsetCache = offset;

		const Transform& worldTransform = GetTransform().GetWorldTransform();
		bool transformDirtyFlag = GetTransform().GetWorldDirtyFlag(TransformDirtyFlagIndex);

		if (transformDirtyFlag)
		{
			switch (Type)
			{
			case LightType::DIRECTIONAL:
				lightsUBO->offsetCache += sizeof(Vec4f);
				lightsUBO->SubData4fv(worldTransform.GetFrontVec(), lightsUBO->offsetCache); break;
			case LightType::POINT:
				lightsUBO->SubData4fv(worldTransform.GetPos(), lightsUBO->offsetCache);
				lightsUBO->offsetCache += sizeof(Vec4f); break;
			case LightType::SPOT:
				lightsUBO->SubData4fv(worldTransform.GetPos(), lightsUBO->offsetCache);
				lightsUBO->SubData4fv(worldTransform.GetFrontVec(), lightsUBO->offsetCache); break;
			}
		}
		else
			lightsUBO->offsetCache += sizeof(Vec4f) * 2;

		if (DirtyFlag)
		{
			lightsUBO->SubData4fv(std::vector <Vec4f> {Vec4f(Ambient, ShadowBias), Vec4f(Diffuse, 0.0f), Vec4f(Specular, 0.0f)}, lightsUBO->offsetCache);
			float additionalData[3] = { Attenuation, CutOff, OuterCutOff };
			lightsUBO->SubData(12, additionalData, lightsUBO->offsetCache);
			lightsUBO->SubData1f(static_cast<float>(Type), lightsUBO->offsetCache);
			lightsUBO->SubData1f(static_cast<float>(ShadowMapNr), lightsUBO->offsetCache);
			lightsUBO->SubData1f(Far, lightsUBO->offsetCache);

			DirtyFlag = false;
		}
		else
			lightsUBO->offsetCache += 72;

		if (transformDirtyFlag)
			lightsUBO->SubDataMatrix4fv(Projection * worldTransform.GetViewMatrix(), lightsUBO->offsetCache + 8);
		else
			lightsUBO->offsetCache += sizeof(Mat4f);

		lightsUBO->PadOffset();
	}

	Vec3f& LightComponent::operator[](unsigned int i)
	{
		DirtyFlag = true;
		switch (i)
		{
		case 0:
			return Ambient;
		case 1:
			return Diffuse;
		case 2:
			return Specular;
		}

		return Ambient;
	}

	MaterialInstance LightComponent::GetDebugMatInst(ButtonMaterialType type)
	{
		LoadDebugRenderMaterial("GEE_Mat_Default_Debug_LightComponent", "Assets/Editor/lightcomponent_icon.png");
		return Component::GetDebugMatInst(type);
	}

	void LightComponent::GetEditorDescription(ComponentDescriptionBuilder descBuilder)
	{
		Component::GetEditorDescription(descBuilder);

		descBuilder.AddField("Ambient").GetTemplates().VecInput(Ambient);
		descBuilder.AddField("Diffuse").GetTemplates().VecInput(Diffuse);
		descBuilder.AddField("Specular").GetTemplates().VecInput(Specular);
		UIInputBoxActor& attenuationInputBox = descBuilder.AddField("Attenuation").CreateChild<UIInputBoxActor>("Attenuation");
		attenuationInputBox.SetOnInputFunc([this](float val) { SetAttenuation(val); }, [this]() { return Attenuation; }, true);
		UIInputBoxActor& cutOffInputBox = descBuilder.AddField("Cutoff angle").CreateChild<UIInputBoxActor>("CutoffAngle");
		cutOffInputBox.SetOnInputFunc([this](float val) { SetCutOff(glm::cos(glm::radians(val))); }, [this]() { return glm::degrees(glm::acos(CutOff)); }, true);
		UIInputBoxActor& outerCutOffInputBox = descBuilder.AddField("Outer cutoff angle").CreateChild<UIInputBoxActor>("OuterCutoffAngle");
		outerCutOffInputBox.SetOnInputFunc([this](float val) { SetOuterCutOff(glm::cos(glm::radians(val))); }, [this]() { return glm::degrees(glm::acos(OuterCutOff)); }, true);
		UIInputBoxActor& shadowBiasInputBox = descBuilder.AddField("Shadow bias").CreateChild<UIInputBoxActor>("ShadowBias");
		shadowBiasInputBox.SetOnInputFunc([this](float val) { SetShadowBias(val); }, [this]() -> float { return ShadowBias; });
		descBuilder.AddField("Cull fronts in shadow maps").GetTemplates().TickBox(bShadowMapCullFronts);
		descBuilder.AddField("Light index").CreateChild<UIButtonActor>("LightIndexInfo",
			std::to_string(LightIndex) + "/" + std::to_string(GetScene().GetRenderData()->GetMaxShadowedLightCount())).SetDisableInput(true);


		UICanvasField& typeField = descBuilder.AddField("Type");
		const std::string types[] = { "Directional", "Point", "Spot" };
		for (int i = 0; i < 3; i++)
			typeField.CreateChild<UIButtonActor>(types[i] + "TypeButton", types[i], [this, i]() {SetType(static_cast<LightType>(i)); CalculateLightRadius(); }).GetTransform()->Move(Vec2f(static_cast<float>(i) * 2.0f, 0.0f));
	}

	void LightComponent::DebugRender(SceneMatrixInfo info, Shader& shader, const Vec3f& debugIconScale) const
	{
		Component::DebugRender(info, shader, debugIconScale);

		if (auto editorCast = dynamic_cast<Editor::EditorManager*>(GetGameHandle()); editorCast && editorCast->GetActions().GetSelectedComponent() != this)
			return;
			

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		Renderer(*GetGameHandle()->GetRenderEngineHandle()).StaticMeshInstances(info, { GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(LightVolume(*this).GetShape()) }, LightVolume(*this).GetRenderTransform(), shader);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	LightComponent::~LightComponent()
	{
		GetScene().GetRenderData()->EraseLight(*this);
	}

	LightVolume::LightVolume(const LightComponent& lightCompPtr) :
		LightCompPtr(&lightCompPtr)
	{
	}

	EngineBasicShape LightVolume::GetShape() const
	{
		return LightCompPtr->GetVolumeType();
	}

	Transform LightVolume::GetRenderTransform() const
	{
		Transform lightTransform = LightCompPtr->GetTransform().GetWorldTransform();
		lightTransform.SetScale(LightCompPtr->GetTransform().GetScale());

		return lightTransform;
	}

	Shader* LightVolume::GetRenderShader(const RenderToolboxCollection& renderCol) const
	{
		return LightCompPtr->GetRenderShader(renderCol);
	}

	void LightVolume::SetupRenderUniforms(const Shader& shader) const
	{
		shader.Uniform<int>("lightIndex", LightCompPtr->GetLightIndex());
	}



	LightType toLightType(std::string str)
	{
		if (str == "direction")
			return LightType::DIRECTIONAL;
		if (str == "spot")
			return LightType::SPOT;
		if (str != "point")
			std::cerr << str << " is not a known light type.\n";
		return LightType::POINT;
	}

}
