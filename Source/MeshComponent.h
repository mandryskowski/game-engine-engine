#pragma once
#include "Material.h"

enum MeshType
{
	MESH_NONE,
	MESH_FORWARD,
	MESH_DEFERRED,
	MESH_ALL
};

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

	MeshType Type;
	/* A mesh can be of different types (they are only for semantic purposes!):
		NONE - doesn't clasiffy as a normal mesh; it shouldn't be rendered
		FORWARD - ignore this object in deferred-rendering. When an algorithm like SSAO (or other that uses textures from G-Buffer) is enabled, this mesh should be rendered just like deferred-type objects (it will be rendered again in the forward pass).
		DEFERRED - render in the geometry pass, don't render in the forward pass
		ALL - always render it, no matter what (bad for optimisation, stability and barely fits in the semantic way - avoid assigning this state to a MeshComponent)

		Basically, if you follow the semantics, the performance from highest to lowest is: NONE(not rendered) - DEFERRED - FORWARD - ALL
	*/

public:
	MeshComponent(std::string="undefined");
	MeshType GetMeshType();
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