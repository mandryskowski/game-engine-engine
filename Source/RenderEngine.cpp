#include "RenderEngine.h"
#include <random>

RenderEngine::RenderEngine() :
	SSAOShader(nullptr),
	SSAOFramebuffer(nullptr)
{
	//configure some openGL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

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

	LightShaders[LightType::DIRECTIONAL].LoadShaders("Shaders/directional.vs", "Shaders/directional.fs");
	LightShaders[LightType::DIRECTIONAL].UniformBlockBinding("Matrices", 0);
	LightShaders[LightType::DIRECTIONAL].UniformBlockBinding("Lights", 1);
	LightShaders[LightType::POINT].LoadShaders("Shaders/point.vs", "Shaders/point.fs");
	LightShaders[LightType::POINT].UniformBlockBinding("Matrices", 0);
	LightShaders[LightType::POINT].UniformBlockBinding("Lights", 1);
	LightShaders[LightType::SPOT].LoadShaders("Shaders/spot.vs", "Shaders/spot.fs");
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

	//generate engine's empty texture
	glGenTextures(1, &EmptyTexture);
	glBindTexture(GL_TEXTURE_2D, EmptyTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, glm::value_ptr(glm::vec3(0.0f, 0.0f, 1.0f)));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
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

	EngineObjects[EngineObjectType::QUAD] = new Mesh("ENG_QUAD_NoShadow");
	EngineObjects[EngineObjectType::QUAD]->LoadFromGLBuffers(6, quadVAO, quadVBO, quadEBO);

	unsigned int cubeVBO;
	unsigned int cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);

	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)(nullptr));
	glEnableVertexAttribArray(0);

	EngineObjects[EngineObjectType::CUBE] = new Mesh("ENG_CUBE");
	EngineObjects[EngineObjectType::CUBE]->LoadFromGLBuffers(36, cubeVAO, cubeVBO);

	size_t vertexCount = 0;

	EngineObjects[EngineObjectType::SPHERE] = new Mesh("ENG_SPHERE");
	EngineObjects[EngineObjectType::SPHERE]->LoadSingleFromAiFile("EngineObjects/sphere.obj", false);

	EngineObjects[EngineObjectType::CONE] = new Mesh("ENG_CONE");
	EngineObjects[EngineObjectType::CONE]->LoadSingleFromAiFile("EngineObjects/cone.obj", false);

	//Add the engine objects to the mesh collection. This way models loaded from the level file can simply use the internal engine meshes for stuff like particles
	//Preferably, the engine objects should be added to the mesh collection first, so searching for them takes little time and they'll possibly be used often.

	for (int i = 0; i < 4; i++)
		MeshCollection.push_back(EngineObjects[i]);
}

unsigned int RenderEngine::GetSSAOTex()
{
	return SSAOFramebuffer->GetColorBuffer(0).OpenGLBuffer;
}

