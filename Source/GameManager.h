#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>
class Actor;
class Component;
class LightComponent;
class ModelComponent;
class CameraComponent;
class SoundSourceComponent;
class Transform;

struct CollisionObject;
class Mesh;
class Texture;
class Material;
class Shader;
class LightProbe;
class SearchEngine;
class GameSettings;
class RenderInfo;


enum LightType;
enum EngineBasicShape;

namespace GEE_FB
{
	class Framebuffer;
	class FramebufferAttachment;
}

typedef unsigned int GLenum;

namespace MeshSystem
{
	class MeshTree;
}

class PostprocessManager
{
public:
	virtual glm::mat4 GetJitterMat(int optionalIndex = -1) = 0;	//dont pass anything to receive the current jitter matrix
	virtual unsigned int GetFrameIndex() = 0;
};

class PhysicsEngineManager
{
public:
	virtual void AddCollisionObject(CollisionObject*) = 0;
	virtual CollisionObject* CreateCollisionObject(glm::vec3 pos) = 0;

	virtual CollisionObject* FindCollisionObject(std::string name) = 0;

	virtual void ApplyForce(CollisionObject*, glm::vec3 force) = 0;
};

class RenderEngineManager
{
public:
	virtual const std::shared_ptr<Mesh> GetBasicShapeMesh(EngineBasicShape) = 0;
	virtual int GetAvailableLightIndex() = 0;
	virtual Shader* GetLightShader(LightType) = 0;
	virtual Material* AddMaterial(Material*) = 0;
	virtual std::shared_ptr<Shader> AddShader(std::shared_ptr<Shader>, bool bForwardShader = false) = 0;
	virtual std::shared_ptr<ModelComponent> AddModel(std::shared_ptr<ModelComponent>) = 0;
	virtual std::shared_ptr<LightProbe> AddLightProbe(std::shared_ptr<LightProbe> probe) = 0;
	virtual std::shared_ptr<LightComponent> AddLight(std::shared_ptr<LightComponent> light) = 0;
	virtual std::shared_ptr<ModelComponent> CreateModel(const ModelComponent&) = 0;
	virtual std::shared_ptr<ModelComponent> CreateModel(ModelComponent&&) = 0;
	virtual MeshSystem::MeshTree* CreateMeshTree(std::string path) = 0;
	virtual MeshSystem::MeshTree* FindMeshTree(std::string path, MeshSystem::MeshTree* ignore = nullptr) = 0;
	virtual Shader* FindShader(std::string) = 0;
	virtual void RenderCubemapFromTexture(Texture targetTex, Texture tex, glm::uvec2 size, Shader&, int* layer = nullptr, int mipLevel = 0) = 0;;
	virtual void RenderCubemapFromScene(RenderInfo& info, GEE_FB::Framebuffer target, GEE_FB::FramebufferAttachment targetTex, GLenum attachmentType, Shader* shader = nullptr, int* layer = nullptr, bool fullRender = false) = 0;
	virtual void Render(RenderInfo& info, const ModelComponent& model, Shader* shader, const Mesh* boundMesh = nullptr, Material* materialBound = nullptr) = 0;	//Note: this function does not call the Use method of passed Shader. Do it manually
	virtual void Render(RenderInfo& info, const std::shared_ptr<Mesh>&, const Transform&, Shader* shader, const Mesh* boundMesh = nullptr, Material* materialBound = nullptr) = 0; //Note: this function does not call the Use method of passed Shader. Do it manually
};

class AudioEngineManager
{
public:
	virtual SoundSourceComponent* AddSource(SoundSourceComponent*) = 0;
	virtual SoundSourceComponent* AddSource(std::string path, std::string name) = 0;
};

class GameManager
{
public:
	virtual void BindActiveCamera(CameraComponent*) = 0;
	virtual std::shared_ptr<Actor> AddActorToScene(std::shared_ptr<Actor>) = 0;

	virtual SearchEngine* GetSearchEngine() = 0;
	virtual PhysicsEngineManager* GetPhysicsHandle() = 0;
	virtual RenderEngineManager* GetRenderEngineHandle() = 0;
	virtual AudioEngineManager* GetAudioEngineHandle() = 0;

	virtual const GameSettings* GetGameSettings() = 0;
};

struct PrimitiveDebugger
{
	static bool bDebugMeshTrees;
	static bool bDebugProbeLoading;
	static bool bDebugCubemapFromTex;
	static bool bDebugFramebuffers;
	static bool bDebugHierarchy;
};