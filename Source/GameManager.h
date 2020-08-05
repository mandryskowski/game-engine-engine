#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>
class Actor;
class LightComponent;
class ModelComponent;
class CameraComponent;
class SoundSourceComponent;

struct CollisionObject;
class Material;
class Shader;
class SearchEngine;
class GameSettings;

namespace MeshSystem
{
	class MeshTree;
}

class PostprocessManager
{
public:
	virtual glm::mat4 GetJitterMat() = 0;
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
	virtual void AddMaterial(Material*) = 0;
	virtual void AddExternalShader(Shader*) = 0;
	virtual void AddModel(std::shared_ptr<ModelComponent>) = 0;
	virtual std::shared_ptr<ModelComponent> CreateModel(const ModelComponent&) = 0;
	virtual std::shared_ptr<ModelComponent> CreateModel(ModelComponent&&) = 0;
	virtual MeshSystem::MeshTree* CreateMeshTree(std::string path) = 0;
	virtual MeshSystem::MeshTree* FindMeshTree(std::string path, MeshSystem::MeshTree* ignore = nullptr) = 0;
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
	virtual void AddLightToScene(LightComponent*) = 0;
	virtual int GetAvailableLightIndex() = 0;

	virtual SearchEngine* GetSearchEngine() = 0;
	virtual PostprocessManager* GetPostprocessHandle() = 0;
	virtual PhysicsEngineManager* GetPhysicsHandle() = 0;
	virtual RenderEngineManager* GetRenderEngineHandle() = 0;
	virtual AudioEngineManager* GetAudioEngineHandle() = 0;

	virtual const GameSettings* GetGameSettings() = 0;
};