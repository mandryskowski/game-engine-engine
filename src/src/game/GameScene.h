#pragma once
#include <game/GameManager.h>
#include <scene/Actor.h>
#include <utility/Utility.h>

namespace GEE
{
	class GameScene;
	class Event;
	class RenderableVolume;

	class GameSceneRenderData;
	class GameSceneUIData;
	namespace Physics
	{
		class GameScenePhysicsData;
	}
	namespace Audio
	{
		class GameSceneAudioData;
	}

	namespace Editor
	{
		class GameEngineEngineEditor;
	}

	class GameScene
	{
	public:
		GameScene(GameManager&, const std::string& name);
		GameScene(GameManager&, const std::string& name, bool isAnUIScene, SystemWindow& associatedWindow);
		GameScene(const GameScene&) = delete;
		GameScene(GameScene&&);

		bool HasStarted() const { return bHasStarted; }
		void MarkAsStarted();

		const std::string& GetName() const;
		Actor* GetRootActor();
		const Actor* GetRootActor() const;
		CameraComponent* GetActiveCamera();
		GameSceneRenderData* GetRenderData();
		const GameSceneRenderData* GetRenderData() const;
		Physics::GameScenePhysicsData* GetPhysicsData();
		Audio::GameSceneAudioData* GetAudioData();
		GameSceneUIData* GetUIData();
		GameManager* GetGameHandle();
		EventPusher GetEventPusher();
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
		 * @brief Returns an actor name that is guaranteed to be unique (no other actor in the scene has it). Always use this function if there is a possibility of duplicating actor names - this can cause crashes.
		 * @param name: the name that should be converted to unique
		 * @return a string of characters containing an unique actor name (can be the same as the parameter)
		*/
		std::string GetUniqueActorName(const std::string& name) const;

		//You use this function to make the game interact (update, draw...) with the actor; without adding it to the scene, the Actor instance isnt updated real-time by default. Pass nullptr to use the Main Scene.
		Actor& AddActorToRoot(UniquePtr<Actor> actor);
		template <typename ActorClass, typename... Args>
		ActorClass& CreateActorAtRoot(Args&&...);

		/**
		 * @brief Creates a new HierarchyTree with the passed string as its name.
		 * @param name: the name of the tree.
		 * @return: a reference to the newly created tree.
		*/
		HierarchyTemplate::HierarchyTreeT& CreateHierarchyTree(const std::string& name);
		
		
		HierarchyTemplate::HierarchyTreeT& CopyHierarchyTree(const HierarchyTemplate::HierarchyTreeT& tree);

		HierarchyTemplate::HierarchyTreeT* FindHierarchyTree(const std::string& name,
			HierarchyTemplate::HierarchyTreeT* treeToIgnore = nullptr);

		void HandleEventAll(Event&);
		void Update(float deltaTime);

		void BindActiveCamera(CameraComponent*);

		Actor* FindActor(std::string name);

		template <typename Archive> void Save(Archive& archive) const;
		template <typename Archive> void Load(Archive& archive);

		~GameScene();
		//private:	
		void MarkAsKilled(); //Note: this should probably be encapsulated


	private:
		void Delete();

	private:
		std::string Name;
		GameManager* GameHandle;

		UniquePtr<Actor> RootActor;
		UniquePtr<GameSceneRenderData> RenderData;
		UniquePtr<Physics::GameScenePhysicsData> PhysicsData;
		UniquePtr<Audio::GameSceneAudioData> AudioData;
		UniquePtr<GameSceneUIData> UIData;

		bool bHasStarted;

	public:
		std::vector<UniquePtr<HierarchyTemplate::HierarchyTreeT>> HierarchyTrees;
	private:

		std::vector<std::function<void()>> PostLoadLambdas;

		CameraComponent* ActiveCamera;

		unsigned long long KillingProcessFrame;

		friend class Game;
		friend class Editor::GameEngineEngineEditor;
	};

	class GameSceneData
	{
	public:
		GameSceneData(GameScene& scene);

		std::string GetSceneName() const;
	protected:
		GameScene& GetScene();
		const GameScene& GetScene() const;

		GameScene& Scene;
	};

	class GameSceneRenderDataRef
	{
	public:
		GameSceneRenderDataRef(GameScene&);
		GameSceneRenderDataRef(GameSceneRenderData&);
	};

	/**
	 * @brief Only for UI scenes.
	*/
	class GameSceneUIData : public GameSceneData
	{
	public:
		GameSceneUIData(GameScene&, SystemWindow&);
		GameSceneUIData(const GameSceneUIData&) = delete;

		WindowData GetWindowData();
		SystemWindow* GetWindow();
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
		void EraseBlockingCanvas(UICanvas& canvas);

