#include <rendering/Mesh.h>
#include <assetload/FileLoader.h>
#include <scene/hierarchy/HierarchyTree.h>

namespace GEE
{
	Mesh::Mesh(const MeshLoc& name) :
		VBO(0),
		EBO(0),
		VertexCount(0),
		VertsData(nullptr),
		IndicesData(nullptr),
		DefaultMeshMaterial(nullptr),
		Localization(name),
		CastsShadow(true),
		BoundingBox(Vec3f(0.0f), Vec3f(0.0f))
	{
		if (Localization.NodeName.find("_NoShadow") != std::string::npos || Localization.SpecificName.find("_NoShadow") != std::string::npos)
			CastsShadow = false;
	}

	unsigned int Mesh::GetVAO(unsigned int VAOcontext) const
	{
		if (VAOs.empty())
			return 0;

		if (auto found = VAOs.find(VAOcontext); found != VAOs.end())
			return found->second;
		return 0;
	}

	unsigned int Mesh::GetVertexCount() const
	{
		return VertexCount;
	}

	Mesh::MeshLoc Mesh::GetLocalization() const
	{
		return Localization;
	}

	SharedPtr<Material> Mesh::GetMaterial()
	{
		return DefaultMeshMaterial;
	}

	const SharedPtr<Material> Mesh::GetMaterial() const
	{
		return DefaultMeshMaterial;
	}

	std::vector<Vertex>* Mesh::GetVertsData() const
	{
		return VertsData.get();
	}

	std::vector<unsigned int>* Mesh::GetIndicesData() const
	{
		return IndicesData.get();
	}

	Boxf<Vec3f> Mesh::GetBoundingBox() const
	{
		return BoundingBox;
	}

	void Mesh::RemoveVertsAndIndicesData() const
	{
		VertsData = nullptr;
		IndicesData = nullptr;
	}


	bool Mesh::CanCastShadow() const
	{
		return CastsShadow;
	}

	void Mesh::SetMaterial(SharedPtr<Material> material)
	{
		DefaultMeshMaterial = material;
	}

	void Mesh::SetBoundingBox(const Boxf<Vec3f>& bbox)
	{
		BoundingBox = bbox;
	}

	void Mesh::Bind(unsigned int VAOcontext) const
	{
		if (!GetVAO(VAOcontext))
			const_cast<Mesh*>(this)->GenerateVAO(VAOcontext);
		GEE_CORE_ASSERT(VAOs.at(VAOcontext));
		glBindVertexArray(VAOs.at(VAOcontext));
	}

	void Mesh::LoadFromGLBuffers(unsigned int vertexCount, unsigned int vao, unsigned int vbo, unsigned int indexCount, unsigned int ebo)
	{
		VertexCount = vertexCount;
		IndexCount = indexCount;
		VAOs[0] = vao;
		VBO = vbo;
		EBO = ebo;
		DefaultMeshMaterial = nullptr;
	}

	void Mesh::Generate(const std::vector <Vertex>& vertices, const std::vector <unsigned int>& indices, bool keepVerts)
	{
		glBindVertexArray(0);
		glGenBuffers(1, &VBO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &(vertices)[0], GL_STATIC_DRAW);

		if (!indices.empty())
		{
			glGenBuffers(1, &EBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &(indices)[0], GL_STATIC_DRAW);
		}

		VertexCount = vertices.size();
		IndexCount = indices.size();

		if (keepVerts)
		{
			VertsData = MakeShared<std::vector<Vertex>>(vertices);	//copy all vertices to heap
			IndicesData = MakeShared<std::vector<unsigned int>>(indices);	//copy all indices to heap
			std::cout << "Keeping vertices for a mesh that has verts and index count: " << VertexCount << " " << IndexCount << '\n';
		}
	}

