#pragma once
#include "Postprocess.h"
#include "GameManager.h"
#include "MeshSystem.h"

class Framebuffer;

class RenderEngine: public RenderEngineManager
{
	Shader GShader;
	Shader DefaultShader;
	std::vector <std::shared_ptr<ModelComponent>> Models;
	std::vector <Material*> Materials;
	std::vector <Mesh*> MeshCollection;
	std::vector <std::unique_ptr<MeshSystem::MeshTree>> MeshTrees;

	glm::uvec2 ShadowMapSize;
	unsigned int ShadowFBO;
	unsigned int ShadowMapArray;
	unsigned int ShadowCubemapArray;
	Shader DepthShader, DepthLinearizeShader;

	Shader DebugShader;
	Mesh* EngineObjects[4];
	Shader LightShaders[3];
	std::vector<Shader*> ExternalShaders;
	unsigned int EmptyTexture;

	GameManager* GameHandle;


	void GenerateEngineObjects();
public:
	RenderEngine(GameManager*);
	void Init();

	std::vector<Shader*> GetForwardShaders();

	virtual void AddModel(std::shared_ptr<ModelComponent> model) override;
	virtual std::shared_ptr<ModelComponent> CreateModel(const ModelComponent&) override;
	virtual std::shared_ptr<ModelComponent> CreateModel(ModelComponent&&) override;
	virtual MeshSystem::MeshTree* CreateMeshTree(std::string path) override;
	virtual MeshSystem::MeshTree* FindMeshTree(std::string path, MeshSystem::MeshTree* ignore = nullptr) override;
	virtual void AddMaterial(Material* material) override;
	virtual void AddExternalShader(Shader* shader) override;

	void SetupShadowmaps(glm::uvec2);

	ModelComponent* FindModel(std::string);
	Material* FindMaterial(std::string);
	Shader* FindShader(std::string);

	void RenderBoundInDebug(RenderInfo&, GLenum mode, GLint first, GLint count, glm::vec3 color = glm::vec3(1.0f));
	void RenderEngineObject(RenderInfo&, std::pair<EngineObjectType, Transform*>, Shader* = nullptr);
	void RenderEngineObjects(RenderInfo&, std::vector<std::pair<EngineObjectType, Transform*>>, Shader* = nullptr);
	void RenderShadowMaps(std::vector<LightComponent*>);
	void RenderLightVolume(RenderInfo&, LightComponent*, Transform* = nullptr, bool = true);
	void RenderLightVolumes(RenderInfo&, std::vector<LightComponent*>, glm::vec2);
	void RenderScene(RenderInfo& info, Shader* shader = nullptr);
	void Render(const ModelComponent& model, Shader* shader, RenderInfo& info, unsigned int VAOBound, Material* materialBound, unsigned int emptyTexture);
};

glm::vec3 GetCubemapFront(unsigned int);
glm::vec3 GetCubemapUp(unsigned int);