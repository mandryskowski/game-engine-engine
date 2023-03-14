#include <scene/Actor.h>
#include <rendering/LightProbe.h>
#include <scene/RenderableComponent.h>
#include <scene/LightComponent.h>
#include <scene/LightProbeComponent.h>
#include <scene/SoundSourceComponent.h>
#include <scene/CameraComponent.h>
#include <game/GameScene.h>
#include <physics/CollisionObject.h>
#include <scene/hierarchy/HierarchyTree.h>
#include <animation/SkeletonInfo.h>
#include <UI/UICanvas.h>

#include <input/InputDevicesStateRetriever.h>
#include <UI/UICanvasActor.h>
#include <editor/EditorMessageLogger.h>

namespace GEE
{
	GameScene::GameScene(GameManager& gameHandle, const std::string& name):
		RenderData(MakeUnique<GameSceneRenderData>(*this, false)),
		PhysicsData(MakeUnique<Physics::GameScenePhysicsData>(*this)),
		AudioData(MakeUnique<Audio::GameSceneAudioData>(*this)),
		UIData(nullptr),
		Name(name),
		ActiveCamera(nullptr),
		GameHandle(&gameHandle),
		KillingProcessFrame(0),
		bHasStarted(false)
	{
		RootActor = MakeUnique<Actor>(*this, nullptr, "SceneRoot");
	}
	GameScene::GameScene(GameManager& gameHandle, const std::string& name, bool isAnUIScene, SystemWindow& associatedWindow) :
		RenderData(MakeUnique<GameSceneRenderData>(*this, isAnUIScene)),
		PhysicsData(MakeUnique<Physics::GameScenePhysicsData>(*this)),
		AudioData(MakeUnique<Audio::GameSceneAudioData>(*this)),
		UIData(MakeUnique<GameSceneUIData>(*this, associatedWindow)),
		Name(name),
		ActiveCamera(nullptr),
		GameHandle(&gameHandle),
		KillingProcessFrame(0),
		bHasStarted(false)
	{
		RootActor = MakeUnique<Actor>(*this, nullptr, "SceneRoot");
	}

	GameScene::GameScene(GameScene&& scene) :
		RootActor(nullptr),
		RenderData(std::move(scene.RenderData)),
		PhysicsData(std::move(scene.PhysicsData)),
		AudioData(std::move(scene.AudioData)),
		Name(scene.Name),
		ActiveCamera(scene.ActiveCamera),
		GameHandle(scene.GameHandle),
		KillingProcessFrame(scene.KillingProcessFrame),
		bHasStarted(scene.bHasStarted)
	{
		RootActor = MakeUnique<Actor>(*this, nullptr, "SceneRoot");
	}

	void GameScene::MarkAsStarted()
	{
		if (bHasStarted)
			return;

		bHasStarted = true;
		RootActor->OnStartAll();
	}

	const std::string& GameScene::GetName() const
	{
		return Name;
	}

	Actor* GameScene::GetRootActor()
	{
		return RootActor.get();
	}

	const Actor* GameScene::GetRootActor() const
	{
		return RootActor.get();
	}

	CameraComponent* GameScene::GetActiveCamera()
	{
		return ActiveCamera;
	}

	GameSceneRenderData* GameScene::GetRenderData()
	{
		return RenderData.get();
	}

	const GameSceneRenderData* GameScene::GetRenderData() const
	{
		return RenderData.get();
	}

	Physics::GameScenePhysicsData* GameScene::GetPhysicsData()
	{
		return PhysicsData.get();
	}

	Audio::GameSceneAudioData* GameScene::GetAudioData()
	{
		return AudioData.get();
	}

	GameSceneUIData* GameScene::GetUIData()
	{
		return UIData.get();
	}

	GameManager* GameScene::GetGameHandle()
	{
		return GameHandle;
	}

	EventPusher GameScene::GetEventPusher()
	{
		return GameHandle->GetEventPusher(*this);
	}

	bool GameScene::IsBeingKilled() const
	{
		return KillingProcessFrame != 0;
	}

	int GameScene::GetHierarchyTreeCount() const
	{
		return static_cast<int>(HierarchyTrees.size());
	}

