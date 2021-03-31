#pragma once
#include "GameSettings.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

class Actor;
class UIActor;
class UICanvasActor;
class Controller;
class Component;
class RenderableComponent;
class LightComponent;
class ModelComponent;
class CameraComponent;
class BoneComponent;
class SoundSourceComponent;
class Transform;
struct Animation;

struct SoundBuffer;

class GameScene;
class GameSceneRenderData;
class GameScenePhysicsData;

struct CollisionObject;
class Mesh;
class MeshInstance;

class Texture;
class Material;
struct MaterialInstance;
class AtlasMaterial;

class Shader;
class LightProbe;
class SkeletonInfo;


struct GameSettings;
struct VideoSettings;
class InputDevicesStateRetriever;
class RenderInfo;
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

typedef unsigned int GLenum;

namespace MeshSystem
{
	class MeshTree;
	class TemplateNode;
	class BoneNode;
}

namespace HierarchyTemplate
{
	class HierarchyNodeBase;
	template <typename CompType> class HierarchyNode;
	class HierarchyTreeT;
}

enum class TextAlignment
{
	LEFT,
	CENTER,
	RIGHT,
	BOTTOM = LEFT,
	TOP = RIGHT
};

namespace physx
{
	class PxScene;
	class PxController;
	class PxControllerManager;
}

class PostprocessManager
{
public:
	virtual glm::mat4 GetJitterMat(const GameSettings::VideoSettings& usedSettings, int optionalIndex = -1) = 0;	//dont pass anything to receive the current jitter matrix
	virtual unsigned int GetFrameIndex() = 0;
};

class PhysicsEngineManager
{
public:
	virtual void AddCollisionObjectToPxPipeline(GameScenePhysicsData& scenePhysicsData, CollisionObject&) = 0;

	virtual physx::PxController* CreateController(GameScenePhysicsData& scenePhysicsData) = 0;

	virtual void ApplyForce(CollisionObject&, glm::vec3 force) = 0;

	virtual void DebugRender(GameScenePhysicsData&, RenderEngine&, RenderInfo&) = 0;
};


class RenderEngineManager
{
public:
	virtual const ShadingModel& GetShadingModel() = 0;
	virtual Mesh& GetBasicShapeMesh(EngineBasicShape) = 0;
	virtual Shader* GetLightShader(const RenderToolboxCollection& renderCol, LightType) = 0;
	virtual RenderToolboxCollection* GetCurrentTbCollection() = 0;

	virtual Material* AddMaterial(Material*) = 0;
	virtual std::shared_ptr<Shader> AddShader(std::shared_ptr<Shader>, bool bForwardShader = false) = 0;
	virtual void AddSceneRenderDataPtr(GameSceneRenderData*) = 0;
	virtual MeshSystem::MeshTree* CreateMeshTree(std::string path) = 0;
	virtual MeshSystem::MeshTree* FindMeshTree(std::string path, MeshSystem::MeshTree* ignore = nullptr) = 0;
	virtual const MeshSystem::MeshTree* FindMeshTree(std::string path, MeshSystem::MeshTree* ignore = nullptr) const = 0;

	virtual Shader* FindShader(std::string) = 0;
	virtual Material* FindMaterial(std::string) = 0;

	virtual void RenderCubemapFromTexture(Texture targetTex, Texture tex, glm::uvec2 size, Shader&, int* layer = nullptr, int mipLevel = 0) = 0;
	virtual void RenderCubemapFromScene(RenderInfo info, GameSceneRenderData* sceneRenderData, GEE_FB::Framebuffer target, GEE_FB::FramebufferAttachment targetTex, GLenum attachmentType, Shader* shader = nullptr, int* layer = nullptr, bool fullRender = false) = 0;
	virtual void RenderText(const RenderInfo& info, const Font& font, std::string content, Transform t, glm::vec3 color = glm::vec3(1.0f), Shader* shader = nullptr, bool convertFromPx = false, const std::pair<TextAlignment, TextAlignment>& = std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::BOTTOM)) = 0; //Pass a shader if you do not want the default shader to be used.
	virtual void RenderStaticMesh(const RenderInfo& info, const MeshInstance& mesh, const Transform& transform, Shader* shader, glm::mat4* lastFrameMVP = nullptr, Material* overrideMaterial = nullptr, bool billboard = false) = 0; //Note: this function does not call the Use method of passed Shader. Do it manually.
	virtual void RenderStaticMeshes(const RenderInfo&, const std::vector<std::unique_ptr<MeshInstance>>& meshes, const Transform& transform, Shader* shader, glm::mat4* lastFrameMVP = nullptr, Material* overrideMaterial = nullptr, bool billboard = false) = 0; //Note: this function does not call the Use method of passed Shader. Do it manually.
	virtual void RenderSkeletalMeshes(const RenderInfo& info, const std::vector<std::unique_ptr<MeshInstance>>& meshes, const Transform& transform, Shader* shader, SkeletonInfo& skelInfo, Material* overrideMaterial = nullptr) = 0; //Note: this function does not call the Use method of passed Shader. Do it manually
};

class AudioEngineManager
{
public:
	virtual SoundBuffer* LoadBufferFromFile(std::string path) = 0;
	virtual void CheckError() = 0;
};

class GameManager
{
public:
	virtual void BindAudioListenerTransformPtr(Transform*) = 0;
	virtual void PassMouseControl(Controller* controller) = 0;
	virtual const Controller* GetCurrentMouseController() const = 0;
	virtual InputDevicesStateRetriever GetInputRetriever() = 0;
	virtual bool HasStarted() const = 0;

	virtual Actor* GetRootActor(GameScene* = nullptr) = 0;
	virtual GameScene* GetMainScene() = 0;

	virtual PhysicsEngineManager* GetPhysicsHandle() = 0;
	virtual RenderEngineManager* GetRenderEngineHandle() = 0;
	virtual AudioEngineManager* GetAudioEngineHandle() = 0;

	virtual GameSettings* GetGameSettings() = 0;

	virtual HierarchyTemplate::HierarchyTreeT* FindHierarchyTree(const std::string& name, HierarchyTemplate::HierarchyTreeT* treeToIgnore = nullptr) = 0;
	virtual std::shared_ptr<Font> FindFont(const std::string& path) = 0;
};

class EditorManager
{
public:
	virtual void SelectComponent(Component* comp, GameScene& editorScene) = 0;
	virtual void SelectActor(Actor* comp, GameScene& editorScene) = 0;
	virtual void SelectScene(GameScene* comp, GameScene& editorScene) = 0;
	template <typename T> void Select(T* obj, GameScene& editorScene);

	virtual GameManager* GetGameHandle() = 0;

	virtual void PreviewMeshNode(MeshSystem::MeshTree&) = 0;

};

struct PrimitiveDebugger
{
	static bool bDebugMeshTrees;
	static bool bDebugProbeLoading;
	static bool bDebugCubemapFromTex;
	static bool bDebugFramebuffers;
	static bool bDebugHierarchy;
};