#include <scene/Actor.h>
#include <scene/BoneComponent.h>
#include <rendering/LightProbe.h>
#include <scene/RenderableComponent.h>
#include <scene/LightComponent.h>
#include <scene/SoundSourceComponent.h>
#include <game/GameScene.h>
#include <physics/CollisionObject.h>

GameScene::GameScene(GameManager& gameHandle) :
	RenderData(std::make_unique<GameSceneRenderData>(GameSceneRenderData(gameHandle.GetRenderEngineHandle()))),
	PhysicsData(std::make_unique<GameScenePhysicsData>(GameScenePhysicsData(gameHandle.GetPhysicsHandle()))),
	AudioData(std::make_unique<GameSceneAudioData>(GameSceneAudioData(gameHandle.GetAudioEngineHandle()))),
	GameHandle(&gameHandle)
{
	RootActor = std::make_unique<Actor>(Actor(*this, "SceneRoot"));
}

GameScene::GameScene(GameScene&& scene):
	RootActor(nullptr),
	RenderData(std::move(scene.RenderData)),
	PhysicsData(std::move(scene.PhysicsData)),
	AudioData(std::move(scene.AudioData)),
	ActiveCamera(scene.ActiveCamera),
	GameHandle(scene.GameHandle)
{
	RootActor = std::make_unique<Actor>(Actor(*this, "SceneRoot"));
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

Actor& GameScene::AddActorToRoot(std::unique_ptr<Actor> actor)
{
	return RootActor->AddChild(std::move(actor));
}

void GameScene::BindActiveCamera(CameraComponent* cam)
{
	ActiveCamera = cam;
}

Actor* GameScene::FindActor(std::string name)
{
	return RootActor->FindActor(name);
}

GameSceneRenderData::GameSceneRenderData(RenderEngineManager* renderHandle) :
	RenderHandle(renderHandle),
	ProbeTexArrays(std::make_shared<LightProbeTextureArrays>(LightProbeTextureArrays())),
	LightBlockBindingSlot(-1),
	ProbesLoaded(false)
{
	if (LightProbes.empty() && RenderHandle->GetShadingModel() == ShadingModel::SHADING_PBR_COOK_TORRANCE)
	{
		LightProbeLoader::LoadLightProbeTextureArrays(this);
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


void GameSceneRenderData::AddRenderable(RenderableComponent& renderable)
{
	Renderables.push_back(renderable);
//	std::cout << "Adding renderable " << renderable.GetName() << " " << &renderable << "\n";
}

std::shared_ptr<LightProbe> GameSceneRenderData::AddLightProbe(std::shared_ptr<LightProbe> probe)
{
	LightProbes.push_back(probe);
	return probe;
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

void GameSceneRenderData::AddLight(LightComponent& light)
{
	Lights.push_back(light);
	if (LightBlockBindingSlot != -1)	//If light block was already set up, do it again because there isn't enough space for the new light.
		SetupLights(LightBlockBindingSlot);
}

void GameSceneRenderData::EraseRenderable(RenderableComponent& renderable)
{
	Renderables.erase(std::remove_if(Renderables.begin(), Renderables.end(), [&renderable](std::reference_wrapper<RenderableComponent>& renderableVec) {return &renderableVec.get() == &renderable; }), Renderables.end());
//	std::cout << "Erasing renderable " << renderable.GetName() << " " << &renderable << "\n";
}

void GameSceneRenderData::EraseLight(LightComponent& light)
{
	Lights.erase(std::remove_if(Lights.begin(), Lights.end(), [&light](std::reference_wrapper<LightComponent>& lightVec) {return &lightVec.get() == &light; }), Lights.end());
	if (LightBlockBindingSlot != -1)	//If light block was already set up, do it again because there isn't enough space for the new light.
		SetupLights(LightBlockBindingSlot);
}

void GameSceneRenderData::SetupLights(unsigned int blockBindingSlot)
{
	LightBlockBindingSlot = blockBindingSlot;

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
	std::cout << "Removing collision object " << &object << "\n";
	if (found != CollisionObjects.end())
	{
		std::cout << "Removed collision object " << &object << "\n";
		(*found)->ActorPtr->release();
		CollisionObjects.erase(found, CollisionObjects.end());
	}
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
