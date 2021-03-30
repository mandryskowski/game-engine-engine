#pragma once
#include "Material.h"


struct VertexBoneData
{
	glm::ivec4 BoneIDs;
	glm::vec4 BoneWeights;
	VertexBoneData();
	void AddWeight(unsigned int boneID, float boneWeight);
};

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoord;
	glm::vec3 Tangent;
	glm::vec3 Bitangent;
	VertexBoneData BoneData;
};

class Mesh
{
	friend class RenderEngine;	//TODO: usun te linijke po zmianie
	friend class ModelComponent;

	unsigned int VAO, VBO, EBO;
	unsigned int VertexCount, IndexCount;
	Material* DefaultMeshMaterial;

	std::string Name;
	bool CastsShadow;

	GameManager* GameHandle;

public:
	Mesh(std::string = "undefinedMesh");
	unsigned int GetVAO() const;
	unsigned int GetVertexCount() const;
	std::string GetName() const;
	Material* GetMaterial();
	const Material* GetMaterial() const;
	bool CanCastShadow() const;

	void SetMaterial(Material*);

	void Bind() const;
	void LoadFromGLBuffers(unsigned int vertexCount, unsigned int VAO, unsigned int VBO, unsigned int indexCount = 0, unsigned int EBO = 0);
	void GenerateVAO(std::vector<Vertex>*, std::vector<unsigned int>*);
	void Render() const;
};

class MeshInstance
{
	Mesh& MeshRef;
	std::shared_ptr<MaterialInstance> MaterialInst;

public:
	MeshInstance(Mesh& mesh, Material* overrideMaterial = nullptr);
	MeshInstance(Mesh& mesh, std::shared_ptr<MaterialInstance>);
	MeshInstance(const MeshInstance&);
	MeshInstance(MeshInstance&&) noexcept;

	const Mesh& GetMesh() const;
	const Material* GetMaterialPtr() const;
	MaterialInstance* GetMaterialInst() const;

	void SetMaterial(Material*);
	void SetMaterialInst(std::shared_ptr<MaterialInstance>);
};

unsigned int generateVAO(unsigned int&, std::vector<unsigned int>, size_t, float*, size_t = -1);