	Hierarchy::Tree* GameScene::GetHierarchyTree(int index)
	{
		if (index > GetHierarchyTreeCount() - 1 || index < 0)
			return nullptr;

		return HierarchyTrees[index].get();
	}

	void GameScene::AddPostLoadLambda(std::function<void()> postLoadLambda)
	{
		PostLoadLambdas.push_back(postLoadLambda);
	}

	std::string GameScene::GetUniqueActorName(const std::string& name) const
	{
		std::vector <Actor*> actorsWithSimilarNames;
		bool sameNameExists = false;
		std::function<void(Actor&)> getActorsWithSimilarNamesFunc = [&actorsWithSimilarNames, name, &getActorsWithSimilarNamesFunc, &sameNameExists](Actor& currentActor) { if (currentActor.GetName().rfind(name, 0) == 0) /* if currentActor's name stars with name */ { actorsWithSimilarNames.push_back(&currentActor); if (currentActor.GetName() == name) sameNameExists = true; }	for (auto& it : currentActor.GetChildren()) getActorsWithSimilarNamesFunc(*it); };

		getActorsWithSimilarNamesFunc(*RootActor);
		if (!sameNameExists)
			return name;

		return GetUniqueName(actorsWithSimilarNames.begin(), actorsWithSimilarNames.end(), [](Actor* actor) { return actor->GetName(); }, name);
	}

	Actor& GameScene::AddActorToRoot(UniquePtr<Actor> actor)
	{
		return RootActor->AddChild(std::move(actor));
	}

	Hierarchy::Tree& GameScene::CreateHierarchyTree(const std::string& name)
	{
		HierarchyTrees.push_back(MakeUnique<Hierarchy::Tree>(Hierarchy::Tree(*this, Hierarchy::HierarchyLocalization::Filepath(name))));
		std::cout << "Utworzono drzewo z root " << &HierarchyTrees.back()->GetRoot() << " - " << HierarchyTrees.back()->GetName().GetPath() << '\n';
		return *HierarchyTrees.back();
	}

	Hierarchy::Tree& GameScene::CopyHierarchyTree(const Hierarchy::Tree& tree)
	{
		HierarchyTrees.push_back(MakeUnique<Hierarchy::Tree>(tree));
		return *HierarchyTrees.back();
	}

	Hierarchy::Tree* GameScene::FindHierarchyTree(const std::string& name, Hierarchy::Tree* treeToIgnore)
	{
		auto found = std::find_if(HierarchyTrees.begin(), HierarchyTrees.end(), [name, treeToIgnore](const UniquePtr<Hierarchy::Tree>& tree) { return tree->GetName().GetPath() == name && tree.get() != treeToIgnore; });
		if (found != HierarchyTrees.end())
			return (*found).get();

		return nullptr;
	}

	void GameScene::HandleEventAll(const Event& ev)
	{
		if (ev.GetEventRoot() && &ev.GetEventRoot()->GetScene() != this)
			return;

		if (UIData)
			UIData->HandleEventAll(ev);

		if (ev.GetEventRoot())
			const_cast<Actor*>(ev.GetEventRoot())->HandleEventAll(ev);
		else
			RootActor->HandleEventAll(ev);
	}

	void GameScene::Update(Time deltaTime)
	{
		if (IsBeingKilled())
		{
			Delete();
			return;
		}
		if (ActiveCamera && ActiveCamera->IsBeingKilled())
			BindActiveCamera(nullptr);

		RootActor->UpdateAll(deltaTime);
		for (auto& it : RenderData->SkeletonBatches)
			it->VerifySkeletonsLives();	//verify if any SkeletonInfos are invalid and get rid of any garbage objects
	}

	void GameScene::BindActiveCamera(CameraComponent* cam)
	{
		ActiveCamera = cam;
	}

	Actor* GameScene::FindActor(std::string name)
	{
		return RootActor->FindActor(name);
	}

	Actor* GameScene::FindActor(GEEID geeid)
	{
		return RootActor->FindActor(geeid);
	}

