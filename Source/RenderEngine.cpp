#include "RenderEngine.h"
#include "Postprocess.h"
#include "FileLoader.h"
#include "LightComponent.h"
#include "ModelComponent.h"
#include <random> //DO WYJEBANIA

RenderEngine::RenderEngine(GameManager* gameHandle) :
	GameHandle(gameHandle)
{
	//configure some openGL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);


	//generate engine's empty texture
	glGenTextures(1, &EmptyTexture);
	glBindTexture(GL_TEXTURE_2D, EmptyTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, glm::value_ptr(glm::vec3(0.0f, 0.0f, 1.0f)));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void RenderEngine::Init()
{
	GShader.LoadShaders("Shaders/geometry.vs", "Shaders/geometry.fs");
	GShader.UniformBlockBinding("Matrices", 0);

	GShader.Use();
	std::vector<std::pair<unsigned int, std::string>> gShaderTextureUnits = {
		std::pair<unsigned int, std::string>(0, "diffuse1"),
		std::pair<unsigned int, std::string>(1, "specular1"),
		std::pair<unsigned int, std::string>(2, "normal1"),
		std::pair<unsigned int, std::string>(3, "depth1")
	};
	GShader.SetTextureUnitNames(gShaderTextureUnits);
	GShader.SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MODEL, MatrixType::MVP, MatrixType::NORMAL});

	//load the default shader
	DefaultShader.LoadShaders("Shaders/default.vs", "Shaders/default.fs");
	DefaultShader.UniformBlockBinding("Matrices", 0);
	DefaultShader.UniformBlockBinding("Lights", 1);
	DefaultShader.SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MODEL, MatrixType::MVP, MatrixType::NORMAL});

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
	DepthShader.LoadShaders("Shaders/depth.vs", "Shaders/depth.fs");
	DepthShader.SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MVP});
	DepthLinearizeShader.LoadShaders("Shaders/depth_linearize.vs", "Shaders/depth_linearize.fs");
	DepthLinearizeShader.SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MODEL, MatrixType::MVP});

	//load debug shaders
	DebugShader.LoadShaders("Shaders/debug.vs", "Shaders/debug.fs");
	DebugShader.SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MVP});
	DebugShader.UniformBlockBinding("Matrices", 0);

	GenerateEngineObjects();

	std::string settingsDefines = GameHandle->GetGameSettings()->GetShaderDefines();
	LightShaders[LightType::DIRECTIONAL].LoadShadersWithInclData(settingsDefines, "Shaders/directional.vs", "Shaders/directional.fs");
	LightShaders[LightType::DIRECTIONAL].UniformBlockBinding("Matrices", 0);
	LightShaders[LightType::DIRECTIONAL].UniformBlockBinding("Lights", 1);
	LightShaders[LightType::POINT].LoadShadersWithInclData(settingsDefines, "Shaders/point.vs", "Shaders/point.fs");
	LightShaders[LightType::POINT].UniformBlockBinding("Matrices", 0);
	LightShaders[LightType::POINT].UniformBlockBinding("Lights", 1);
	LightShaders[LightType::SPOT].LoadShadersWithInclData(settingsDefines, "Shaders/spot.vs", "Shaders/spot.fs");
	LightShaders[LightType::SPOT].UniformBlockBinding("Matrices", 0);
	LightShaders[LightType::SPOT].UniformBlockBinding("Lights", 1);


	for (int i = 0; i < 3; i++)
	{
		LightShaders[i].Use();
		LightShaders[i].Uniform1i("gPosition", 0);
		LightShaders[i].Uniform1i("gNormal", 1);
		LightShaders[i].Uniform1i("gAlbedoSpec", 2);
		LightShaders[i].Uniform1i("ssaoTex", 3);

		if (i == (int)LightType::POINT)
			LightShaders[i].Uniform1i("shadowCubemaps", 11);
		else
			LightShaders[i].Uniform1i("shadowMaps", 10);

		if (i != (int)LightType::DIRECTIONAL)
			LightShaders[i].SetExpectedMatrices(std::vector<MatrixType>{MatrixType::VIEW, MatrixType::MVP});
	}
}

