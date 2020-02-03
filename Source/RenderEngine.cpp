#include "RenderEngine.h"

RenderEngine::RenderEngine()
{
	//configure some openGL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	//load the default shader
	DefaultShader.LoadShaders("default.vs", "default.fs");
	DefaultShader.UniformBlockBinding("Matrices", 0);
	DefaultShader.UniformBlockBinding("Lights", 1);

	//bind texture units of the default shader
	DefaultShader.Use();
	std::vector<std::pair<unsigned int, std::string>> shaderTextureUnits = {
		std::pair<unsigned int, std::string>(0, "diffuse"),
		std::pair<unsigned int, std::string>(1, "normal"),
		std::pair<unsigned int, std::string>(2, "displacement")
	};
	DefaultShader.SetTextureUnitNames(shaderTextureUnits);
	DefaultShader.Uniform1i("shadowMaps", 10);
	DefaultShader.Uniform1i("shadowCubeMaps", 11);

	ShadowMapSize = glm::uvec2(0);
	ShadowFBO = 0;
	ShadowMapArray = 0;
	ShadowCubemapArray = 0;
	
	//load shadow shaders
	ShadowShader.LoadShaders("shadow.vs", "shadow.fs");
	ShadowLinearizeShader.LoadShaders("shadow_linearize.vs", "shadow_linearize.fs");

	//generate engine's empty texture
	glGenTextures(1, &EmptyTexture);
	glBindTexture(GL_TEXTURE_2D, EmptyTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, glm::value_ptr(glm::vec3(0.0f)));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void RenderEngine::SetupShadowmaps(glm::uvec2 size)
{
	ShadowMapSize = size;

	glGenFramebuffers(1, &ShadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ShadowFBO);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glGenTextures(1, &ShadowMapArray);
	glBindTexture(GL_TEXTURE_2D_ARRAY, ShadowMapArray);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, ShadowMapSize.x, ShadowMapSize.y, 16, 0, GL_DEPTH_COMPONENT, GL_FLOAT, (void*)(nullptr));
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(glm::vec4(1.0f)));

	glGenTextures(1, &ShadowCubemapArray);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, ShadowCubemapArray);
	glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_DEPTH_COMPONENT, ShadowMapSize.x, ShadowMapSize.y, 16 * 6, 0, GL_DEPTH_COMPONENT, GL_FLOAT, (void*)(nullptr));
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderEngine::RenderShadowMaps(std::vector <LightComponent*> lights)
{
	float timeSum = 0.0f;
	float time1, time2;
	glBindFramebuffer(GL_FRAMEBUFFER, ShadowFBO);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glViewport(0, 0, ShadowMapSize.x, ShadowMapSize.y);

	bool bCubemapBound = false;

	for (unsigned int i = 0; i < lights.size(); i++)
	{
		LightComponent* light = lights[i];
		if (light->GetType() == LightType::POINT)
		{
			time1 = glfwGetTime();
			if (!bCubemapBound)
			{
				ShadowLinearizeShader.Use();
				glActiveTexture(GL_TEXTURE0 + 11);
				glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, ShadowCubemapArray);
				bCubemapBound = true;
			}

			glm::vec3 lightPos = light->GetTransform()->GetWorldTransform().Position;

			ShadowLinearizeShader.Uniform1f("far", light->GetFar());
			ShadowLinearizeShader.Uniform3fv("lightPos", lightPos);

			unsigned int cubemapFirst = light->GetShadowMapNr() * 6;
		
			for (unsigned int i = cubemapFirst; i < cubemapFirst + 6; i++)
			{
				ShadowLinearizeShader.UniformMatrix4fv("lightSpaceMatrix", light->GetProjection() * glm::lookAt(lightPos, lightPos + GetCubemapFront(i), GetCubemapUp(i)));
				time1 = glfwGetTime();
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ShadowCubemapArray, 0, i);
				glClear(GL_DEPTH_BUFFER_BIT);
				time2 = glfwGetTime();
				timeSum += time2 - time1;
				RenderScene(light->GetTransform()->GetWorldTransform().GetViewMatrix(), &ShadowLinearizeShader, false);
			}
		}
		else
		{
			if (bCubemapBound || i == 0)
			{
				ShadowShader.Use();
				glActiveTexture(GL_TEXTURE0 + 10);
				glBindTexture(GL_TEXTURE_2D_ARRAY, ShadowMapArray);
				bCubemapBound = false;
			}

			ShadowShader.UniformMatrix4fv("lightSpaceMatrix", light->GetProjection() * light->GetTransform()->GetWorldTransform().GetViewMatrix());
			time1 = glfwGetTime();
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ShadowMapArray, 0, light->GetShadowMapNr());
			glClear(GL_DEPTH_BUFFER_BIT);
			time2 = glfwGetTime();
			timeSum += time2 - time1;
			RenderScene(light->GetTransform()->GetWorldTransform().GetViewMatrix(), &ShadowShader, false);
		}
	}

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_CULL_FACE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	std::cout << "Wyczyscilem sobie " << timeSum * 1000.0f << "ms.\n";
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

void RenderEngine::RenderScene(glm::mat4 view, Shader* shader, bool bUseMaterials)
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
		if (bUseMaterials && materialBound != Meshes[i]->MeshMaterial && Meshes[i]->MeshMaterial)	//jesli ostatni zbindowany material jest taki sam, to nie musimy zmieniac danych w shaderze; oszczedzmy sobie roboty
		{
			Meshes[i]->MeshMaterial->UpdateUBOData(shader, EmptyTexture);
			materialBound = Meshes[i]->MeshMaterial;
		}

		Meshes[i]->Render(shader, view);
	}
}

glm::vec3 GetCubemapFront(unsigned int index)
{
	GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + (index % 6);
	glm::vec3 front(0.0f);
	
	switch (face)
	{
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
		front.x = 1.0f; break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
		front.x = -1.0f; break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
		front.y = 1.0f; break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
		front.y = -1.0f; break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
		front.z = 1.0f; break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
		front.z = -1.0f; break;
	}

	return front;
}

glm::vec3 GetCubemapUp(unsigned int index)
{
	GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + (index % 6);
	
	if (face == GL_TEXTURE_CUBE_MAP_POSITIVE_Y)
		return glm::vec3(0.0f, 0.0f, 1.0f);
	else if (face == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y)
		return glm::vec3(0.0f, 0.0f, -1.0f);
	return glm::vec3(0.0f, -1.0f, 0.0f);
}