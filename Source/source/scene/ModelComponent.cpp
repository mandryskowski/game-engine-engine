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
	UICanvasElement(),
	LastFrameMVP(glm::mat4(1.0f)),
	SkelInfo(info),
	RenderAsBillboard(false),
	Hide(false)
{
}

ModelComponent::ModelComponent(ModelComponent&& model) :
	RenderableComponent(std::move(model)),
	UICanvasElement(std::move(model)),
	MeshInstances(std::move(model.MeshInstances)),
	SkelInfo(model.SkelInfo),
	RenderAsBillboard(model.RenderAsBillboard),
	LastFrameMVP(model.LastFrameMVP),
	Hide(model.Hide)
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

void ModelComponent::SetHide(bool hide)
{
	Hide = hide;
}

void ModelComponent::DRAWBATCH() const
{
	if (SkelInfo)
	{
		SkelInfo->DRAWBATCH();
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

bool ModelComponent::GetHide() const
{
	return Hide;
}


MeshInstance* ModelComponent::FindMeshInstance(std::string name)
{
	for (std::unique_ptr<MeshInstance>& it : MeshInstances)
	{
		if (it->GetMesh().GetName() == name)
			return it.get();
	}
	//auto found = std::find_if(MeshInstances.begin(), MeshInstances.end(), [name](const std::unique_ptr<MeshInstance>& meshInst) { return meshInst->GetMesh().GetName().find(name) != std::string::npos; });

	//if (found != MeshInstances.end())
	//	return found->get();


	for (unsigned int i = 0; i < Children.size(); i++)
	{
		ModelComponent* modelCast = dynamic_cast<ModelComponent*>(Children[i].get());
		MeshInstance* meshInst = modelCast->FindMeshInstance(name);
		if (meshInst)
			return meshInst;
	}


	return nullptr;
}

void ModelComponent::AddMeshInst(const MeshInstance& meshInst)
{
	MeshInstances.push_back(std::make_unique<MeshInstance>(meshInst));
	std::cout << "Added mesh inst " << MeshInstances.back()->GetMesh().GetName() << '\n';
}

void ModelComponent::Update(float deltaTime)
{
	for (auto& it : MeshInstances)
		if (it->GetMaterialInst())
			it->GetMaterialInst()->Update(deltaTime);
}

void ModelComponent::Render(const RenderInfo& info, Shader* shader)
{
	if (Hide)
		return;

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

	Material* tickMaterial = new Material("TickMaterial", 0.0f, 0.0f, GameHandle->GetRenderEngineHandle()->FindShader("Forward_NoLight"));
	tickMaterial->AddTexture(new NamedTexture(textureFromFile("EditorAssets/tick_icon.png", GL_SRGB_ALPHA, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, true), "albedo1"));

	descBuilder.AddField("Render as billboard").GetTemplates().TickBox(RenderAsBillboard);
	descBuilder.AddField("Hide").GetTemplates().TickBox(Hide);

	UICanvasField& testField = descBuilder.AddField("TEST");
	UIAutomaticListActor& testList = testField.CreateChild<UIAutomaticListActor>("TESTHELP");
	Actor& testTextAc = testList.CreateChild<Actor>("TESTTEXTAC");
	testTextAc.SetTransform(Transform(glm::vec2(0.0f), glm::vec2(0.1f)));
	testTextAc.CreateComponent<TextComponent>("ElementText", Transform(glm::vec2(-10.0f, 0.0f), glm::vec2(1.0f)), "Lorem ipsum test 123", "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::CENTER));

	Actor& testTextAc2 = testList.CreateChild<Actor>("TESTTEXTAC");
	testTextAc2.SetTransform(Transform(glm::vec2(0.0f, -10.0f), glm::vec2(0.1f)));
	testTextAc2.CreateComponent<TextComponent>("ElementText", Transform(glm::vec2(-10.0f, 0.0f), glm::vec2(1.0f)), "Kolejny ipsum lorem test 456", "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::CENTER));
}
