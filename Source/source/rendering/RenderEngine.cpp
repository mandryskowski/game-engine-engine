#include <rendering/RenderEngine.h>
#include <rendering/Postprocess.h>
#include <assetload/FileLoader.h>
#include <scene/LightComponent.h>
#include <scene/ModelComponent.h>
#include <rendering/LightProbe.h>
#include <rendering/RenderableVolume.h>
#include <UI/Font.h>
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

	Postprocessing.Init(GameHandle, Resolution);

	CubemapData.DefaultV[0] = glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	CubemapData.DefaultV[1] = glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	CubemapData.DefaultV[2] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	CubemapData.DefaultV[3] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	CubemapData.DefaultV[4] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	CubemapData.DefaultV[5] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

	glm::mat4 cubemapProj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 2.0f);

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
	//Add the engine objects to the tree collection. This way models loaded from the level file can simply use the internal engine meshes for stuff like particles
	//Preferably, the engine objects should be added to the tree collection first, so searching for them takes less time as they'll possibly be used often.

	GameScene& engineObjScene = GameHandle->CreateScene("GEE_Engine_Objects");

	EngineDataLoader::LoadHierarchyTree(engineObjScene, "EngineObjects/quad.obj", &engineObjScene.CreateHierarchyTree("ENG_QUAD"));
	EngineDataLoader::LoadHierarchyTree(engineObjScene, "EngineObjects/cube.obj", &engineObjScene.CreateHierarchyTree("ENG_CUBE"));
	EngineDataLoader::LoadHierarchyTree(engineObjScene, "EngineObjects/sphere.obj", &engineObjScene.CreateHierarchyTree("ENG_SPHERE"));
	EngineDataLoader::LoadHierarchyTree(engineObjScene, "EngineObjects/cone.obj", &engineObjScene.CreateHierarchyTree("ENG_CONE"));

	GameHandle->FindHierarchyTree("ENG_QUAD")->FindMesh(Mesh::MeshLoc::FromNodeName("Quad"))->Localization.HierarchyTreePath = "ENG_QUAD";
	GameHandle->FindHierarchyTree("ENG_QUAD")->FindMesh(Mesh::MeshLoc::FromNodeName("Quad"))->Localization.SpecificName = "Quad_NoShadow";
}

