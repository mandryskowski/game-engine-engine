#include <scene/LightProbeComponent.h>
#include <rendering/Framebuffer.h>
#include <rendering/RenderToolbox.h>
#include <rendering/Material.h>

#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <rendering/LightProbe.h>
#include <scene/UIInputBoxActor.h>

namespace GEE
{
	LightProbeComponent::LightProbeComponent(Actor& actor, Component* parentComp, const std::string& name, EngineBasicShape shape) :
		Component(actor, parentComp, name),
		Shape(shape),
		ProbeIndex(0),
		ProbeIntensity(1.0f)
	{
		EnvironmentMap = *GEE_FB::reserveColorBuffer(Vec2u(1024, 1024), GL_RGB16F, GL_FLOAT, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_TEXTURE_CUBE_MAP, 0, "Environment cubemap");
		Scene.GetRenderData()->AddLightProbe(*this);
	}

	LightProbeComponent::LightProbeComponent(LightProbeComponent&& comp) :
		Component(std::move(comp)),
		Shape(comp.Shape),
		ProbeIndex(comp.ProbeIndex),
		EnvironmentMap(comp.EnvironmentMap),
		OptionalFilepath(comp.OptionalFilepath),
		ProbeIntensity(comp.ProbeIntensity)
	{
		Scene.GetRenderData()->AddLightProbe(*this);
	}

	EngineBasicShape LightProbeComponent::GetShape() const
	{
		return Shape;
	}

	Texture LightProbeComponent::GetEnvironmentMap() const
	{
		return EnvironmentMap;
	}

	unsigned int LightProbeComponent::GetProbeIndex() const
	{
		return ProbeIndex;
	}

	float LightProbeComponent::GetProbeIntensity() const
	{
		return ProbeIntensity;
	}

	Shader* LightProbeComponent::GetRenderShader(const RenderToolboxCollection& renderCol) const
	{
		return renderCol.FindShader("CookTorranceIBL");
	}

	void LightProbeComponent::SetProbeIndex(unsigned int index)
	{
		ProbeIndex = index;
	}

	MaterialInstance LightProbeComponent::GetDebugMatInst(EditorIconState state)
	{
		LoadDebugRenderMaterial("GEE_Mat_Default_Debug_LightProbeComponent", "EditorAssets/lightprobecomponent_icon.png");
		return Component::GetDebugMatInst(state);
	}

#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <rendering/LightProbe.h>
#include <scene/UIInputBoxActor.h>
	void LightProbeComponent::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Component::GetEditorDescription(descBuilder);

		UICanvasFieldCategory& cat = descBuilder.GetCanvas().AddCategory("Light probe");
		cat.AddField("Global cubemap filepath").GetTemplates().PathInput([this](const std::string& path) { LightProbeLoader::LoadLightProbeFromFile(*this, path); }, [this]() {return OptionalFilepath; }, { "*.hdr" });
		cat.AddField("Probe intensity").CreateChild<UIInputBoxActor>("ProbeIntensityInputBox", [this](float value) { ProbeIntensity = value; }, [this]() { return ProbeIntensity; });

		descBuilder.AddField("Dupa").CreateChild<UIButtonActor>("dupa1234");
		descBuilder.AddField("Kupa").CreateChild<UIButtonActor>("dup31234");
	}

	LightProbeComponent::~LightProbeComponent()
	{
		Scene.GetRenderData()->EraseLightProbe(*this);
	}

}