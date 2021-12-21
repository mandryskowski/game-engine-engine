#pragma once
#include <game/GameSettings.h>
#include <math/Vec.h>
#include <string>
#include <vector>
#include <memory>

typedef unsigned int GLenum;

namespace physx
{
	class PxScene;
	class PxController;
	class PxControllerManager;
}

namespace GEE
{
	class EditorDescriptionBuilder;

	class Actor;
	class GunActor;
	class PawnActor;
	class ShootingController;
	class UIActor;
	class UICanvas;
	class UICanvasActor;
	class Controller;
	class Component;
	class Renderable;
	class RenderableComponent;
	class LightComponent;
	class LightProbeComponent;
	class ModelComponent;
	class TextComponent;
	class TextConstantSizeComponent;
	class CameraComponent;
	class BoneComponent;

	class AnimationManagerComponent;
	class Transform;
	class Interpolation;
	struct Animation;

	class Event;
	class GameScene;
	class GameSceneRenderData;

	class Mesh;
	class MeshInstance;

	class Texture;
	class Material;
	enum class MaterialShaderHint;
	struct MaterialInstance;
	class AtlasMaterial;

	class Shader;
	class LightProbe;

	class SkeletonBatch;
	class SkeletonInfo;


	struct GameSettings;
	struct VideoSettings;
	class InputDevicesStateRetriever;

	class MatrixInfo;
	class MatrixInfoExt;
	class SceneMatrixInfo;

	class RenderToolboxCollection;

	struct LightProbeTextureArrays;

	class RenderEngine;

	enum LightType;
	enum class EngineBasicShape;

	class Font;

	namespace GEE_FB
	{
		class Framebuffer;
		class FramebufferAttachment;
	}

	namespace HierarchyTemplate
	{
		class HierarchyNodeBase;
		template <typename CompType> class HierarchyNode;
		class HierarchyTreeT;
	}

	enum class MaterialShaderHint
	{
		None,
		Simple,
		Shaded,
	};

	struct VaryingMaterialShaderHints
	{
	public:
		static MaterialShaderHint Shaded() { return MaterialShaderHint::Shaded; }
	};
	enum class TextAlignment
	{
		LEFT,
		CENTER,
		RIGHT,
		BOTTOM = LEFT,
		TOP = RIGHT
	};

	// These values correspond to GLFW macros. Check official GLFW documentation.
	enum class DefaultCursorIcon
	{
		Regular = 0x00036001,
		TextInput = 0x00036002,
		Crosshair = 0x00036003,
		Hand = 0x00036004,
		HorizontalResize = 0x00036005,
		VerticalResize = 0x00036006
	};

	class PostprocessManager
	{
	public:
		virtual Mat4f GetJitterMat(const GameSettings::VideoSettings& usedSettings, int optionalIndex = -1) = 0;	//dont pass anything to receive the current jitter matrix
		virtual unsigned int GetFrameIndex() = 0;

		virtual ~PostprocessManager() = default;
	};

	namespace Physics
	{
		class GameScenePhysicsData;
		struct CollisionObject;
		struct CollisionShape;

		class PhysicsEngineManager
		{
		public:
			virtual void CreatePxShape(CollisionShape&, CollisionObject&) = 0;
			virtual void AddCollisionObjectToPxPipeline(GameScenePhysicsData& scenePhysicsData, CollisionObject&) = 0;

			virtual physx::PxController* CreateController(GameScenePhysicsData& scenePhysicsData, const Transform& t) = 0;

			virtual ~PhysicsEngineManager() = default;
		protected:
			virtual void RemoveScenePhysicsDataPtr(GameScenePhysicsData& scenePhysicsData) = 0;
			friend class GameScene;
		};
	}

	namespace Audio
	{
		class SoundSourceComponent;
		struct SoundBuffer;

		class AudioEngineManager
		{
		public:
			virtual void CheckError() = 0;
			virtual SoundBuffer FindBuffer(const std::string& path) = 0;
			virtual void AddBuffer(const SoundBuffer&) = 0;

			virtual ~AudioEngineManager() = default;
		};
	}

	class RenderEngineManager
	{
	public:
		virtual const ShadingAlgorithm& GetShadingModel() = 0;
		virtual Mesh& GetBasicShapeMesh(EngineBasicShape) = 0;
		virtual Shader* GetSimpleShader() = 0;
		virtual Shader* GetLightShader(const RenderToolboxCollection& renderCol, LightType) = 0;
		virtual RenderToolboxCollection* GetCurrentTbCollection() = 0;
		virtual Texture GetEmptyTexture() = 0;
		virtual SkeletonBatch* GetBoundSkeletonBatch() = 0;

		//TODO: THIS SHOULD NOT BE HERE
		virtual std::vector<Shader*> GetCustomShaders() = 0;
		virtual std::vector<Material*> GetMaterials() = 0;

		virtual void BindSkeletonBatch(SkeletonBatch* batch) = 0;
		virtual void BindSkeletonBatch(GameSceneRenderData* sceneRenderData, unsigned int index) = 0;

