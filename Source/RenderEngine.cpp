#include "RenderEngine.h"
#include "Postprocess.h"
#include "FileLoader.h"
#include "LightComponent.h"
#include "ModelComponent.h"
#include "LightProbe.h"
#include "RenderableVolume.h"
#include <random> //DO WYJEBANIA

RenderEngine::RenderEngine(GameManager* gameHandle, const ShadingModel& shading) :
	GameHandle(gameHandle),
	Shading(shading),
	PreviousFrameView(glm::mat4(1.0f))
{
	//configure some openGL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);


	//generate engine's empty texture
	EmptyTexture = std::make_shared<Texture>(textureFromBuffer(glm::value_ptr(glm::vec3(0.5f, 0.5f, 1.0f)), 1, 1, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST));

	LightProbeLoader::RenderHandle = this;
}

void RenderEngine::Init(glm::uvec2 resolution)
{
	Resize(resolution);

	LoadInternalShaders();
	GenerateEngineObjects();
	SetupShadowmaps();

	Postprocessing.Init(GameHandle, Resolution);

	CubemapData.DefaultV[0] = glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	CubemapData.DefaultV[1] = glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	CubemapData.DefaultV[2] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	CubemapData.DefaultV[3] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	CubemapData.DefaultV[4] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	CubemapData.DefaultV[5] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

	glm::mat4 cubemapProj = glm::ortho(-0.5f, 0.5f, -0.5f, 0.5f, 0.0f, 2.0f);

	for (int i = 0; i < 6; i++)
		CubemapData.DefaultVP[i] = cubemapProj * CubemapData.DefaultV[i];

	if (LightProbes.empty())
	{
		LightProbeLoader::LoadLightProbeTextureArrays();
		LightProbeTextureArrays::IrradianceMapArr.Bind(12);
		LightProbeTextureArrays::PrefilterMapArr.Bind(13);
		LightProbeTextureArrays::BRDFLut.Bind(14);
		glActiveTexture(GL_TEXTURE0);
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

	CreateMeshTree("ENG_CUBE")->GetRoot().AddChild("Cube").AddMesh("Cube").LoadFromGLBuffers(36, cubeVAO, cubeVBO);
	CreateMeshTree("ENG_QUAD")->GetRoot().AddChild("Quad").AddMesh("Quad_NoShadow").LoadFromGLBuffers(6, quadVAO, quadVBO, quadEBO);
	EngineDataLoader::LoadMeshTree(GameHandle, this, GameHandle->GetSearchEngine(), "EngineObjects/sphere.obj", CreateMeshTree("ENG_SPHERE"))->FindMesh("Sphere");
	EngineDataLoader::LoadMeshTree(GameHandle, this, GameHandle->GetSearchEngine(), "EngineObjects/cone.obj", CreateMeshTree("ENG_CONE"))->FindMesh("Cone");
}

void RenderEngine::LoadInternalShaders()
{
	std::string settingsDefines = GameHandle->GetGameSettings()->GetShaderDefines(Resolution);
	switch (Shading)
	{
	case ShadingModel::SHADING_PHONG: settingsDefines += "#define PHONG_SHADING 1\n"; break;
	case ShadingModel::SHADING_PBR_COOK_TORRANCE: settingsDefines += "#define PBR_SHADING 1\n"; break;
	}
	std::vector<std::pair<unsigned int, std::string>> gShaderTextureUnits = {
		std::pair<unsigned int, std::string>(0, "albedo1"),
		std::pair<unsigned int, std::string>(1, "specular1"),
		std::pair<unsigned int, std::string>(2, "normal1"),
		std::pair<unsigned int, std::string>(3, "depth1")
	};
	if (Shading == ShadingModel::SHADING_PBR_COOK_TORRANCE)
	{
		std::vector<std::pair<unsigned int, std::string>> pbrTextures = {
			std::pair<unsigned int, std::string>(4, "roughness1"),
			std::pair<unsigned int, std::string>(5, "metallic1"),
			std::pair<unsigned int, std::string>(6, "ao1") };
		gShaderTextureUnits.insert(gShaderTextureUnits.begin(), pbrTextures.begin(), pbrTextures.end());
	}
	
	Shaders.push_back(ShaderLoader::LoadShadersWithInclData("Geometry", settingsDefines, "Shaders/geometry.vs", "Shaders/geometry.fs"));
	Shaders.back()->SetTextureUnitNames(gShaderTextureUnits);
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MODEL, MatrixType::MVP, MatrixType::NORMAL});

	//load the default shader
	std::vector<std::pair<unsigned int, std::string>> defaultShaderTextureUnits = {
		std::pair<unsigned int, std::string>(0, "albedo1"),
		std::pair<unsigned int, std::string>(1, "specular1"),
		std::pair<unsigned int, std::string>(2, "normal1"),
		std::pair<unsigned int, std::string>(3, "depth1")
	};

	Shaders.push_back(ShaderLoader::LoadShaders("Default", "Shaders/default.vs", "Shaders/default.fs"));
	Shaders.back()->UniformBlockBinding("Matrices", 0);
	Shaders.back()->UniformBlockBinding("Lights", 1);
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MODEL, MatrixType::MVP, MatrixType::NORMAL});
	Shaders.back()->SetTextureUnitNames(defaultShaderTextureUnits);
	Shaders.back()->Uniform1i("shadowMaps", 10);
	Shaders.back()->Uniform1i("shadowCubeMaps", 11);

	//load shadow shaders
	Shaders.push_back(ShaderLoader::LoadShaders("Depth", "Shaders/depth.vs", "Shaders/depth.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MVP});
	Shaders.push_back(ShaderLoader::LoadShaders("DepthLinearize", "Shaders/depth_linearize.vs", "Shaders/depth_linearize.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MODEL, MatrixType::MVP});

	Shaders.push_back(ShaderLoader::LoadShaders("ErToCubemap", "Shaders/LightProbe/erToCubemap.vs", "Shaders/LightProbe/erToCubemap.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::VP});

	Shaders.push_back(ShaderLoader::LoadShadersWithInclData("Cubemap", settingsDefines, "Shaders/cubemap.vs", "Shaders/cubemap.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::VP});

	Shaders.push_back(ShaderLoader::LoadShaders("CubemapToIrradiance", "Shaders/irradianceCubemap.vs", "Shaders/irradianceCubemap.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::VP});

	Shaders.push_back(ShaderLoader::LoadShaders("CubemapToPrefilter", "Shaders/prefilterCubemap.vs", "Shaders/prefilterCubemap.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::VP});

	Shaders.push_back(ShaderLoader::LoadShaders("BRDFLutGeneration", "Shaders/brdfLutGeneration.vs", "Shaders/brdfLutGeneration.fs"));

	//load debug shaders
	Shaders.push_back(ShaderLoader::LoadShaders("Debug", "Shaders/debug.vs", "Shaders/debug.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MVP});

	std::vector<std::string> lightShadersNames;
	std::vector<std::string> lightShadersDefines;
	std::pair<std::string, std::string> lightShadersPath;
	switch (Shading)
	{
	case ShadingModel::SHADING_PHONG:
		lightShadersNames = { "PhongDirectional", "PhongPoint", "PhongSpot" };
		lightShadersDefines = { "#define DIRECTIONAL_LIGHT 1\n", "#define POINT_LIGHT 1\n", "#define SPOT_LIGHT 1\n" };
		lightShadersPath = std::pair<std::string, std::string>("Shaders/phong.vs", "Shaders/phong.fs");
		break;
	case ShadingModel::SHADING_PBR_COOK_TORRANCE:
		lightShadersNames = { "CookTorranceDirectional", "CookTorrancePoint", "CookTorranceSpot", "CookTorranceIBL" };
		lightShadersDefines = { "#define DIRECTIONAL_LIGHT 1\n", "#define POINT_LIGHT 1\n", "#define SPOT_LIGHT 1\n", "#define IBL_PASS 1\n" };
		lightShadersPath = std::pair<std::string, std::string>("Shaders/pbrCookTorrance.vs", "Shaders/pbrCookTorrance.fs");
		break;
	}

	std::cout << settingsDefines << '\n';

	for (int i = 0; i < static_cast<int>(lightShadersNames.size()); i++)
	{
		Shaders.push_back(ShaderLoader::LoadShadersWithInclData(lightShadersNames[i], settingsDefines + lightShadersDefines[i], lightShadersPath.first, lightShadersPath.second));
		LightShaders.push_back(Shaders.back().get());
		Shaders.back()->UniformBlockBinding("Lights", 1);
		Shaders.back()->Use();
		Shaders.back()->Uniform1i("gPosition", 0);
		Shaders.back()->Uniform1i("gNormal", 1);
		Shaders.back()->Uniform1i("gAlbedoSpec", 2);
		Shaders.back()->Uniform1i("gAlphaMetalAo", 3);
		if (GameHandle->GetGameSettings()->AmbientOcclusionSamples > 0)
			Shaders.back()->Uniform1i("ssaoTex", 4);

		Shaders.back()->Uniform1i("shadowMaps", 10);
		Shaders.back()->Uniform1i("shadowCubemaps", 11);
		Shaders.back()->Uniform1i("irradianceCubemaps", 12);
		Shaders.back()->Uniform1i("prefilterCubemaps", 13);
		Shaders.back()->Uniform1i("BRDFLutTex", 14);

		Shaders.back()->Uniform1f("lightProbeNr", 3.0f);

		if (i == (int)LightType::POINT || i == (int)LightType::SPOT)
			Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::VIEW, MatrixType::MVP});
	}
}

void RenderEngine::Resize(glm::uvec2 resolution)
{
	Resolution = resolution;
	const GameSettings* settings = GameHandle->GetGameSettings();
	std::shared_ptr<GEE_FB::FramebufferAttachment> sharedDepthStencil = GEE_FB::reserveDepthBuffer(Resolution, GL_DEPTH24_STENCIL8, GL_UNSIGNED_INT_24_8, GL_NEAREST, GL_NEAREST);	//we share this buffer between the geometry and main framebuffer - we want deferred-rendered objects depth to influence light volumes&forward rendered objects
	std::shared_ptr<GEE_FB::FramebufferAttachment> velocityBuffer = GEE_FB::reserveColorBuffer(Resolution, GL_RGB16F, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_TEXTURE_2D, 0, "velocityTex");
	std::vector<std::shared_ptr<GEE_FB::FramebufferAttachment>> gColorBuffers = {
		GEE_FB::reserveColorBuffer(Resolution, GL_RGB16F, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_TEXTURE_2D, 0, "gPosition"),			//gPosition texture
		GEE_FB::reserveColorBuffer(Resolution, GL_RGB16F, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_TEXTURE_2D, 0, "gNormal"),				//gNormal texture
		GEE_FB::reserveColorBuffer(Resolution, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST, GL_TEXTURE_2D, 0, "gAlbedoSpec")	//gAlbedoSpec texture
		//std::make_shared<GEE_FB::ColorBuffer>(GL_TEXTURE_2D, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST)		//alpha, metallic, ao texture
	};
	if (Shading == SHADING_PBR_COOK_TORRANCE)
		gColorBuffers.push_back(GEE_FB::reserveColorBuffer(Resolution, GL_RGB, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST, GL_TEXTURE_2D, 0, "gAlphaMetalAo")); //alpha, metallic, ao texture

	if (settings->IsVelocityBufferNeeded())
		gColorBuffers.push_back(velocityBuffer);
	GFramebuffer.SetAttachments(Resolution, gColorBuffers, sharedDepthStencil);

	std::vector<std::shared_ptr<GEE_FB::FramebufferAttachment>> colorBuffers;
	colorBuffers.push_back(GEE_FB::reserveColorBuffer(Resolution, GL_RGBA16F, GL_FLOAT, GL_LINEAR, GL_LINEAR));	//add color buffer
	if (settings->bBloom)
		colorBuffers.push_back(GEE_FB::reserveColorBuffer(Resolution, GL_RGBA16F, GL_FLOAT, GL_LINEAR, GL_LINEAR));	//add blur buffer
	if (settings->IsVelocityBufferNeeded())
		colorBuffers.push_back(velocityBuffer);
	MainFramebuffer.SetAttachments(Resolution, colorBuffers, sharedDepthStencil);	//color and blur buffers
}

void RenderEngine::SetupShadowmaps()
{
	glm::uvec2 shadowMapSize(512);
	switch (GameHandle->GetGameSettings()->ShadowLevel)
	{
	case SETTING_LOW: shadowMapSize = glm::vec2(1024); break;
	case SETTING_MEDIUM: shadowMapSize = glm::vec2(2048); break;
	case SETTING_HIGH: shadowMapSize = glm::vec2(512); break;
	case SETTING_ULTRA: shadowMapSize = glm::vec2(1024); break;
	}
	ShadowFramebuffer.SetAttachments(shadowMapSize);


	ShadowMapArray = std::make_shared<Texture>();
	ShadowMapArray->GenerateID(GL_TEXTURE_2D_ARRAY);
	ShadowMapArray->Bind();
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, shadowMapSize.x, shadowMapSize.y, 16, 0, GL_DEPTH_COMPONENT, GL_FLOAT, (void*)(nullptr));
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(glm::vec4(1.0f)));

	ShadowCubemapArray = std::make_shared<Texture>();
	ShadowCubemapArray->GenerateID(GL_TEXTURE_CUBE_MAP_ARRAY);
	ShadowCubemapArray->Bind();
	glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_DEPTH_COMPONENT, shadowMapSize.x, shadowMapSize.y, 16 * 6, 0, GL_DEPTH_COMPONENT, GL_FLOAT, (void*)(nullptr));
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

const std::shared_ptr<Mesh> RenderEngine::GetBasicShapeMesh(EngineBasicShape type)
{
	switch (type)
	{
	case EngineBasicShape::QUAD:
		return FindMeshTree("ENG_QUAD")->FindMeshPtr("Quad_NoShadow");
	case EngineBasicShape::CUBE:
		return FindMeshTree("ENG_CUBE")->FindMeshPtr("Cube");
	case EngineBasicShape::SPHERE:
		return FindMeshTree("ENG_SPHERE")->FindMeshPtr("Sphere");
	case EngineBasicShape::CONE:
		return FindMeshTree("ENG_CONE")->FindMeshPtr("Cone");
	}

	std::cout << "ERROR! Type " << type << " does not represent a basic shape mesh.\n";
	return nullptr;
}

int RenderEngine::GetAvailableLightIndex()
{
	return LightData.GetAvailableLightIndex();
}

Shader* RenderEngine::GetLightShader(LightType type)
{
	return LightShaders[static_cast<unsigned int>(type)];
}


std::shared_ptr<ModelComponent> RenderEngine::AddModel(std::shared_ptr<ModelComponent> model)
{
	Models.push_back(model);
	return Models.back();
}

std::shared_ptr<LightProbe> RenderEngine::AddLightProbe(std::shared_ptr<LightProbe> probe)
{
	LightProbes.push_back(probe);
	return LightProbes.back();
}

std::shared_ptr<LightComponent> RenderEngine::AddLight(std::shared_ptr<LightComponent> light)
{
	LightData.AddLight(light);
	return light;
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
	auto found = std::find_if(MeshTrees.begin(), MeshTrees.end(), [path, ignore](const std::unique_ptr<MeshSystem::MeshTree>& tree) { return tree->GetPath() == path && tree.get() != ignore; });
	if (found != MeshTrees.end())
		return (*found).get();

	return nullptr;
}

Material* RenderEngine::AddMaterial(Material* material)
{
	Materials.push_back(material);
	return Materials.back();
}

std::shared_ptr<Shader> RenderEngine::AddShader(std::shared_ptr<Shader> shader, bool bForwardShader)
{
	if (bForwardShader)
		ForwardShaders.push_back(shader);

	Shaders.push_back(shader);

	return Shaders.back();
}

ModelComponent* RenderEngine::FindModel(std::string name)
{
	auto modelIt = std::find_if(Models.begin(), Models.end(), [name](std::shared_ptr<ModelComponent>& model) { return model->GetName() == name; });

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
	//std::cout << "Searching for " + name + ".\n";
	auto shaderIt = std::find_if(Shaders.begin(), Shaders.end(), [name](std::shared_ptr<Shader> shader) { return shader->GetName() == name; });

	//std::cout << *shaderIt << '\n';
	if (*shaderIt == nullptr)
	{
		std::cout << "Ups.\n";
		for (int i = 0; i < Shaders.size(); i++)
			std::cout << "zjebane: " << Shaders[i]->GetName() << ".\n";
	}

	if (shaderIt != Shaders.end())
		return shaderIt->get();

	std::cerr << "ERROR! Can't find a shader named " << name << "!\n";
	return nullptr;
}

void RenderEngine::RenderBoundInDebug(RenderInfo& info, GLenum mode, GLint first, GLint count, glm::vec3 color)
{
	Shader* debugShader = FindShader("Debug");
	debugShader->Use();
	debugShader->UniformMatrix4fv("MVP", *info.VP);
	debugShader->Uniform3fv("color", color);
	glDrawArrays(mode, first, count);
}

void RenderEngine::PreRenderPass()
{
	LightData.SetupLights();
	RenderLightProbes();
	LightShaders[3]->Use();
	for (int i = 0; i < static_cast<int>(LightProbes.size()); i++)
	{
		LocalLightProbe* localProbeCast = dynamic_cast<LocalLightProbe*>(LightProbes[i].get());
		if (!localProbeCast)
			continue;
		LightShaders[3]->Uniform3fv("lightProbePositions[" + std::to_string(i) + "]", localProbeCast->ProbeTransform.PositionRef);
	}
}

void RenderEngine::RenderShadowMaps(std::vector <std::shared_ptr<LightComponent>> lights)
{
	ShadowFramebuffer.Bind(true);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glDrawBuffer(GL_NONE);

	bool bCubemapBound = false;
	float timeSum = 0.0f;
	float time1;

	ShadowMapArray->Bind(10);
	ShadowCubemapArray->Bind(11);

	for (unsigned int i = 0; i < lights.size(); i++)
	{
		LightComponent* light = lights[i].get();
		if (light->GetType() == LightType::POINT)
		{
			time1 = (float)glfwGetTime();
			if (!bCubemapBound)
			{
				FindShader("DepthLinearize")->Use();
				bCubemapBound = true;
			}

			Transform lightWorld = light->GetTransform().GetWorldTransform();
			glm::vec3 lightPos = lightWorld.PositionRef;
			glm::mat4 viewTranslation = glm::translate(glm::mat4(1.0f), -lightPos);
			glm::mat4 projection = light->GetProjection();

			FindShader("DepthLinearize")->Uniform1f("far", light->GetFar());
			FindShader("DepthLinearize")->Uniform3fv("lightPos", lightPos);

			int cubemapFirst = light->GetShadowMapNr() * 6;
			timeSum += (float)glfwGetTime() - time1;
			
			
			RenderInfo info(&viewTranslation, &projection, nullptr, false, true, false);
			ShadowFramebuffer.DepthBuffer = std::make_shared<GEE_FB::FramebufferAttachment>(*ShadowCubemapArray, GL_DEPTH_ATTACHMENT);
			RenderCubemapFromScene(info, ShadowFramebuffer, *ShadowFramebuffer.DepthBuffer.get(), GL_DEPTH_ATTACHMENT, FindShader("DepthLinearize"), &cubemapFirst);
		}
		else
		{
			time1 = glfwGetTime();
			if (bCubemapBound || i == 0)
			{
				FindShader("Depth")->Use();
				bCubemapBound = false;
			}

			glm::mat4 VP = light->GetVP();

			timeSum += (float)glfwGetTime() - time1;
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ShadowMapArray->GetID(), 0, light->GetShadowMapNr());
			glClear(GL_DEPTH_BUFFER_BIT);
			
			RenderInfo info(nullptr, nullptr, &VP, false, true, false);
			RenderScene(info, FindShader("Depth"));
		}
	}

	glActiveTexture(GL_TEXTURE0);
	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//std::cout << "Wyczyscilem sobie " << timeSum * 1000.0f << "ms.\n";
}

void RenderEngine::RenderVolume(RenderInfo& info, EngineBasicShape shape, Shader& shader, const Transform* transform)
{
	if (shape == EngineBasicShape::QUAD)
		glDisable(GL_CULL_FACE);

	shader.Use();
	Render(info, GetBasicShapeMesh(shape), (transform) ? (*transform) : (Transform()), &shader);

	if (shape == EngineBasicShape::QUAD)
		glEnable(GL_CULL_FACE);
}

void RenderEngine::RenderVolume(RenderInfo& info, RenderableVolume* volume, Shader* boundShader, bool shadedRender)
{
	if (shadedRender)
	{
		if (boundShader != volume->GetRenderShader())
		{
			boundShader = volume->GetRenderShader();
			boundShader->Use();
		}

		volume->SetupRenderUniforms(*boundShader);
	}
	else if (!boundShader || boundShader->GetName() != "Depth")
	{
		boundShader = FindShader("Depth");
		boundShader->Use();
	}

	RenderVolume(info, volume->GetShape(), *boundShader, &volume->GetRenderTransform());
}

void RenderEngine::RenderVolumes(RenderInfo& info, const GEE_FB::Framebuffer& framebuffer, const std::vector<std::unique_ptr<RenderableVolume>>& volumes, bool bIBLPass)
{
	Shader* boundShader = nullptr;

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

	for (unsigned int i = 0; i < volumes.size(); i++)
	{
		//1st pass: stencil
		if (volumes[i]->GetShape() != EngineBasicShape::QUAD)		//if this is a quad everything will be in range; don't waste time
		{
			glStencilMask(0xFF);
			glClear(GL_STENCIL_BUFFER_BIT);
			glDisable(GL_CULL_FACE);
			glStencilFunc(GL_ALWAYS, 1, 0xFF);
			glDepthFunc(GL_LEQUAL);
			glDrawBuffer(GL_NONE);
			RenderVolume(info, volumes[i].get(), boundShader, false);
		}

		//2nd pass: lighting
		glStencilMask(0x00);
		glStencilFunc(((volumes[i]->GetShape() == EngineBasicShape::QUAD) ? (GL_ALWAYS) : (GL_EQUAL)), 0, 0xFF);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_ALWAYS);
		framebuffer.SetDrawBuffers();
		RenderVolume(info, volumes[i].get(), boundShader, true);
	}

	if (bIBLPass)
	{
		glDisable(GL_CULL_FACE);
		glDisable(GL_STENCIL_TEST);
		FindShader("CookTorranceIBL")->Use();
		Render(info, GetBasicShapeMesh(EngineBasicShape::QUAD), Transform(), FindShader("CookTorranceIBL"));
	}

	glDisable(GL_BLEND);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(0xFF);
	glStencilMask(0xFF);
	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
}

void RenderEngine::RenderLightProbes()
{
	std::cout << "Initting.\n";
	Dispose();
	Init(glm::uvec2(1024));

	//RenderInfo info;

	//info.projection = &p;

	GEE_FB::Framebuffer framebuffer;
	std::shared_ptr<GEE_FB::FramebufferAttachment> depthBuffer = GEE_FB::reserveDepthBuffer(glm::uvec2(1024), GL_DEPTH_COMPONENT, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_TEXTURE_2D);
	framebuffer.Bind(); 
	framebuffer.SetAttachments(glm::uvec2(1024), GEE_FB::FramebufferAttachment(), nullptr);
	framebuffer.Bind(true);

	for (int i = 0; i < static_cast<int>(LightProbes.size()); i++)
	{
		LocalLightProbe* localProbeCast = dynamic_cast<LocalLightProbe*>(LightProbes[i].get());
		if (!localProbeCast)
			continue;
		glm::vec3 camPos = localProbeCast->ProbeTransform.PositionRef;
		glm::mat4 viewTranslation = glm::translate(glm::mat4(1.0f), -camPos);
		glm::mat4 p = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

		RenderInfo info;
		info.view = &viewTranslation;
		info.projection = &p;
		info.camPos = &camPos;
		FindShader("Geometry")->Use();
		RenderCubemapFromScene(info, framebuffer, GEE_FB::FramebufferAttachment(LightProbes[i]->EnvironmentMap), GL_COLOR_ATTACHMENT0, FindShader("Geometry"), 0, true);
		
		LightProbes[i]->EnvironmentMap.Bind();
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
;
		LightProbeLoader::ConvoluteLightProbe(*LightProbes[i], &LightProbes[i]->EnvironmentMap); 
		if (PrimitiveDebugger::bDebugProbeLoading)
			printVector(camPos, "Rendering probe");
	}

	Dispose();
	Init(GameHandle->GetGameSettings()->WindowSize);
}

void RenderEngine::RenderScene(RenderInfo& info, Shader* shader)
{
	if (!shader)
		shader = FindShader("Default");

	shader->Use();
	Mesh* VAOBound = nullptr;
	Material* materialBound = nullptr;

	for (unsigned int i = 0; i < Models.size(); i++)
	{
		Render(info, *Models[i], shader, VAOBound, materialBound);
	}
}

void RenderEngine::FullRender(RenderInfo& info, GEE_FB::Framebuffer* framebuffer)
{
	const GameSettings* settings = GameHandle->GetGameSettings();
	bool DebugMode = false;
	bool DUPA = false;
	glm::mat4 currentFrameView = *info.view;
	info.previousFrameView = &PreviousFrameView;

	if (DebugMode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	////////////////////1. Shadow maps pass
	if (1 || (settings->ShadowLevel > SettingLevel::SETTING_MEDIUM))
	{
		RenderShadowMaps(LightData.Lights);
		DUPA = true;
	}

	LightData.LightsBuffer.SubData4fv(*info.camPos, 16);

	////////////////////2. Geometry pass
	GFramebuffer.Bind(true);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	info.MainPass = true;
	info.CareAboutShader = true;

	Shader* gShader = FindShader("Geometry");
	gShader->Use();
	gShader->Uniform3fv("camPos", *info.camPos);

	RenderScene(info, gShader);
	info.MainPass = false;
	info.CareAboutShader = false;

	////////////////////2.5 SSAO pass

	const Texture* SSAOtex = nullptr;
	if (settings->AmbientOcclusionSamples > 0)
		SSAOtex = Postprocessing.SSAOPass(info, GFramebuffer.GetColorBuffer(0).get(), GFramebuffer.GetColorBuffer(1).get());	//pass gPosition and gNormal

	////////////////////3. Lighting pass
	LightData.UpdateLightUniforms();
	MainFramebuffer.Bind();

	glClearBufferfv(GL_COLOR, 0, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.0f)));
	glClearBufferfv(GL_COLOR, 1, glm::value_ptr(glm::vec3(0.0f)));

	for (int i = 0; i < 4; i++)
	{
		GFramebuffer.GetColorBuffer(i)->Bind(i);
	}

	if (SSAOtex)
		SSAOtex->Bind(4);
	std::vector<std::unique_ptr<RenderableVolume>> volumes;
	volumes.resize(LightData.Lights.size());
	std::transform(LightData.Lights.begin(), LightData.Lights.end(), volumes.begin(), [](const std::shared_ptr<LightComponent>& light) {return std::make_unique<LightVolume>(LightVolume(*light.get())); });
	RenderVolumes(info, MainFramebuffer, volumes, false);// Shading == ShadingModel::SHADING_PBR_COOK_TORRANCE);

	std::vector<std::unique_ptr<RenderableVolume>> probeVolumes;
	probeVolumes.resize(LightProbes.size());
	std::transform(LightProbes.begin(), LightProbes.end(), probeVolumes.begin(), [](const std::shared_ptr<LightProbe>& probe) {return std::make_unique<LightProbeVolume>(LightProbeVolume(*probe.get())); });
	probeVolumes.erase(probeVolumes.begin());
	RenderVolumes(info, MainFramebuffer, probeVolumes, false);


	////////////////////3.5 Forward rendering pass
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	info.MainPass = true;
	info.CareAboutShader = true;

	for (unsigned int i = 0; i < ForwardShaders.size(); i++)
		RenderScene(info, ForwardShaders[i].get());

	//if (DebugMode)
		//PhysicsEng.DebugRender(&RenderEng, info);
	if (!framebuffer) 
		TestRenderCubemap(info);
	info.MainPass = false;
	info.CareAboutShader = false;

	////////////////////4. Postprocessing pass (Blur + Tonemapping & Gamma Correction)
	Postprocessing.Render(MainFramebuffer.GetColorBuffer(0).get(), (settings->bBloom) ? (MainFramebuffer.GetColorBuffer(1).get()) : (nullptr), MainFramebuffer.DepthBuffer.get(), (settings->IsVelocityBufferNeeded()) ? (MainFramebuffer.GetColorBuffer("velocityTex").get()) : (nullptr), framebuffer);
	      
	PreviousFrameView = currentFrameView;
}

void RenderEngine::TestRenderCubemap(RenderInfo& info)
{
	LightProbeTextureArrays::PrefilterMapArr.Bind(0);
	//LightProbes[1]->EnvironmentMap.Bind(0);
	FindShader("Cubemap")->Use();
	FindShader("Cubemap")->Uniform1f("mipLevel", (float)std::fmod(glfwGetTime(), 4.0));

	bool bCalcVelocity = GameHandle->GetGameSettings()->IsVelocityBufferNeeded() && info.MainPass;
	bool jitter = info.MainPass && GameHandle->GetGameSettings()->IsTemporalReprojectionEnabled();

	glm::mat4 previousJitteredVP = (jitter) ? (Postprocessing.GetJitterMat((Postprocessing.GetFrameIndex() - 1) % 2) * *info.VP) : (*info.VP);

	if (bCalcVelocity)
		FindShader("Cubemap")->UniformMatrix4fv("prevVP", *info.projection * *info.previousFrameView);

	Render(info, GetBasicShapeMesh(EngineBasicShape::CUBE), Transform(), FindShader("Cubemap"));
}

void RenderEngine::RenderCubemapFromTexture(Texture targetTex, Texture tex, glm::uvec2 size, Shader& shader, int* layer, int mipLevel)
{
	CubemapData.DefaultFramebuffer.SetAttachments(size);
	CubemapData.DefaultFramebuffer.Bind(true);
	glActiveTexture(GL_TEXTURE0);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST); 

	shader.Use();
	MeshSystem::MeshNode* cubeNode = FindMeshTree("ENG_CUBE")->FindNode("Cube");
	Mesh* cubeMesh = cubeNode->FindMesh("Cube");
	cubeMesh->Bind();

	if (PrimitiveDebugger::bDebugCubemapFromTex)
	{
		std::cout << "0: ";
		debugFramebuffer();
	}
	 
	for (int i = 0; i < 6; i++)
	{
		targetTex.Bind();
		switch (targetTex.GetType())
		{
		case GL_TEXTURE_CUBE_MAP:		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, targetTex.GetID(), mipLevel); break;
		case GL_TEXTURE_CUBE_MAP_ARRAY: glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, targetTex.GetID(), mipLevel, *layer); *layer = *layer + 1; break;
		default:						std::cerr << "ERROR: Can't render a scene to cubemap texture, when the texture's type is " << targetTex.GetType() << ".\n"; return;
		}
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		if (PrimitiveDebugger::bDebugCubemapFromTex)
		{
			std::cout << "1: ";
			debugFramebuffer();
		}

		tex.Bind();

		RenderInfo info;
		info.VP = &CubemapData.DefaultVP[i];

		Render(info, ModelComponent(GameHandle, *cubeNode), &shader, cubeMesh);
	}

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
}

void RenderEngine::RenderCubemapFromScene(RenderInfo& info, GEE_FB::Framebuffer target, GEE_FB::FramebufferAttachment targetTex, GLenum attachmentType, Shader* shader, int* layer, bool fullRender)
{
	target.Bind(true);
	glActiveTexture(GL_TEXTURE0);

	glm::mat4 viewTranslation = (info.view) ? (*info.view) : (glm::mat4(1.0f));
	glm::mat4 pCopy = *info.projection;

	for (int i = 0; i < 6; i++)
	{
		targetTex.Bind();
		switch (targetTex.GetType())
		{
			case GL_TEXTURE_CUBE_MAP:		glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, targetTex.GetID(), 0); break;
			case GL_TEXTURE_CUBE_MAP_ARRAY: glFramebufferTextureLayer(GL_FRAMEBUFFER, attachmentType, targetTex.GetID(), 0, *layer); *layer = *layer + 1; break;
			default:						std::cerr << "ERROR: Can't render a scene to cubemap texture, when the texture's type is " << targetTex.GetType() << ".\n"; return;
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 view = CubemapData.DefaultV[i] * viewTranslation;
		glm::mat4 proj = pCopy;
		glm::mat4 VP = (info.projection) ? ((*info.projection) * view) : (view);
		info.view = &view;
		info.VP = &VP;
		info.projection = &proj;
		(fullRender) ? (FullRender(info, &target)) : (RenderScene(info, shader));
	}
}

void RenderEngine::Render(RenderInfo& info, const ModelComponent& model, Shader* shader, const Mesh* boundMesh, Material* materialBound)
{
	if (model.GetMeshInstanceCount() == 0)
		return;

	const glm::mat4& modelMat = model.GetTransform().GetWorldTransformMatrix();	//the ComponentTransform's world transform is cached
	bool bCalcVelocity = GameHandle->GetGameSettings()->IsVelocityBufferNeeded() && info.MainPass;
	bool jitter = info.MainPass && GameHandle->GetGameSettings()->IsTemporalReprojectionEnabled();

	glm::mat4 jitteredVP = (info.VP) ? ((jitter) ? (Postprocessing.GetJitterMat() * *info.VP) : (*info.VP)) : (glm::mat4(1.0f));
	shader->BindMatrices(modelMat, info.view, info.projection, &jitteredVP);

	if (bCalcVelocity)
	{
		shader->UniformMatrix4fv("prevMVP", model.GetLastFrameMVP());
		model.SetLastFrameMVP(jitteredVP * modelMat);
	}

	for (int i = 0; i < static_cast<int>(model.GetMeshInstanceCount()); i++)
	{
		const MeshInstance& meshInst = model.GetMeshInstance(i);
		const Mesh& mesh = meshInst.GetMesh();
		Material* material = meshInst.GetMaterialPtr();
		MaterialInstance* materialInst = meshInst.GetMaterialInst();

		if ((info.CareAboutShader && material && shader->GetName() != material->GetRenderShaderName()) || (info.OnlyShadowCasters && !mesh.CanCastShadow()) || !materialInst->ShouldBeDrawn())
			continue;

		if (boundMesh != &mesh || i == 0)
		{
			mesh.Bind();
			boundMesh = &mesh;
		}

		if (info.UseMaterials && materialBound != material && material) //jesli zbindowany jest inny material niz potrzebny obecnie, musimy zmienic go w shaderze
		{
			materialInst->UpdateWholeUBOData(shader, *EmptyTexture);
			materialBound = material;
		}
		else if (materialBound) //jesli ostatni zbindowany material jest taki sam, to nie musimy zmieniac wszystkich danych w shaderze; oszczedzmy sobie roboty
			materialInst->UpdateInstanceUBOData(shader);

		mesh.Render();
	}
}

void RenderEngine::Render(RenderInfo& info, const std::shared_ptr<Mesh>& mesh, const Transform& transform, Shader* shader, const Mesh* boundMesh, Material* materialBound)
{
	Render(info, ModelComponent(GameHandle, MeshSystem::MeshNode(mesh), transform), shader, boundMesh, materialBound);
}

void RenderEngine::Dispose()
{
	GFramebuffer.Dispose(true);
	MainFramebuffer.Dispose(true);
	CubemapData.DefaultFramebuffer.Dispose();

	Postprocessing.Dispose();
	//ShadowFramebuffer.Dispose();

	ShadowMapArray->Dispose();
	ShadowCubemapArray->Dispose();
	//EmptyTexture->Dispose();

	for (auto i = Shaders.begin(); i != Shaders.end(); i++)
		if (std::find(ForwardShaders.begin(), ForwardShaders.end(), *i) == ForwardShaders.end())
		{
			(*i)->Dispose();
			Shaders.erase(i);
			i--;
		}

	LightShaders.clear();
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