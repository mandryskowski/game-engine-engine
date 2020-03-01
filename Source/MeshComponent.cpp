#include "MeshComponent.h"

MeshComponent::MeshComponent(std::string name):
	Component(name, Transform())
{
	VAO = 0;
	VBO = 0;
	EBO = 0;
	VerticesCount = 0;
	MeshMaterial = nullptr;
}

void MeshComponent::SetMaterial(Material* material)
{
	MeshMaterial = material;
}

void MeshComponent::LoadFromFile(std::string path, std::string* materialName)
{
	std::fstream file;
	file.open(path);

	////////////////// MODEL

	size_t nrAttributes;
	std::vector<unsigned int> attributes;
	float* data = nullptr;
	size_t dataSize = 0;

	file >> VerticesCount >> nrAttributes;
	attributes.assign(nrAttributes, 0);
	for (unsigned int i = 0; i < nrAttributes; i++)
	{
		file >> attributes[i];
		dataSize += attributes[i] * VerticesCount;
	}

	data = new float[dataSize];

	for (int i = 0; i < (int)dataSize; i++)
		file >> data[i];

	VAO = generateVAO(VBO, attributes, VerticesCount, data, sizeof(float) * dataSize);
	delete[] data;

	/////////////////// MATERIAL

	std::string nextWord;

	file >> nextWord;

	if (nextWord != "Material" || materialName == nullptr)
		return;

	file >> *materialName;
}

void MeshComponent::LoadFromAiMesh(const aiScene* scene, aiMesh* mesh, MaterialLoadingData* matLoadingData)
{
	std::vector <Vertex> vertices;
	std::vector <unsigned int> indices;

	Name = mesh->mName.C_Str();

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vert;

		aiVector3D pos = mesh->mVertices[i];
		vert.Position.x = pos.x;
		vert.Position.y = pos.y;
		vert.Position.z = pos.z;

		aiVector3D normal = mesh->mNormals[i];
		vert.Normal.x = normal.x;
		vert.Normal.y = normal.y;
		vert.Normal.z = normal.z;

		if (mesh->mTextureCoords[0])
		{
			vert.TexCoord.x = mesh->mTextureCoords[0][i].x;
			vert.TexCoord.y = mesh->mTextureCoords[0][i].y;
		}
		else
			vert.TexCoord = glm::vec2(0.0f);

		aiVector3D tangent = mesh->mTangents[i];
		vert.Tangent.x = tangent.x;
		vert.Tangent.y = tangent.y;
		vert.Tangent.z = tangent.z;

		aiVector3D bitangent = mesh->mBitangents[i];
		vert.Bitangent.x = bitangent.x;
		vert.Bitangent.y = bitangent.y;
		vert.Bitangent.z = bitangent.z;

		vertices.push_back(vert);
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	GenerateVAO(vertices, indices);

	if (mesh->mMaterialIndex >= 0)
	{
		std::vector<aiMaterial*>* loadedAiMaterialsPtr = &matLoadingData->LoadedAiMaterials;
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		for (unsigned int i = 0; i < loadedAiMaterialsPtr->size(); i++)
		{
			if ((*loadedAiMaterialsPtr)[i] == material)
			{
				MeshMaterial = matLoadingData->LoadedMaterials[i];	//MaterialLoadingData::LoadedMaterials and MaterialLoadingData::LoadedAiMaterials are the same size, so the (assimp material -> engine material) indices match too
				return;
			}
		}


		MeshMaterial = new Material("error");
		MeshMaterial->LoadFromAiMaterial(material, matLoadingData);
		matLoadingData->LoadedMaterials.push_back(MeshMaterial);
		matLoadingData->LoadedAiMaterials.push_back(material);
	}
} 

void MeshComponent::LoadAiScene(std::string path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cerr << "ERROR! Can't load mesh " << path << ".\n";
		return;
	}

	//ProcessAiNode(scene, scene->mRootNode);
}

void MeshComponent::GenerateVAO(std::vector <Vertex>& vertices, std::vector <unsigned int>& indices)
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	if (!indices.empty())
	{
		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW);
	}

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(nullptr));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::Normal)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::TexCoord)));
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::Tangent)));
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Vertex::Bitangent)));
	for (int i = 0; i < 5; i++)
		glEnableVertexAttribArray(i);

	VerticesCount = vertices.size();
}

void MeshComponent::Render(Shader* shader, glm::mat4& view)
{
	//TODO: zmienic zaleznie od shadera
	Transform worldTransform = ComponentTransform.GetWorldTransform();
	glm::mat4 modelMat = worldTransform.GetMatrix();
	shader->Uniform3fv("scale", ComponentTransform.Scale);
	shader->UniformMatrix4fv("model", modelMat);
	shader->UniformMatrix4fv("modelView", view * modelMat);
	shader->UniformMatrix3fv("normalMat", ModelToNormal(modelMat));

	if (EBO) 
		glDrawElements(GL_TRIANGLES, VerticesCount, GL_UNSIGNED_INT, nullptr);
	else
		glDrawArrays(GL_TRIANGLES, 0, VerticesCount);
}

////////////////////////////////

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