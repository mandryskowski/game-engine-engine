#include "RenderEngine.h"
#include "Postprocess.h"
#include "FileLoader.h"
#include "LightComponent.h"
#include "ModelComponent.h"
#include "LightProbe.h"
#include "RenderableVolume.h"
#include "Font.h"
#include "Actor.h"
#include <random> //DO WYJEBANIA

RenderEngine::RenderEngine(GameManager* gameHandle) :
	GameHandle(gameHandle),
	PreviousFrameView(glm::mat4(1.0f)),
	BoundSkeletonBatch(nullptr),
	BoundMesh(nullptr),
	BoundMaterial(nullptr),
	CurrentTbCollection(nullptr)
{
	//configure some openGL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	//generate engine's empty texture
	EmptyTexture = std::make_shared<Texture>(textureFromBuffer(glm::value_ptr(glm::vec3(0.5f, 0.5f, 1.0f)), 1, 1, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST));
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

	for (int i = 0; i < static_cast<int>(ScenesRenderData.size()); i++)
	{
		if (ScenesRenderData[i]->LightProbes.empty() && GameHandle->GetGameSettings()->Video.Shading == ShadingModel::SHADING_PBR_COOK_TORRANCE)
		{
			LightProbeLoader::LoadLightProbeTextureArrays(ScenesRenderData[i]);
			ScenesRenderData[i]->ProbeTexArrays->IrradianceMapArr.Bind(12);
			ScenesRenderData[i]->ProbeTexArrays->PrefilterMapArr.Bind(13);
			ScenesRenderData[i]->ProbeTexArrays->BRDFLut.Bind(14);
			glActiveTexture(GL_TEXTURE0);
		}
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

	CreateMeshTree("ENG_CUBE")->GetRoot().AddChild<MeshSystem::MeshNode>("Cube")->AddMesh("Cube").LoadFromGLBuffers(36, cubeVAO, cubeVBO);
	CreateMeshTree("ENG_QUAD")->GetRoot().AddChild<MeshSystem::MeshNode>("Quad")->AddMesh("Quad_NoShadow").LoadFromGLBuffers(6, quadVAO, quadVBO, quadEBO);
	EngineDataLoader::LoadMeshTree(GameHandle, this, "EngineObjects/sphere.obj", CreateMeshTree("ENG_SPHERE"))->FindMesh("Sphere");
	EngineDataLoader::LoadMeshTree(GameHandle, this, "EngineObjects/cone.obj", CreateMeshTree("ENG_CONE"))->FindMesh("Cone");
}

void RenderEngine::LoadInternalShaders()
{
	std::string settingsDefines = GameHandle->GetGameSettings()->Video.GetShaderDefines(Resolution);

	//load the default shader
	std::vector<std::pair<unsigned int, std::string>> defaultShaderTextureUnits = {
		std::pair<unsigned int, std::string>(0, "albedo1"),
		std::pair<unsigned int, std::string>(1, "specular1"),
		std::pair<unsigned int, std::string>(2, "normal1"),
		std::pair<unsigned int, std::string>(3, "depth1")
	};
	//load shadow shaders
	Shaders.push_back(ShaderLoader::LoadShaders("Depth", "Shaders/depth.vs", "Shaders/depth.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MVP});
	Shaders.back()->UniformBlockBinding("BoneMatrices", 2);
	Shaders.push_back(ShaderLoader::LoadShaders("DepthLinearize", "Shaders/depth_linearize.vs", "Shaders/depth_linearize.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MODEL, MatrixType::MVP});
	Shaders.back()->UniformBlockBinding("BoneMatrices", 2);

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
}

void RenderEngine::Resize(glm::uvec2 resolution)
{
	Resolution = resolution;
	const GameSettings* settings = GameHandle->GetGameSettings();
}

void RenderEngine::SetupShadowmaps()
{
	/*
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
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);*/
}

const ShadingModel& RenderEngine::GetShadingModel()
{
	return GameHandle->GetGameSettings()->Video.Shading;
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

Shader* RenderEngine::GetLightShader(const RenderToolboxCollection& renderCol, LightType type)
{
	return renderCol.GetTb<DeferredShadingToolbox>()->LightShaders[static_cast<unsigned int>(type)];
}

RenderToolboxCollection* RenderEngine::GetCurrentTbCollection()
{
	return CurrentTbCollection;
}

RenderToolboxCollection& RenderEngine::AddRenderTbCollection(const RenderToolboxCollection& tbCollection, bool setupToolboxesAccordingToSettings)
{
	RenderTbCollections.push_back(std::make_unique<RenderToolboxCollection>(tbCollection));
	if (setupToolboxesAccordingToSettings)
		RenderTbCollections.back()->AddTbsRequiredBySettings();

	CurrentTbCollection = RenderTbCollections.back().get();

	return *RenderTbCollections.back();
}

void RenderEngine::AddSceneRenderDataPtr(GameSceneRenderData* sceneRenderData)
{
	ScenesRenderData.push_back(sceneRenderData);
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

void RenderEngine::BindSkeletonBatch(SkeletonBatch* batch)
{
	if (!batch)
		return;

	batch->BindToUBO();
	BoundSkeletonBatch = batch;
}

void RenderEngine::BindSkeletonBatch(GameSceneRenderData* sceneRenderData, unsigned int index)
{
	if (index >= sceneRenderData->SkeletonBatches.size())
		return;

	sceneRenderData->SkeletonBatches[index]->BindToUBO();
	BoundSkeletonBatch = sceneRenderData->SkeletonBatches[index].get();
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

void RenderEngine::RenderShadowMaps(RenderToolboxCollection& tbCollection, GameSceneRenderData* sceneRenderData, std::vector <std::shared_ptr<LightComponent>> lights)
{
	ShadowMappingToolbox* shadowsTb = tbCollection.GetTb<ShadowMappingToolbox>();
	shadowsTb->ShadowFramebuffer->Bind(true);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glDrawBuffer(GL_NONE);

	bool bCubemapBound = false;
	float timeSum = 0.0f;
	float time1;

	shadowsTb->ShadowMapArray->Bind(10);
	shadowsTb->ShadowCubemapArray->Bind(11);

	BindSkeletonBatch(sceneRenderData, static_cast<unsigned int>(0));

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
			
			
			RenderInfo info(tbCollection, viewTranslation, projection, glm::mat4(1.0f), glm::vec3(0.0f), false, true, false);
			shadowsTb->ShadowFramebuffer->DepthBuffer = std::make_shared<GEE_FB::FramebufferAttachment>(*shadowsTb->ShadowCubemapArray, GL_DEPTH_ATTACHMENT);
			RenderCubemapFromScene(info, sceneRenderData, *shadowsTb->ShadowFramebuffer, *shadowsTb->ShadowFramebuffer->DepthBuffer, GL_DEPTH_ATTACHMENT, FindShader("DepthLinearize"), &cubemapFirst);
		}
		else
		{
			time1 = glfwGetTime();
			if (bCubemapBound || i == 0)
			{
				FindShader("Depth")->Use();
				bCubemapBound = false;
			}

			glm::mat4 view = light->GetTransform().GetWorldTransform().GetViewMatrix();
			glm::mat4 projection = light->GetProjection();
			glm::mat4 VP = projection * view;

			timeSum += (float)glfwGetTime() - time1;
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowsTb->ShadowMapArray->GetID(), 0, light->GetShadowMapNr());
			glClear(GL_DEPTH_BUFFER_BIT);
			
			RenderInfo info(tbCollection, view, projection, VP, glm::vec3(0.0f), false, true, false);
			RenderRawScene(info, sceneRenderData, FindShader("Depth"));
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
	BoundMaterial = nullptr;
	BoundMesh = nullptr;
	RenderStaticMesh(info, GetBasicShapeMesh(shape).get(), (transform) ? (*transform) : (Transform()), &shader);

	if (shape == EngineBasicShape::QUAD)
		glEnable(GL_CULL_FACE);
}

void RenderEngine::RenderVolume(RenderInfo& info, RenderableVolume* volume, Shader* boundShader, bool shadedRender)
{
	if (shadedRender)
	{
		if (boundShader != volume->GetRenderShader(info.TbCollection))
		{
			boundShader = volume->GetRenderShader(info.TbCollection);
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
		RenderStaticMesh(info, GetBasicShapeMesh(EngineBasicShape::QUAD).get(), Transform(), FindShader("CookTorranceIBL"));
	}

	glDisable(GL_BLEND);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(0xFF);
	glStencilMask(0xFF);
	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
}

void RenderEngine::RenderLightProbes(GameSceneRenderData* sceneRenderData)
{
	if (sceneRenderData->LightProbes.empty())
		return;

	GameSettings::VideoSettings settings;
	settings.AAType = AA_SMAA1X;
	settings.AALevel = SettingLevel::SETTING_ULTRA;
	settings.bBloom = true;
	settings.AmbientOcclusionSamples = 64;
	settings.POMLevel = SettingLevel::SETTING_ULTRA;
	settings.ShadowLevel = SettingLevel::SETTING_ULTRA;
	settings.Resolution = glm::uvec2(1024);
	settings.Shading = ShadingModel::SHADING_PBR_COOK_TORRANCE;

	std::cout << "Initting.\n";
	//Dispose();
	//Init(glm::uvec2(1024));

	RenderToolboxCollection probeRenderingCollection("LightProbeRendering", settings);
	CurrentTbCollection = &probeRenderingCollection;
	CurrentTbCollection->AddTbsRequiredBySettings();


	//RenderInfo info;

	//info.projection = &p;

	GEE_FB::Framebuffer framebuffer;
	std::shared_ptr<GEE_FB::FramebufferAttachment> depthBuffer = GEE_FB::reserveDepthBuffer(glm::uvec2(1024), GL_DEPTH_COMPONENT, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_TEXTURE_2D);
	framebuffer.Bind(); 
	framebuffer.SetAttachments(glm::uvec2(1024), GEE_FB::FramebufferAttachment(), nullptr);
	framebuffer.Bind(true);

	Shader* gShader = probeRenderingCollection.GetTb<DeferredShadingToolbox>()->GeometryShader;

	PrepareScene(probeRenderingCollection, sceneRenderData);

	for (int i = 0; i < static_cast<int>(sceneRenderData->LightProbes.size()); i++)
	{
		LocalLightProbe* localProbeCast = dynamic_cast<LocalLightProbe*>(sceneRenderData->LightProbes[i].get());
		if (!localProbeCast)
			continue;
		glm::vec3 camPos = localProbeCast->ProbeTransform.PositionRef;
		glm::mat4 viewTranslation = glm::translate(glm::mat4(1.0f), -camPos);
		glm::mat4 p = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

		RenderInfo info(probeRenderingCollection);
		info.view = viewTranslation;
		info.projection = p;
		info.camPos = camPos;
		gShader->Use();
		RenderCubemapFromScene(info, sceneRenderData, framebuffer, GEE_FB::FramebufferAttachment(sceneRenderData->LightProbes[i]->EnvironmentMap), GL_COLOR_ATTACHMENT0, gShader, 0, true);
		
		sceneRenderData->LightProbes[i]->EnvironmentMap.Bind();
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
;
		LightProbeLoader::ConvoluteLightProbe(sceneRenderData, *sceneRenderData->LightProbes[i], &sceneRenderData->LightProbes[i]->EnvironmentMap); 
		if (PrimitiveDebugger::bDebugProbeLoading)
			printVector(camPos, "Rendering probe");
	}

	//Dispose();

	//Init(glm::uvec2(GameHandle->GetGameSettings()->ViewportData.z, GameHandle->GetGameSettings()->ViewportData.w));
	CurrentTbCollection = RenderTbCollections[0].get();
	probeRenderingCollection.Dispose();
}

void RenderEngine::RenderRawScene(RenderInfo& info, GameSceneRenderData* sceneRenderData, Shader* shader)
{
	if (!shader)
		shader = FindShader("Default");

	shader->Use();
	BoundMesh = nullptr;
	BoundMaterial = nullptr;

	for (unsigned int i = 0; i < sceneRenderData->Renderables.size(); i++)
	{
		sceneRenderData->Renderables[i]->Render(info, shader);
	}
}

void RenderEngine::RenderBoundInDebug(RenderInfo& info, GLenum mode, GLint first, GLint count, glm::vec3 color)
{
	Shader* debugShader = FindShader("Debug");
	debugShader->Use();
	debugShader->UniformMatrix4fv("MVP", info.VP);
	debugShader->Uniform3fv("color", color);
	glDrawArrays(mode, first, count);
}

void RenderEngine::PreLoopPass()
{
	for (int sceneIndex = 0; sceneIndex < static_cast<int>(ScenesRenderData.size()); sceneIndex++)
	{
		ScenesRenderData[sceneIndex]->SetupLights(sceneIndex);
		RenderLightProbes(ScenesRenderData[sceneIndex]);
	}
}

void RenderEngine::PrepareScene(RenderToolboxCollection& tbCollection, GameSceneRenderData* sceneRenderData)
{
	if (sceneRenderData->ContainsLights() && (1 || (GameHandle->GetGameSettings()->Video.ShadowLevel > SettingLevel::SETTING_MEDIUM)))
		RenderShadowMaps(tbCollection, sceneRenderData, sceneRenderData->Lights);
}

void RenderEngine::FullSceneRender(RenderInfo& info, GameSceneRenderData* sceneRenderData, GEE_FB::Framebuffer* framebuffer, Viewport viewport)
{
	const GameSettings::VideoSettings& settings = info.TbCollection.Settings;
	bool debugPhysics = false;
	bool debugComponents = true;
	bool useLightingAlgorithms = sceneRenderData->ContainsLights();
	glm::mat4 currentFrameView = info.view;
	info.previousFrameView = PreviousFrameView;

	const GEE_FB::Framebuffer& target = ((framebuffer) ? (*framebuffer) : (GEE_FB::getDefaultFramebuffer(GameHandle->GetGameSettings()->WindowSize)));
	if (viewport.GetSize() == glm::uvec2(0))
		viewport = Viewport(glm::uvec2(0), target.GetSize());

	sceneRenderData->LightsBuffer.SubData4fv(info.camPos, 16);

	MainFramebufferToolbox* mainTb = info.TbCollection.GetTb<MainFramebufferToolbox>();
	GEE_FB::Framebuffer& MainFramebuffer = *mainTb->MainFb;

	////////////////////2. Geometry pass
	if (useLightingAlgorithms)
	{
		DeferredShadingToolbox* deferredTb = info.TbCollection.GetTb<DeferredShadingToolbox>();
		GEE_FB::Framebuffer& GFramebuffer = *deferredTb->GFb;

		GFramebuffer.Bind(true);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		info.MainPass = true;
		info.CareAboutShader = true;


		if (debugPhysics)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		Shader* gShader = deferredTb->GeometryShader;
		gShader->Use();
		gShader->Uniform3fv("camPos", info.camPos);

		RenderRawScene(info, sceneRenderData, gShader);
		info.MainPass = false;
		info.CareAboutShader = false;  


		if (debugPhysics)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		////////////////////2.5 SSAO pass
		const Texture* SSAOtex = nullptr;
		if (settings.AmbientOcclusionSamples > 0)
			SSAOtex = Postprocessing.SSAOPass(info, GFramebuffer.GetColorBuffer(0).get(), GFramebuffer.GetColorBuffer(1).get());	//pass gPosition and gNormal
		
		////////////////////3. Lighting pass
		for (int i = 0; i < static_cast<int>(deferredTb->LightShaders.size()); i++)
			deferredTb->LightShaders[i]->UniformBlockBinding("Lights", sceneRenderData->LightsBuffer.BlockBindingSlot);
		sceneRenderData->UpdateLightUniforms();
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
		volumes.resize(sceneRenderData->Lights.size());
		std::transform(sceneRenderData->Lights.begin(), sceneRenderData->Lights.end(), volumes.begin(), [](const std::shared_ptr<LightComponent>& light) {return std::make_unique<LightVolume>(LightVolume(*light.get())); });
		RenderVolumes(info, MainFramebuffer, volumes, false);// Shading == ShadingModel::SHADING_PBR_COOK_TORRANCE);

		info.TbCollection.FindShader("CookTorranceIBL")->Use();
		for (int i = 0; i < static_cast<int>(sceneRenderData->LightProbes.size()); i++)
		{
			LocalLightProbe* localProbeCast = dynamic_cast<LocalLightProbe*>(sceneRenderData->LightProbes[i].get());
			if (!localProbeCast)
				continue;

			info.TbCollection.FindShader("CookTorranceIBL")->Uniform3fv("lightProbePositions[" + std::to_string(i) + "]", localProbeCast->ProbeTransform.PositionRef);
		}

		std::vector<std::unique_ptr<RenderableVolume>> probeVolumes;
		probeVolumes.resize(sceneRenderData->LightProbes.size());
		std::transform(sceneRenderData->LightProbes.begin(), sceneRenderData->LightProbes.end(), probeVolumes.begin(), [](const std::shared_ptr<LightProbe>& probe) {return std::make_unique<LightProbeVolume>(LightProbeVolume(*probe.get())); });
		if (!probeVolumes.empty())	//TODO: DELETE. VERY NASTY!!!!!! ADD LOADING LIGHT PROBES FROM FILE AND DONT DELETE THE FIRST PROBE FOR NO REASON
		{
			probeVolumes.erase(probeVolumes.begin());
			sceneRenderData->ProbeTexArrays->IrradianceMapArr.Bind(12);
			sceneRenderData->ProbeTexArrays->PrefilterMapArr.Bind(13);
			sceneRenderData->ProbeTexArrays->BRDFLut.Bind(14);
			RenderVolumes(info, MainFramebuffer, probeVolumes, false);
		}
	}
	else
	{
		MainFramebuffer.Bind(true, &viewport);
		
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}


	////////////////////3.5 Forward rendering pass
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	info.MainPass = true;
	info.CareAboutShader = true;

	if (!framebuffer)
		TestRenderCubemap(info, sceneRenderData);

	for (unsigned int i = 0; i < ForwardShaders.size(); i++)
		RenderRawScene(info, sceneRenderData, ForwardShaders[i].get());

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	FindShader("Forward_NoLight")->Use();
	FindShader("Forward_NoLight")->Uniform2fv("atlasData", glm::vec2(0.0f));
	FindShader("Forward_NoLight")->Uniform2fv("atlasTexOffset", glm::vec2(0.0f));

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	if (debugPhysics)
		//GameHandle->GetPhysicsHandle()->DebugRender(this, info);
	info.MainPass = false;
	info.CareAboutShader = false;

	////////////////////4. Postprocessing pass (Blur + Tonemapping & Gamma Correction)
	Postprocessing.Render(info.TbCollection, target, &viewport, MainFramebuffer.GetColorBuffer(0).get(), (settings.bBloom) ? (MainFramebuffer.GetColorBuffer(1).get()) : (nullptr), MainFramebuffer.DepthBuffer.get(), (settings.IsVelocityBufferNeeded()) ? (MainFramebuffer.GetColorBuffer("velocityTex").get()) : (nullptr));

	      
	PreviousFrameView = currentFrameView;
}

void RenderEngine::PrepareFrame()
{
	BindSkeletonBatch(GameHandle->GetMainScene()->GetRenderData(), static_cast<unsigned int>(0));

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderEngine::PostFrame()
{
	//Antialiasing();
}

void RenderEngine::RenderFrame(RenderInfo& info) 
{
	/*PreFrame();
	for (int i = 0; i < Scenes.size(); i++)
	{
		FullSceneRender(Scene, Viewport, ...);
	}
	PostFrame();*/

	PrepareFrame();

	FullSceneRender(info, ScenesRenderData[0]);
	info.view = glm::mat4(1.0f);
	info.projection = glm::mat4(1.0f);
	info.VP = glm::mat4(1.0f);

	glViewport(0, 0, 800, 600);
	info.MainPass = true;
	info.CareAboutShader = true;
	RenderRawScene(info, ScenesRenderData[1], FindShader("Forward_NoLight"));
	RenderRawScene(info, ScenesRenderData[1], FindShader("TextShader"));

	PostFrame();
}

void RenderEngine::TestRenderCubemap(RenderInfo& info, GameSceneRenderData* sceneRenderData)
{
	sceneRenderData->ProbeTexArrays->PrefilterMapArr.Bind(0);
	//LightProbes[1]->EnvironmentMap.Bind(0);
	FindShader("Cubemap")->Use();
	FindShader("Cubemap")->Uniform1f("mipLevel", (float)std::fmod(glfwGetTime(), 4.0));

	bool bCalcVelocity = GameHandle->GetGameSettings()->Video.IsVelocityBufferNeeded() && info.MainPass;
	bool jitter = info.MainPass && GameHandle->GetGameSettings()->Video.IsTemporalReprojectionEnabled();

	glm::mat4 jitteredVP = (jitter) ? (Postprocessing.GetJitterMat(info.TbCollection.GetSettings(), Postprocessing.GetFrameIndex()) * info.VP) : (info.VP);

	if (bCalcVelocity)
	{
		FindShader("Cubemap")->UniformMatrix4fv("prevVP", info.projection * info.previousFrameView);
		FindShader("Cubemap")->UniformMatrix4fv("flickerMat", Postprocessing.GetJitterMat(info.TbCollection.GetSettings(), Postprocessing.GetFrameIndex()));
		FindShader("Cubemap")->UniformMatrix4fv("prevFlickerMat", Postprocessing.GetJitterMat(info.TbCollection.GetSettings(), (Postprocessing.GetFrameIndex() + 1) % 2));
	}
	else
	{
		FindShader("Cubemap")->UniformMatrix4fv("prevVP", glm::mat4(1.0f));
		FindShader("Cubemap")->UniformMatrix4fv("flickerMat", glm::mat4(1.0f));
		FindShader("Cubemap")->UniformMatrix4fv("prevFlickerMat", glm::mat4(1.0f));
	}


	RenderStaticMesh(info, GetBasicShapeMesh(EngineBasicShape::CUBE).get(), Transform(), FindShader("Cubemap"));
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
	MeshSystem::MeshNode* cubeNode = dynamic_cast<MeshSystem::MeshNode*>(FindMeshTree("ENG_CUBE")->FindNode("Cube"));
	Mesh* cubeMesh = cubeNode->FindMesh("Cube");
	cubeMesh->Bind();
	BoundMesh = cubeMesh;

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

		RenderInfo info(*CurrentTbCollection);
		info.VP = CubemapData.DefaultVP[i];

		RenderStaticMesh(info, GetBasicShapeMesh(EngineBasicShape::CUBE).get(), Transform(), &shader);
	}

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
}

void RenderEngine::RenderCubemapFromScene(RenderInfo info, GameSceneRenderData* sceneRenderData, GEE_FB::Framebuffer target, GEE_FB::FramebufferAttachment targetTex, GLenum attachmentType, Shader* shader, int* layer, bool fullRender)
{
	target.Bind(true);
	glActiveTexture(GL_TEXTURE0);

	glm::mat4 viewTranslation = info.view;

	for (int i = 0; i < 6; i++)
	{
		targetTex.Bind();
		switch (targetTex.GetType())
		{
			case GL_TEXTURE_CUBE_MAP:		glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, targetTex.GetID(), 0); break;
			case GL_TEXTURE_CUBE_MAP_ARRAY: glFramebufferTextureLayer(GL_FRAMEBUFFER, attachmentType, targetTex.GetID(), 0, *layer); *layer = *layer + 1; break;
			default:						std::cerr << "ERROR: Can't render a scene to cubemap texture, when the texture's type is " << targetTex.GetType() << ".\n"; return;
		}
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		info.view = CubemapData.DefaultV[i] * viewTranslation;
		info.CalculateVP();

		(fullRender) ? (FullSceneRender(info, sceneRenderData, &target)) : (RenderRawScene(info, sceneRenderData, shader));
	}
}

void RenderEngine::RenderText(RenderInfo info, const Font& font, std::string content, Transform t, glm::vec3 color, Shader* shader, bool convertFromPx)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	if (!shader)
	{
		shader = FindShader("TextShader");
		shader->Use();
	}

	shader->Uniform3fv("color", color);
	font.GetBitmapsArray().Bind(0);

	glm::vec2 resolution(GameHandle->GetGameSettings()->WindowSize);// (GameHandle->GetGameSettings()->ViewportData.z, GameHandle->GetGameSettings()->ViewportData.w);

	glm::mat4 pxConvertMatrix = (convertFromPx) ? (glm::ortho(0.0f, resolution.x, 0.0f, resolution.y)) : (glm::mat4(1.0f));
	glm::mat4 proj = info.projection;
	glm::mat4 vp = info.CalculateVP() * pxConvertMatrix;
	info.projection = proj;
	info.VP = vp;
	info.CareAboutShader = true;
	info.MainPass = false;
	info.UseMaterials = true;

	glm::vec3 scale = t.ScaleRef / 64.0f;
	t.Move(glm::vec3(0.0f, -t.ScaleRef.y + font.GetBaselineHeight() * scale.y, 0.0f));

	//t.Move(glm::vec3(0.0f, -64.0f, 0.0f));
	//t.Move(glm::vec3(0.0f, 11.0f, 0.0f) / scale);

	float advancesSum = 0.0f;
	for (int i = 0; i < static_cast<int>(content.length()); i++)
		advancesSum += (font.GetCharacter(content[i]).Advance / 64.0f) * scale.x / 2.0f;

	//t.Move(glm::vec3(-advancesSum, -t.ScaleRef.y / 2.0f, 0.0f));

	Material textMaterial("TextMaterial", 0.0f, 0.0f, FindShader("TextShader"));

	for (int i = 0; i < static_cast<int>(content.length()); i++)
	{
		shader->Uniform1i("glyphNr", content[i]);
		const Character& c = font.GetCharacter(content[i]);

		t.Move(glm::vec3((float)c.Bearing.x, (float)c.Bearing.y, 0.0f) * scale);
		t.SetScale(glm::vec3(glm::vec2(64.0f), 0.0f) * scale);
		//printVector(vp * t.GetWorldTransformMatrix() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), "Letter " + std::to_string(i));
		RenderStaticMesh(info, GetBasicShapeMesh(EngineBasicShape::QUAD).get(), t, shader, nullptr, &textMaterial);
		t.Move(-glm::vec3((float)c.Bearing.x, (float)c.Bearing.y, 0.0f) * scale);

		t.Move(glm::vec3(c.Advance / 64.0f, 0.0f, 0.0f) * scale);
	}

	glDisable(GL_BLEND);
}

void RenderEngine::RenderStaticMesh(RenderInfo info, const MeshInstance& mesh, const Transform& transform, Shader* shader, glm::mat4* lastFrameMVP, Material* material, bool billboard)
{
	std::vector<std::unique_ptr<MeshInstance>> vec;
	vec.push_back(std::make_unique<MeshInstance>(MeshInstance(mesh)));
	RenderStaticMeshes(info, vec, transform, shader, lastFrameMVP, material, billboard);
	vec.begin()->release();
}

/*
void RenderEngine::Render(RenderInfo info, const ModelComponent& model, Shader* shader)
{
	if (model.GetMeshInstanceCount() == 0)
		return;

	model.DRAWBATCH();

	if (SkeletonInfo* skelInfo = model.GetSkeletonInfo())
	{
		if (skelInfo->GetBatchPtr() != BoundSkeletonBatch)
			BindSkeletonBatch(model.GetSkeletonInfo()->GetBatchPtr());

		shader->Uniform1i("boneIDOffset", skelInfo->GetIDOffset());
	}

	glm::mat4 multiply(1.0f);
	if (model.GetName() == "Quad")
		//multiply = glm::inverse(glm::mat4(glm::transpose(glm::inverse(glm::mat3(info.GetView())))));
		multiply = glm::inverse(model.GetTransform().GetWorldTransform().GetRotationMatrix()) * glm::inverse(glm::mat3(info.GetView()));

	const glm::mat4& modelMat = model.GetTransform().GetWorldTransformMatrix() * multiply;	//the ComponentTransform's world transform is cached
	bool bCalcVelocity = GameHandle->GetGameSettings()->IsVelocityBufferNeeded() && info.MainPass;
	bool jitter = info.MainPass && GameHandle->GetGameSettings()->IsTemporalReprojectionEnabled();

	glm::mat4 jitteredVP = (info.VP) ? ((jitter) ? (Postprocessing.GetJitterMat() * info.VP) : (info.VP)) : (glm::mat4(1.0f));
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

		if (BoundMesh != &mesh || i == 0)
		{
			mesh.Bind();
			BoundMesh = &mesh;
		}
		
		if (info.UseMaterials && material)
		{
			if (BoundMaterial != material) //jesli zbindowany jest inny material niz potrzebny obecnie, musimy zmienic go w shaderze
			{
				materialInst->UpdateWholeUBOData(shader, *EmptyTexture);
				BoundMaterial = material;
			}
			else if (BoundMaterial) //jesli ostatni zbindowany material jest taki sam, to nie musimy zmieniac wszystkich danych w shaderze; oszczedzmy sobie roboty
				materialInst->UpdateInstanceUBOData(shader);
		}

		mesh.Render();
	}
}

void RenderEngine::Render(RenderInfo info, const std::shared_ptr<Mesh>& mesh, const Transform& transform, Shader* shader, Material* material)
{
	Render(info, ModelComponent(GameHandle, MeshSystem::MeshNode(mesh), "", transform, nullptr, material), shader);
}*/

void RenderEngine::RenderStaticMeshes(RenderInfo info, const std::vector<std::unique_ptr<MeshInstance>>& meshes, const Transform& transform, Shader* shader, glm::mat4* lastFrameMVP, Material* overrideMaterial, bool billboard)
{
	if (meshes.empty())
		return;

	glm::mat4 modelMat = transform.GetWorldTransformMatrix();	//the ComponentTransform's world transform is cached
	if (billboard)
		modelMat = modelMat * glm::mat4(glm::inverse(transform.GetWorldTransform().GetRotationMatrix()) * glm::inverse(glm::mat3(info.view)));

	bool bCalcVelocity = GameHandle->GetGameSettings()->Video.IsVelocityBufferNeeded() && info.MainPass;
	bool jitter = info.MainPass && GameHandle->GetGameSettings()->Video.IsTemporalReprojectionEnabled();

	glm::mat4 jitteredVP = (jitter) ? (Postprocessing.GetJitterMat(info.TbCollection.GetSettings(), (Postprocessing.GetFrameIndex() + 1) % 2) * info.VP) : (info.VP);
	shader->BindMatrices(modelMat, &info.view, &info.projection, &jitteredVP);

	if (bCalcVelocity)
	{
		if (!lastFrameMVP)
			;// std::cerr << "ERROR: Velocity buffer calculation is enabled, but no lastFrameMVP is passed to render call.\n";
		else
		{
			shader->UniformMatrix4fv("prevMVP", Postprocessing.GetJitterMat(info.TbCollection.GetSettings(), (Postprocessing.GetFrameIndex() + 1) % 2) * *lastFrameMVP);
			*lastFrameMVP = info.VP * modelMat;
		}
	}

	std::unique_ptr<MaterialInstance> createdInstance = ((overrideMaterial) ? (std::make_unique<MaterialInstance>(MaterialInstance(overrideMaterial))) : (nullptr));

	for (int i = 0; i < static_cast<int>(meshes.size()); i++)
	{
		const MeshInstance& meshInst = *meshes[i];
		const Mesh& mesh = meshInst.GetMesh();
		MaterialInstance* materialInst = ((overrideMaterial) ?  (createdInstance.get()) : (meshInst.GetMaterialInst()));
		Material* material = ((overrideMaterial) ? (overrideMaterial) : (meshInst.GetMaterialPtr()));

		if ((info.CareAboutShader && material && shader->GetName() != material->GetRenderShaderName()) || (info.OnlyShadowCasters && !mesh.CanCastShadow()) || !materialInst->ShouldBeDrawn())
			continue;

		if (BoundMesh != &mesh || i == 0)
		{
			mesh.Bind();
			BoundMesh = &mesh;
		}

		if (info.UseMaterials && material)
		{
			if (BoundMaterial != material) //jesli zbindowany jest inny material niz potrzebny obecnie, musimy zmienic go w shaderze
			{
				materialInst->UpdateWholeUBOData(shader, *EmptyTexture);
				BoundMaterial = material;
			}
			else if (BoundMaterial) //jesli ostatni zbindowany material jest taki sam, to nie musimy zmieniac wszystkich danych w shaderze; oszczedzmy sobie roboty
				materialInst->UpdateInstanceUBOData(shader);
		}

		mesh.Render();
	}
}

void RenderEngine::RenderSkeletalMeshes(RenderInfo info, const std::vector<std::unique_ptr<MeshInstance>>& meshes, const Transform& transform, Shader* shader, SkeletonInfo& skelInfo, Material* overrideMaterial)
{
	if (meshes.empty())
		return;

	if (skelInfo.GetBatchPtr() != BoundSkeletonBatch)
		BindSkeletonBatch(skelInfo.GetBatchPtr());

	shader->Uniform1i("boneIDOffset", skelInfo.GetIDOffset());

	glm::mat4 modelMat = transform.GetWorldTransformMatrix();	//the ComponentTransform's world transform is cached
	bool bCalcVelocity = GameHandle->GetGameSettings()->Video.IsVelocityBufferNeeded() && info.MainPass;
	bool jitter = info.MainPass && GameHandle->GetGameSettings()->Video.IsTemporalReprojectionEnabled();

	glm::mat4 jitteredVP = (jitter) ? (Postprocessing.GetJitterMat(info.TbCollection.GetSettings()) * info.VP) : (info.VP);
	shader->BindMatrices(modelMat, &info.view, &info.projection, &jitteredVP);


	//TODO: Pass the bone matrices from the last frame to fix velocity buffer calculation


	std::unique_ptr<MaterialInstance> createdInstance = ((overrideMaterial) ? (std::make_unique<MaterialInstance>(MaterialInstance(overrideMaterial))) : (nullptr));

	for (int i = 0; i < static_cast<int>(meshes.size()); i++)
	{
		const MeshInstance& meshInst = *meshes[i];
		const Mesh& mesh = meshInst.GetMesh();
		MaterialInstance* materialInst = ((overrideMaterial) ? (createdInstance.get()) : (meshInst.GetMaterialInst()));
		Material* material = ((overrideMaterial) ? (overrideMaterial) : (meshInst.GetMaterialPtr()));

		if ((info.CareAboutShader && material && shader->GetName() != material->GetRenderShaderName()) || (info.OnlyShadowCasters && !mesh.CanCastShadow()) || !materialInst->ShouldBeDrawn())
			continue;

		if (BoundMesh != &mesh || i == 0)
		{
			mesh.Bind();
			BoundMesh = &mesh;
		}

		if (info.UseMaterials && material)
		{
			if (BoundMaterial != material) //jesli zbindowany jest inny material niz potrzebny obecnie, musimy zmienic go w shaderze
			{
				materialInst->UpdateWholeUBOData(shader, *EmptyTexture);
				BoundMaterial = material;
			}
			else if (BoundMaterial) //jesli ostatni zbindowany material jest taki sam, to nie musimy zmieniac wszystkich danych w shaderze; oszczedzmy sobie roboty
				materialInst->UpdateInstanceUBOData(shader);
		}

		mesh.Render();
	}
}

void RenderEngine::Dispose()
{
	RenderTbCollections.clear();
	CubemapData.DefaultFramebuffer.Dispose();

	Postprocessing.Dispose();
	//ShadowFramebuffer.Dispose();

	//CurrentTbCollection->ShadowsTb->ShadowMapArray->Dispose();
	//CurrentTbCollection->ShadowsTb->ShadowCubemapArray->Dispose();
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