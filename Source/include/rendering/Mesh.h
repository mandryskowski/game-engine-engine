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
public:

	struct MeshLoc	//exact localization of the mesh
	{
		std::string HierarchyTreePath;
		std::string NodeName;
		std::string SpecificName;
		MeshLoc(const std::string& path, const std::string& nodeName, const std::string& specificName) : HierarchyTreePath(path), NodeName(nodeName), SpecificName(specificName) {}
		static MeshLoc FromNodeName(const std::string& nodeName, const std::string& path = std::string())
		{
			return MeshLoc(path, nodeName, "");
		}
	};

	Mesh(const MeshLoc&);
	unsigned int GetVAO() const;
	unsigned int GetVertexCount() const;
	MeshLoc GetLocalization() const;
	Material* GetMaterial();
	const Material* GetMaterial() const;
	bool CanCastShadow() const;

	void SetMaterial(Material*);

	void Bind() const;
	void LoadFromGLBuffers(unsigned int vertexCount, unsigned int VAO, unsigned int VBO, unsigned int indexCount = 0, unsigned int EBO = 0);
	void GenerateVAO(std::vector<Vertex>*, std::vector<unsigned int>*);
	void Render() const;
	template <typename Archive> void Save(Archive& archive) const
	{
		archive(cereal::make_nvp("HierarchyTreePath", Localization.HierarchyTreePath), cereal::make_nvp("NodeName", Localization.NodeName), cereal::make_nvp("SpecificName", Localization.SpecificName), cereal::make_nvp("CastsShadow", CastsShadow));
	}
	template <typename Archive> void Load(Archive& archive)
	{
		archive(cereal::make_nvp("HierarchyTreePath", Localization.HierarchyTreePath), cereal::make_nvp("NodeName", Localization.NodeName), cereal::make_nvp("SpecificName", Localization.SpecificName), cereal::make_nvp("CastsShadow", CastsShadow));

		HierarchyTemplate::HierarchyTreeT* tree = EngineDataLoader::LoadHierarchyTree(*GameManager::DefaultScene, Localization.HierarchyTreePath);

		if (auto found = tree->FindMesh(Localization))
			*this = *found;
		else
		{
			std::cout << "ERROR! Could not find mesh " << Localization.NodeName + " (" + Localization.SpecificName + ")"  << " in hierarchytree " << Localization.HierarchyTreePath << '\n';
			exit(0);
		}
	}

private:
	friend class RenderEngine;	//TODO: usun te linijke po zmianie
	friend class ModelComponent;

	MeshLoc Localization;

	unsigned int VAO, VBO, EBO;
	unsigned int VertexCount, IndexCount;
	Material* DefaultMeshMaterial;

	bool CastsShadow;
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

	template <typename Archive> void Save(Archive& archive)	const
	{
		Mesh mesh = MeshRef;
		std::shared_ptr<MaterialInstance> materialInst = MaterialInst;
		if (&MaterialInst->GetMaterialRef() == mesh.GetMaterial())
			materialInst = nullptr;	//don't save the material instance if its the same as the mesh default material.

		archive(cereal::make_nvp("Mesh", mesh));
		archive(cereal::make_nvp("MaterialInst", materialInst));
	}
	template <typename Archive> static void load_and_construct(Archive& archive, cereal::construct<MeshInstance>& construct)
	{
		Mesh* mesh = new Mesh(Mesh::MeshLoc("if_you_see_this_serializing_error_ocurred", "if_you_see_this_serializing_error_ocurred", "if_you_see_this_serializing_error_ocurred"));
		std::shared_ptr<MaterialInstance> materialInst;
		archive(cereal::make_nvp("Mesh", *mesh), cereal::make_nvp("MaterialInst", materialInst));

		if (materialInst)
			construct(MeshInstance(*mesh, materialInst));
		else
			construct(MeshInstance(*mesh));
	}
};

unsigned int generateVAO(unsigned int&, std::vector<unsigned int>, size_t, float*, size_t = -1);