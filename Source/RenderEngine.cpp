#include "RenderEngine.h"

RenderEngine::RenderEngine()
{
	//configure some openGL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	GShader.LoadShaders("geometry.vs", "geometry.fs");
	GShader.UniformBlockBinding("Matrices", 0);
	
	GShader.Use();
	std::vector<std::pair<unsigned int, std::string>> gShaderTextureUnits = {
		std::pair<unsigned int, std::string>(0, "diffuse1"),
		std::pair<unsigned int, std::string>(1, "specular1"),
		std::pair<unsigned int, std::string>(2, "normal1"),
		std::pair<unsigned int, std::string>(3, "depth1")
	};
	GShader.SetTextureUnitNames(gShaderTextureUnits);

	//load the default shader
	DefaultShader.LoadShaders("default.vs", "default.fs");
	DefaultShader.UniformBlockBinding("Matrices", 0);
	DefaultShader.UniformBlockBinding("Lights", 1);

	DefaultShader.Use();
	std::vector<std::pair<unsigned int, std::string>> defaultShaderTextureUnits = {
		std::pair<unsigned int, std::string>(0, "diffuse1"),
		std::pair<unsigned int, std::string>(1, "specular1"),
		std::pair<unsigned int, std::string>(2, "normal1"),
		std::pair<unsigned int, std::string>(3, "depth1")
	};
	DefaultShader.SetTextureUnitNames(defaultShaderTextureUnits);
	DefaultShader.Uniform1i("shadowMaps", 10);
	DefaultShader.Uniform1i("shadowCubeMaps", 11);

	//initialize shadow mapping stuff
	ShadowMapSize = glm::uvec2(0);
	ShadowFBO = 0;
	ShadowMapArray = 0;
	ShadowCubemapArray = 0;
	
	//load shadow shaders
	DepthShader.LoadShaders("depth.vs", "depth.fs");
	DepthLinearizeShader.LoadShaders("depth_linearize.vs", "depth_linearize.fs");

	//load debug shaders
	DebugShader.LoadShaders("debug.vs", "debug.fs");
	DebugShader.UniformBlockBinding("Matrices", 0);

	GenerateEngineObjects();

	//LightShaders[LightType::DIRECTIONAL].LoadShaders("directional.vs", "directional.fs");
	//LightShaders[LightType::DIRECTIONAL].UniformBlockBinding("Lights", 1);
	LightShaders[LightType::POINT].LoadShaders("point.vs", "point.fs");
	LightShaders[LightType::POINT].UniformBlockBinding("Matrices", 0);
	LightShaders[LightType::POINT].UniformBlockBinding("Lights", 1);
	LightShaders[LightType::SPOT].LoadShaders("spot.vs", "spot.fs");
	LightShaders[LightType::SPOT].UniformBlockBinding("Matrices", 0);
	LightShaders[LightType::SPOT].UniformBlockBinding("Lights", 1);


	for (int i = 0; i < 3; i++)
	{
		if (LightType::POINT != i && LightType::SPOT != i)		//!!!!!!!!!!!!!!!!!!!!!!!!!DELETE THIS!!!!!!!!!!!!!!!!!!!!!!!!!1
			continue;
		LightShaders[i].Use();
		LightShaders[i].Uniform1i("gPosition", 0);
		LightShaders[i].Uniform1i("gNormal", 1);
		LightShaders[i].Uniform1i("gAlbedoSpec", 2);
		
		if (i == (int)LightType::POINT)
			LightShaders[i].Uniform1i("shadowCubemaps", 11);
		else
			LightShaders[i].Uniform1i("shadowMaps", 10);
	}

	//generate engine's empty texture
	glGenTextures(1, &EmptyTexture);
	glBindTexture(GL_TEXTURE_2D, EmptyTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, glm::value_ptr(glm::vec3(0.0f, 0.0f, 1.0f)));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void RenderEngine::GenerateEngineObjects()
{
	glm::vec4 quadVertices[] = {
		glm::vec4(-1.0f, -1.0f,		0.0f, 0.0f),
		glm::vec4( 1.0f, -1.0f,		1.0f, 0.0f),
		glm::vec4( 1.0f,  1.0f,		1.0f, 1.0f),

		glm::vec4( 1.0f,  1.0f,		1.0f, 1.0f),
		glm::vec4(-1.0f,  1.0f,		0.0f, 1.0f),
		glm::vec4(-1.0f, -1.0f,		0.0f, 0.0f)
	};

	glm::vec3 cubeVertices[] = {
		//FRONT
		glm::vec3(0.5f, -0.5f, -0.5f),
		glm::vec3(-0.5f, -0.5f, -0.5f),
		glm::vec3(-0.5f,  0.5f, -0.5f),

		glm::vec3(-0.5f,  0.5f, -0.5f),
		glm::vec3(0.5f,  0.5f, -0.5f),
		glm::vec3(0.5f, -0.5f, -0.5f),

		//BACK
		glm::vec3(-0.5f, -0.5f,  0.5f),
		glm::vec3(0.5f, -0.5f,  0.5f),
		glm::vec3(0.5f,  0.5f,  0.5f),

		glm::vec3(0.5f,  0.5f,  0.5f),
		glm::vec3(-0.5f,  0.5f,  0.5f),
		glm::vec3(-0.5f, -0.5f,  0.5f),

		//LEFT
		glm::vec3(-0.5f, -0.5f, -0.5f),
		glm::vec3(-0.5f, -0.5f,  0.5f),
		glm::vec3(-0.5f,  0.5f,  0.5f),

		glm::vec3(-0.5f,  0.5f,  0.5f),
		glm::vec3(-0.5f,  0.5f, -0.5f),
		glm::vec3(-0.5f, -0.5f, -0.5f),

		//RIGHT
		glm::vec3(0.5f, -0.5f,  0.5f),
		glm::vec3(0.5f, -0.5f, -0.5f),
		glm::vec3(0.5f,  0.5f, -0.5f),

		glm::vec3(0.5f,  0.5f, -0.5f),
		glm::vec3(0.5f,  0.5f,  0.5f),
		glm::vec3(0.5f, -0.5f,  0.5f),

		//TOP
		glm::vec3(-0.5f,  0.5f,  0.5f),
		glm::vec3(0.5f,  0.5f,  0.5f),
		glm::vec3(0.5f,  0.5f, -0.5f),

		glm::vec3(0.5f,  0.5f, -0.5f),
		glm::vec3(-0.5f,  0.5f, -0.5f),
		glm::vec3(-0.5f,  0.5f,  0.5f),

		//BOTTOM
		glm::vec3(-0.5f, -0.5f, -0.5f),
		glm::vec3(0.5f, -0.5f, -0.5f),
		glm::vec3(0.5f, -0.5f,  0.5f),

		glm::vec3(0.5f, -0.5f,  0.5f),
		glm::vec3(-0.5f, -0.5f,  0.5f),
		glm::vec3(-0.5f, -0.5f, -0.5f)
	};

	unsigned int quadVBO;
	unsigned int quadVAO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);

	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)(nullptr));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)(sizeof(float) * 2));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	EngineObjects[EngineObjectTypes::QUAD].first = quadVAO;
	EngineObjects[EngineObjectTypes::QUAD].second = 6;

	unsigned int cubeVBO;
	unsigned int cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);

	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)(nullptr));
	glEnableVertexAttribArray(0);

	EngineObjects[EngineObjectTypes::CUBE].first = cubeVAO;
	EngineObjects[EngineObjectTypes::CUBE].second = 36;

	size_t vertexCount = 0;
	EngineObjects[EngineObjectTypes::SPHERE].first = loadMeshToVAO("sphere.obj", &vertexCount);
	EngineObjects[EngineObjectTypes::SPHERE].second = vertexCount;

	EngineObjects[EngineObjectTypes::CONE].first = loadMeshToVAO("cone3.obj", &vertexCount);
	EngineObjects[EngineObjectTypes::CONE].second = vertexCount;
}

