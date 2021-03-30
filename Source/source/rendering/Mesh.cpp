#include <rendering/Mesh.h>
#include <assetload/FileLoader.h>

Mesh::Mesh(std::string name):
	VAO(0),
	VBO(0),
	EBO(0),
	VertexCount(0),
	DefaultMeshMaterial(nullptr),
	Name(name),
	CastsShadow(true)
{
	if (Name.find("_NoShadow") != std::string::npos)
		CastsShadow = false;
}

unsigned int Mesh::GetVAO() const
{
	return VAO;
}

unsigned int Mesh::GetVertexCount() const
{
	return VertexCount;
}

std::string Mesh::GetName() const
{
	return Name;
}

Material* Mesh::GetMaterial()
{
	return DefaultMeshMaterial;
}

const Material* Mesh::GetMaterial() const
{
	return DefaultMeshMaterial;
}


bool Mesh::CanCastShadow() const
{
	return CastsShadow;
}

void Mesh::SetMaterial(Material* material)
{
	DefaultMeshMaterial = material;
}

void Mesh::Bind() const
{
	glBindVertexArray(VAO);
}

void Mesh::LoadFromGLBuffers(unsigned int vertexCount, unsigned int vao, unsigned int vbo, unsigned int indexCount, unsigned int ebo)
{
	VertexCount = vertexCount;
	IndexCount = indexCount;
	VAO = vao;
	VBO = vbo;
	EBO = ebo;
	DefaultMeshMaterial = nullptr;
}

void Mesh::GenerateVAO(std::vector <Vertex>* vertices, std::vector <unsigned int>* indices)
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices->size(), &(*vertices)[0], GL_STATIC_DRAW);

	if (!indices->empty())
	{
		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		//std::cout << "uwagaaa: " + std::to_string(indices->size()) + "\n";
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices->size(), &(*indices)[0], GL_STATIC_DRAW);
		//std::cout << "po uwadze\n";
	}

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::Position)));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::Normal)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::TexCoord)));

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::Tangent)));
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::Bitangent)));

	glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::BoneData) + offsetof(VertexBoneData, VertexBoneData::BoneIDs)));
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::BoneData) + offsetof(VertexBoneData, VertexBoneData::BoneWeights)));

	for (int i = 0; i < 7; i++)
		glEnableVertexAttribArray(i);

	VertexCount = (unsigned int)vertices->size();
	IndexCount = (unsigned int)indices->size();
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


MeshInstance::MeshInstance(Mesh& mesh, Material* overrideMaterial):
	MeshRef(mesh),
	MaterialInst(nullptr)
{
	Material* material = (overrideMaterial) ? (overrideMaterial) : (mesh.GetMaterial());
	if (material)
		MaterialInst = std::make_shared<MaterialInstance>(MaterialInstance(*material));
}

MeshInstance::MeshInstance(Mesh& mesh, std::shared_ptr<MaterialInstance> matInst):
	MeshInstance(mesh, nullptr)
{
	SetMaterialInst(matInst);
}

MeshInstance::MeshInstance(const MeshInstance& mesh):
	MaterialInst(nullptr),
	MeshRef(mesh.MeshRef)
{
	if (mesh.MaterialInst)
		MaterialInst = std::make_shared<MaterialInstance>(mesh.MaterialInst->GetMaterialRef());	//create another instance of the same material
}


MeshInstance::MeshInstance(MeshInstance&& mesh) noexcept:
	MeshRef(mesh.MeshRef)
{
	if (mesh.MaterialInst)
		MaterialInst = std::move(mesh.MaterialInst);	//move the material instance
}

const Mesh& MeshInstance::GetMesh() const
{
	return MeshRef;
}

const Material* MeshInstance::GetMaterialPtr() const
{
	if (MaterialInst)
		return &MaterialInst->GetMaterialRef();

	return nullptr;
}

MaterialInstance* MeshInstance::GetMaterialInst() const
{
	return MaterialInst.get();
}

void MeshInstance::SetMaterial(Material* mat)
{
	MaterialInst = std::make_shared<MaterialInstance>(*mat);
}

void MeshInstance::SetMaterialInst(std::shared_ptr<MaterialInstance> matInst)
{
	MaterialInst = matInst;
}

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
	BoneWeights(glm::vec4(0.0f))
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
