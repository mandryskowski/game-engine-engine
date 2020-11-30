#pragma once
#include "GameManager.h"
#include "Utility.h"

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

	std::shared_ptr<RenderableComponent> AddRenderable(std::shared_ptr<RenderableComponent> renderable);
	std::shared_ptr<LightProbe> AddLightProbe(std::shared_ptr<LightProbe> probe);
	std::shared_ptr<SkeletonInfo> AddSkeletonInfo();
	void AddLight(std::shared_ptr<LightComponent> light); // this function lets the engine know that the passed light component's data needs to be forwarded to shaders (therefore lighting our meshes)

	void SetupLights(unsigned int blockBindingSlot);

	void UpdateLightUniforms();


	RenderableComponent* FindRenderable(std::string name);

	friend class RenderEngine;

private:
	RenderEngineManager* RenderHandle;

	std::vector <std::shared_ptr <RenderableComponent>> Renderables;
	std::vector <std::shared_ptr <SkeletonBatch>> SkeletonBatches;
	std::vector <std::shared_ptr <LightProbe>> LightProbes;

	std::vector<std::shared_ptr<LightComponent>> Lights;
	UniformBuffer LightsBuffer;
	

	std::shared_ptr<LightProbeTextureArrays> ProbeTexArrays;
};

class GameScenePhysicsData
{
public:
	GameScenePhysicsData(PhysicsEngineManager*);
	void AddCollisionObject(CollisionObject*);

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

	SoundSourceComponent* AddSource(SoundSourceComponent*);

	SoundSourceComponent* FindSource(std::string name);

private:
	std::vector <SoundSourceComponent*> Sources;
	AudioEngineManager* AudioHandle;
};



class GameScene
{
public:
	GameScene(GameManager*);

	const Actor* GetRootActor() const;
	CameraComponent* GetActiveCamera();
	GameSceneRenderData* GetRenderData();
	GameScenePhysicsData* GetPhysicsData();
	GameSceneAudioData* GetAudioData();
	GameManager* GetGameHandle();

	Actor* AddActorToRoot(std::shared_ptr<Actor> actor);	//you use this function to make the game interact (update, draw...) with the actor; without adding it to the scene, the Actor instance isnt updated real-time by default. Pass nullptr to use the Main Scene.
	void BindActiveCamera(CameraComponent*);

	Actor* FindActor(std::string name);

private:
	GameManager* GameHandle;

	std::unique_ptr<Actor> RootActor;
	std::unique_ptr<GameSceneRenderData> RenderData;
	std::unique_ptr<GameScenePhysicsData> PhysicsData;
	std::unique_ptr<GameSceneAudioData> AudioData;

	CameraComponent* ActiveCamera;

	friend class Game;
	friend class GameEngineEngineEditor;
};