void RenderEngine::AddMesh(MeshComponent* mesh)
{
	Meshes.push_back(mesh);
}
void RenderEngine::AddModel(ModelComponent* model)
{
	Models.push_back(model);
}
void RenderEngine::AddModels(std::vector<ModelComponent*>& models)
{
	for (unsigned int i = 0; i < models.size(); i++)
		AddModel(models[i]);
}
void RenderEngine::AddMaterial(Material* material)
{
	Materials.push_back(material);
}
void RenderEngine::LoadModels()
{
	std::vector <std::string> filePaths;
	for (unsigned int i = 0; i < Models.size(); i++)
	{
		std::string path = Models[i]->GetFilePath();
		for (unsigned int j = 0; j < i; j++)	//check for a duplicate
		{
			if (filePaths[j] == path)
			{
				path.clear();
				break;
			}
		}

		if (!path.empty())
			filePaths.push_back(path);
	}

	Assimp::Importer importer;
	std::vector <const aiScene*> scenes;
	MaterialLoadingData matLoadingData;

	for (unsigned int i = 0; i < filePaths.size(); i++)
	{
		scenes.push_back(importer.ReadFile(filePaths[i], aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace));
		if (!scenes[i] || scenes[i]->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scenes[i]->mRootNode)
		{
			std::cerr << "Can't load scene " << filePaths[i] << ".\n";
			continue;
		}

		std::vector<ModelComponent*> modelsPtr;
		for (unsigned int j = 0; j < Models.size(); j++)	//for all models that use this file path
			if (Models[j]->GetFilePath() == filePaths[i])
				Models[j]->ProcessAiNode(scenes[i], scenes[i]->mRootNode, modelsPtr, &matLoadingData);

		AddModels(modelsPtr);
	}
}

