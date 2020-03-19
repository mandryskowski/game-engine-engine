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

struct RenderInfo
{
	glm::mat4* view;
	glm::mat4* projection;
	glm::mat4* VP;

	RenderInfo(glm::mat4* v = nullptr, glm::mat4* p = nullptr, glm::mat4* vp = nullptr)
	{
		view = v;
		projection = p;
		VP = vp;
	}
};

class MeshComponent : public Component
{
	friend class RenderEngine;	//TODO: usun te linijke po zmianie
	friend class ModelComponent;

	unsigned int VAO, VBO, EBO;
	size_t VertexCount;
	Material* MeshMaterial;

public:
	MeshComponent(std::string="undefined");
	unsigned int GetVAO();
	unsigned int GetVertexCount();
	void SetMaterial(Material*);
	void LoadFromFile(std::string, std::string* = nullptr); //you can pass a std::string* to receive the object's default material name (defined in file)
	void LoadFromAiMesh(const aiScene*, aiMesh*, bool = true, MaterialLoadingData* = nullptr);
	void LoadAiScene(std::string);
	void GenerateVAO(std::vector<Vertex>&, std::vector<unsigned int>&);
	virtual void Render(Shader*, RenderInfo&);
};
unsigned int generateVAO(unsigned int&, std::vector<unsigned int>, size_t, float*, size_t = -1);
unsigned int loadMeshToVAO(std::string, size_t* = nullptr);