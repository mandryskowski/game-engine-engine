#pragma once
#include "Material.h"

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoord;
	glm::vec3 Tangent;
	glm::vec3 Bitangent;
};

class MeshComponent : public Component
{
	friend class RenderEngine;	//TODO: usun te linijke po zmianie
	friend class ModelComponent;

	unsigned int VAO, VBO, EBO;
	size_t VerticesCount;
	Material* MeshMaterial;

public:
	MeshComponent(std::string="error");
	void SetMaterial(Material*);
	void LoadFromFile(std::string, std::string* = nullptr); //you can pass a std::string* to receive the object's default material name (defined in file)
	void LoadFromAiMesh(const aiScene*, aiMesh*, MaterialLoadingData*);
	void LoadAiScene(std::string);
	void GenerateVAO(std::vector<Vertex>&, std::vector<unsigned int>&);
	virtual void Render(Shader*, glm::mat4&);
};
unsigned int generateVAO(unsigned int&, std::vector<unsigned int>, size_t, float*, size_t = -1);