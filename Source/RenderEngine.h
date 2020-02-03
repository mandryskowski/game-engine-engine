#pragma once
#include "MeshComponent.h"
#include "LightComponent.h"

class RenderEngine
{
	Shader DefaultShader;
	std::vector <MeshComponent*> Meshes;
	std::vector <Material*> Materials;

	glm::uvec2 ShadowMapSize;
	unsigned int ShadowFBO;
	unsigned int ShadowMapArray;
	unsigned int ShadowCubemapArray;
	Shader ShadowShader, ShadowLinearizeShader;

	unsigned int EmptyTexture;

public:
	RenderEngine();
	void AddMesh(MeshComponent* mesh)
	{
		Meshes.push_back(mesh);
	}
	void AddMaterial(Material* material)
	{
		Materials.push_back(material);
	}
	void SetupShadowmaps(glm::uvec2);
	void RenderShadowMaps(std::vector<LightComponent*> lights);
	MeshComponent* FindMesh(std::string);
	Material* FindMaterial(std::string);
	void RenderScene(glm::mat4, Shader* = nullptr, bool = true);
};

glm::vec3 GetCubemapFront(unsigned int);
glm::vec3 GetCubemapUp(unsigned int);