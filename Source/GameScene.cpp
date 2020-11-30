#include "Actor.h"
#include "BoneComponent.h"
#include "LightProbe.h"
#include "RenderableComponent.h"
#include "LightComponent.h"
#include "SoundSourceComponent.h"
#include "GameScene.h"

GameScene::GameScene(GameManager* gameHandle) :
	RenderData(std::make_unique<GameSceneRenderData>(GameSceneRenderData(gameHandle->GetRenderEngineHandle()))),
	PhysicsData(std::make_unique<GameScenePhysicsData>(GameScenePhysicsData(gameHandle->GetPhysicsHandle()))),
	AudioData(std::make_unique<GameSceneAudioData>(GameSceneAudioData(gameHandle->GetAudioEngineHandle()))),
	GameHandle(gameHandle)
{
	RootActor = std::make_unique<Actor>(Actor(this, "SceneRoot"));
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

Actor* GameScene::AddActorToRoot(std::shared_ptr<Actor> actor)
{
	RootActor->AddChild(actor);
	return actor.get();
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
	ProbeTexArrays(std::make_shared<LightProbeTextureArrays>(LightProbeTextureArrays()))
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


std::shared_ptr<RenderableComponent> GameSceneRenderData::AddRenderable(std::shared_ptr<RenderableComponent> renderable)
{
	Renderables.push_back(renderable);
	return Renderables.back();
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
		SkeletonBatches.back()->AddSkeleton(info);
	}
	return info;
}

void GameSceneRenderData::AddLight(std::shared_ptr<LightComponent> light)
{
	Lights.push_back(light);
}

void GameSceneRenderData::SetupLights(unsigned int blockBindingSlot)
{
	LightsBuffer.Generate(blockBindingSlot, sizeof(glm::vec4) * 2 + Lights.size() * 192);
	LightsBuffer.SubData1i((int)Lights.size(), (size_t)0);

	UpdateLightUniforms();
}

void GameSceneRenderData::UpdateLightUniforms()
{
	LightsBuffer.offsetCache = sizeof(glm::vec4) * 2;
	for (int i = 0; i < static_cast<int>(Lights.size()); i++)
		Lights[i]->UpdateUBOData(&LightsBuffer);
}

RenderableComponent* GameSceneRenderData::FindRenderable(std::string name)
{
	auto found = std::find_if(Renderables.begin(), Renderables.end(), [name](const std::shared_ptr<RenderableComponent>& comp) { return comp->GetName() == name; });
	if (found != Renderables.end())
		return found->get();

	return nullptr;
}

GameScenePhysicsData::GameScenePhysicsData(PhysicsEngineManager* physicsHandle) :
	PhysicsHandle(physicsHandle),
	WasSetup(false)
{
}

void GameScenePhysicsData::AddCollisionObject(CollisionObject* object)
{
	std::cout << "KURWAAAA\n";
	CollisionObjects.push_back(object);

	if (WasSetup)
		PhysicsHandle->AddCollisionObject(this, object);
}

GameSceneAudioData::GameSceneAudioData(AudioEngineManager* audioHandle) :
	AudioHandle(audioHandle)
{
}

SoundSourceComponent* GameSceneAudioData::AddSource(SoundSourceComponent* sourceComp)
{
	Sources.push_back(sourceComp);
	return Sources.back();
}

SoundSourceComponent* GameSceneAudioData::FindSource(std::string name)
{
	auto found = std::find_if(Sources.begin(), Sources.end(), [name](SoundSourceComponent* sourceComp) { return sourceComp->GetName() == name; });
	if (found != Sources.end())
		return *found;

	return nullptr;
}