		/**
		 * @brief Get the canvas that contains the cursor in this frame. Events handled by UIElements outside the canvas should be treated differently.
		 * @return a UICanvas pointer to the blocking canvas (or nullptr if there isn't any)
		*/
		UICanvas* GetCurrentBlockingCanvas() const;

		void HandleEventAll(const Event&);
	private:
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

		/**
		 * @brief Optional window that can be associated with this GameScene.
		 * Generally, you don't have to do it for 3D scenes.
		 * For UI scenes you almost always have to do it.
		 * The AssociatedWindow is used mostly for converting NDC to px - e.g. to render elements inside an UICanvas.
		*/
		SystemWindow* AssociatedWindow;
	};

	class GameSceneRenderData : public GameSceneData
	{
	public:
		GameSceneRenderData(GameScene&, bool isAnUIScene);

		RenderEngineManager* GetRenderHandle();
		LightProbeTextureArrays* GetProbeTexArrays();
		int GetAvailableLightIndex();

		bool ContainsLights() const;
		bool ContainsLightProbes() const;
		bool HasLightWithoutShadowMap() const;

		void AddRenderable(Renderable&);
		void AddLight(LightComponent& light);
		// this function lets the engine know that the passed light component's data needs to be forwarded to shaders (therefore lighting our meshes)
		void AddLightProbe(LightProbeComponent& probe);
		SharedPtr<SkeletonInfo> AddSkeletonInfo();

		void EraseRenderable(Renderable&);
		void EraseLight(LightComponent&);
		void EraseLightProbe(LightProbeComponent&);

		/**
		 * @return the volumes of all lights (not light probes!) in the scene.
		*/
		std::vector<UniquePtr<RenderableVolume>> GetSceneLightsVolumes() const;

		/**
		 * @param putGlobalProbeAtEnd: ensure that the global probe is the final element of the vector. This is used in rendering the global probe only in areas without any local probe.
		 * @return the volumes of all light probes (not lights!) in the scene.
		*/
		std::vector<UniquePtr<RenderableVolume>> GetLightProbeVolumes(bool putGlobalProbeAtEnd = true);

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

		template <typename Archive>
		void SaveSkeletonBatches(Archive& archive) const
		{
			archive(CEREAL_NVP(SkeletonBatches));
		}

		template <typename Archive>
		void LoadSkeletonBatches(Archive& archive)
		{
			SkeletonBatches.clear();
			archive(CEREAL_NVP(SkeletonBatches));
			std::cout << "Loaded " << SkeletonBatches.size() << " skeleton batches.\n";
		}

		friend struct SceneRenderer;
		friend class RenderEngine;

	private:
		void AssertThatUIRenderablesAreSorted();
	public:
		RenderEngineManager* RenderHandle;

		std::vector<Renderable*> Renderables;
		/**
		 * @brief If bIsAnUIScene is true, Renderables are sorted by UI depth upon insertion.
		 * This allows to render UIElements with different depths correctly
		*/
		bool bIsAnUIScene;
		bool bUIRenderableDepthsSortedDirtyFlag, bLightProbesSortedDirtyFlag;

		std::vector<SharedPtr<SkeletonBatch>> SkeletonBatches;
		std::vector<LightProbeComponent*> LightProbes;

		std::vector<std::reference_wrapper<LightComponent>> Lights;
		UniformBuffer LightsBuffer;

		int LightBlockBindingSlot;
		bool ProbesLoaded;

		SharedPtr<LightProbeTextureArrays> ProbeTexArrays;
	};

	namespace Physics
	{
		class GameScenePhysicsData : public GameSceneData
		{
		public:
			GameScenePhysicsData(GameScene&);
			void AddCollisionObject(CollisionObject&, Transform& t);
			void EraseCollisionObject(CollisionObject&);
			Physics::PhysicsEngineManager* GetPhysicsHandle();
			physx::PxScene* GetPxScene() { return PhysXScene; }

			friend class PhysicsEngine;
			friend class PhysicsDebugRenderer;

		private:
			Physics::PhysicsEngineManager* PhysicsHandle;
			std::vector<CollisionObject*> CollisionObjects;
			physx::PxScene* PhysXScene;
			physx::PxControllerManager* PhysXControllerManager;
			bool WasSetup;
		};
	}

	namespace Audio
	{
		class GameSceneAudioData : public GameSceneData
		{
		public:
			GameSceneAudioData(GameScene&);

			void AddSource(SoundSourceComponent&);

			SoundSourceComponent* FindSource(std::string name);

		private:
			std::vector<std::reference_wrapper<SoundSourceComponent>> Sources;
			AudioEngineManager* AudioHandle;
		};
	}

	template <typename ActorClass, typename... Args>
	ActorClass& GameScene::CreateActorAtRoot(Args&&... args)
	{
		return RootActor->CreateChild<ActorClass>(std::forward<Args>(args)...);
	}
}