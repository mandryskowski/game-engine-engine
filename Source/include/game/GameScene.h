#pragma once
#include "GameManager.h"
#include <utility/Utility.h>

class SkeletonBatch;
class GameScene;

class GameSceneRenderData
{
public:
	GameSceneRenderData(RenderEngineManager* renderHandle);

	RenderEngineManager* GetRenderHandle();
	LightProbeTextureArrays* GetProbeTexArrays();
	int GetAvailableLightIndex();
	bool ContainsLights() const;

	void AddRenderable(RenderableComponent&);
	std::shared_ptr<LightProbe> AddLightProbe(std::shared_ptr<LightProbe> probe);
	std::shared_ptr<SkeletonInfo> AddSkeletonInfo();
	void AddLight(LightComponent& light); // this function lets the engine know that the passed light component's data needs to be forwarded to shaders (therefore lighting our meshes)

	void EraseRenderable(RenderableComponent&);
	void EraseLight(LightComponent&);

	void SetupLights(unsigned int blockBindingSlot);

	void UpdateLightUniforms();


	RenderableComponent* FindRenderable(std::string name);

	friend class RenderEngine;

private:
public:
	RenderEngineManager* RenderHandle;

	std::vector <std::reference_wrapper<RenderableComponent>> Renderables;
	std::vector <std::shared_ptr <SkeletonBatch>> SkeletonBatches;
	std::vector <std::shared_ptr <LightProbe>> LightProbes;

	std::vector<std::reference_wrapper<LightComponent>> Lights;
	UniformBuffer LightsBuffer;
	
	int LightBlockBindingSlot;
	bool ProbesLoaded;

	std::shared_ptr<LightProbeTextureArrays> ProbeTexArrays;
};

class GameScenePhysicsData
{
public:
	GameScenePhysicsData(PhysicsEngineManager*);
	void AddCollisionObject(CollisionObject&, Transform& t);
	void EraseCollisionObject(CollisionObject&);

	friend class PhysicsEngine;

private:
	PhysicsEngineManager* PhysicsHandle;
	std::vector <CollisionObject*> CollisionObjects;
	physx::PxScene* PhysXScene;
	physx::PxControllerManager* PhysXControllerManager;
	bool WasSetup;
};

class GameSceneAudioData
{
public:
	GameSceneAudioData(AudioEngineManager* audioHandle);

	void AddSource(SoundSourceComponent&);

	SoundSourceComponent* FindSource(std::string name);

private:
	std::vector <std::reference_wrapper<SoundSourceComponent>> Sources;
	AudioEngineManager* AudioHandle;
};



class GameScene
{
public:
	GameScene(GameManager&);
	GameScene(const GameScene&) = delete;
	GameScene(GameScene&&);

	const Actor* GetRootActor() const;
	CameraComponent* GetActiveCamera();
	GameSceneRenderData* GetRenderData();
	GameScenePhysicsData* GetPhysicsData();
	GameSceneAudioData* GetAudioData();
	GameManager* GetGameHandle();

	Actor& AddActorToRoot(std::unique_ptr<Actor> actor);	//you use this function to make the game interact (update, draw...) with the actor; without adding it to the scene, the Actor instance isnt updated real-time by default. Pass nullptr to use the Main Scene.
	template <class T> T& CreateActorAtRoot(T&&);

	HierarchyTemplate::HierarchyTreeT& CreateHierarchyTree(const std::string& name);
	HierarchyTemplate::HierarchyTreeT* FindHierarchyTree(const std::string& name, HierarchyTemplate::HierarchyTreeT* treeToIgnore = nullptr);

	void BindActiveCamera(CameraComponent*);

	Actor* FindActor(std::string name);

private:
	GameManager* GameHandle;

	std::unique_ptr<Actor> RootActor;
	std::unique_ptr<GameSceneRenderData> RenderData;
	std::unique_ptr<GameScenePhysicsData> PhysicsData;
	std::unique_ptr<GameSceneAudioData> AudioData;

	std::vector<std::unique_ptr<HierarchyTemplate::HierarchyTreeT>> HierarchyTrees;

	CameraComponent* ActiveCamera;

	friend class Game;
	friend class GameEngineEngineEditor;
};

template<class T>
inline T& GameScene::CreateActorAtRoot(T&& actorCRef)
{
	return RootActor->CreateChild<T>(std::move(actorCRef));
}