void RenderEngine::GenerateEngineObjects()
{
	glm::vec3 quadVertices[] = {
		glm::vec3(-0.5f, -0.5f, 0.0f),
		glm::vec3( 0.5f, -0.5f, 0.0f),
		glm::vec3( 0.5f,  0.5f, 0.0f),
		glm::vec3(-0.5f,  0.5f, 0.0f)
	};

	unsigned int quadEBOBuffer[] =
	{
		0, 1, 2,
		2, 3, 0
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
	unsigned int quadEBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glGenBuffers(1, &quadEBO);

	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadEBOBuffer), quadEBOBuffer, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)(nullptr));
	glEnableVertexAttribArray(0);
	

	unsigned int cubeVBO;
	unsigned int cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);

	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)(nullptr));
	glEnableVertexAttribArray(0);

	//Add the engine objects to the tree collection. This way models loaded from the level file can simply use the internal engine meshes for stuff like particles
	//Preferably, the engine objects should be added to the tree collection first, so searching for them takes less time and they'll possibly be used often.

	EngineObjects[EngineObjectType::CUBE] = &CreateMeshTree("ENG_CUBE")->GetRoot().AddChild("Cube").AddMesh("Cube");
	EngineObjects[EngineObjectType::CUBE]->LoadFromGLBuffers(36, cubeVAO, cubeVBO);

	EngineObjects[EngineObjectType::QUAD] = &CreateMeshTree("ENG_QUAD")->GetRoot().AddChild("Quad").AddMesh("Quad_NoShadow");
	EngineObjects[EngineObjectType::QUAD]->LoadFromGLBuffers(6, quadVAO, quadVBO, quadEBO);

	EngineObjects[EngineObjectType::SPHERE] = EngineDataLoader::LoadMeshTree(GameHandle, this, GameHandle->GetSearchEngine(), "EngineObjects/sphere.obj", CreateMeshTree("ENG_SPHERE"))->FindMesh("Sphere");
	EngineObjects[EngineObjectType::CONE] = EngineDataLoader::LoadMeshTree(GameHandle, this, GameHandle->GetSearchEngine(), "EngineObjects/cone.obj", CreateMeshTree("ENG_CONE"))->FindMesh("Cone");
}

std::vector<Shader*> RenderEngine::GetForwardShaders()
{
	std::vector<Shader*> shaders;
	shaders.push_back(FindShader("forward"));	//add the default forward shader

	for (unsigned int i = 0; i < ExternalShaders.size(); i++)
		shaders.push_back(ExternalShaders[i]);

	return shaders;
}


void RenderEngine::AddModel(std::shared_ptr<ModelComponent> model)
{
	Models.push_back(model);
}

std::shared_ptr<ModelComponent> RenderEngine::CreateModel(const ModelComponent& model)
{
	Models.push_back(std::make_shared<ModelComponent>(ModelComponent(model)));
	return Models.back();
}

std::shared_ptr<ModelComponent> RenderEngine::CreateModel(ModelComponent&& model)
{
	Models.push_back(std::make_shared<ModelComponent>(ModelComponent(model)));
	return Models.back();
}

MeshSystem::MeshTree* RenderEngine::CreateMeshTree(std::string path)
{
	MeshTrees.push_back(std::make_unique<MeshSystem::MeshTree>(MeshSystem::MeshTree(path)));
	return MeshTrees.back().get();
}

MeshSystem::MeshTree* RenderEngine::FindMeshTree(std::string path, MeshSystem::MeshTree* ignore)
{
	auto found = std::find_if(MeshTrees.begin(), MeshTrees.end(), [path, ignore](const std::unique_ptr<MeshSystem::MeshTree>& tree) { return tree->GetFilePath() == path && tree.get() != ignore; });
	if (found != MeshTrees.end())
		return (*found).get();

	return nullptr;
}

