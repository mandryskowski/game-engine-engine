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
	bool UseMaterials;
	bool OnlyShadowCasters;
	bool CareAboutShader;

	RenderInfo(glm::mat4* v = nullptr, glm::mat4* p = nullptr, glm::mat4* vp = nullptr, bool materials = true, bool onlyshadow = false, bool careAboutShader = true)
	{
		view = v;
		projection = p;
		VP = vp;
		UseMaterials = materials;
		OnlyShadowCasters = onlyshadow;
		CareAboutShader = careAboutShader;
	}
};

class Mesh
{
	friend class RenderEngine;	//TODO: usun te linijke po zmianie
	friend class ModelComponent;

	unsigned int VAO, VBO, EBO;
	unsigned int VertexCount;
	Material* DefaultMeshMaterial;

	std::string Name;
	bool CastsShadow;

public:
	Mesh(std::string="undefined");
	unsigned int GetVAO();
	unsigned int GetVertexCount();
	std::string GetName();
	Material* GetMaterial();
	bool CanCastShadow();
	void SetMaterial(Material*);
	void LoadFromAiMesh(const aiScene*, aiMesh*, std::string directory, bool = true, MaterialLoadingData* = nullptr);
	void LoadFromGLBuffers(unsigned int vertexCount, unsigned int VAO, unsigned int VBO, unsigned int EBO = 0);
	void LoadSingleFromAiFile(std::string, bool = true);
	void GenerateVAO(std::vector<Vertex>&, std::vector<unsigned int>&);
	void Render();
};

class MeshInstance
{
	Mesh* MeshPtr;
	MaterialInstance* MaterialInst;

public:
	MeshInstance(Mesh* mesh, Material* overrideMaterial = nullptr);
	Mesh* GetMesh();
	Material* GetMaterialPtr();
	MaterialInstance* GetMaterialInst();
};

unsigned int generateVAO(unsigned int&, std::vector<unsigned int>, size_t, float*, size_t = -1);
unsigned int loadMeshToVAO(std::string, size_t* = nullptr);