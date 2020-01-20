#pragma once
#include "Material.h"

class MeshComponent : public Component
{
	friend class RenderEngine;

	unsigned int VAO, VBO;
	size_t VerticesCount;
	Material* MeshMaterial;

public:
	MeshComponent(std::string);
	void SetMaterial(Material*);
	void LoadFromFile(std::string, std::string* = nullptr); //you can pass a std::string* to receive the object's default material name (defined in file)
	virtual void Render(Shader*, glm::mat4);
};
unsigned int generateVAO(unsigned int&, std::vector<unsigned int>, size_t, float*, size_t = -1);