void RenderEngine::LoadInternalShaders()
{
	std::string settingsDefines = GameHandle->GetGameSettings()->Video.GetShaderDefines(Resolution);

	//load shadow shaders
	Shaders.push_back(ShaderLoader::LoadShaders("Depth", "Shaders/depth.vs", "Shaders/depth.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MVP});
	Shaders.back()->UniformBlockBinding("BoneMatrices", 10);
	Shaders.push_back(ShaderLoader::LoadShaders("DepthLinearize", "Shaders/depth_linearize.vs", "Shaders/depth_linearize.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MODEL, MatrixType::MVP});
	Shaders.back()->UniformBlockBinding("BoneMatrices", 10);

	Shaders.push_back(ShaderLoader::LoadShaders("ErToCubemap", "Shaders/LightProbe/erToCubemap.vs", "Shaders/LightProbe/erToCubemap.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::VP});

	Shaders.push_back(ShaderLoader::LoadShadersWithInclData("Cubemap", settingsDefines, "Shaders/cubemap.vs", "Shaders/cubemap.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::VP});

	Shaders.push_back(ShaderLoader::LoadShaders("CubemapToIrradiance", "Shaders/irradianceCubemap.vs", "Shaders/irradianceCubemap.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::VP});

	Shaders.push_back(ShaderLoader::LoadShaders("CubemapToPrefilter", "Shaders/prefilterCubemap.vs", "Shaders/prefilterCubemap.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::VP});

	Shaders.push_back(ShaderLoader::LoadShaders("BRDFLutGeneration", "Shaders/brdfLutGeneration.vs", "Shaders/brdfLutGeneration.fs"));

	AddShader(ShaderLoader::LoadShaders("Forward_NoLight", "Shaders/forward_nolight.vs", "Shaders/forward_nolight.fs"), true);
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MVP});
	Shaders.back()->AddTextureUnit(0, "albedo1");

	AddShader(ShaderLoader::LoadShaders("TextShader", "Shaders/text.vs", "Shaders/text.fs"), true);
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MVP});

	//load debug shaders
	Shaders.push_back(ShaderLoader::LoadShaders("Debug", "Shaders/debug.vs", "Shaders/debug.fs"));
	Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MVP});
}

void RenderEngine::Resize(glm::uvec2 resolution)
{
	Resolution = resolution;
	const GameSettings* settings = GameHandle->GetGameSettings();
}

const ShadingModel& RenderEngine::GetShadingModel()
{
	return GameHandle->GetGameSettings()->Video.Shading;
}

Mesh& RenderEngine::GetBasicShapeMesh(EngineBasicShape type)
{
	switch (type)
	{
	default:
		std::cout << "ERROR! Enum value " << static_cast<int>(type) << " does not represent a basic shape mesh.\n";
	case EngineBasicShape::QUAD:
		return *GameHandle->FindHierarchyTree("ENG_QUAD")->FindMesh(Mesh::MeshLoc::FromNodeName("Quad"));
	case EngineBasicShape::CUBE:
		return *GameHandle->FindHierarchyTree("ENG_CUBE")->FindMesh(Mesh::MeshLoc::FromNodeName("Cube"));
	case EngineBasicShape::SPHERE:
		return *GameHandle->FindHierarchyTree("ENG_SPHERE")->FindMesh(Mesh::MeshLoc::FromNodeName("Sphere"));
	case EngineBasicShape::CONE:
		return *GameHandle->FindHierarchyTree("ENG_CONE")->FindMesh(Mesh::MeshLoc::FromNodeName("Cone"));
	}
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

Material* RenderEngine::AddMaterial(std::shared_ptr<Material>  material)
{
	std::cout << "Adding material " << Materials.size() << " (" << material << ")";
	if (material)
		std::cout << " (" << material->GetLocalization().Name << ")\n";
	Materials.push_back(material);
	return Materials.back().get();
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

std::shared_ptr<Material> RenderEngine::FindMaterial(std::string name)
{
	if (name.empty())
		return nullptr;

	auto materialIt = std::find_if(Materials.begin(), Materials.end(), [name](const std::shared_ptr<Material>& material) {	return material->GetLocalization().Name == name; });

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

void RenderEngine::RenderShadowMaps(RenderToolboxCollection& tbCollection, GameSceneRenderData* sceneRenderData, std::vector <std::reference_wrapper<LightComponent>> lights)
{
	ShadowMappingToolbox* shadowsTb = tbCollection.GetTb<ShadowMappingToolbox>();
	bool dynamicShadowRender = tbCollection.GetSettings().ShadowLevel > SettingLevel::SETTING_MEDIUM;
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

	for (int i = 0; i < static_cast<int>(lights.size()); i++)
	{
		LightComponent& light = lights[i].get();
		if (!dynamicShadowRender && light.HasValidShadowMap())
			continue;

		if (light.GetType() == LightType::POINT)
		{
			time1 = (float)glfwGetTime();
			if (!bCubemapBound)
			{
				FindShader("DepthLinearize")->Use();
				bCubemapBound = true;
			}

			Transform lightWorld = light.GetTransform().GetWorldTransform();
			glm::vec3 lightPos = lightWorld.PositionRef;
			glm::mat4 viewTranslation = glm::translate(glm::mat4(1.0f), -lightPos);
			glm::mat4 projection = light.GetProjection();

			FindShader("DepthLinearize")->Uniform1f("far", light.GetFar());
			FindShader("DepthLinearize")->Uniform3fv("lightPos", lightPos);

			int cubemapFirst = light.GetShadowMapNr() * 6;
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

			glm::mat4 view = light.GetTransform().GetWorldTransform().GetViewMatrix();
			glm::mat4 projection = light.GetProjection();
			glm::mat4 VP = projection * view;

			timeSum += (float)glfwGetTime() - time1;
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowsTb->ShadowMapArray->GetID(), 0, light.GetShadowMapNr());
			glClear(GL_DEPTH_BUFFER_BIT);
			
			RenderInfo info(tbCollection, view, projection, VP, glm::vec3(0.0f), false, true, false);
			RenderRawScene(info, sceneRenderData, FindShader("Depth"));
		}

		if (!dynamicShadowRender)
			light.MarkValidShadowMap();
	}

	glActiveTexture(GL_TEXTURE0);
	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//std::cout << "Wyczyscilem sobie " << timeSum * 1000.0f << "ms.\n";
}

void RenderEngine::RenderVolume(const RenderInfo& info, EngineBasicShape shape, Shader& shader, const Transform* transform)
{
	if (shape == EngineBasicShape::QUAD)
		glDisable(GL_CULL_FACE);

	shader.Use();
	BoundMaterial = nullptr;
	BoundMesh = nullptr;
	RenderStaticMesh(info, GetBasicShapeMesh(shape), (transform) ? (*transform) : (Transform()), &shader);

	if (shape == EngineBasicShape::QUAD)
		glEnable(GL_CULL_FACE);
}

void RenderEngine::RenderVolume(const RenderInfo& info, RenderableVolume* volume, Shader* boundShader, bool shadedRender)
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

void RenderEngine::RenderVolumes(const RenderInfo& info, const GEE_FB::Framebuffer& framebuffer, const std::vector<std::unique_ptr<RenderableVolume>>& volumes, bool bIBLPass)
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
	glStencilMask(0xFF);
	glClear(GL_STENCIL_BUFFER_BIT);

	for (unsigned int i = 0; i < volumes.size(); i++)
	{
		//1st pass: stencil
		if (volumes[i]->GetShape() != EngineBasicShape::QUAD)		//if this is a quad everything will be in range; don't waste time
		{
			glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_INVERT, GL_KEEP);
			glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INVERT);
			glStencilMask(0xFF);
			glDisable(GL_CULL_FACE);
			glStencilFunc(GL_GREATER, 128, 0xFF);
			glDepthFunc(GL_LEQUAL);
			glDrawBuffer(GL_NONE);
			RenderVolume(info, volumes[i].get(), boundShader, false);
		}

		//2nd pass: lighting
		glStencilOp(GL_INVERT, GL_INCR, GL_INCR);
		if (volumes[i]->GetShape() == EngineBasicShape::QUAD)
			glStencilMask(0x00);
		if (bIBLPass)
			glStencilFunc(GL_EQUAL, 0, 0xFF);
		else
			glStencilFunc(((volumes[i]->GetShape() == EngineBasicShape::QUAD) ? (GL_ALWAYS) : (GL_GREATER)), 128, 0xFF);

		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_ALWAYS);

		framebuffer.SetDrawBuffers();

		RenderInfo xdCopy = info;
		xdCopy.view = glm::mat4(1.0f);
		xdCopy.projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
		xdCopy.CalculateVP();
		RenderVolume((volumes[i]->GetShape() == EngineBasicShape::QUAD) ? (xdCopy) : (info), volumes[i].get(), boundShader, true);
		//RenderVolume(info, volumes[i].get(), boundShader, true);
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
	settings.AAType = AA_NONE;
	settings.AALevel = SettingLevel::SETTING_ULTRA;
	settings.bBloom = true;
	settings.AmbientOcclusionSamples = 64;
	settings.POMLevel = SettingLevel::SETTING_ULTRA;
	settings.ShadowLevel = SettingLevel::SETTING_ULTRA;
	settings.Resolution = glm::uvec2(1024);
	settings.Shading = ShadingModel::SHADING_PBR_COOK_TORRANCE;

	std::cout << "Initting for " << sceneRenderData->LightProbes.size() << " probes.\n";
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
	depthBuffer->Dispose();

	std::cout << "Initting done.\n";
}

void RenderEngine::RenderRawScene(const RenderInfo& info, GameSceneRenderData* sceneRenderData, Shader* shader)
{
	if (!shader)
		shader = FindShader("Default");

	shader->Use();
	BoundMesh = nullptr;
	BoundMaterial = nullptr;

	for (unsigned int i = 0; i < sceneRenderData->Renderables.size(); i++)
	{
		sceneRenderData->Renderables[i].get().Render(info, shader);
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
		for (int i = 0; i < 2; i++)
		{
			GLenum error = glGetError();
			std::cout << "pre initting probes error: " << "(" << sceneIndex << ") " << int(error) << "\n";
			RenderLightProbes(ScenesRenderData[sceneIndex]);
		}
		ScenesRenderData[sceneIndex]->ProbesLoaded = true;
	}
}

void RenderEngine::PrepareScene(RenderToolboxCollection& tbCollection, GameSceneRenderData* sceneRenderData)
{
	if (sceneRenderData->ContainsLights() && (GameHandle->GetGameSettings()->Video.ShadowLevel > SettingLevel::SETTING_MEDIUM || sceneRenderData->HasLightWithoutShadowMap()))
		RenderShadowMaps(tbCollection, sceneRenderData, sceneRenderData->Lights);
}

#include <input/InputDevicesStateRetriever.h>
void RenderEngine::FullSceneRender(RenderInfo& info, GameSceneRenderData* sceneRenderData, GEE_FB::Framebuffer* framebuffer, Viewport viewport)
{
	const GameSettings::VideoSettings& settings = info.TbCollection.Settings;
	bool debugPhysics = GameHandle->GetInputRetriever().IsKeyPressed(Key::P);
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

		{
			Viewport onlySizeViewport(glm::uvec2(0), viewport.GetSize());
			GFramebuffer.Bind(true, &onlySizeViewport);
		}
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
		if (info.TbCollection.GetSettings().bBloom)
			glClearBufferfv(GL_COLOR, 1, glm::value_ptr(glm::vec3(0.0f)));

		for (int i = 0; i < 4; i++)
		{
			GFramebuffer.GetColorBuffer(i)->Bind(i);
		}

		if (SSAOtex)
			SSAOtex->Bind(4);
		std::vector<std::unique_ptr<RenderableVolume>> volumes;
		volumes.resize(sceneRenderData->Lights.size());
		std::transform(sceneRenderData->Lights.begin(), sceneRenderData->Lights.end(), volumes.begin(), [](const std::reference_wrapper<LightComponent>& light) {return std::make_unique<LightVolume>(LightVolume(light.get())); });
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
			//if (framebuffer)
				;// probeVolumes.pop_back();
			sceneRenderData->ProbeTexArrays->IrradianceMapArr.Bind(12);
			sceneRenderData->ProbeTexArrays->PrefilterMapArr.Bind(13);
			sceneRenderData->ProbeTexArrays->BRDFLut.Bind(14);
			RenderVolumes(info, MainFramebuffer, probeVolumes, sceneRenderData->ProbesLoaded == true);
		}
	}
	else
	{
		{
			Viewport onlySizeViewport(glm::uvec2(0), viewport.GetSize());
			MainFramebuffer.Bind(true, &viewport);
		}
		
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	////////////////////3.25 Render global cubemap
	info.MainPass = false;
	info.CareAboutShader = false;

	TestRenderCubemap(info, sceneRenderData);


	////////////////////3.5 Forward rendering pass
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	info.MainPass = true;
	info.CareAboutShader = true;

	for (unsigned int i = 0; i < ForwardShaders.size(); i++)
		RenderRawScene(info, sceneRenderData, ForwardShaders[i].get());

	FindShader("Forward_NoLight")->Use();
	FindShader("Forward_NoLight")->Uniform2fv("atlasData", glm::vec2(0.0f));
	FindShader("Forward_NoLight")->Uniform2fv("atlasTexOffset", glm::vec2(0.0f));


	info.MainPass = false;
	info.CareAboutShader = false;

	glDisable(GL_DEPTH_TEST);


	if (debugPhysics)
		GameHandle->GetPhysicsHandle()->DebugRender(*GameHandle->GetScene("GEE_Main")->GetPhysicsData(), *this, info);

	////////////////////4. Postprocessing pass (Blur + Tonemapping & Gamma Correction)
	Postprocessing.Render(info.TbCollection, target, &viewport, MainFramebuffer.GetColorBuffer(0).get(), (settings.bBloom) ? (MainFramebuffer.GetColorBuffer(1).get()) : (nullptr), MainFramebuffer.DepthBuffer.get(), (settings.IsVelocityBufferNeeded()) ? (MainFramebuffer.GetColorBuffer("velocityTex").get()) : (nullptr));
	      
	PreviousFrameView = currentFrameView;
}

void RenderEngine::PrepareFrame()
{
	BindSkeletonBatch(GameHandle->GetScene("GEE_Main")->GetRenderData(), static_cast<unsigned int>(0));

	//std::cout << "***Shaders deubg***\n";
	//for (auto& it : Shaders)
	//	std::cout << it->GetName() << '\n';

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
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


	RenderStaticMesh(info, GetBasicShapeMesh(EngineBasicShape::CUBE), Transform(), FindShader("Cubemap"));
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
	Mesh& cubeMesh = GetBasicShapeMesh(EngineBasicShape::CUBE);
	cubeMesh.Bind();
	BoundMesh = &cubeMesh;

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
		info.CareAboutShader = false;

		RenderStaticMesh(info, GetBasicShapeMesh(EngineBasicShape::CUBE), Transform(), &shader);
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

void RenderEngine::RenderText(const RenderInfo& infoPreConvert, const Font& font, std::string content, Transform t, glm::vec3 color, Shader* shader, bool convertFromPx, const std::pair<TextAlignment, TextAlignment>& alignment)
{
	if (infoPreConvert.CareAboutShader && shader && shader != FindShader("TextShader"))	///TODO: CHANGE IT SO TEXTS USE MATERIALS SO THEY CAN BE RENDERED USING DIFFERENT SHADERS!!!!
		return;

	if (!shader)
	{
		shader = FindShader("TextShader");
		shader->Use();
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);


	shader->Uniform3fv("color", color);
	font.GetBitmapsArray().Bind(0);

	glm::vec2 resolution(GameHandle->GetGameSettings()->WindowSize);// (GameHandle->GetGameSettings()->ViewportData.z, GameHandle->GetGameSettings()->ViewportData.w);s

	RenderInfo info = infoPreConvert;
	if (convertFromPx)
	{
		glm::mat4 pxConvertMatrix = glm::ortho(0.0f, resolution.x, 0.0f, resolution.y);
		const glm::mat4& proj = info.projection;
		glm::mat4 vp = info.CalculateVP() * pxConvertMatrix;
		info.projection = proj;
		info.VP = vp;

		info.UseMaterials = true;
		info.CareAboutShader = true;
		info.MainPass = false;
	}

	glm::vec2 halfExtent = static_cast<glm::vec2>(t.ScaleRef);
	t.Move(glm::vec3(0.0f, -t.ScaleRef.y * 2.0f + font.GetBaselineHeight() * halfExtent.y * 2.0f, 0.0f));	//align to bottom (-t.ScaleRef.y), move down to the bottom of it (-t.ScaleRef.y), and then move up to baseline height (-t.ScaleRef.y * 2.0f + font.GetBaselineHeight() * halfExtent.y * 2.0f)

	if (alignment.first != TextAlignment::LEFT)
	{
		float advancesSum = 0.0f;
		for (int i = 0; i < static_cast<int>(content.length()); i++)
			advancesSum += (font.GetCharacter(content[i]).Advance);

		t.Move(t.RotationRef * glm::vec3(-advancesSum * halfExtent.x * (static_cast<float>(alignment.first) - static_cast<float>(TextAlignment::LEFT)), 0.0f, 0.0f));
	}

	if (alignment.second != TextAlignment::BOTTOM)
		t.Move(t.RotationRef * glm::vec3(0.0f, -t.ScaleRef.y * (static_cast<float>(alignment.second) - static_cast<float>(TextAlignment::BOTTOM)), 0.0f));

	//t.Move(glm::vec3(0.0f, -64.0f, 0.0f));
	//t.Move(glm::vec3(0.0f, 11.0f, 0.0f) / scale);

	const glm::mat3& textRot = t.GetRotationMatrix();

	Material textMaterial("TextMaterial", 0.0f, FindShader("TextShader"));

	for (int i = 0; i < static_cast<int>(content.length()); i++)
	{
		shader->Uniform1i("glyphNr", content[i]);
		const Character& c = font.GetCharacter(content[i]);

		t.Move(textRot * glm::vec3(static_cast<glm::vec2>(c.Bearing) * halfExtent, 0.0f) * 2.0f);
		t.SetScale(halfExtent);
		//printVector(vp * t.GetWorldTransformMatrix() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), "Letter " + std::to_string(i));
		RenderStaticMesh(info, GetBasicShapeMesh(EngineBasicShape::QUAD), t, shader, nullptr, &textMaterial);
		t.Move(textRot * -glm::vec3(static_cast<glm::vec2>(c.Bearing) * halfExtent, 0.0f) * 2.0f);

		t.Move(textRot * glm::vec3(c.Advance * halfExtent.x, 0.0f, 0.0f) * 2.0f);
	}

	glDisable(GL_BLEND);
}

void RenderEngine::RenderStaticMesh(const RenderInfo& info, const MeshInstance& mesh, const Transform& transform, Shader* shader, glm::mat4* lastFrameMVP, Material* material, bool billboard)
{
	std::vector<std::unique_ptr<MeshInstance>> vec;
	vec.push_back(std::make_unique<MeshInstance>(MeshInstance(mesh)));
	RenderStaticMeshes(info, vec, transform, shader, lastFrameMVP, material, billboard);
}

void RenderEngine::RenderStaticMeshes(const RenderInfo& info, const std::vector<std::unique_ptr<MeshInstance>>& meshes, const Transform& transform, Shader* shader, glm::mat4* lastFrameMVP, Material* overrideMaterial, bool billboard)
{
	if (meshes.empty())
		return;

	std::unique_ptr<MaterialInstance> createdInstance = ((overrideMaterial) ? (std::make_unique<MaterialInstance>(MaterialInstance(*overrideMaterial))) : (nullptr));

	bool handledShader = false;
	for (int i = 0; i < static_cast<int>(meshes.size()); i++)
	{
		const MeshInstance& meshInst = *meshes[i];
		const Mesh& mesh = meshInst.GetMesh();
		MaterialInstance* materialInst = ((overrideMaterial) ?  (createdInstance.get()) : (meshInst.GetMaterialInst()));
		const Material* material = ((overrideMaterial) ? (overrideMaterial) : (meshInst.GetMaterialPtr()));

		if ((info.CareAboutShader && material && shader->GetName() != material->GetRenderShaderName()) || (info.OnlyShadowCasters && !mesh.CanCastShadow()) || (materialInst && !materialInst->ShouldBeDrawn()))
			continue;

		if (!handledShader)
		{
			handledShader = true;

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

		}

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

void RenderEngine::RenderSkeletalMeshes(const RenderInfo& info, const std::vector<std::unique_ptr<MeshInstance>>& meshes, const Transform& transform, Shader* shader, SkeletonInfo& skelInfo, Material* overrideMaterial)
{
	if (meshes.empty())
		return;

	std::unique_ptr<MaterialInstance> createdInstance = ((overrideMaterial) ? (std::make_unique<MaterialInstance>(MaterialInstance(*overrideMaterial))) : (nullptr));


	//TODO: Pass the bone matrices from the last frame to fix velocity buffer calculation

	bool handledShader = false;
	for (int i = 0; i < static_cast<int>(meshes.size()); i++)
	{
		const MeshInstance& meshInst = *meshes[i];
		const Mesh& mesh = meshInst.GetMesh();
		MaterialInstance* materialInst = ((overrideMaterial) ? (createdInstance.get()) : (meshInst.GetMaterialInst()));
		const Material* material = ((overrideMaterial) ? (overrideMaterial) : (meshInst.GetMaterialPtr()));

		if ((info.CareAboutShader && material && shader->GetName() != material->GetRenderShaderName()) || (info.OnlyShadowCasters && !mesh.CanCastShadow()) || !materialInst->ShouldBeDrawn())
			continue;

		if (!handledShader)
		{
			handledShader = true;

			shader->Uniform1i("boneIDOffset", skelInfo.GetBoneIDOffset());

			glm::mat4 modelMat = transform.GetWorldTransformMatrix();	//the ComponentTransform's world transform is cached
			bool bCalcVelocity = GameHandle->GetGameSettings()->Video.IsVelocityBufferNeeded() && info.MainPass;
			bool jitter = info.MainPass && GameHandle->GetGameSettings()->Video.IsTemporalReprojectionEnabled();

			glm::mat4 jitteredVP = (jitter) ? (Postprocessing.GetJitterMat(info.TbCollection.GetSettings()) * info.VP) : (info.VP);
			shader->BindMatrices(modelMat, &info.view, &info.projection, &jitteredVP);
		}
		if (skelInfo.GetBatchPtr() != BoundSkeletonBatch)
			BindSkeletonBatch(skelInfo.GetBatchPtr());

		shader->Uniform1i("boneIDOffset", skelInfo.GetBoneIDOffset());

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