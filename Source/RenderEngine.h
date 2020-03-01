#pragma once
#include "ModelComponent.h"
#include "LightComponent.h"

class RenderEngine
{
	Shader DefaultShader;
	std::vector <MeshComponent*> Meshes;
	std::vector <ModelComponent*> Models;
	std::vector <Material*> Materials;

	glm::uvec2 ShadowMapSize;
	unsigned int ShadowFBO;
	unsigned int ShadowMapArray;
	unsigned int ShadowCubemapArray;
	Shader ShadowShader, ShadowLinearizeShader;

	unsigned int EmptyTexture;

public:
	RenderEngine();
	void AddMesh(MeshComponent* mesh);
	void AddModel(ModelComponent* model);
	void AddModels(std::vector<ModelComponent*>& models);
	void AddMaterial(Material* material);
	void LoadModels();
	void SetupShadowmaps(glm::uvec2);
	void RenderShadowMaps(std::vector<LightComponent*> lights);
	void LoadMaterials(std::string);
	MeshComponent* FindMesh(std::string);
	Material* FindMaterial(std::string);
	void RenderScene(glm::mat4, Shader* = nullptr, bool = true);
};

glm::vec3 GetCubemapFront(unsigned int);
glm::vec3 GetCubemapUp(unsigned int);