void RenderEngine::AddMaterial(Material* material)
{
	Materials.push_back(material);
}

void RenderEngine::AddExternalShader(Shader* shader)
{
	ExternalShaders.push_back(shader);
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

ModelComponent* RenderEngine::FindModel(std::string name)
{
	auto modelIt = std::find_if(Models.begin(), Models.end(), [name](std::shared_ptr<ModelComponent>& model) { if (name == "FireParticle Child")std::cout << model->GetName() << "!!!!!!!!!!!!!!!!!!!!\n"; return model->GetName() == name; });

	if (modelIt != Models.end())
		return modelIt->get();

	std::cerr << "ERROR! Can't find model " << name << "!\n";
	return nullptr;
}

Material* RenderEngine::FindMaterial(std::string name)
{
	if (name.empty())
		return nullptr;

	auto materialIt = std::find_if(Materials.begin(), Materials.end(), [name](Material* material) {	return material->GetName() == name; });

	if (materialIt != Materials.end())
		return *materialIt;

	std::cerr << "ERROR! Can't find material " << name << "!\n";
	return nullptr;
}

Shader* RenderEngine::FindShader(std::string name)
{
	if (name == "forward")
		return &DefaultShader;
	if (name == "deferred" || name == "geometry")
		return &GShader;

	auto shaderIt = std::find_if(ExternalShaders.begin(), ExternalShaders.end(), [name](Shader* shader) { return shader->GetName() == name; });

	if (shaderIt != ExternalShaders.end())
		return *shaderIt;

	std::cerr << "ERROR! Can't find a shader named " << name << "!\n";
	return nullptr;
}

void RenderEngine::RenderBoundInDebug(RenderInfo& info, GLenum mode, GLint first, GLint count, glm::vec3 color)
{
	DebugShader.Use();
	DebugShader.UniformMatrix4fv("MVP", *info.VP);
	DebugShader.Uniform3fv("color", color);
	glDrawArrays(mode, first, count);
}

void RenderEngine::RenderEngineObject(RenderInfo& info, std::pair<EngineObjectType, Transform*> object, Shader* shader)
{
	RenderEngineObjects(info, std::vector<std::pair<EngineObjectType, Transform*>> {object}, shader);
}

void RenderEngine::RenderEngineObjects(RenderInfo& info, std::vector<std::pair<EngineObjectType, Transform*>> objects, Shader* shader)
{
	if (!shader)
	{
		shader = &DebugShader;
		shader->Use();
	}

	EngineObjectType boundObject = objects[0].first;
	glBindVertexArray(EngineObjects[boundObject]->GetVAO());	//first = VAO

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
			glBindVertexArray(EngineObjects[boundObject]->GetVAO());	//first = VAO
		}

		if (objects[i].second != nullptr)	//second = transform of drawn object
		{
			glm::mat4 model = objects[i].second->GetWorldTransformMatrix();
			if (shader->ExpectsMatrix(MatrixType::MODEL))
				shader->UniformMatrix4fv("model", model);
			if (shader->ExpectsMatrix(MatrixType::MVP))
				shader->UniformMatrix4fv("MVP", (*info.VP) * model);
			//std::cout << objects[i].second->bConstrain << '\n';
		}

		if (shader->ExpectsMatrix(MatrixType::PROJECTION))
			shader->UniformMatrix4fv("projection", *info.projection);

		if (shader == &DebugShader)
			shader->Uniform3fv("color", colors[i % 4]);

		EngineObjects[boundObject]->Render();
	}
}