	template<typename Archive>
	inline void GameScene::Save(Archive& archive) const
	{
		try
		{
			GetRenderData()->SaveSkeletonBatches(archive);

			/* Save only the local hierarchy trees.
			*  We use a hack to get a different vector containing only unique pointers.
			*  They have to be unique, since cereal cannot serialize raw pointers - only the smart ones.
			*  This looks ugly, but no trees are destroyed since we release the pointers.
			*/
			{
				std::vector<UniquePtr<Hierarchy::Tree>> hierarchyTrees;
				hierarchyTrees.reserve(HierarchyTrees.size());

				for (auto& treePtr : HierarchyTrees)
					if (treePtr->GetName().IsALocalResource())
						hierarchyTrees.push_back(UniquePtr<Hierarchy::Tree>(treePtr.get()));

				archive(cereal::make_nvp("HierarchyTrees", hierarchyTrees));

				for (auto& hackedPtr : hierarchyTrees)
					hackedPtr.release();
			}

			GetRootActor()->Save(archive);
			GameHandle->GetGameSettings()->Serialize(archive);
		}
		catch (cereal::Exception& ex)
		{
			std::cout << "ERROR While saving scene: " << ex.what() << '\n';
			if (Editor::EditorManager* editorHandle = dynamic_cast<Editor::EditorManager*>(GameHandle))
				editorHandle->GetEditorLogger(*GameHandle->GetScene("GEE_Editor")).Log(ex.what(), Editor::EditorMessageLogger::MessageType::Error);
		}
	}
	template<typename Archive>
	inline void GameScene::Load(Archive& archive)
	{
		for (auto& it : PostLoadLambdas)
			it();
	}

	template void GameScene::Save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const;
	template void GameScene::Load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

	GameScene::~GameScene()
	{

	}

	void GameScene::MarkAsKilled()
	{
		KillingProcessFrame = GameHandle->GetTotalFrameCount();
		if (RootActor)
			RootActor->MarkAsKilled();

		if (PhysicsData)	GameHandle->GetPhysicsHandle()->RemoveScenePhysicsDataPtr(*PhysicsData);
		if (RenderData)		GameHandle->GetRenderEngineHandle()->RemoveSceneRenderDataPtr(*RenderData);
	}

	void GameScene::Delete()
	{
		GameHandle->DeleteScene(*this);
	}

	GameSceneData::GameSceneData(GameScene& scene) :
		Scene(scene)
	{
	}

	std::string GameSceneData::GetSceneName() const
	{
		return Scene.GetName();
	}

	GameScene& GameSceneData::GetScene()
	{
		return Scene;
	}

	const GameScene& GameSceneData::GetScene() const
	{
		return Scene;
	}

	GameSceneRenderData::GameSceneRenderData(GameScene& scene, bool isAnUIScene) :
		GameSceneData(scene),
		RenderHandle(scene.GetGameHandle()->GetRenderEngineHandle()),
		ProbeTexArrays(MakeShared<LightProbeTextureArrays>(LightProbeTextureArrays())),
		LightBlockBindingSlot(-1),
		ProbesLoaded(false),
		bIsAnUIScene(isAnUIScene),
		bUIRenderableDepthsSortedDirtyFlag(false),
		bLightProbesSortedDirtyFlag(false)
	{
		if (LightProbes.empty() && RenderHandle->GetShadingModel() == ShadingAlgorithm::SHADING_PBR_COOK_TORRANCE)
		{
			//LightProbeLoader::LoadLightProbeTextureArrays(this);
			ProbeTexArrays->IrradianceMapArr.Bind(12);
			ProbeTexArrays->PrefilterMapArr.Bind(13);
			ProbeTexArrays->BRDFLut.Bind(14);
			glActiveTexture(GL_TEXTURE0);
		}
	}

	RenderEngineManager* GameSceneRenderData::GetRenderHandle() const
	{
		return RenderHandle;
	}

	LightProbeTextureArrays* GameSceneRenderData::GetProbeTexArrays() const
	{
		return ProbeTexArrays.get();
	}

	int GameSceneRenderData::GetAvailableLightIndex() const
	{
		return static_cast<int>(Lights.size());
	}

