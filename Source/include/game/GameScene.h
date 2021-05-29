#pragma once
#include "GameManager.h"
#include <utility/Utility.h>
#include <cereal/access.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>
#include <animation/SkeletonInfo.h>

namespace GEE
{
	class GameScene;
	class Event;

	class GameSceneRenderData
	{
	public:
		GameSceneRenderData(RenderEngineManager* renderHandle, bool isAnUIScene);

		RenderEngineManager* GetRenderHandle();
		LightProbeTextureArrays* GetProbeTexArrays();
		int GetAvailableLightIndex();
		bool ContainsLights() const;
		bool ContainsLightProbes() const;
		bool HasLightWithoutShadowMap() const;

		void AddRenderable(Renderable&);
		void AddLight(LightComponent& light); // this function lets the engine know that the passed light component's data needs to be forwarded to shaders (therefore lighting our meshes)
		void AddLightProbe(LightProbeComponent& probe);
		std::shared_ptr<SkeletonInfo> AddSkeletonInfo();

		void EraseRenderable(Renderable&);
		void EraseLight(LightComponent&);
		void EraseLightProbe(LightProbeComponent&);

		/**
		 * @brief Applies if bIsAnUIScene is true. Should be called after the UIDepth of a Renderable has changed.
		 * If bIsAnUIScene is true, we must keep the Renderables vector sorted by depth (ascending order).
		*/
		void MarkUIRenderableDepthsDirty();

		void SetupLights(unsigned int blockBindingSlot);
		void UpdateLightUniforms();

		int GetBatchID(SkeletonBatch&) const;
		SkeletonBatch* GetBatch(int ID);

		~GameSceneRenderData();

		//RenderableComponent* FindRenderable(std::string name);

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
		void AssertThatUIRenderablesAreSorted();
	public:
		RenderEngineManager* RenderHandle;

		std::vector <Renderable*> Renderables;
		/**
		 * @brief If bIsAnUIScene is true, Renderables are sorted by UI depth upon insertion.
		 * This allows to render UIElements with different depths correctly
		*/
		bool bIsAnUIScene;
		bool bUIRenderableDepthsDirtyFlag;

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
		GameScene(GameManager&, const std::string& name, bool isAnUIScene = false);
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
		/**
		 * @brief Use the function to check if this is a valid scene. If it returns true, you should remove all references to the scene and avoid processing it at all. The root actor, its children and their components become invalid along with the scene.
		 * @return a boolean indicating whether the GameScene is about to be destroyed
		*/
		bool IsBeingKilled() const;

		int GetHierarchyTreeCount() const;
		HierarchyTemplate::HierarchyTreeT* GetHierarchyTree(int index);

		/**
		* @brief Pass a function that you want to be called after this GameScene has been loaded. **Warning: Using Archive in the lambda will not work unless you really know what you're doing.**
		* @param postLoadLambda: the lambda that will be called after the scene will be loaded
		*/
		void AddPostLoadLambda(std::function<void()> postLoadLambda);

		/**
		 * @brief Add a UICanvas to check every frame if it contains the cursor. This information can be used to disable some input outside the canvas that contains the cursor. Should be called automatically by the constructor of UICanvasActor.
		 * @see UICanvasActor::UICanvasActor()
		 * @param canvas: the canvas to be added
		*/
		void AddBlockingCanvas(UICanvas& canvas);
		/**
		 * @brief Erase a UICanvas to stop checking every frame if it contains a cursor. Must be called when the UICanvas is about to be killed (gets destroyed). Should be called automatically by the destructor of UICanvasActor.
		 * @see UICanvasActor::~UICanvasActor()
		 * @param canvas: the canvas to be erased
		*/
		void EraseBlockingCanvas(UICanvas&);

		/**
		 * @brief Get the canvas that contains the cursor in this frame. Events handled by UIElements outside the canvas should be treated differently.
		 * @return a UICanvas pointer to the blocking canvas (or nullptr if there isn't any)
		*/
		UICanvas* GetCurrentBlockingCanvas() const;

		/**
		 * @brief Returns an actor name that is guaranteed to be unique (no other actor in the scene has it). Always use this function if there is a possibility of duplicating actor names - this can cause crashes.
		 * @param name: the name that should be converted to unique
		 * @return a string of characters containing an unique actor name (can be the same as the parameter)
		*/
		std::string GetUniqueActorName(const std::string& name) const;

		//You use this function to make the game interact (update, draw...) with the actor; without adding it to the scene, the Actor instance isnt updated real-time by default. Pass nullptr to use the Main Scene.
		Actor& AddActorToRoot(std::unique_ptr<Actor> actor);
		template <typename ActorClass, typename... Args> ActorClass& CreateActorAtRoot(Args&&...);

		HierarchyTemplate::HierarchyTreeT& CreateHierarchyTree(const std::string& name);
		HierarchyTemplate::HierarchyTreeT* FindHierarchyTree(const std::string& name, HierarchyTemplate::HierarchyTreeT* treeToIgnore = nullptr);

		void HandleEventAll(const Event&);
		void Update(float deltaTime);

		void BindActiveCamera(CameraComponent*);

		Actor* FindActor(std::string name);

		void Load()
		{
			for (auto& it : PostLoadLambdas)
				it();
		}

		~GameScene();
		//private:
		void MarkAsKilled();	//Note: this should probably be encapsulated
	private:
		void Delete();
		std::string Name;
		GameManager* GameHandle;

		std::unique_ptr<Actor> RootActor;
		std::unique_ptr<GameSceneRenderData> RenderData;
		std::unique_ptr<GameScenePhysicsData> PhysicsData;
		std::unique_ptr<GameSceneAudioData> AudioData;

		std::vector<std::unique_ptr<HierarchyTemplate::HierarchyTreeT>> HierarchyTrees;

		/**
		* @brief If the cursor is contained within one of these canvases, you most likely should disable some interaction with UI elements outside the canvas.
		* The canvas with the highest depth has the priority if a mouse is contained within many canvases (if many canvases with the same depth contain the cursor, one will be chosen).
		* Must be sorted by CanvasDepth in ascending order.
		* Note: this should perhaps be moved to a separate class GameSceneUIData
		* @see UICanvas::GetCanvasDepth()
		*/
		std::vector<UICanvas*> BlockingCanvases;

		/**
		 * @brief Points to the canvas (if there is any) containing the cursor in the current frame.
		*/
		UICanvas* CurrentBlockingCanvas;


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

}