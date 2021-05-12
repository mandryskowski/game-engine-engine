#include <scene/Actor.h>
#include <scene/BoneComponent.h>
#include <rendering/LightProbe.h>
#include <scene/RenderableComponent.h>
#include <scene/LightComponent.h>
#include <scene/LightProbeComponent.h>
#include <scene/SoundSourceComponent.h>
#include <game/GameScene.h>
#include <physics/CollisionObject.h>
#include <scene/HierarchyTemplate.h>

GameScene::GameScene(GameManager& gameHandle, const std::string& name) :
	RenderData(std::make_unique<GameSceneRenderData>(GameSceneRenderData(gameHandle.GetRenderEngineHandle()))),
	PhysicsData(std::make_unique<GameScenePhysicsData>(GameScenePhysicsData(gameHandle.GetPhysicsHandle()))),
	AudioData(std::make_unique<GameSceneAudioData>(GameSceneAudioData(gameHandle.GetAudioEngineHandle()))),
	ActiveCamera(nullptr),
	Name(name),
	GameHandle(&gameHandle),
	bKillingProcessStarted(false)
{
	RootActor = std::make_unique<Actor>(Actor(*this, "SceneRoot"));
}

GameScene::GameScene(GameScene&& scene):
	RootActor(nullptr),
	RenderData(std::move(scene.RenderData)),
	PhysicsData(std::move(scene.PhysicsData)),
	AudioData(std::move(scene.AudioData)),
	ActiveCamera(scene.ActiveCamera),
	Name(scene.Name),
	GameHandle(scene.GameHandle),
	bKillingProcessStarted(scene.bKillingProcessStarted)
{
	RootActor = std::make_unique<Actor>(Actor(*this, "SceneRoot"));
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

GameScenePhysicsData* GameScene::GetPhysicsData()
{
	return PhysicsData.get();
}

GameSceneAudioData* GameScene::GetAudioData()
{
	return AudioData.get();
}

GameManager* GameScene::GetGameHandle()
{
	return GameHandle;
}

bool GameScene::IsBeingKilled() const
{
	return bKillingProcessStarted;
}

int GameScene::GetHierarchyTreeCount() const
{
	return static_cast<int>(HierarchyTrees.size());
}

HierarchyTemplate::HierarchyTreeT* GameScene::GetHierarchyTree(int index)
{
	if (index > GetHierarchyTreeCount() - 1 || index < 0)
		return nullptr;

	return HierarchyTrees[index].get();
}

void GameScene::AddPostLoadLambda(std::function<void()> postLoadLambda)
{
	PostLoadLambdas.push_back(postLoadLambda);
}

Actor& GameScene::AddActorToRoot(std::unique_ptr<Actor> actor)
{
	return RootActor->AddChild(std::move(actor));
}

HierarchyTemplate::HierarchyTreeT& GameScene::CreateHierarchyTree(const std::string& name)
{
	HierarchyTrees.push_back(std::make_unique<HierarchyTemplate::HierarchyTreeT>(HierarchyTemplate::HierarchyTreeT(*this, name)));
	std::cout << "Utworzono drzewo z root " << &HierarchyTrees.back()->GetRoot() << " - " << HierarchyTrees.back()->GetName() << '\n';
	return *HierarchyTrees.back();
}

HierarchyTemplate::HierarchyTreeT* GameScene::FindHierarchyTree(const std::string& name, HierarchyTemplate::HierarchyTreeT* treeToIgnore)
{
	auto found = std::find_if(HierarchyTrees.begin(), HierarchyTrees.end(), [name, treeToIgnore](const std::unique_ptr<HierarchyTemplate::HierarchyTreeT>& tree) { return tree->GetName() == name && tree.get() != treeToIgnore; });
	if (found != HierarchyTrees.end())
		return (*found).get();

	return nullptr;
}

void GameScene::Update(float deltaTime)
{
	if (IsBeingKilled())
	{
		Delete();
		return;
	}
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

void GameScene::MarkAsKilled()
{
	bKillingProcessStarted = true;
	if (RootActor)
		RootActor->MarkAsKilled();

	if (PhysicsData)	GameHandle->GetPhysicsHandle()->RemoveScenePhysicsDataPtr(*PhysicsData);
	if (RenderData)		GameHandle->GetRenderEngineHandle()->RemoveSceneRenderDataPtr(*RenderData);
}

void GameScene::Delete()
{
	GameHandle->DeleteScene(*this);
}

GameSceneRenderData::GameSceneRenderData(RenderEngineManager* renderHandle) :
	RenderHandle(renderHandle),
	ProbeTexArrays(std::make_shared<LightProbeTextureArrays>(LightProbeTextureArrays())),
	LightBlockBindingSlot(-1),
	ProbesLoaded(false)
{
	if (LightProbes.empty() && RenderHandle->GetShadingModel() == ShadingModel::SHADING_PBR_COOK_TORRANCE)
	{
		//LightProbeLoader::LoadLightProbeTextureArrays(this);
		ProbeTexArrays->IrradianceMapArr.Bind(12);
		ProbeTexArrays->PrefilterMapArr.Bind(13);
		ProbeTexArrays->BRDFLut.Bind(14);
		glActiveTexture(GL_TEXTURE0);
	}
}

RenderEngineManager* GameSceneRenderData::GetRenderHandle()
{
	return RenderHandle;
}

LightProbeTextureArrays* GameSceneRenderData::GetProbeTexArrays()
{
	return ProbeTexArrays.get();
}

int GameSceneRenderData::GetAvailableLightIndex()
{
	return Lights.size();
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


void GameSceneRenderData::AddRenderable(RenderableComponent& renderable)
{
	Renderables.push_back(renderable);
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
}

std::shared_ptr<SkeletonInfo> GameSceneRenderData::AddSkeletonInfo()
{
	std::shared_ptr<SkeletonInfo> info = std::make_shared<SkeletonInfo>(SkeletonInfo());
	if (SkeletonBatches.empty() || !SkeletonBatches.back()->AddSkeleton(info))
	{
		SkeletonBatches.push_back(std::make_shared<SkeletonBatch>());
		if (!SkeletonBatches.back()->AddSkeleton(info))
			std::cerr << "CRITICAL ERROR! Too many bones in SkeletonInfo for a SkeletonBatch\n";
	}
	return info;
}

void GameSceneRenderData::EraseRenderable(RenderableComponent& renderable)
{
	Renderables.erase(std::remove_if(Renderables.begin(), Renderables.end(), [&renderable](std::reference_wrapper<RenderableComponent>& renderableVec) {return &renderableVec.get() == &renderable; }), Renderables.end());
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

void GameSceneRenderData::SetupLights(unsigned int blockBindingSlot)
{
	LightBlockBindingSlot = blockBindingSlot;
	std::cout << "Setupping lights for bbindingslot " << blockBindingSlot << '\n';

	LightsBuffer.Generate(blockBindingSlot, sizeof(glm::vec4) * 2 + Lights.size() * 192);
	LightsBuffer.SubData1i((int)Lights.size(), (size_t)0);

	for (auto& light : Lights)
	{
		light.get().InvalidateCache();
	}

	//UpdateLightUniforms();
}

void GameSceneRenderData::UpdateLightUniforms()
{
	LightsBuffer.offsetCache = sizeof(glm::vec4) * 2;
	for (auto& light : Lights)
	{
		light.get().InvalidateCache();
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

RenderableComponent* GameSceneRenderData::FindRenderable(std::string name)
{
	auto found = std::find_if(Renderables.begin(), Renderables.end(), [name](const std::reference_wrapper<RenderableComponent>& comp) { if (name == "FireParticle") std::cout << comp.get().GetName() << "\n"; return comp.get().GetName() == name; });
	if (found != Renderables.end())
		return &found->get();

	return nullptr;
}

GameScenePhysicsData::GameScenePhysicsData(PhysicsEngineManager* physicsHandle) :
	PhysicsHandle(physicsHandle),
	WasSetup(false)
{
}

void GameScenePhysicsData::AddCollisionObject(CollisionObject& object, Transform& t)	//tytaj!!!!!!!!!!!!!!!!!!!!!!!!!!!! TUTAJ TUTAJ TU
{
	CollisionObjects.push_back(&object);
	object.ScenePhysicsData = this;
	object.TransformPtr = &t;

	if (WasSetup)
		PhysicsHandle->AddCollisionObjectToPxPipeline(*this, object);
}

void GameScenePhysicsData::EraseCollisionObject(CollisionObject& object)
{
	auto found = std::find(CollisionObjects.begin(), CollisionObjects.end(), &object);
	if (found != CollisionObjects.end())
	{
		if ((*found)->ActorPtr)
			(*found)->ActorPtr->release();
		CollisionObjects.erase(found);
	}
}

PhysicsEngineManager* GameScenePhysicsData::GetPhysicsHandle()
{
	return PhysicsHandle;
}

GameSceneAudioData::GameSceneAudioData(AudioEngineManager* audioHandle) :
	AudioHandle(audioHandle)
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