	bool GameSceneRenderData::ContainsLights() const
	{
		return !Lights.empty();
	}

	bool GameSceneRenderData::ContainsLightProbes() const
	{
		return !LightProbes.empty();
	}

	bool GameSceneRenderData::HasLightWithoutShadowMap() const
	{
		for (auto& it : Lights)
			if (!it.get().HasValidShadowMap())
				return true;

		return false;
	}

	void GameSceneRenderData::InvalidateAllShadowmaps()
	{
		for (auto light : Lights)
			light.get().InvalidateCache();
	}


	void GameSceneRenderData::AddRenderable(Renderable& renderable)
	{
		//if (bIsAnUIScene)
			//insertSorted(Renderables, &renderable, [](Renderable* value, Renderable* vecElement) { return value->GetUIDepth() < vecElement->GetUIDepth(); });
		//else
		Renderables.push_back(&renderable);
		if (bIsAnUIScene)
			MarkUIRenderableDepthsDirty();
		/*	std::cout << "***POST-ADD RENDERABLES***\n";
			i = 0;
			for (auto it : Renderables)
				std::cout << ++i << ". " << it << "### Depth:" << it->GetUIDepth() << '\n';*/
				//std::cout << "Adding renderable " << renderable.GetName() << " " << &renderable << "\n";
	}

	void GameSceneRenderData::AddLight(LightComponent& light)
	{
		Lights.push_back(light);
		for (int i = 0; i < static_cast<int>(Lights.size()); i++)
			Lights[i].get().SetIndex(i);
		if (LightBlockBindingSlot != -1)	//If light block was already set up, do it again because there isn't enough space for the new light.
			SetupLights(LightBlockBindingSlot);
	}

	void GameSceneRenderData::AddLightProbe(LightProbeComponent& probe)
	{
		LightProbes.push_back(&probe);
		for (int i = 0; i < static_cast<int>(LightProbes.size()); i++)
			LightProbes[i]->SetProbeIndex(i);

		bLightProbesSortedDirtyFlag = true;

		if (!LightsBuffer.HasBeenGenerated())
			SetupLights(LightBlockBindingSlot);
	}

	SharedPtr<SkeletonInfo> GameSceneRenderData::AddSkeletonInfo()
	{
		SharedPtr<SkeletonInfo> info = MakeShared<SkeletonInfo>(SkeletonInfo());
		if (SkeletonBatches.empty() || !SkeletonBatches.back()->AddSkeleton(info))
		{
			SkeletonBatches.push_back(MakeShared<SkeletonBatch>());
			if (!SkeletonBatches.back()->AddSkeleton(info))
				std::cerr << "CRITICAL ERROR! Too many bones in SkeletonInfo for a SkeletonBatch\n";
		}
		return info;
	}

	void GameSceneRenderData::EraseRenderable(Renderable& renderable)
	{
		Renderables.erase(std::remove_if(Renderables.begin(), Renderables.end(), [&renderable](Renderable* renderableVec) {return renderableVec == &renderable; }), Renderables.end());
		//std::cout << "Erasing renderable " << renderable.GetName() << " " << &renderable << "\n";
	}

	void GameSceneRenderData::EraseLight(LightComponent& light)
	{
		Lights.erase(std::remove_if(Lights.begin(), Lights.end(), [&light](std::reference_wrapper<LightComponent>& lightVec) {return &lightVec.get() == &light; }), Lights.end());
		for (int i = 0; i < static_cast<int>(Lights.size()); i++)
			Lights[i].get().SetIndex(i);
		if (LightBlockBindingSlot != -1)	//If light block was already set up, do it again because there isn't enough space for the new light.
			SetupLights(LightBlockBindingSlot);
	}

	void GameSceneRenderData::EraseLightProbe(LightProbeComponent& lightProbe)
	{
		LightProbes.erase(std::remove_if(LightProbes.begin(), LightProbes.end(), [&lightProbe](LightProbeComponent* lightProbeVec) {return lightProbeVec == &lightProbe; }), LightProbes.end());
	}

