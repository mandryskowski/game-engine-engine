#include <scene/ModelComponent.h>
#include <rendering/MeshSystem.h>
#include <game/GameScene.h>
#include <rendering/RenderInfo.h>
#include <UI/UICanvasActor.h>
#include <scene/UIInputBoxActor.h>
#include <rendering/Texture.h>
#include <scene/TextComponent.h>
#include <UI/UICanvasField.h>

using namespace MeshSystem;

ModelComponent::ModelComponent(GameScene& scene, const std::string& name, const Transform& transform, SkeletonInfo* info, Material* overrideMat) :
	RenderableComponent(scene, name, transform),
	UICanvasElement(),
	LastFrameMVP(glm::mat4(1.0f)),
	SkelInfo(info),
	RenderAsBillboard(false),
	Hide(false)
{
}

ModelComponent::ModelComponent(GameScene& scene, const MeshSystem::MeshNode& node, const std::string& name, const Transform& transform, SkeletonInfo* info, Material* overrideMat) :
	ModelComponent(scene, name, transform, info, overrideMat)
{
	GenerateFromNode(node, overrideMat);
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
		AddMeshInst(compT.GetMeshInstance(i));

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

void ModelComponent::GenerateFromNode(const MeshSystem::TemplateNode& node, Material* overrideMaterial)
{
	Component::GenerateFromNode(node, overrideMaterial);
	const MeshSystem::MeshNode* meshNodeCast = dynamic_cast<const MeshSystem::MeshNode*>(&node);
	for (int i = 0; i < meshNodeCast->GetMeshCount(); i++)
		AddMeshInst(*meshNodeCast->GetMesh(i));

	if (overrideMaterial)
		OverrideInstancesMaterial(overrideMaterial);
	else if (meshNodeCast->GetOverrideMaterial())
		OverrideInstancesMaterial(meshNodeCast->GetOverrideMaterial());
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
	auto found = std::find_if(MeshInstances.begin(), MeshInstances.end(), [name](const std::unique_ptr<MeshInstance>& meshInst) { return meshInst->GetMesh().GetName().find(name) != std::string::npos; });

	if (found != MeshInstances.end())
		return found->get();


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

void ModelComponent::GetEditorDescription(UIActor& editorParent, GameScene& editorScene)
{
	RenderableComponent::GetEditorDescription(editorParent, editorScene);

	Material* tickMaterial = new Material("TickMaterial", 0.0f, 0.0f, GameHandle->GetRenderEngineHandle()->FindShader("Forward_NoLight"));
	tickMaterial->AddTexture(new NamedTexture(textureFromFile("EditorAssets/tick_icon.png", GL_SRGB_ALPHA, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, true), "albedo1"));

	//canvas.AddField("Position").GetTemplates().Vec3Input([this](int i, float val) {glm::vec3 pos = GetTransform().PositionRef; pos[i] = val; GetTransform().SetPosition(pos); });

	AddFieldToCanvas("Render as billboard", editorParent).GetTemplates().TickBox(RenderAsBillboard);
	AddFieldToCanvas("Hide", editorParent).GetTemplates().TickBox(Hide);

	UICanvasField& testField = AddFieldToCanvas("TEST", editorParent);
	UIAutomaticListActor& testList = testField.CreateChild(UIAutomaticListActor(editorScene, "TESTHELP"));
	Actor& testTextAc = testList.CreateChild(Actor(editorScene, "TESTTEXTAC"));
	testTextAc.SetTransform(Transform(glm::vec2(0.0f), glm::vec2(0.1f)));
	testTextAc.CreateComponent(TextComponent(editorScene, "ElementText", Transform(glm::vec2(-10.0f, 0.0f), glm::vec2(1.0f)), "Lorem ipsum test 123", "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::CENTER)));

//	Actor& testTextAc11 = testTextAc.CreateChild(Actor(editorScene, "TESTTEXTAC"));
//	testTextAc11.SetTransform(Transform(glm::vec2(0.0f, -10.0f), glm::vec3(0.0f), glm::vec2(1.0f)));
//	testTextAc11.CreateComponent(TextComponent(editorScene, "ElementText", Transform(glm::vec2(-10.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f)), "Dziecko lorem", "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::CENTER)));

	Actor& testTextAc2 = testList.CreateChild(Actor(editorScene, "TESTTEXTAC"));
	testTextAc2.SetTransform(Transform(glm::vec2(0.0f, -10.0f), glm::vec2(0.1f)));
	testTextAc2.CreateComponent(TextComponent(editorScene, "ElementText", Transform(glm::vec2(-10.0f, 0.0f), glm::vec2(1.0f)), "Kolejny ipsum lorem test 456", "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::CENTER)));
//	canvas.AddField("Hide").GetTemplates().TickBox(&Hide);
	//	EditorFieldTemplates::TickBox(canvas, "Render as billboard", &RenderAsBillboard);																									//identico
	//	EditorFieldTemplates::TickBox(canvas, [this](ModelComponent& tickModel) { RenderAsBillboard = !RenderAsBillboard; tickModel.SetHide(!RenderAsBillboard);});	//identico

	//	canvas.AddField(billboardTickBox);
	/*{
		Actor& elementActor = canvas.CreateChild(Actor(editorScene, "ElementActor"));
		elementActor.SetTransform(Transform(glm::vec2(0.0f, -3.0f), glm::vec3(0.0f), glm::vec2(0.1F)));
		elementActor.CreateComponent(TextComponent(editorScene, "ElementText", Transform(glm::vec2(-10.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f)), "Render as billboard", "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::CENTER)));


		UIButtonActor& billboardTickBoxActor = elementActor.CreateChild(UIButtonActor(editorScene, "BillboardTickBox"));

		ModelComponent& billboardTickModel = billboardTickBoxActor.CreateComponent(ModelComponent(editorScene, "TickModel"));
		billboardTickModel.AddMeshInst(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
		billboardTickModel.SetHide(true);
		billboardTickModel.OverrideInstancesMaterial(tickMaterial);

		billboardTickBoxActor.SetTransform(Transform(glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f)));
		billboardTickBoxActor.SetOnClickFunc([this, &billboardTickBoxActor, &billboardTickModel]() { RenderAsBillboard = !RenderAsBillboard; billboardTickModel.SetHide(!billboardTickModel.GetHide()); });
		canvas.AddUIElement(elementActor);
	}

	{
		Actor& elementActor = canvas.CreateChild(Actor(editorScene, "ElementActor"));
		elementActor.SetTransform(Transform(glm::vec2(0.0f, -3.3f), glm::vec3(0.0f), glm::vec2(0.1f)));
		elementActor.CreateComponent(TextComponent(editorScene, "ElementText", Transform(glm::vec2(-10.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f)), "Hide", "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::CENTER)));


		UIButtonActor& hideTickBoxActor = elementActor.CreateChild(UIButtonActor(editorScene, "HideTickBox"));

		ModelComponent& hideTickModel = hideTickBoxActor.CreateComponent(ModelComponent(editorScene, "TickModel"));
		hideTickModel.AddMeshInst(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
		hideTickModel.SetHide(true);
		hideTickModel.OverrideInstancesMaterial(tickMaterial);

		hideTickBoxActor.SetTransform(Transform(glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f)));
		hideTickBoxActor.SetOnClickFunc([this, &hideTickBoxActor, &hideTickModel]() { Hide = !Hide; hideTickModel.SetHide(!hideTickModel.GetHide()); });
		canvas.AddUIElement(elementActor);
	}

	/*UIButtonActor& hideTickBoxActor = canvas.CreateChild(UIButtonActor(editorScene, "HideTickBox"));

	ModelComponent& hideTickModel = hideTickBoxActor.CreateComponent(ModelComponent(editorScene, "TickModel"));
	hideTickModel.AddMeshInst(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
	hideTickModel.SetHide(true);
	hideTickModel.OverrideInstancesMaterial(tickMaterial);

	canvas.AddUIElement(hideTickBoxActor);
	hideTickBoxActor.SetTransform(Transform(glm::vec3(-0.75f, -3.5f, 0.0f), glm::vec3(0.0f), glm::vec3(0.25f)));
	hideTickBoxActor.SetOnClickFunc([this, &hideTickBoxActor, &hideTickModel]() { Hide = !Hide; hideTickModel.SetHide(!hideTickModel.GetHide()); });*/
}
