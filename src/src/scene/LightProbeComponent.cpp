#include <scene/LightProbeComponent.h>
#include <game/GameScene.h>
#include <rendering/Framebuffer.h>
#include <rendering/RenderToolbox.h>
#include <rendering/material/Material.h>

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
		EnvironmentMap = NamedTexture(Texture::Loader<float>::ReserveEmptyCubemap(Vec2u(1024), Texture::Format::Float16::RGB()), "Environment cubemap");
		EnvironmentMap.SetMinFilter(Texture::MinFilter::Trilinear(), true, false);
		GetScene().GetRenderData()->AddLightProbe(*this);
	}

	LightProbeComponent::LightProbeComponent(LightProbeComponent&& comp) :
		Component(std::move(comp)),
		Shape(comp.Shape),
		ProbeIndex(comp.ProbeIndex),
		EnvironmentMap(comp.EnvironmentMap),
		OptionalFilepath(comp.OptionalFilepath),
		ProbeIntensity(comp.ProbeIntensity)
	{
		GetScene().GetRenderData()->AddLightProbe(*this);
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

	Shader* LightProbeComponent::GetRenderShader(const RenderToolboxCollection& renderCol)
	{
		return renderCol.FindShader("CookTorranceIBL");
	}

	bool LightProbeComponent::IsGlobalProbe() const
	{
		return Shape == EngineBasicShape::Quad;
	}

	void LightProbeComponent::SetProbeIndex(unsigned int index)
	{
		ProbeIndex = index;
	}

	MaterialInstance LightProbeComponent::GetDebugMatInst(ButtonMaterialType type)
	{
		LoadDebugRenderMaterial("GEE_Mat_Default_Debug_LightProbeComponent", "Assets/Editor/lightprobecomponent_icon.png");
		return Component::GetDebugMatInst(type);
	}

	void LightProbeComponent::GetEditorDescription(ComponentDescriptionBuilder descBuilder)
	{
		Component::GetEditorDescription(descBuilder);

		UICanvasFieldCategory& cat = descBuilder.GetCanvas().AddCategory("Light probe");
		cat.AddField("Global cubemap filepath").GetTemplates().PathInput([this](const std::string& path) { LightProbeLoader::LoadLightProbeFromFile(*this, path); }, [this]() { return OptionalFilepath; }, { "*.hdr" });
		cat.AddField("Probe intensity").CreateChild<UIInputBoxActor>("ProbeIntensityInputBox", [this](float value) { ProbeIntensity = value; }, [this]() { return ProbeIntensity; });

		descBuilder.AddField("Test").CreateChild<UIButtonActor>("test1234");
		descBuilder.AddField("Test2").CreateChild<UIButtonActor>("t5678");
	}

	LightProbeComponent::~LightProbeComponent()
	{
		GetScene().GetRenderData()->EraseLightProbe(*this);
		EnvironmentMap.Dispose();
	}

}