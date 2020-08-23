#include "Mesh.h"

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

void Mesh::LoadFromGLBuffers(unsigned int vertexCount, unsigned int vao, unsigned int vbo, unsigned int ebo)
{
	VertexCount = vertexCount;
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
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices->size(), &(*indices)[0], GL_STATIC_DRAW);
	}

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(nullptr));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::Normal)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::TexCoord)));
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::Tangent)));
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::Bitangent)));

	for (int i = 0; i < 5; i++)
		glEnableVertexAttribArray(i);

	VertexCount = (unsigned int)vertices->size();
}

void Mesh::Render() const
{
	if (EBO)
		glDrawElements(GL_TRIANGLES, VertexCount, GL_UNSIGNED_INT, nullptr);
	else
		glDrawArrays(GL_TRIANGLES, 0, VertexCount);
}

/*
	====================================================================
	====================================================================
	====================================================================
*/

MeshInstance::MeshInstance(Mesh* mesh, Material* overrideMaterial):
	MeshPtr(mesh)
{
	Material* material = (overrideMaterial) ? (overrideMaterial) : (mesh->GetMaterial());
	MaterialInst = std::make_unique<MaterialInstance>(material);
}

MeshInstance::MeshInstance(const MeshInstance& mesh)
{
	MeshPtr = mesh.MeshPtr;
	MaterialInst = std::make_unique<MaterialInstance>(mesh.MaterialInst->GetMaterialPtr());	//create another instance of the same material
}


MeshInstance::MeshInstance(MeshInstance&& mesh) noexcept
{
	MeshPtr = mesh.MeshPtr;
	MaterialInst = std::make_unique<MaterialInstance>(mesh.MaterialInst->GetMaterialPtr());	//create another instance of the same material
}

const Mesh& MeshInstance::GetMesh() const
{
	return *MeshPtr;
}

Material* MeshInstance::GetMaterialPtr() const
{
	return MaterialInst->GetMaterialPtr();
}

MaterialInstance* MeshInstance::GetMaterialInst() const
{
	return MaterialInst.get();
}

void MeshInstance::SetMaterial(Material* mat)
{
	MaterialInst.release();
	MaterialInst = std::make_unique<MaterialInstance>(mat);
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