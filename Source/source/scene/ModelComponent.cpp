#include <scene/ModelComponent.h>
#include <scene/HierarchyTemplate.h>
#include <game/GameScene.h>
#include <rendering/RenderInfo.h>
#include <UI/UICanvasActor.h>
#include <scene/UIInputBoxActor.h>
#include <rendering/Texture.h>
#include <scene/TextComponent.h>
#include <UI/UICanvasField.h>

using namespace MeshSystem;

ModelComponent::ModelComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform, SkeletonInfo* info, Material* overrideMat) :
	RenderableComponent(actor, parentComp, name, transform),
	UIComponent(actor, parentComp),
	LastFrameMVP(glm::mat4(1.0f)),
	SkelInfo(info),
	RenderAsBillboard(false)
{
}

ModelComponent::ModelComponent(ModelComponent&& model) :
	RenderableComponent(std::move(model)),
	UIComponent(std::move(model)),
	MeshInstances(std::move(model.MeshInstances)),
	SkelInfo(model.SkelInfo),
	RenderAsBillboard(model.RenderAsBillboard),
	LastFrameMVP(model.LastFrameMVP)
{
}

ModelComponent& ModelComponent::operator=(const ModelComponent& compT)
{
	Component::operator=(compT);

	for (int i = 0; i < compT.GetMeshInstanceCount(); i++)
		AddMeshInst(MeshInstance(const_cast<Mesh&>(compT.GetMeshInstance(i).GetMesh()), const_cast<Material*>(compT.GetMeshInstance(i).GetMaterialPtr())));

	/*if (overrideMaterial)
		OverrideInstancesMaterial(overrideMaterial);
	else if (compT.GetOverrideMaterial())
		OverrideInstancesMaterial(meshNodeCast->GetOverrideMaterial());*/
	return *this;
}

void ModelComponent::OverrideInstancesMaterial(Material* overrideMat)
{
	for (auto& it : MeshInstances)
		it->SetMaterial(overrideMat);
}

void ModelComponent::OverrideInstancesMaterialInstances(std::shared_ptr<MaterialInstance> overrideMatInst)
{
	for (auto& it : MeshInstances)
		it->SetMaterialInst(overrideMatInst);
}

void ModelComponent::SetLastFrameMVP(const glm::mat4& lastMVP) const
{
	LastFrameMVP = lastMVP;
}

void ModelComponent::SetSkeletonInfo(SkeletonInfo* info)
{
	SkelInfo = info;
}

void ModelComponent::SetRenderAsBillboard(bool billboard)
{
	RenderAsBillboard = billboard;
}

void ModelComponent::DRAWBATCH() const
{
	if (SkelInfo)
	{
		;// SkelInfo->DRAWBATCH();
	}
}

int ModelComponent::GetMeshInstanceCount() const
{
	return MeshInstances.size();
}

const MeshInstance& ModelComponent::GetMeshInstance(int index) const
{
	return *MeshInstances[index];
}

const glm::mat4& ModelComponent::GetLastFrameMVP() const
{
	return LastFrameMVP;
}

SkeletonInfo* ModelComponent::GetSkeletonInfo() const
{
	return SkelInfo;
}

MeshInstance* ModelComponent::FindMeshInstance(const std::string& nodeName, const std::string& specificMeshName)
{
	/*for (int i = 0; i < 2; i++)	//Search 2 times; first look at the specific names and then at node names.
	{
		std::function<bool(const std::string&, const std::string&)> checkEqual = [](const std::string& str1, const std::string& str2) -> bool { return !str1.empty() && str1 == str2; };
		for (std::unique_ptr<MeshInstance>& it : MeshInstances)
			if ((i == 0 && (checkEqual(specificMeshName, it->GetMesh().GetLocalization().SpecificName)) || checkEqual(specificMeshName, it->GetMesh().GetLocalization().NodeName) ||
				(i == 1 && (checkEqual(nodeName, it->GetMesh().GetLocalization().SpecificName)) || checkEqual(nodeName, it->GetMesh().GetLocalization().NodeName))))
				return it.get();
	}*/

	if (nodeName.empty() && specificMeshName.empty())
		return nullptr;

	std::function<bool(const std::string&, const std::string&)> checkEqual = [](const std::string& str1, const std::string& str2) -> bool { return str1.empty() || str1 == str2; };
	for (std::unique_ptr<MeshInstance>& it : MeshInstances)
		if ((checkEqual(nodeName, it->GetMesh().GetLocalization().NodeName)) && checkEqual(specificMeshName, it->GetMesh().GetLocalization().SpecificName))
			return it.get();

	for (auto& it: Children)
	{
		ModelComponent* modelCast = dynamic_cast<ModelComponent*>(it.get());
		MeshInstance* meshInst = modelCast->FindMeshInstance(nodeName, specificMeshName);
		if (meshInst)
			return meshInst;
	}


	return nullptr;
}