std::vector<Shader*> RenderEngine::GetForwardShaders()
{
	std::vector<Shader*> shaders;
	shaders.push_back(FindShader("forward"));	//add the default forward shader

	for (unsigned int i = 0; i < ExternalShaders.size(); i++)
		shaders.push_back(ExternalShaders[i]);

	return shaders;
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
	std::vector <ModelComponent*> copyModels;
	for (unsigned int i = 0; i < Models.size(); i++)
	{
		std::string path = Models[i]->GetFilePath();
		if (path.size() > 3 && path.substr(0, 3) == "CPY") //check if the model should really be loaded from a file, or copy an existing mesh instead
		{
			copyModels.push_back(Models[i]);
			continue;
		}

		for (unsigned int j = 0; j < filePaths.size(); j++)	//check for a duplicate
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
		scenes.push_back(importer.ReadFile(filePaths[i], aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace));
		if (!scenes[i] || scenes[i]->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scenes[i]->mRootNode)
		{
			std::cerr << "Can't load scene " << filePaths[i] << ".\n";
			continue;
		}

		std::string directory;
		size_t dirPos = filePaths[i].find_last_of('/');
		if (dirPos != std::string::npos)
			directory = filePaths[i].substr(0, dirPos + 1);

		std::vector<ModelComponent*> modelsPtr;
		for (unsigned int j = 0; j < Models.size(); j++)	//for all models that use this file path
			if (Models[j]->GetFilePath() == filePaths[i])
				Models[j]->ProcessAiNode(scenes[i], directory, scenes[i]->mRootNode, modelsPtr, &matLoadingData);

		std::vector<Mesh*> meshesFromThisFile = modelsPtr[0]->GetReferencedMeshes();
		MeshCollection.insert(MeshCollection.end(), meshesFromThisFile.begin(), meshesFromThisFile.end());

		AddModels(modelsPtr);
	}

	for (unsigned int i = 0; i < copyModels.size(); i++)	//Do the copying at the end
	{
		std::string name = copyModels[i]->GetFilePath().substr(4);

		for (unsigned int j = 0; j < MeshCollection.size(); j++)
		{
			if (MeshCollection[j]->GetName().find(name) != std::string::npos)
			{
				copyModels[i]->AddMeshInst(MeshCollection[j]);
				break;
			}
		}

		std::cout << "Copying " + name << '\n';
	}

	for (unsigned int i = 0; i < matLoadingData.LoadedMaterials.size(); i++)
		matLoadingData.LoadedMaterials[i]->SetRenderShader(&GShader);
}

void RenderEngine::LoadMaterials(std::string path, std::string directory)
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

	if (isNextWordEqual(filestr, "shaders"))
		LoadShaders(filestr, directory);

	while (!isNextWordEqual(filestr, "end"))
	{
		Material* material = nullptr;
		std::string materialName, shaderName;
		float shininess, depthScale;
		Shader* shader = &GShader;
		unsigned int texCount;

		filestr >> materialName;

		if (isNextWordEqual(filestr, "atlas"))
		{
			glm::vec2 size;
			filestr >> size.x >> size.y;
			material = new AtlasMaterial(materialName, size);
		}
		else
			material = new Material(materialName);

		filestr >> shaderName >> shininess >> depthScale >> texCount;

		if (shaderName != "deferred")
			shader = FindShader(shaderName);

		material->SetRenderShader(shader);
		material->SetShininess(shininess);
		material->SetDepthScale(depthScale);

		Materials.push_back(material);

		for (unsigned int i = 0; i < texCount; i++)
		{
			std::string path, shaderUniformName, isSRGB;
			filestr >> path >> shaderUniformName >> isSRGB;

			material->AddTexture(new Texture(directory + path, shaderUniformName, toBool(isSRGB)));
		}
	}
}