void RenderEngine::SetupShadowmaps(glm::uvec2 size)
{
	ShadowMapSize = size;

	glGenFramebuffers(1, &ShadowFBO);	//TODO: Update this so it uses Framebuffer class
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

void RenderEngine::LoadMaterials(std::string path)
{
	std::ifstream file;
	file.open(path);
	std::stringstream filestr;
	filestr << file.rdbuf();

	if (!file.good())
	{
		std::cerr << "Cannot open materials file " << path << "!\n";
		return;
	}

	while (!isNextWordEqual(filestr, "end"))
	{ 
		std::string materialName;
		float shininess, depthScale;
		unsigned int texCount;

		filestr >> materialName >> shininess >> depthScale >> texCount;

		Material* material = new Material(materialName, shininess, depthScale);
		Materials.push_back(material);

		for (unsigned int i = 0; i < texCount; i++)
		{
			std::string path, shaderName, isSRGB;
			filestr >> path >> shaderName >> isSRGB;

			material->AddTexture(new Texture(path, shaderName, toBool(isSRGB)));
		}
	}
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

void RenderEngine::RenderEngineObject(RenderInfo& info, std::pair<EngineObjectTypes, const Transform*> object, Shader* shader)
{
	RenderEngineObjects(info, std::vector<std::pair<EngineObjectTypes, const Transform*>> {object}, shader);
}

void RenderEngine::RenderEngineObjects(RenderInfo& info, std::vector<std::pair<EngineObjectTypes, const Transform*>> objects, Shader* shader)
{
	if (!shader)
	{
		shader = &DebugShader;
		shader->Use();
	}

	EngineObjectTypes boundObject = EngineObjectTypes::SPHERE;
	glBindVertexArray(EngineObjects[boundObject].first);	//first = VAO

	glm::vec3 colors[4] = {
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, 0.0f)
	};

	for (unsigned int i = 0; i < objects.size(); i++)
	{
		if (objects[i].first != boundObject)	//first = type of drawn object
		{
			boundObject = objects[i].first;		//first = type of drawn object
			glBindVertexArray(EngineObjects[boundObject].first);	//first = VAO
		}

		if (objects[i].second != nullptr)	//second = transform of drawn object
		{
			glm::mat4 model = objects[i].second->GetWorldTransform().GetMatrix();
			shader->UniformMatrix4fv("model", model);
			shader->UniformMatrix4fv("MVP", (*info.VP) * model);
		}
		shader->Uniform3fv("color", colors[i % 4]);

		if (boundObject == EngineObjectTypes::SPHERE || boundObject == EngineObjectTypes::CONE)
			glDrawElements(GL_TRIANGLES, EngineObjects[boundObject].second, GL_UNSIGNED_INT, nullptr);
		else
			glDrawArrays(GL_TRIANGLES, 0, EngineObjects[boundObject].second);	//second = vertex count
	}
}

void RenderEngine::RenderShadowMaps(std::vector <LightComponent*> lights)
{
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
			if (!bCubemapBound)
			{
				DepthLinearizeShader.Use();
				glActiveTexture(GL_TEXTURE0 + 11);
				glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, ShadowCubemapArray);
				bCubemapBound = true;
			}

			Transform lightWorld = light->GetTransform()->GetWorldTransform();
			glm::vec3 lightPos = lightWorld.Position;
			glm::mat4 projection = light->GetProjection();

			DepthLinearizeShader.Uniform1f("far", light->GetFar());
			DepthLinearizeShader.Uniform3fv("lightPos", lightPos);

			unsigned int cubemapFirst = light->GetShadowMapNr() * 6;
		
			for (unsigned int i = cubemapFirst; i < cubemapFirst + 6; i++)
			{
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ShadowCubemapArray, 0, i);
				glClear(GL_DEPTH_BUFFER_BIT);

				glm::mat4 VP = projection * glm::lookAt(lightPos, lightPos + GetCubemapFront(i), GetCubemapUp(i));
				RenderInfo info(nullptr, nullptr, &VP);
				RenderScene(info, &DepthLinearizeShader, false);
			}
		}
		else
		{
			if (bCubemapBound || i == 0)
			{
				DepthShader.Use();
				glActiveTexture(GL_TEXTURE0 + 10);
				glBindTexture(GL_TEXTURE_2D_ARRAY, ShadowMapArray);
				bCubemapBound = false;
			}

			glm::mat4 VP = light->GetVP();

			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ShadowMapArray, 0, light->GetShadowMapNr());
			glClear(GL_DEPTH_BUFFER_BIT);
			RenderInfo info(nullptr, nullptr, &VP);
			RenderScene(info, &DepthShader, false);
		}
	}

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_CULL_FACE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//std::cout << "Wyczyscilem sobie " << timeSum * 1000.0f << "ms.\n";
}