void ModelComponent::AddMeshInst(const MeshInstance& meshInst)
{
	MeshInstances.push_back(std::make_unique<MeshInstance>(meshInst));
}

void ModelComponent::Update(float deltaTime)
{
	for (auto& it : MeshInstances)
		if (it->GetMaterialInst())
			it->GetMaterialInst()->Update(deltaTime);
}

void ModelComponent::Render(const RenderInfo& info, Shader* shader)
{
	if (GetHide())
		return;

	std::shared_ptr<AtlasMaterial> found = std::dynamic_pointer_cast<AtlasMaterial>(GameHandle->GetRenderEngineHandle()->FindMaterial("Kopec"));
	if (Name == "KOPEC")
	{
		OverrideInstancesMaterialInstances(std::make_shared<MaterialInstance>(*found, found->GetTextureIDInterpolatorTemplate(Interpolation(0.0f, 0.2f, InterpolationType::LINEAR, true, AnimBehaviour::STOP, AnimBehaviour::REPEAT), 0.0f, 1.0f)));
		Name = "Kopec";
	}

	if (SkelInfo && SkelInfo->GetBoneCount() > 0)
		GameHandle->GetRenderEngineHandle()->RenderSkeletalMeshes(info, MeshInstances, GetTransform().GetWorldTransform(), shader, *SkelInfo);
	else
	{
		GameHandle->GetRenderEngineHandle()->RenderStaticMeshes((CanvasPtr) ? (CanvasPtr->BindForRender(info, GameHandle->GetGameSettings()->WindowSize)) : (info), MeshInstances, GetTransform().GetWorldTransform(), shader, &LastFrameMVP, nullptr, RenderAsBillboard);
		if (CanvasPtr)
			CanvasPtr->UnbindForRender(GameHandle->GetGameSettings()->WindowSize);
	}
}

#include <UI/UIListActor.h>

void ModelComponent::GetEditorDescription(EditorDescriptionBuilder descBuilder)
{
	RenderableComponent::GetEditorDescription(descBuilder);

	Material* tickMaterial = new Material("TickMaterial", 0.0f, GameHandle->GetRenderEngineHandle()->FindShader("Forward_NoLight"));
	tickMaterial->AddTexture(std::make_shared<NamedTexture>(textureFromFile("EditorAssets/tick_icon.png", GL_SRGB_ALPHA, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, true), "albedo1"));

	descBuilder.AddField("Render as billboard").GetTemplates().TickBox(RenderAsBillboard);
	descBuilder.AddField("Hide").GetTemplates().TickBox(Hide);

	UICanvasField& testField = descBuilder.AddField("TEST");
	UIAutomaticListActor& testList = testField.CreateChild<UIAutomaticListActor>("TESTHELP");
	Actor& testTextAc = testList.CreateChild<Actor>("TESTTEXTAC");
	testTextAc.SetTransform(Transform(glm::vec2(0.0f), glm::vec2(0.1f)));
	testTextAc.CreateComponent<TextComponent>("ElementText", Transform(glm::vec2(-10.0f, 0.0f), glm::vec2(1.0f)), "Lorem ipsum test 123", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::CENTER));

	Actor& testTextAc2 = testList.CreateChild<Actor>("TESTTEXTAC");
	testTextAc2.SetTransform(Transform(glm::vec2(0.0f, -10.0f), glm::vec2(0.1f)));
	testTextAc2.CreateComponent<TextComponent>("ElementText", Transform(glm::vec2(-10.0f, 0.0f), glm::vec2(1.0f)), "Kolejny ipsum lorem test 456", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::CENTER));
}
