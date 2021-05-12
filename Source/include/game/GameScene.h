#pragma once
#include "GameManager.h"
#include <utility/Utility.h>
#include <cereal/access.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>
#include <animation/SkeletonInfo.h>

class GameScene;

class GameSceneRenderData
{
public:
	GameSceneRenderData(RenderEngineManager* renderHandle);

	RenderEngineManager* GetRenderHandle();
	LightProbeTextureArrays* GetProbeTexArrays();
	int GetAvailableLightIndex();
	bool ContainsLights() const;
	bool ContainsLightProbes() const;
	bool HasLightWithoutShadowMap() const;

	void AddRenderable(RenderableComponent&);
	void AddLight(LightComponent& light); // this function lets the engine know that the passed light component's data needs to be forwarded to shaders (therefore lighting our meshes)
	void AddLightProbe(LightProbeComponent& probe);
	std::shared_ptr<SkeletonInfo> AddSkeletonInfo();

	void EraseRenderable(RenderableComponent&);
	void EraseLight(LightComponent&);
	void EraseLightProbe(LightProbeComponent&);

	void SetupLights(unsigned int blockBindingSlot);
	void UpdateLightUniforms();

	int GetBatchID(SkeletonBatch&) const;
	SkeletonBatch* GetBatch(int ID);

	RenderableComponent* FindRenderable(std::string name);

	template<typename Archive> void SaveSkeletonBatches(Archive& archive) const
	{ 
		archive(CEREAL_NVP(SkeletonBatches));
	}
	template<typename Archive> void LoadSkeletonBatches(Archive& archive)
	{
		SkeletonBatches.clear();
		archive(CEREAL_NVP(SkeletonBatches));
	}

	friend class RenderEngine;

private:
public:
	RenderEngineManager* RenderHandle;

	std::vector <std::reference_wrapper<RenderableComponent>> Renderables;
	std::vector <std::shared_ptr <SkeletonBatch>> SkeletonBatches;
	std::vector <LightProbeComponent*> LightProbes;

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
	PhysicsEngineManager* GetPhysicsHandle();

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
	GameScene(GameManager&, const std::string& name);
	GameScene(const GameScene&) = delete;
	GameScene(GameScene&&);

	const std::string& GetName() const;
	Actor* GetRootActor();
	const Actor* GetRootActor() const;
	CameraComponent* GetActiveCamera();
	GameSceneRenderData* GetRenderData();
	GameScenePhysicsData* GetPhysicsData();
	GameSceneAudioData* GetAudioData();
	GameManager* GetGameHandle();

	bool IsBeingKilled() const;

	int GetHierarchyTreeCount() const;
	HierarchyTemplate::HierarchyTreeT* GetHierarchyTree(int index);

	void AddPostLoadLambda(std::function<void()>);	//Pass a function that you want to be called after this GameScene has been loaded. Warning: Using Archive in the lambda will not work unless you really know what you're doing.

	Actor& AddActorToRoot(std::unique_ptr<Actor> actor);	//you use this function to make the game interact (update, draw...) with the actor; without adding it to the scene, the Actor instance isnt updated real-time by default. Pass nullptr to use the Main Scene.
	template <typename ActorClass, typename... Args> ActorClass& CreateActorAtRoot(Args&&...);

	HierarchyTemplate::HierarchyTreeT& CreateHierarchyTree(const std::string& name);
	HierarchyTemplate::HierarchyTreeT* FindHierarchyTree(const std::string& name, HierarchyTemplate::HierarchyTreeT* treeToIgnore = nullptr);

	void Update(float deltaTime);

	void BindActiveCamera(CameraComponent*);

	Actor* FindActor(std::string name);
	
	void Load()
	{
		for (auto& it : PostLoadLambdas)
			it();
	}

private:
	void MarkAsKilled();
	void Delete();
	std::string Name;
	GameManager* GameHandle;

	std::unique_ptr<Actor> RootActor;
	std::unique_ptr<GameSceneRenderData> RenderData;
	std::unique_ptr<GameScenePhysicsData> PhysicsData;
	std::unique_ptr<GameSceneAudioData> AudioData;

	std::vector<std::unique_ptr<HierarchyTemplate::HierarchyTreeT>> HierarchyTrees;

	std::vector<std::function<void()>> PostLoadLambdas;

	CameraComponent* ActiveCamera;

	bool bKillingProcessStarted;

	friend class Game;
	friend class GameEngineEngineEditor;
};

template<typename ActorClass, typename... Args>
inline ActorClass& GameScene::CreateActorAtRoot(Args&&... args)
{
	return RootActor->CreateChild<ActorClass>(std::forward<Args>(args)...);
}