	std::vector<UniquePtr<RenderableVolume>> GameSceneRenderData::GetSceneLightsVolumes() const
	{
		std::vector<UniquePtr<RenderableVolume>> lightsVolumes;
		lightsVolumes.resize(Lights.size());
		std::transform(Lights.begin(), Lights.end(), lightsVolumes.begin(), [](const std::reference_wrapper<LightComponent>& light) { return MakeUnique<LightVolume>(light.get()); });

		return lightsVolumes;
	}

	std::vector<UniquePtr<RenderableVolume>> GameSceneRenderData::GetLightProbeVolumes(bool putGlobalProbeAtEnd)
	{
		//1. Ensure that probes are sorted so the global probe is the last element of the vector (optional)
		if (putGlobalProbeAtEnd && bLightProbesSortedDirtyFlag)
		{
			std::sort(LightProbes.begin(), LightProbes.end(), [](LightProbeComponent* lhs, LightProbeComponent* rhs) { return rhs->IsGlobalProbe() && !lhs->IsGlobalProbe(); });	//move global probes to the end
			for (auto& it : LightProbes)
				std::cout << "Probe " << it->IsGlobalProbe() << "\n";
			bLightProbesSortedDirtyFlag = false;
		}
		//2. Get volumes vector
		std::vector<UniquePtr<RenderableVolume>> probeVolumes;
		probeVolumes.resize(LightProbes.size());
		std::transform(LightProbes.begin(), LightProbes.end(), probeVolumes.begin(), [](LightProbeComponent* probe) {return MakeUnique<LightProbeVolume>(*probe); });

		return probeVolumes;
	}

	void GameSceneRenderData::MarkUIRenderableDepthsDirty()
	{
		bUIRenderableDepthsSortedDirtyFlag = true;
	}

	void GameSceneRenderData::SetupLights(unsigned int blockBindingSlot)
	{
		LightBlockBindingSlot = blockBindingSlot;
		std::cout << "Setupping lights for bbindingslot " << blockBindingSlot << '\n';

		LightsBuffer.Generate(blockBindingSlot, sizeof(Vec4f) * 2 + Lights.size() * 192);
		LightsBuffer.SubData1i((int)Lights.size(), (size_t)0);

		for (auto& light : Lights)
		{
			if (!light.get().IsBeingKilled())
				light.get().InvalidateCache();
		}

		//UpdateLightUniforms();
	}

	void GameSceneRenderData::UpdateLightUniforms()
	{
		LightsBuffer.offsetCache = sizeof(Vec4f) * 2;
		for (auto& light : Lights)
		{
			light.get().InvalidateCache(false);
			light.get().UpdateUBOData(&LightsBuffer);
		}
	}

	int GameSceneRenderData::GetBatchID(SkeletonBatch& batch) const
	{
		for (int i = 0; i < static_cast<int>(SkeletonBatches.size()); i++)
			if (SkeletonBatches[i].get() == &batch)
				return i;

		return -1;	//batch not found; return invalid ID
	}

	SkeletonBatch* GameSceneRenderData::GetBatch(int ID)
	{
		if (ID < 0 || ID > SkeletonBatches.size() - 1)
			return nullptr;

		return SkeletonBatches[ID].get();
	}

	GameSceneRenderData::~GameSceneRenderData()
	{
		LightsBuffer.Dispose();
	}

	void GameSceneRenderData::AssertThatUIRenderablesAreSorted()
	{
		if (!bUIRenderableDepthsSortedDirtyFlag)
			return;

		std::stable_sort(Renderables.begin(), Renderables.end(), [](Renderable* lhs, Renderable* rhs) { return lhs->GetUIDepth() < rhs->GetUIDepth(); });
		bUIRenderableDepthsSortedDirtyFlag = false;
	}

	/*
	RenderableComponent* GameSceneRenderData::FindRenderable(std::string name)
	{
		auto found = std::find_if(Renderables.begin(), Renderables.end(), [name](Renderable* comp) { return comp->GetName() == name; });
		if (found != Renderables.end())
			return *found;

		return nullptr;
	}*/

	namespace Physics
	{
		GameScenePhysicsData::GameScenePhysicsData(GameScene& scene) :
			GameSceneData(scene),
			PhysicsHandle(scene.GetGameHandle()->GetPhysicsHandle()),
			WasSetup(false)
		{
		}

