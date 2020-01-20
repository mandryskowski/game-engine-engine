#pragma once
#include "MeshComponent.h"
class RenderEngine
{
	Shader DefaultShader;
	std::vector <MeshComponent*> Meshes;
	std::vector <Material*> Materials;

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
	MeshComponent* FindMesh(std::string);
	Material* FindMaterial(std::string);
	void RenderScene(glm::mat4, Shader* = nullptr);
};