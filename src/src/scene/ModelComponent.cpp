#include <scene/ModelComponent.h>
#include <game/GameScene.h>
#include <rendering/RenderInfo.h>
#include <UI/UICanvasActor.h>
#include <rendering/Texture.h>
#include <scene/TextComponent.h>
#include <UI/UICanvasField.h>

#include <UI/UIListActor.h>
#include <scene/UIWindowActor.h>
#include <scene/LightProbeComponent.h>
#include <rendering/RenderToolbox.h>
#include <scene/CameraComponent.h>
#include <scene/Controller.h>
#include <game/IDSystem.h>

#include <editor/EditorActions.h>
#include <scene/hierarchy/HierarchyTree.h>

#include <rendering/Renderer.h>

namespace GEE
{
	using namespace MeshSystem;

	ModelComponent::ModelComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform, SkeletonInfo* info, Material* overrideMat) :
		RenderableComponent(actor, parentComp, name, transform),
		UIComponent(actor, parentComp),
		LastFrameMVP(Mat4f(1.0f)),
		SkelInfo(info),
		RenderAsBillboard(false)
	{
		if (SkelInfo)
			SkelInfo->AddModelCompRef(*this);
	}

	ModelComponent::ModelComponent(ModelComponent&& model) :
		RenderableComponent(std::move(model)),
		UIComponent(std::move(model)),
		MeshInstances(std::move(model.MeshInstances)),
		SkelInfo(model.SkelInfo),
		RenderAsBillboard(model.RenderAsBillboard),
		LastFrameMVP(model.LastFrameMVP)
	{
		if (SkelInfo)
			SkelInfo->AddModelCompRef(*this);
	}

	ModelComponent& ModelComponent::operator=(const ModelComponent& compT)
	{
		Component::operator=(compT);

		for (int i = 0; i < compT.GetMeshInstanceCount(); i++)
			AddMeshInst(MeshInstance(const_cast<Mesh&>(compT.GetMeshInstance(i).GetMesh()), compT.GetMeshInstance(i).GetMaterialPtr()));

		/*if (overrideMaterial)
			OverrideInstancesMaterial(overrideMaterial);
		else if (compT.GetOverrideMaterial())
			OverrideInstancesMaterial(meshNodeCast->GetOverrideMaterial());*/
		return *this;
	}

	void ModelComponent::OverrideInstancesMaterial(SharedPtr<Material> overrideMat)
	{
		for (auto& it : MeshInstances)
			it->SetMaterial(overrideMat);
	}

	void ModelComponent::OverrideInstancesMaterialInstances(SharedPtr<MaterialInstance> overrideMatInst)
	{
		for (auto& it : MeshInstances)
			it->SetMaterialInst(overrideMatInst);
	}

	void ModelComponent::SetLastFrameMVP(const Mat4f& lastMVP) const
	{
		LastFrameMVP = lastMVP;
	}

	void ModelComponent::SetSkeletonInfo(SkeletonInfo* info)
	{
		if (SkelInfo)
			SkelInfo->EraseModelCompRef(*this);

		SkelInfo = info;

		if (SkelInfo)
			SkelInfo->AddModelCompRef(*this);
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

	const Mat4f& ModelComponent::GetLastFrameMVP() const
	{
		return LastFrameMVP;
	}

	SkeletonInfo* ModelComponent::GetSkeletonInfo() const
	{
		return SkelInfo;
	}

	std::vector<const Material*> ModelComponent::GetMaterials() const
	{
		std::vector<const Material*> materials;
		for (auto& it : MeshInstances)
		{
			if (std::find(materials.begin(), materials.end(), it->GetMaterialPtr().get()) == materials.end())	// do not repeat any materials
				materials.push_back(it->GetMaterialPtr().get());
		}

		return materials;
	}

	MeshInstance* ModelComponent::FindMeshInstance(const std::string& nodeName, const std::string& specificMeshName)
	{
		/*for (int i = 0; i < 2; i++)	//Search 2 times; first look at the specific names and then at node names.
		{
			std::function<bool(const std::string&, const std::string&)> checkEqual = [](const std::string& str1, const std::string& str2) -> bool { return !str1.empty() && str1 == str2; };
			for (UniquePtr<MeshInstance>& it : MeshInstances)
				if ((i == 0 && (checkEqual(specificMeshName, it->GetMesh().GetLocalization().SpecificName)) || checkEqual(specificMeshName, it->GetMesh().GetLocalization().NodeName) ||
					(i == 1 && (checkEqual(nodeName, it->GetMesh().GetLocalization().SpecificName)) || checkEqual(nodeName, it->GetMesh().GetLocalization().NodeName))))
					return it.get();
		}*/

		if (nodeName.empty() && specificMeshName.empty())
			return nullptr;

		std::function<bool(const std::string&, const std::string&)> checkEqual = [](const std::string& str1, const std::string& str2) -> bool { return str1.empty() || str1 == str2; };
		for (UniquePtr<MeshInstance>& it : MeshInstances)
			if ((checkEqual(nodeName, it->GetMesh().GetLocalization().NodeName)) && checkEqual(specificMeshName, it->GetMesh().GetLocalization().SpecificName))
				return it.get();

		for (auto& it : Children)
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
		MeshInstances.push_back(MakeUnique<MeshInstance>(meshInst));
	}

	void ModelComponent::AddMeshInst(MeshInstance&& meshInst)
	{
		MeshInstances.push_back(MakeUnique<MeshInstance>(meshInst));
	}

	void ModelComponent::Update(float deltaTime)
	{
		for (auto& it : MeshInstances)
			if (it->GetMaterialInst())
				it->GetMaterialInst()->Update(deltaTime);

		if (Name == "MeshPreviewModel")
			;// ComponentTransform.SetRotation(glm::rotate(Mat4f(1.0f), (float)glfwGetTime(), Vec3f(0.0f, 1.0f, 0.0f)));
	}

	void ModelComponent::Render(const SceneMatrixInfo& info, Shader* shader)
	{
		if (GetHide() || IsBeingKilled() || MeshInstances.empty())
			return;

		bool anyMeetsShaderRequirements = false;
		for (auto& it : MeshInstances)
			if (!info.GetRequiredShaderInfo().IsValid() || (it->GetMaterialPtr() && it->GetMaterialPtr()->GetShaderInfo().MatchesRequiredInfo(info.GetRequiredShaderInfo())))
				anyMeetsShaderRequirements = true;

		if (!anyMeetsShaderRequirements)
			return;
			
		std::vector<MeshInstance> meshInstances;
		std::transform(MeshInstances.begin(), MeshInstances.end(), std::back_inserter(meshInstances), [](UniquePtr<MeshInstance>& instVec) { return *instVec; });

		if (SkelInfo && SkelInfo->GetBoneCount() > 0)
			SkeletalMeshRenderer(*GameHandle->GetRenderEngineHandle()).SkeletalMeshInstances(info, meshInstances, *SkelInfo, GetTransform().GetWorldTransform(), *shader);
		else
		{
			Renderer(*GameHandle->GetRenderEngineHandle()).StaticMeshInstances((CanvasPtr) ? (CanvasPtr->BindForRender(info)) : (info), meshInstances, GetTransform().GetWorldTransform(), *shader, RenderAsBillboard);
			if (CanvasPtr)
				CanvasPtr->UnbindForRender();
		}
	}

	void ModelComponent::GetEditorDescription(ComponentDescriptionBuilder descBuilder)
	{
		RenderableComponent::GetEditorDescription(descBuilder);

		descBuilder.AddField("Render as billboard").GetTemplates().TickBox(RenderAsBillboard);

		UICanvasFieldCategory& cat = descBuilder.GetCanvas().AddCategory("Mesh instances (" + std::to_string(MeshInstances.size()) + ")");
		cat.GetTemplates().ListSelection<UniquePtr<MeshInstance>>(MeshInstances.begin(), MeshInstances.end(), [this, descBuilder](UIAutomaticListActor& listActor, UniquePtr<MeshInstance>& meshInst)
			{
				std::string name = meshInst->GetMesh().GetLocalization().NodeName + " (" + meshInst->GetMesh().GetLocalization().SpecificName + ")";
				auto& meshButton = listActor.CreateChild<UIButtonActor>(name + "Button", name, [this, descBuilder, &meshInst]() mutable {
					UIWindowActor& window = dynamic_cast<UICanvasActor*>(&descBuilder.GetCanvas())->CreateChildCanvas<UIWindowActor>("MeshViewport");
					window.SetTransform(Transform(Vec2f(0.0f), Vec2f(0.5f)));

					GameScene& meshPreviewScene = GameHandle->CreateScene("GEE_Mesh_Preview_Scene");

					LightProbeComponent& probe = meshPreviewScene.GetRootActor()->CreateComponent<LightProbeComponent>("PreviewLightProbe");
					LightProbeLoader::LoadLightProbeFromFile(probe, "Assets/Editor/winter_lake_01_4k.hdr");

					ModelComponent& model = meshPreviewScene.CreateActorAtRoot<Actor>("MeshPreviewActor").CreateComponent<ModelComponent>("MeshPreviewModel");
					model.AddMeshInst(*meshInst);

					Actor& camActor = meshPreviewScene.CreateActorAtRoot<Actor>("MeshPreviewCameraActor");
					CameraComponent& cam = camActor.CreateComponent<CameraComponent>("MeshPreviewCamera", glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 100.0f));
					{
						auto meshBB = meshInst->GetMesh().GetBoundingBox();
						camActor.GetTransform()->SetPosition(meshBB.Position + glm::normalize(Vec3f(1.0f, 1.0f, 1.0f)) * glm::length(meshBB.Size) * 1.5f);
						camActor.GetTransform()->SetRotation(quatFromDirectionVec(glm::normalize(meshBB.Position - camActor.GetTransform()->GetPos())));
						meshPreviewScene.BindActiveCamera(&cam);
					}
					auto& camController = camActor.CreateChild<FreeRoamingController>("MeshPreviewCameraController");
					camController.SetPossessedActor(&camActor);

					UIButtonActor& viewportButton = window.CreateChild<UIButtonActor>("MeshPreviewViewportActor", [this, &meshPreviewScene, &camController]() { std::cout << "VIEWPORT WCISIETY\n"; GameHandle->SetActiveScene(&meshPreviewScene); GameHandle->PassMouseControl(&camController); });

					RenderEngineManager& renderHandle = *GameHandle->GetRenderEngineHandle();

					GameSettings* settings = new GameSettings(*GameHandle->GetGameSettings());

					settings->Video.Resolution = Vec2u(1024);
					RenderToolboxCollection& renderTbCollection = renderHandle.AddRenderTbCollection(RenderToolboxCollection("GEE_E_Mesh_Preview_Toolbox_Collection", settings->Video, *GameHandle->GetRenderEngineHandle()));

					SharedPtr<Material> viewportMaterial = MakeShared<Material>("GEE_E_Mesh_Preview_Viewport");
					renderHandle.AddMaterial(viewportMaterial);
					viewportMaterial->AddTexture(MakeShared<NamedTexture>(renderTbCollection.GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorTexture(0), "albedo1"));
					viewportButton.SetMatIdle(viewportMaterial);
					viewportButton.SetMatClick(viewportMaterial);
					viewportButton.SetMatHover(viewportMaterial);

					{
						std::stringstream boxSizeStream;
						boxSizeStream << meshInst->GetMesh().GetBoundingBox().Size;
						window.CreateChild<UIActorDefault>("MeshSizeTextActor").CreateComponent<TextConstantSizeComponent>("MeshSizeText", Transform(Vec2f(1.0f, -1.0f), Vec2f(0.2f)), "Size: " + boxSizeStream.str(), "", Alignment2D::RightBottom());
					}

					window.SetOnCloseFunc([&meshPreviewScene, &renderHandle, viewportMaterial, &renderTbCollection]() { meshPreviewScene.MarkAsKilled();  renderHandle.EraseMaterial(*viewportMaterial); renderHandle.EraseRenderTbCollection(renderTbCollection); });
				});

				meshButton.SetPopupCreationFunc([this, descBuilder, treePtr = GameHandle->FindHierarchyTree(meshInst->GetMesh().GetLocalization().GetTreeName())](PopupDescription popupDesc) { popupDesc.AddOption("Open hierarchy tree", [this, descBuilder, treePtr]() mutable { descBuilder.GetEditorHandle().GetActions().PreviewHierarchyTree(*treePtr); }); });

				auto& materialButton = listActor.CreateChild<UIButtonActor>("EditMaterialButton", (meshInst->GetMaterialPtr()) ? (meshInst->GetMaterialPtr()->GetName()) : ("No material"), nullptr, Transform(Vec2f(2.5f, 0.0f), Vec2f(3.0f, 1.0f)));

				materialButton.SetOnClickFunc([&meshInst, descBuilder, name]() mutable { 
					UIWindowActor& matWindow = descBuilder.GetEditorScene().CreateActorAtRoot<UIWindowActor>(name + "MaterialWindow");
					EditorDescriptionBuilder descBuilder(descBuilder.GetEditorHandle(), matWindow, matWindow);
					
					if (!meshInst->GetMaterialPtr())
						std::cout << "Mesh inst doesn't have material ptr!\n";
					meshInst->GetMaterialPtr()->GetEditorDescription(descBuilder);
					matWindow.AutoClampView();
					matWindow.RefreshFieldsList();
				});

				if (!meshInst->GetMaterialPtr())
					materialButton.SetDisableInput(true);
			});


		descBuilder.AddField("Override materials").GetTemplates().ObjectInput<Material>(
			[this]() {  auto materials = GameHandle->GetRenderEngineHandle()->GetMaterials(); std::vector<Material*> materialsPtr; materialsPtr.resize(materials.size()); std::transform(materials.begin(), materials.end(), materialsPtr.begin(), [](SharedPtr<Material> mat) { return mat.get(); }); return materialsPtr; },
			[this](Material* material) {  auto materials = GameHandle->GetRenderEngineHandle()->GetMaterials(); auto found = std::find_if(materials.begin(), materials.end(), [material](SharedPtr<Material> matVec) { return matVec.get() == material; }); OverrideInstancesMaterial(*found); });
	}


	template<typename Archive>
	void ModelComponent::Save(Archive& archive) const
	{
		archive(CEREAL_NVP(RenderAsBillboard), CEREAL_NVP(MeshInstances), cereal::make_nvp("SkelInfoBatchID", (SkelInfo) ? (SkelInfo->GetBatchPtr()->GetBatchID()) : (-1)), cereal::make_nvp("SkelInfoID", (SkelInfo) ? (SkelInfo->GetInfoID()) : (-1)), cereal::make_nvp("RenderableComponent", cereal::base_class<RenderableComponent>(this)));
	}

	template<typename Archive>
	void ModelComponent::Load(Archive& archive)	//Assumptions: Order of batches and their Skeletons is not changed during loading.
	{
		int skelInfoBatchID, skelInfoID;
		archive(CEREAL_NVP(RenderAsBillboard), CEREAL_NVP(MeshInstances), cereal::make_nvp("SkelInfoBatchID", skelInfoBatchID), cereal::make_nvp("SkelInfoID", skelInfoID), cereal::make_nvp("RenderableComponent", cereal::base_class<RenderableComponent>(this)));
		if (skelInfoID >= 0)
		{
			std::cout << "!!!Name of scene: " << Scene.GetName() << "\n";
			std::cout << "!!!Nr of batches: " << Scene.GetRenderData()->SkeletonBatches.size() << "\n";
			SetSkeletonInfo(Scene.GetRenderData()->GetBatch(skelInfoBatchID)->GetInfo(skelInfoID));
			std::cout << "Setting skelinfo of " << Name << " to " << skelInfoBatchID << ", " << skelInfoID << '\n';
		}
	}

	ModelComponent::~ModelComponent()
	{
		if (SkelInfo)
			SkelInfo->EraseModelCompRef(*this);
	}

	unsigned int ModelComponent::GetUIDepth() const
	{
		return GetElementDepth();
	}

	void ModelComponent::SignalSkeletonInfoDeath()
	{
		SkelInfo = nullptr;
	}

	template void ModelComponent::Save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const;
	template void ModelComponent::Load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);
}