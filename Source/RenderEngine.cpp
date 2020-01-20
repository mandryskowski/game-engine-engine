#include "RenderEngine.h"

RenderEngine::RenderEngine()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	DefaultShader.LoadShaders("default.vs", "default.fs");
	DefaultShader.UniformBlockBinding("Matrices", 0);
	DefaultShader.UniformBlockBinding("Lights", 1);

	DefaultShader.Use();
	std::vector<std::pair<unsigned int, std::string>> shaderTextureUnits = {
		std::pair<unsigned int, std::string>(0, "diffuse"),
		std::pair<unsigned int, std::string>(1, "normal"),
		std::pair<unsigned int, std::string>(2, "displacement")
	};
	DefaultShader.SetTextureUnitNames(shaderTextureUnits);

	//generate engine's empty texture
	glGenTextures(1, &EmptyTexture);
	glBindTexture(GL_TEXTURE_2D, EmptyTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, glm::value_ptr(glm::vec3(0.0f)));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

MeshComponent* RenderEngine::FindMesh(std::string name)
{
	for (unsigned int i = 0; i < Meshes.size(); i++)
		if (Meshes[i]->GetName() == name)
			return Meshes[i];

	return nullptr;
}

Material* RenderEngine::FindMaterial(std::string name)
{
	for (unsigned int i = 0; i < Materials.size(); i++)
		if (Materials[i]->GetName() == name)
			return Materials[i];

	std::cerr << "Can't find material " << name << "!\n";
	
	return nullptr;
}

void RenderEngine::RenderScene(glm::mat4 view, Shader* shader)
{
	if (!shader)
		shader = &DefaultShader;

	shader->Use();
	unsigned int VAOBound = 0;
	Material* materialBound = nullptr;
	Shader* shaderBound = shader;

	for (unsigned int i = 0; i < Meshes.size(); i++)
	{
		if (VAOBound != Meshes[i]->VAO || i == 0)
		{
			glBindVertexArray(Meshes[i]->VAO);
			VAOBound = Meshes[i]->VAO;
		}
		if (materialBound != Meshes[i]->MeshMaterial && Meshes[i]->MeshMaterial)	//jesli ostatni zbindowany material jest taki sam, to nie musimy zmieniac danych w shaderze; oszczedzmy sobie roboty
		{
			Meshes[i]->MeshMaterial->UpdateUBOData(shader, EmptyTexture);
			materialBound = Meshes[i]->MeshMaterial;
		}

		Meshes[i]->Render(shader, view);
	}
}