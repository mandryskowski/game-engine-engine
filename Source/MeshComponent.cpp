#include "MeshComponent.h"

MeshComponent::MeshComponent(std::string name):
	Component(name, Transform())
{
	VAO = -1;
	VBO = -1;
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

void MeshComponent::Render(Shader* shader, glm::mat4 view)
{
	//TODO: zmienic zaleznie od shadera
	Transform worldTransform = ComponentTransform.GetWorldTransform();
	glm::mat4 modelMat = worldTransform.GetMatrix();
	shader->Uniform3fv("scale", ComponentTransform.Scale);
	shader->UniformMatrix4fv("model", modelMat);
	shader->UniformMatrix4fv("modelView", view * modelMat);
	shader->UniformMatrix3fv("normalMat", ModelToNormal(modelMat));

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