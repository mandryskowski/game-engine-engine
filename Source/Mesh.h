#pragma once
#include "Material.h"
#include "FileLoader.h"

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoord;
	glm::vec3 Tangent;
	glm::vec3 Bitangent;
	//glm::ivec4 BoneIDs;
	//glm::vec4 BoneWeights;
};


struct RenderInfo
{
	glm::vec3* camPos;
	glm::mat4* view;
	glm::mat4* projection;
	glm::mat4* VP;
	glm::mat4* previousFrameView;
	bool UseMaterials;
	bool OnlyShadowCasters;
	bool CareAboutShader;
	bool MainPass;

	RenderInfo(glm::mat4* v = nullptr, glm::mat4* p = nullptr, glm::mat4* vp = nullptr, bool materials = true, bool onlyshadow = false, bool careAboutShader = true, bool mainPass = false):
		camPos(nullptr),
		view(v),
		projection(p),
		VP(vp),
		previousFrameView(nullptr),
		UseMaterials(materials),
		OnlyShadowCasters(onlyshadow),
		CareAboutShader(careAboutShader),
		MainPass(mainPass)
	{
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
	Mesh(std::string="undefinedMesh");
	unsigned int GetVAO() const;
	unsigned int GetVertexCount() const;
	std::string GetName() const;
	Material* GetMaterial();
	bool CanCastShadow() const;

	void SetMaterial(Material*);

	void Bind() const;
	void LoadFromGLBuffers(unsigned int vertexCount, unsigned int VAO, unsigned int VBO, unsigned int EBO = 0);
	void GenerateVAO(std::vector<Vertex>*, std::vector<unsigned int>*);
	void Render() const;
};

class MeshInstance
{
	Mesh* MeshPtr;
	std::unique_ptr<MaterialInstance> MaterialInst;

public:
	MeshInstance(Mesh* mesh, Material* overrideMaterial = nullptr);
	MeshInstance(const MeshInstance&);
	MeshInstance(MeshInstance&&) noexcept;

	const Mesh& GetMesh() const;
	Material* GetMaterialPtr() const;
	MaterialInstance* GetMaterialInst() const;

	void SetMaterial(Material*);
};

unsigned int generateVAO(unsigned int&, std::vector<unsigned int>, size_t, float*, size_t = -1);