void RenderEngine::RenderLightVolume(RenderInfo& info, LightType type, const Transform* transform, bool shade)
{
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	switch (type)
	{
	case LightType::DIRECTIONAL:	break;// glDisable(GL_CULL_FACE); RenderEngineObject(info, std::pair<EngineObjectTypes, const Transform*>(EngineObjectTypes::QUAD, nullptr)); glEnable(GL_CULL_FACE); break;

	case LightType::POINT:			RenderEngineObject(info, std::pair<EngineObjectTypes, const Transform*>(EngineObjectTypes::SPHERE, transform), ((shade) ? (&LightShaders[(unsigned int)LightType::POINT]) : (&DepthShader))); break;

	case LightType::SPOT:			RenderEngineObject(info, std::pair<EngineObjectTypes, const Transform*>(EngineObjectTypes::CONE, transform), ((shade) ? (&LightShaders[(unsigned int)LightType::SPOT]) : (&DepthShader))); break;

	default:						std::cerr << "ERROR! Invalid light type.\n";
	}
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void RenderEngine::RenderLightVolumes(RenderInfo& info, std::vector <LightComponent*> lights, glm::vec2 resolution)
{
	for (int i = 0; i < 3; i++)	//TODO: Optimize this
	{
		if (i != LightType::POINT && i != LightType::SPOT)
			continue;
		LightShaders[i].Use();
		LightShaders[i].Uniform2fv("resolution", resolution);
	}
	std::pair<LightType, Shader*> shader(LightType::POINT, &LightShaders[LightType::POINT]);	//we use a pair object for some optimisation - when we want to check only the type of the shader we use the first element; the second one contains actual shader object
	shader.second->Use();

	glEnable(GL_DEPTH_TEST);
	glDepthMask(0x00);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_STENCIL_TEST);
	glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
	glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
	glStencilMask(0xFF);

	for (unsigned int i = 0; i < lights.size(); i++)
	{
		LightType type = lights[i]->GetType();
		if (type != shader.first)
		{
			shader.first = type;
			shader.second = &LightShaders[type];
			shader.second->Use();
		}

		if (lights[i]->GetName() != "Flashlight")
			continue;

		lights[i]->CutOff = glm::cos(glm::radians(glfwGetTime() * 1.0f));
		lights[i]->OuterCutOff = glm::cos(glm::radians(glfwGetTime() * 1.1f));
		lights[i]->CalculateLightRadius();

		Transform lightTransform = lights[i]->GetTransform()->GetWorldTransform();
		lightTransform.Scale = lights[i]->GetTransform()->Scale;
		shader.second->Uniform1i("lightIndex", i);	//lights in the array on GPU should be sorted in the same order as they are in this vector!


		//1st pass: stencil
	
		glStencilMask(0xFF);
		glClear(GL_STENCIL_BUFFER_BIT);
		glDisable(GL_CULL_FACE);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glDepthFunc(GL_LEQUAL);
		glDrawBuffer(GL_NONE);
		RenderLightVolume(info, type, &lightTransform);

		//2nd pass: lighting
		glStencilMask(0x00);
		glStencilFunc(GL_EQUAL, 0, 0xFF);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_ALWAYS);
		GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, bufs);
		RenderLightVolume(info, type, &lightTransform);
	}

	glDisable(GL_BLEND);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(0xFF);
	glStencilMask(0xFF);
	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
}

void RenderEngine::RenderScene(RenderInfo& info, Shader* shader, bool bUseMaterials)
{
	if (!shader)
		shader = &DefaultShader;

	shader->Use();
	unsigned int VAOBound = 0;
	Material* materialBound = nullptr;

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

		Meshes[i]->Render(shader, info);
	}
	for (unsigned int i = 0; i < Models.size(); i++)
	{
		Models[i]->Render(shader, info, VAOBound, materialBound, bUseMaterials, EmptyTexture);
	}
}

void RenderEngine::RenderScene(RenderInfo& info, std::string shaderName, bool bUseMaterials)
{
	Shader* shader = nullptr;
	if (shaderName == "default")
		shader = &DefaultShader;
	else if (shaderName == "geometry")
		shader = &GShader;
	else
	{
		std::cerr << "ERROR! Can't find a shader named " << shaderName << ". Scene won't be rendered.\n";
		return;
	}

	RenderScene(info, shader, bUseMaterials);
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