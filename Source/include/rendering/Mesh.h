#pragma once
#include "Material.h"

namespace GEE
{
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

	namespace HierarchyTemplate
	{
		class HierarchyTreeT;
	}

	class Mesh
	{
	public:

		struct MeshLoc : public HTreeObjectLoc	//exact localization of the mesh
		{
			std::string NodeName;
			std::string SpecificName;
			MeshLoc(HTreeObjectLoc treeObjectLoc, const std::string& nodeName, const std::string& specificName) : HTreeObjectLoc(treeObjectLoc), NodeName(nodeName), SpecificName(specificName) {}
		};

		Mesh(const MeshLoc&);
		unsigned int GetVAO() const;
		unsigned int GetVertexCount() const;
		MeshLoc GetLocalization() const;
		Material* GetMaterial();
		const Material* GetMaterial() const;
		std::vector<Vertex>* GetVertsData() const;
		std::vector<unsigned int>* GetIndicesData() const;
		void RemoveVertsAndIndicesData() const;
		bool CanCastShadow() const;

		void SetMaterial(Material*);

		void Bind() const;
		void LoadFromGLBuffers(unsigned int vertexCount, unsigned int VAO, unsigned int VBO, unsigned int indexCount = 0, unsigned int EBO = 0);
		void GenerateVAO(const std::vector<Vertex>&, const std::vector<unsigned int>&, bool keepVerts = false);
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
				std::cout << "ERROR! Could not find mesh " << Localization.NodeName + " (" + Localization.SpecificName + ")" << " in hierarchytree " << Localization.HierarchyTreePath << '\n';
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

		mutable std::shared_ptr<std::vector<Vertex>> VertsData;
		mutable std::shared_ptr<std::vector<unsigned int>> IndicesData;

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

		Mesh& GetMesh();
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

			archive(cereal::make_nvp("MeshTreePath", mesh.GetLocalization().GetTreeName()), cereal::make_nvp("MeshNodeName", mesh.GetLocalization().NodeName), cereal::make_nvp("MeshSpecificName", mesh.GetLocalization().SpecificName));
			archive(cereal::make_nvp("MaterialInst", materialInst));
		}
		template <typename Archive> static void load_and_construct(Archive& archive, cereal::construct<MeshInstance>& construct)
		{
			std::string meshTreePath, meshNodeName, meshSpecificName;
			archive(cereal::make_nvp("MeshTreePath", meshTreePath), cereal::make_nvp("MeshNodeName", meshNodeName), cereal::make_nvp("MeshSpecificName", meshSpecificName));

			HierarchyTemplate::HierarchyTreeT* tree = EngineDataLoader::LoadHierarchyTree(*GameManager::DefaultScene, meshTreePath);
			Mesh* mesh = nullptr;

			if (auto found = tree->FindMesh(meshNodeName, meshSpecificName))
				mesh = found;
			else
			{
				std::cout << "ERROR! Could not find mesh " << meshNodeName + " (" + meshSpecificName + ")" << " in hierarchytree " << meshTreePath << '\n';
				exit(0);
			}

			std::shared_ptr<MaterialInstance> materialInst;
			archive(cereal::make_nvp("MaterialInst", materialInst));

			if (materialInst)
				construct(MeshInstance(*mesh, materialInst));
			else
				construct(MeshInstance(*mesh));
		}
	};

	unsigned int generateVAO(unsigned int&, std::vector<unsigned int>, size_t, float*, size_t = -1);
}