		void GameScenePhysicsData::AddCollisionObject(CollisionObject& object, Transform& t)	//tytaj!!!!!!!!!!!!!!!!!!!!!!!!!!!! TUTAJ TUTAJ TU
		{
			CollisionObjects.push_back(&object);
			object.ScenePhysicsData = this;
			object.TransformPtr = &t;
			object.TransformDirtyFlag = t.AddWorldDirtyFlag();

			if (WasSetup)
				PhysicsHandle->AddCollisionObjectToPxPipeline(*this, object);
		}

		void GameScenePhysicsData::EraseCollisionObject(CollisionObject& object)
		{
			auto found = std::find(CollisionObjects.begin(), CollisionObjects.end(), &object);
			if (found != CollisionObjects.end())
			{
				if ((*found)->ActorPtr)
				{
					(*found)->ActorPtr->release();
					(*found)->ActorPtr = nullptr;
				}
				CollisionObjects.erase(found);
			}
		}

		PhysicsEngineManager* GameScenePhysicsData::GetPhysicsHandle()
		{
			return PhysicsHandle;
		}
	}

	namespace Audio
	{
		GameSceneAudioData::GameSceneAudioData(GameScene& scene) :
			GameSceneData(scene),
			AudioHandle(scene.GetGameHandle()->GetAudioEngineHandle())
		{
		}

		void GameSceneAudioData::AddSource(SoundSourceComponent& sourceComp)
		{
			Sources.push_back(sourceComp);
		}

		SoundSourceComponent* GameSceneAudioData::FindSource(std::string name)
		{
			auto found = std::find_if(Sources.begin(), Sources.end(), [name](std::reference_wrapper<SoundSourceComponent>& sourceComp) { return sourceComp.get().GetName() == name; });
			if (found != Sources.end())
				return &found->get();

			return nullptr;
		}
	}

	GameSceneUIData::GameSceneUIData(GameScene& scene, SystemWindow& window):
		GameSceneData(scene),
		CurrentBlockingCanvas(nullptr),
		AssociatedWindow(&window)
	{
	}

	InputDevicesStateRetriever GameSceneUIData::GetInputRetriever() const
	{
		return InputDevicesStateRetriever(*AssociatedWindow);
	}

	WindowData GameSceneUIData::GetWindowData()
	{
		return WindowData(*AssociatedWindow);
	}

	SystemWindow* GameSceneUIData::GetWindow()
	{
		return AssociatedWindow;
	}

	void GameSceneUIData::AddBlockingCanvas(UICanvas& canvas)
	{
		BlockingCanvases.push_back(&canvas);
		std::sort(BlockingCanvases.begin(), BlockingCanvases.end(), [](UICanvas* lhs, UICanvas* rhs) { return lhs->GetCanvasDepth() < rhs->GetCanvasDepth(); });
	}

	void GameSceneUIData::EraseBlockingCanvas(UICanvas& canvas)
	{
		BlockingCanvases.erase(std::remove_if(BlockingCanvases.begin(), BlockingCanvases.end(), [this, &canvas](UICanvas* canvasVec) { bool matches = canvasVec == &canvas; if (matches && CurrentBlockingCanvas == canvasVec) CurrentBlockingCanvas = nullptr; return matches; }), BlockingCanvases.end());	//should still be sorted
	}

	UICanvas* GameSceneUIData::GetCurrentBlockingCanvas() const
	{
		return CurrentBlockingCanvas;
	}

	void GameSceneUIData::HandleEventAll(const Event& ev)
	{
		const Vec2f mouseNDC = GetWindowData().GetMousePositionNDC();

		auto found = std::find_if(BlockingCanvases.rbegin(), BlockingCanvases.rend(), [&mouseNDC](UICanvas* canvas) { return canvas->ContainsMouseCheck(mouseNDC) && !(dynamic_cast<UICanvasActor*>(canvas)->IsBeingKilled()); });
		if (found != BlockingCanvases.rend())
			CurrentBlockingCanvas = *found;
		else
			CurrentBlockingCanvas = nullptr;
	}


}