void RenderEngine::RenderShadowMaps(std::vector <LightComponent*> lights)
{
	glBindFramebuffer(GL_FRAMEBUFFER, ShadowFBO);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glViewport(0, 0, ShadowMapSize.x, ShadowMapSize.y);

	bool bCubemapBound = false;
	float timeSum = 0.0f;
	float time1;

	glActiveTexture(GL_TEXTURE0 + 10);
	glBindTexture(GL_TEXTURE_2D_ARRAY, ShadowMapArray);
	glActiveTexture(GL_TEXTURE0 + 11);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, ShadowCubemapArray);

	for (unsigned int i = 0; i < lights.size(); i++)
	{
		LightComponent* light = lights[i];
		if (light->GetType() == LightType::POINT)
		{
			time1 = (float)glfwGetTime();
			if (!bCubemapBound)
			{
				DepthLinearizeShader.Use();
				bCubemapBound = true;
			}

			Transform lightWorld = light->GetTransform().GetWorldTransform();
			glm::vec3 lightPos = lightWorld.PositionRef;
			glm::mat4 projection = light->GetProjection();

			DepthLinearizeShader.Uniform1f("far", light->GetFar());
			DepthLinearizeShader.Uniform3fv("lightPos", lightPos);

			unsigned int cubemapFirst = light->GetShadowMapNr() * 6;
			timeSum += (float)glfwGetTime() - time1;
		
			for (unsigned int i = cubemapFirst; i < cubemapFirst + 6; i++)
			{
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ShadowCubemapArray, 0, i);
				glClear(GL_DEPTH_BUFFER_BIT);

				glm::mat4 VP = projection * glm::lookAt(lightPos, lightPos + GetCubemapFront(i), GetCubemapUp(i));
				RenderInfo info(nullptr, nullptr, &VP, false, true, false);
				RenderScene(info, &DepthLinearizeShader);
			}
		}
		else
		{
			time1 = glfwGetTime();
			if (bCubemapBound || i == 0)
			{
				DepthShader.Use();
				bCubemapBound = false;
			}

			glm::mat4 VP = light->GetVP();

			timeSum += (float)glfwGetTime() - time1;
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ShadowMapArray, 0, light->GetShadowMapNr());
			glClear(GL_DEPTH_BUFFER_BIT);
			
			RenderInfo info(nullptr, nullptr, &VP, false, true, false);
			RenderScene(info, &DepthShader);
		}
	}

	glActiveTexture(GL_TEXTURE0);
	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//std::cout << "Wyczyscilem sobie " << timeSum * 1000.0f << "ms.\n";
}

void RenderEngine::RenderLightVolume(RenderInfo& info, LightComponent* light, Transform* transform, bool shade)
{
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	EngineObjectType lightVolumeType = light->GetLightVolumeType();
	switch (light->GetType())
	{
	case LightType::DIRECTIONAL:	glDisable(GL_CULL_FACE); RenderEngineObject(info, std::pair<EngineObjectType, Transform*>(lightVolumeType, nullptr), &LightShaders[(unsigned int)LightType::DIRECTIONAL]); glEnable(GL_CULL_FACE); break;

	case LightType::POINT:			RenderEngineObject(info, std::pair<EngineObjectType, Transform*>(lightVolumeType, transform), ((shade) ? (&LightShaders[(unsigned int)LightType::POINT]) : (&DepthShader))); break;

	case LightType::SPOT:			RenderEngineObject(info, std::pair<EngineObjectType, Transform*>(lightVolumeType, transform), ((shade) ? (&LightShaders[(unsigned int)LightType::SPOT]) : (&DepthShader))); break;

	default:						std::cerr << "ERROR! Invalid light type.\n";
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void RenderEngine::RenderLightVolumes(RenderInfo& info, std::vector <LightComponent*> lights, glm::vec2 resolution)
{
	for (int i = 0; i < 3; i++)	//TODO: Optimize this
	{
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

		Transform lightTransform = lights[i]->GetTransform().GetWorldTransform();
		lightTransform.SetScale(lights[i]->GetTransform().ScaleRef);
		shader.second->Uniform1i("lightIndex", i);	//lights in the array on GPU should be sorted in the same order as they are in this vector!
		lightTransform.bConstrain = true;

		//1st pass: stencil
	
		if (type != LightType::DIRECTIONAL)		//if this is a directional light everything will be in range; don't waste time
		{
			glStencilMask(0xFF);
			glClear(GL_STENCIL_BUFFER_BIT);
			glDisable(GL_CULL_FACE);
			glStencilFunc(GL_ALWAYS, 1, 0xFF);
			glDepthFunc(GL_LEQUAL);
			glDrawBuffer(GL_NONE);
			RenderLightVolume(info, lights[i], &lightTransform);
		}

		//2nd pass: lighting
		glStencilMask(0x00);
		glStencilFunc(((type == LightType::DIRECTIONAL) ? (GL_ALWAYS) : (GL_EQUAL)), 0, 0xFF);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_ALWAYS);
		GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, bufs);
		RenderLightVolume(info, lights[i], &lightTransform);
	}

	glDisable(GL_BLEND);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(0xFF);
	glStencilMask(0xFF);
	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
}

