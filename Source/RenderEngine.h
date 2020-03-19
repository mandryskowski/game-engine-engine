#pragma once
#include "ModelComponent.h"
#include "LightComponent.h"

enum RenderType
{
	FORWARD,
	DEFERRED
};

class RenderEngine
{
	Shader GShader;
	Shader DefaultShader;
	std::vector <MeshComponent*> Meshes;
	std::vector <ModelComponent*> Models;
	std::vector <Material*> Materials;

	glm::uvec2 ShadowMapSize;
	unsigned int ShadowFBO;
	unsigned int ShadowMapArray;
	unsigned int ShadowCubemapArray;
	Shader DepthShader, DepthLinearizeShader;

	Shader DebugShader;
	std::pair<unsigned int, unsigned int> EngineObjects[4];	//the first element is the object's VAO; the latter is its vertex count

	Shader LightShaders[3];

	unsigned int EmptyTexture;


	void GenerateEngineObjects();
public:
	RenderEngine();
	void AddMesh(MeshComponent* mesh);
	void AddModel(ModelComponent* model);
	void AddModels(std::vector<ModelComponent*>& models);
	void AddMaterial(Material* material);
	void LoadModels();
	void SetupShadowmaps(glm::uvec2);
	void LoadMaterials(std::string);
	MeshComponent* FindMesh(std::string);
	Material* FindMaterial(std::string);
	void RenderEngineObject(RenderInfo&, std::pair<EngineObjectTypes, const Transform*>, Shader* = nullptr);
	void RenderEngineObjects(RenderInfo&, std::vector<std::pair<EngineObjectTypes, const Transform*>>, Shader* = nullptr);
	void RenderShadowMaps(std::vector<LightComponent*>);
	void RenderLightVolume(RenderInfo&, LightType, const Transform* = nullptr, bool = true);
	void RenderLightVolumes(RenderInfo&, std::vector<LightComponent*>, glm::vec2);
	void RenderScene(RenderInfo&, Shader* = nullptr, bool = true);
	void RenderScene(RenderInfo&, std::string, bool = true);
};

glm::vec3 GetCubemapFront(unsigned int);
glm::vec3 GetCubemapUp(unsigned int);