void RenderEngine::LoadShaders(std::stringstream& filestr, std::string directory)
{
	while (!isNextWordEqual(filestr, "end"))
	{
		std::string name;
		std::string paths[3];
		unsigned int fileCount;

		filestr >> name >> fileCount;
		for (unsigned int i = 0; i < fileCount; i++)
		{
			filestr >> paths[i];

			if (!(paths[i].size() > 3 && paths[i].substr(0, 2) == "./"))
				paths[i] = directory + paths[i];
		}

		Shader* shader = new Shader(paths[0], paths[1], paths[2]);
		shader->SetName(name);

		unsigned int expectedMatricesCount, texUnitsCount;

		filestr >> expectedMatricesCount;
		for (unsigned int i = 0; i < expectedMatricesCount; i++)
		{
			std::string expectedMatrixType;
			filestr >> expectedMatrixType;
			
			shader->AddExpectedMatrix(expectedMatrixType);
		}

		filestr >> texUnitsCount;
		for (unsigned int i = 0; i < texUnitsCount; i++)
		{
			unsigned int unitIndex;
			std::string texUnitName;
			filestr >> unitIndex >> texUnitName;

			shader->AddTextureUnit(unitIndex, texUnitName);
		}

		ExternalShaders.push_back(shader);
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

void RenderEngine::SetupAmbientOcclusion(glm::uvec2 windowSize, unsigned int samples)
{
	SSAOShader = new Shader("Shaders/ssao.vs", "Shaders/ssao.fs");
	SSAOShader->SetExpectedMatrices(std::vector<MatrixType> {MatrixType::PROJECTION});
	SSAOShader->Use();
	SSAOShader->Uniform1f("radius", 0.3f);
	SSAOShader->Uniform1i("gPosition", 0);
	SSAOShader->Uniform1i("gNormal", 1);
	SSAOShader->Uniform1i("noiseTex", 2);

	SSAOFramebuffer = new Framebuffer;
	SSAOFramebuffer->Load(windowSize, ColorBufferData(GL_TEXTURE_2D, GL_RED, GL_FLOAT, GL_LINEAR, GL_LINEAR));

	std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
	std::default_random_engine generator;

	std::vector<glm::vec3> ssaoSamples;
	for (unsigned int i = 0; i < samples; i++)
	{
		glm::vec3 sample(
			randomFloats(generator) * 2.0f - 1.0f,
			randomFloats(generator) * 2.0f - 1.0f,
			randomFloats(generator)
		);

		sample = glm::normalize(sample) * randomFloats(generator);

		float scale = (float)i / (float)samples;
		scale = glm::mix(0.1f, 1.0f, scale * scale);
		sample *= scale;

		SSAOShader->Uniform3fv("samples[" + std::to_string(i) + "]", sample);
		ssaoSamples.push_back(sample);
	}
	
	std::vector<glm::vec3> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(
			randomFloats(generator) * 2.0f - 1.0f,
			randomFloats(generator) * 2.0f - 1.0f,
			0.0f
		);

		ssaoNoise.push_back(noise);
	}

	glGenTextures(1, &SSAONoiseTex);
	glBindTexture(GL_TEXTURE_2D, SSAONoiseTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

ModelComponent* RenderEngine::FindModel(std::string name)
{
	for (unsigned int i = 0; i < Models.size(); i++)
		if (name == Models[i]->GetName())
			return Models[i];

	std::cerr << "ERROR! Can't find model " << name << "!\n";
	return nullptr;
}

Material* RenderEngine::FindMaterial(std::string name)
{
	for (unsigned int i = 0; i < Materials.size(); i++)
		if (Materials[i]->GetName() == name)
			return Materials[i];

	std::cerr << "ERROR! Can't find material " << name << "!\n";
	return nullptr;
}

Shader* RenderEngine::FindShader(std::string name)
{
	if (name == "forward")
		return &DefaultShader;
	if (name == "geometry")
		return &GShader;

	for (unsigned int i = 0; i < ExternalShaders.size(); i++)
		if (ExternalShaders[i]->GetName() == name)
			return ExternalShaders[i];

	std::cerr << "ERROR! Can't find a shader named " << name << "!\n";
	return nullptr;
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

			Transform lightWorld = light->GetTransform()->GetWorldTransform();
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

void RenderEngine::RenderSSAO(RenderInfo& info, glm::uvec2 resolution)
{
	if (!SSAOFramebuffer)
	{
		std::cerr << "ERROR! Attempting to render SSAO image without calling the setup\n";
		return;
	}

	SSAOFramebuffer->Bind();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//gPosition and gNormal were bound in the Game render method
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, SSAONoiseTex);

	SSAOShader->Use();
	SSAOShader->Uniform2fv("resolution", resolution);
	RenderEngineObject(info, std::pair<EngineObjectType, Transform*>(EngineObjectType::QUAD, nullptr), SSAOShader);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
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

		Transform lightTransform = lights[i]->GetTransform()->GetWorldTransform();
		lightTransform.SetScale(lights[i]->GetTransform()->ScaleRef);
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
		Models[i]->Render(shader, info, VAOBound, materialBound, EmptyTexture);
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