		/**
		 * @brief Add a new RenderToolboxCollection to the render engine to enable updating shadow maps automatically. By default, we load every toolbox that will be needed according to RenderToolboxCollection::Settings
		 * @param tbCollection: a constructed RenderToolboxCollection object containing the name and settings of it
		 * @param setupToolboxesAccordingToSettings: pass false as the second argument to disable loading any toolboxes
		 * @return a reference to the added RenderToolboxCollection
		*/
		virtual RenderToolboxCollection& AddRenderTbCollection(const RenderToolboxCollection& tbCollection,
		                                                       bool setupToolboxesAccordingToSettings = true) = 0;
		virtual Material* AddMaterial(SharedPtr<Material>) = 0;
		virtual SharedPtr<Shader> AddShader(SharedPtr<Shader>) = 0;

		virtual void EraseRenderTbCollection(RenderToolboxCollection& tbCollection) = 0;
		virtual void EraseMaterial(Material&) = 0;

		virtual Shader* FindShader(std::string) = 0;
		virtual SharedPtr<Material> FindMaterial(std::string) = 0;
	
		//Pass a shader if you do not want the default shader to be used.
		virtual void RenderText(const SceneMatrixInfo& info, const Font& font, std::string content, Transform t,
		                        Vec3f color = Vec3f(1.0f), Shader* shader = nullptr, bool convertFromPx = false,
		                        const std::pair<TextAlignment, TextAlignment>& = std::pair<
			                        TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::BOTTOM)) = 0;

		virtual ~RenderEngineManager() = default;
	protected:
		virtual void AddSceneRenderDataPtr(GameSceneRenderData&) = 0;
		virtual void RemoveSceneRenderDataPtr(GameSceneRenderData&) = 0;

		virtual SharedPtr<Shader> AddNonCustomShader(SharedPtr<Shader>) = 0;
		friend class GameScene;
	};
	

	class GameManager
	{
	public:
		static GameScene* DefaultScene;
		static GameManager& Get();
	public:
		virtual void AddInterpolation(const Interpolation&) = 0;
		virtual GameScene& CreateScene(std::string name, bool disallowChangingNameIfTaken = true) = 0;
		virtual GameScene& CreateUIScene(std::string name, SystemWindow& associateWindow, bool disallowChangingNameIfTaken = true) = 0;

		virtual void BindAudioListenerTransformPtr(Transform*) = 0;
		virtual void UnbindAudioListenerTransformPtr(Transform* transform) = 0;
		virtual void PassMouseControl(Controller* controller) = 0;
		virtual const Controller* GetCurrentMouseController() const = 0;
		virtual double GetProgramRuntime() const = 0;
		virtual InputDevicesStateRetriever GetInputRetriever() = 0;
		virtual SharedPtr<Font> GetDefaultFont() = 0;
		
		virtual bool CheckEEForceForwardShading() = 0;

		virtual bool HasStarted() const = 0;

		virtual Actor* GetRootActor(GameScene* = nullptr) = 0;
		virtual GameScene* GetScene(const std::string& name) = 0;
		virtual GameScene* GetMainScene() = 0;
		virtual std::vector<GameScene*> GetScenes() = 0;
		
		virtual unsigned long long GetTotalTickCount() const = 0;
		virtual unsigned long long GetTotalFrameCount() const = 0;

		virtual Physics::PhysicsEngineManager* GetPhysicsHandle() = 0;
		virtual RenderEngineManager* GetRenderEngineHandle() = 0;
		virtual Audio::AudioEngineManager* GetAudioEngineHandle() = 0;

		virtual GameSettings* GetGameSettings() = 0;

		virtual GameScene* GetActiveScene() = 0;
		virtual void SetActiveScene(GameScene* scene) = 0;

		virtual void SetCursorIcon(DefaultCursorIcon) = 0;

		virtual HierarchyTemplate::HierarchyTreeT* FindHierarchyTree(const std::string& name, HierarchyTemplate::HierarchyTreeT* treeToIgnore = nullptr) = 0;
		virtual SharedPtr<Font> FindFont(const std::string& path) = 0;

		virtual ~GameManager() = default;
	protected:
		virtual void DeleteScene(GameScene&) = 0;
	private:
		static GameManager* GamePtr;
		friend class GameScene;
		friend class Game;
	};

	struct PrimitiveDebugger
	{
		static bool bDebugMeshTrees;
		static bool bDebugProbeLoading;
		static bool bDebugCubemapFromTex;
		static bool bDebugFramebuffers;
		static bool bDebugHierarchy;
	};

	class HTreeObjectLoc
	{
		const HierarchyTemplate::HierarchyTreeT* TreePtr;
	protected:
		HTreeObjectLoc() : TreePtr(nullptr) {}
	public:
		HTreeObjectLoc(const HierarchyTemplate::HierarchyTreeT& tree) : TreePtr(&tree) {}
		bool IsValidTreeElement() const;
		std::string GetTreeName() const;	//get path
	};
}