void RenderEngine::RenderScene(RenderInfo& info, Shader* shader)
{
	if (!shader)
		shader = &DefaultShader;

	shader->Use();
	unsigned int VAOBound = 0;
	Material* materialBound = nullptr;

	for (unsigned int i = 0; i < Models.size(); i++)
	{
		//Models[i]->Render(shader, info, VAOBound, materialBound, EmptyTexture);
		Render(*Models[i], shader, info, VAOBound, materialBound, EmptyTexture);
	}
}

void RenderEngine::Render(const ModelComponent& model, Shader* shader, RenderInfo& info, unsigned int VAOBound, Material* materialBound, unsigned int emptyTexture)
{
	if (model.GetMeshInstanceCount() == 0)
		return;

	const glm::mat4& modelMat = model.GetTransform().GetWorldTransformMatrix();	//don't worry, the ComponentTransform's world transform is cached

	if (shader->ExpectsMatrix(MatrixType::MODEL))
		shader->UniformMatrix4fv("model", modelMat);
	if (shader->ExpectsMatrix(MatrixType::VIEW))
		shader->UniformMatrix4fv("view", *info.view);
	if (shader->ExpectsMatrix(MatrixType::MV))
		shader->UniformMatrix4fv("MV", (*info.view) * modelMat);
	if (shader->ExpectsMatrix(MatrixType::MVP))
		shader->UniformMatrix4fv("MVP", (*info.VP) * modelMat);
	if (shader->ExpectsMatrix(MatrixType::NORMAL))
		shader->UniformMatrix3fv("normalMat", ModelToNormal(modelMat));


	for (int i = 0; i < static_cast<int>(model.GetMeshInstanceCount()); i++)
	{
		const MeshInstance& meshInst = model.GetMeshInstance(i);
		const Mesh& mesh = meshInst.GetMesh();
		Material* material = meshInst.GetMaterialPtr();
		MaterialInstance* materialInst = meshInst.GetMaterialInst();

		if ((info.CareAboutShader && material && shader != material->GetRenderShader()) || (info.OnlyShadowCasters && !mesh.CanCastShadow()) || !materialInst->ShouldBeDrawn())
			continue;

		if (VAOBound != mesh.VAO || i == 0)
		{
			glBindVertexArray(mesh.VAO);
			VAOBound = mesh.VAO;
		}

		if (info.UseMaterials && materialBound != material && material) //jesli zbindowany jest inny material niz potrzebny obecnie, musimy zmienic go w shaderze
		{
			materialInst->UpdateWholeUBOData(shader, emptyTexture);
			materialBound = material;
		}
		else if (materialBound) //jesli ostatni zbindowany material jest taki sam, to nie musimy zmieniac wszystkich danych w shaderze; oszczedzmy sobie roboty
			materialInst->UpdateInstanceUBOData(shader);

		mesh.Render();
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