	void Mesh::GenerateVAO(unsigned int VAOcontext)
	{
		GEE_CORE_ASSERT(VBO);

		unsigned int vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		if (EBO)
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::Position)));
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::Normal)));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::TexCoord)));

		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::Tangent)));
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::Bitangent)));

		glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::BoneData) + offsetof(VertexBoneData, VertexBoneData::BoneIDs)));
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::BoneData) + offsetof(VertexBoneData, VertexBoneData::BoneWeights)));

		for (int i = 0; i < 7; i++)
			glEnableVertexAttribArray(i);

		glBindVertexArray(0);

		VAOs[VAOcontext] = vao;
	}

	void Mesh::Render() const
	{
		if (EBO)
			glDrawElements(GL_TRIANGLES, IndexCount, GL_UNSIGNED_INT, nullptr);
		else
			glDrawArrays(GL_TRIANGLES, 0, VertexCount);
	}

	/*
		====================================================================
		====================================================================
		====================================================================
	*/


	MeshInstance::MeshInstance(Mesh& mesh, SharedPtr<Material> overrideMaterial) :
		MeshRef(mesh),
		MaterialInst(nullptr)
	{
		SharedPtr<Material> material = (overrideMaterial) ? (overrideMaterial) : (mesh.GetMaterial());
		if (material)
			MaterialInst = MakeShared<MaterialInstance>(material);
	}

	MeshInstance::MeshInstance(Mesh& mesh, SharedPtr<MaterialInstance> matInst) :
		MeshInstance(mesh, SharedPtr<Material>(nullptr))
	{
		SetMaterialInst(matInst);
	}

	MeshInstance::MeshInstance(const MeshInstance& mesh) :
		MeshRef(mesh.MeshRef),
		MaterialInst(mesh.MaterialInst)
	{
		//if (mesh.MaterialInst)
			//MaterialInst = MakeShared<MaterialInstance>(mesh.MaterialInst->GetMaterialRef());	//create another instance of the same material
	}


	MeshInstance::MeshInstance(MeshInstance&& mesh) noexcept :
		MeshRef(mesh.MeshRef),
		MaterialInst(mesh.MaterialInst)
	{
	}

	Mesh& MeshInstance::GetMesh()
	{
		return MeshRef;
	}

	const Mesh& MeshInstance::GetMesh() const
	{
		return MeshRef;
	}

	SharedPtr<Material> MeshInstance::GetMaterialPtr()
	{
		if (MaterialInst)
			return MaterialInst->GetMaterialPtr();

		return nullptr;
	}

	const SharedPtr<Material> MeshInstance::GetMaterialPtr() const
	{
		if (MaterialInst)
			return MaterialInst->GetMaterialPtr();

		return nullptr;
	}

	MaterialInstance* MeshInstance::GetMaterialInst() const
	{
		return MaterialInst.get();
	}

	void MeshInstance::SetMaterial(SharedPtr<Material> mat)
	{
		MaterialInst = MakeShared<MaterialInstance>(mat);
	}

	void MeshInstance::SetMaterialInst(SharedPtr<MaterialInstance> matInst)
	{
		MaterialInst = matInst;
	}

	template<typename Archive>
	inline void MeshInstance::Save(Archive& archive) const
	{
		Mesh mesh = MeshRef;
		SharedPtr<MaterialInstance> materialInst = MaterialInst;
		if (&MaterialInst->GetMaterialRef() == mesh.GetMaterial().get())
			materialInst = nullptr;	//don't save the material instance if its the same as the mesh default material.

		archive(cereal::make_nvp("MeshTreePath", mesh.GetLocalization().GetTreeName()), cereal::make_nvp("MeshNodeName", mesh.GetLocalization().NodeName), cereal::make_nvp("MeshSpecificName", mesh.GetLocalization().SpecificName));
		archive(cereal::make_nvp("MaterialInst", materialInst));
	}
	template<typename Archive>
	inline void MeshInstance::load_and_construct(Archive& archive, cereal::construct<MeshInstance>& construct)
	{
		std::string meshTreePath, meshNodeName, meshSpecificName;
		archive(cereal::make_nvp("MeshTreePath", meshTreePath), cereal::make_nvp("MeshNodeName", meshNodeName), cereal::make_nvp("MeshSpecificName", meshSpecificName));

		Hierarchy::Tree* tree = EngineDataLoader::LoadHierarchyTree(*GameManager::DefaultScene, meshTreePath);
		Mesh* mesh = nullptr;

		if (auto found = tree->FindMesh(meshNodeName, meshSpecificName))
			mesh = found;
		else
		{
			std::cout << "ERROR! Could not find mesh " << meshNodeName + " (" + meshSpecificName + ")" << " in hierarchytree " << meshTreePath << '\n';
			exit(0);
		}

		SharedPtr<MaterialInstance> materialInst;
		archive(cereal::make_nvp("MaterialInst", materialInst));

		if (materialInst)
			construct(*mesh, materialInst);
		else
			construct(*mesh);
	}


	template void MeshInstance::Save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const;
	template void MeshInstance::load_and_construct<cereal::JSONInputArchive>(cereal::JSONInputArchive&, cereal::construct<MeshInstance>&);

	/*
		====================================================================
		====================================================================
		====================================================================
	*/

	unsigned int generateVAO(unsigned int& VBO, std::vector <unsigned int> attributes, size_t vCount, float* data, size_t dataSize)
	{
		if (dataSize == -1)
		{
			dataSize = 0;
			for (unsigned int i = 0; i < attributes.size(); i++)
				dataSize += attributes[i] * sizeof(float) * vCount;
		}

		unsigned int VAO;
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, dataSize, data, GL_STATIC_DRAW);

		size_t offset = 0;
		for (unsigned int i = 0; i < attributes.size(); i++)
		{
			glVertexAttribPointer(i, attributes[i], GL_FLOAT, GL_FALSE, sizeof(float) * attributes[i], (void*)(offset));
			offset += attributes[i] * vCount * sizeof(float);
			glEnableVertexAttribArray(i);
		}

		return VAO;
	}

	VertexBoneData::VertexBoneData() :
		BoneIDs(glm::ivec4(0)),
		BoneWeights(Vec4f(0.0f))
	{
	}

	void VertexBoneData::AddWeight(unsigned int boneID, float boneWeight)
	{
		for (int i = 0; i < 4; i++)
		{
			if (BoneWeights[i] == 0.0f)
			{
				BoneIDs[i] = boneID;
				BoneWeights[i] = boneWeight;
				return;
			}
		}

		std::cerr << "ERROR! Mesh has more than 4 per vertex bone weights, but the engine supports only 4.\n";
	}

}