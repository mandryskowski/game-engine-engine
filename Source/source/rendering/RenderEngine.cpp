#include <rendering/RenderEngine.h>
#include <rendering/Postprocess.h>
#include <assetload/FileLoader.h>
#include <scene/LightComponent.h>
#include <scene/ModelComponent.h>
#include <rendering/LightProbe.h>
#include <scene/LightProbeComponent.h>
#include <rendering/RenderableVolume.h>
#include <scene/hierarchy/HierarchyTree.h>
#include <UI/Font.h>
#include <random> //DO WYJEBANIA

#include <input/InputDevicesStateRetriever.h>

namespace GEE
{
	RenderEngine::RenderEngine(GameManager* gameHandle) :
		GameHandle(gameHandle),
		PreviousFrameView(Mat4f(1.0f)),
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
		EmptyTexture = Texture::Loader::FromBuffer2D(Vec2u(1, 1), Math::GetDataPtr(Vec3f(0.5f, 0.5f, 1.0f)), 3);
		EmptyTexture.SetMinFilter(Texture::MinTextureFilter::Nearest(), true, true);
		EmptyTexture.SetMagFilter(Texture::MagTextureFilter::Nearest(), true);
	}

	void RenderEngine::Init(Vec2u resolution)
	{
		Resize(resolution);

		LoadInternalShaders();
		GenerateEngineObjects();

		Postprocessing.Init(GameHandle, Resolution);

		CubemapData.DefaultV[0] = glm::lookAt(Vec3f(0.0f), Vec3f(1.0f, 0.0f, 0.0f), Vec3f(0.0f, -1.0f, 0.0f));
		CubemapData.DefaultV[1] = glm::lookAt(Vec3f(0.0f), Vec3f(-1.0f, 0.0f, 0.0f), Vec3f(0.0f, -1.0f, 0.0f));
		CubemapData.DefaultV[2] = glm::lookAt(Vec3f(0.0f), Vec3f(0.0f, 1.0f, 0.0f), Vec3f(0.0f, 0.0f, 1.0f));
		CubemapData.DefaultV[3] = glm::lookAt(Vec3f(0.0f), Vec3f(0.0f, -1.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f));
		CubemapData.DefaultV[4] = glm::lookAt(Vec3f(0.0f), Vec3f(0.0f, 0.0f, 1.0f), Vec3f(0.0f, -1.0f, 0.0f));
		CubemapData.DefaultV[5] = glm::lookAt(Vec3f(0.0f), Vec3f(0.0f, 0.0f, -1.0f), Vec3f(0.0f, -1.0f, 0.0f));

		Mat4f cubemapProj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 2.0f);

		for (int i = 0; i < 6; i++)
			CubemapData.DefaultVP[i] = cubemapProj * CubemapData.DefaultV[i];

		for (auto sceneRenderData : ScenesRenderData)
		{
			if (!(sceneRenderData->LightProbes.empty() && !sceneRenderData->bIsAnUIScene && GameHandle->GetGameSettings()->Video.Shading == ShadingModel::SHADING_PBR_COOK_TORRANCE))
				continue;

			LightProbeLoader::LoadLightProbeTextureArrays(sceneRenderData);
			sceneRenderData->ProbeTexArrays->IrradianceMapArr.Bind(12);
			sceneRenderData->ProbeTexArrays->PrefilterMapArr.Bind(13);
			sceneRenderData->ProbeTexArrays->BRDFLut.Bind(14);
			glActiveTexture(GL_TEXTURE0);
		}
	}

	void RenderEngine::AddSceneRenderDataPtr(GameSceneRenderData& sceneRenderData)
	{
		ScenesRenderData.push_back(&sceneRenderData);
		sceneRenderData.LightBlockBindingSlot = ScenesRenderData.size();
		if (GameHandle->HasStarted() && sceneRenderData.LightProbes.empty() && !sceneRenderData.bIsAnUIScene && GameHandle->GetGameSettings()->Video.Shading == ShadingModel::SHADING_PBR_COOK_TORRANCE)
		{
			LightProbeLoader::LoadLightProbeTextureArrays(&sceneRenderData);
			sceneRenderData.ProbeTexArrays->IrradianceMapArr.Bind(12);
			sceneRenderData.ProbeTexArrays->PrefilterMapArr.Bind(13);
			sceneRenderData.ProbeTexArrays->BRDFLut.Bind(14);
			glActiveTexture(GL_TEXTURE0);
		}
	}

	void RenderEngine::RemoveSceneRenderDataPtr(GameSceneRenderData& sceneRenderData)
	{
		ScenesRenderData.push_back(&sceneRenderData);
		ScenesRenderData.erase(std::remove_if(ScenesRenderData.begin(), ScenesRenderData.end(), [&sceneRenderData](GameSceneRenderData* sceneRenderDataVec) { return sceneRenderDataVec == &sceneRenderData; }), ScenesRenderData.end());
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

		//GameHandle->FindHierarchyTree("ENG_QUAD")->FindMesh("Quad")->Localization.HierarchyTreePath = "ENG_QUAD";
		GameHandle->FindHierarchyTree("ENG_QUAD")->FindMesh("Quad")->Localization.SpecificName = "Quad_NoShadow";
		//GameHandle->FindHierarchyTree("ENG_QUAD")->FindMesh("Quad")->Localization.HierarchyTreePath = "ENG_QUAD";
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

	void RenderEngine::Resize(Vec2u resolution)
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
			return *GameHandle->FindHierarchyTree("ENG_QUAD")->FindMesh("Quad");
		case EngineBasicShape::CUBE:
			return *GameHandle->FindHierarchyTree("ENG_CUBE")->FindMesh("Cube");
		case EngineBasicShape::SPHERE:
			return *GameHandle->FindHierarchyTree("ENG_SPHERE")->FindMesh("Sphere");
		case EngineBasicShape::CONE:
			return *GameHandle->FindHierarchyTree("ENG_CONE")->FindMesh("Cone");
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

	std::vector<Material*> RenderEngine::GetMaterials()
	{
		std::vector<Material*> materials;
		materials.reserve(Materials.size());
		for (auto& it : Materials)
			materials.push_back(it.get());

		return materials;
	}

	RenderToolboxCollection& RenderEngine::AddRenderTbCollection(const RenderToolboxCollection& tbCollection, bool setupToolboxesAccordingToSettings)
	{
		RenderTbCollections.push_back(std::make_unique<RenderToolboxCollection>(tbCollection));
		if (setupToolboxesAccordingToSettings)
			RenderTbCollections.back()->AddTbsRequiredBySettings();

		CurrentTbCollection = RenderTbCollections.back().get();

		return *RenderTbCollections.back();
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

	void RenderEngine::EraseRenderTbCollection(RenderToolboxCollection& tbCollection)
	{
		RenderTbCollections.erase(std::remove_if(RenderTbCollections.begin(), RenderTbCollections.end(), [&tbCollection](std::unique_ptr<RenderToolboxCollection>& tbColVec) {return tbColVec.get() == &tbCollection; }), RenderTbCollections.end());
	}

	void RenderEngine::EraseMaterial(Material& mat)
	{
		Materials.erase(std::remove_if(Materials.begin(), Materials.end(), [&mat](std::shared_ptr<Material>& matVec) {return matVec.get() == &mat; }), Materials.end());
	}

	std::shared_ptr<Material> RenderEngine::FindMaterial(std::string name)
	{
		if (name.empty())
			return nullptr;

		auto materialIt = std::find_if(Materials.begin(), Materials.end(), [name](const std::shared_ptr<Material>& material) {	return material->GetLocalization().Name == name; });

		if (materialIt != Materials.end())
			return *materialIt;

		std::cerr << "INFO: Can't find material " << name << "!\n";
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

			if (light.ShouldCullFrontsForShadowMap())
				glCullFace(GL_FRONT);
			else
				glCullFace(GL_BACK);

			if (light.GetType() == LightType::POINT)
			{
				time1 = (float)glfwGetTime();
				if (!bCubemapBound)
				{
					FindShader("DepthLinearize")->Use();
					bCubemapBound = true;
				}

				Transform lightWorld = light.GetTransform().GetWorldTransform();
				Vec3f lightPos = lightWorld.Pos();
				Mat4f viewTranslation = glm::translate(Mat4f(1.0f), -lightPos);
				Mat4f projection = light.GetProjection();

				FindShader("DepthLinearize")->Uniform1f("far", light.GetFar());
				FindShader("DepthLinearize")->Uniform3fv("lightPos", lightPos);

				int cubemapFirst = light.GetShadowMapNr() * 6;
				timeSum += (float)glfwGetTime() - time1;


				RenderInfo info(tbCollection, viewTranslation, projection, Mat4f(1.0f), Vec3f(0.0f), false, true, false);
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

				Mat4f view = light.GetTransform().GetWorldTransform().GetViewMatrix();
				Mat4f projection = light.GetProjection();
				Mat4f VP = projection * view;

				timeSum += (float)glfwGetTime() - time1;
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowsTb->ShadowMapArray->GetID(), 0, light.GetShadowMapNr());
				glClear(GL_DEPTH_BUFFER_BIT);

				RenderInfo info(tbCollection, view, projection, VP, Vec3f(0.0f), false, true, false);
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
		//glClearStencil(0);	//my AMD gpu forces me to call it...

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
			xdCopy.view = Mat4f(1.0f);
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
		settings.AAType = AA_NONE;	//Disable SMAA; it doesn't work on HDR colorbuffers
		settings.AALevel = SettingLevel::SETTING_ULTRA;
		settings.bBloom = false;
		settings.AmbientOcclusionSamples = 64;
		settings.POMLevel = SettingLevel::SETTING_ULTRA;
		settings.ShadowLevel = SettingLevel::SETTING_ULTRA;
		settings.Resolution = Vec2u(1024);
		settings.Shading = ShadingModel::SHADING_PBR_COOK_TORRANCE;
		settings.MonitorGamma = 1.0f;	//Keep radiance data in linear space
		settings.TMType = ToneMappingType::TM_NONE;	//Do not tonemap when rendering probes; we want to keep HDR data

		std::cout << "Initting for " << sceneRenderData->LightProbes.size() << " probes.\n";
		//Dispose();
		//Init(Vec2u(1024));

		RenderToolboxCollection probeRenderingCollection("LightProbeRendering", settings);
		CurrentTbCollection = &probeRenderingCollection;
		CurrentTbCollection->AddTbsRequiredBySettings();


		//RenderInfo info;

		//info.projection = &p;

		GEE_FB::Framebuffer framebuffer;
		std::shared_ptr<GEE_FB::FramebufferAttachment> depthBuffer = GEE_FB::reserveDepthBuffer(Vec2u(1024), GL_DEPTH_COMPONENT, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_TEXTURE_2D);
		framebuffer.Bind();
		framebuffer.SetAttachments(Vec2u(1024), GEE_FB::FramebufferAttachment(), nullptr);
		framebuffer.Bind(true);

		Shader* gShader = probeRenderingCollection.GetTb<DeferredShadingToolbox>()->GeometryShader;

		PrepareScene(probeRenderingCollection, sceneRenderData);

		for (int i = 0; i < static_cast<int>(sceneRenderData->LightProbes.size()); i++)
		{
			LightProbeComponent* probe = sceneRenderData->LightProbes[i];
			if (probe->GetShape() == EngineBasicShape::QUAD)
				continue;
			Vec3f camPos = probe->GetTransform().GetWorldTransform().Pos();
			Mat4f viewTranslation = glm::translate(Mat4f(1.0f), -camPos);
			Mat4f p = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

			RenderInfo info(probeRenderingCollection);
			info.view = viewTranslation;
			info.projection = p;
			info.camPos = camPos;
			gShader->Use();
			RenderCubemapFromScene(info, sceneRenderData, framebuffer, GEE_FB::FramebufferAttachment(sceneRenderData->LightProbes[i]->GetEnvironmentMap()), GL_COLOR_ATTACHMENT0, gShader, 0, true);

			sceneRenderData->LightProbes[i]->GetEnvironmentMap().Bind();
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			;
			LightProbeLoader::ConvoluteLightProbe(*sceneRenderData->LightProbes[i], &sceneRenderData->LightProbes[i]->GetEnvironmentMap());
			if (PrimitiveDebugger::bDebugProbeLoading)
				printVector(camPos, "Rendering probe");
		}

		//Dispose();

		//Init(Vec2u(GameHandle->GetGameSettings()->ViewportData.z, GameHandle->GetGameSettings()->ViewportData.w));
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
			if (info.OnlyShadowCasters && !sceneRenderData->Renderables[i]->CastsShadow())
				continue;
			else
				sceneRenderData->Renderables[i]->Render(info, shader);
	}

	void RenderEngine::RenderRawSceneUI(const RenderInfo& infoTemplate, GameSceneRenderData* sceneRenderData)
	{
		BoundMesh = nullptr;
		BoundMaterial = nullptr;
		RenderInfo info = infoTemplate;
		const float maxDepth = 2550.0f;

		//std::stable_sort(sceneRenderData->Renderables.begin(), sceneRenderData->Renderables.end(), [](Renderable* lhs, Renderable* rhs) { return lhs->GetUIDepth() < rhs->GetUIDepth(); });
		sceneRenderData->AssertThatUIRenderablesAreSorted();

		glDepthFunc(GL_LEQUAL);
		for (auto& shader : ForwardShaders)
		{
			shader->Use();
			info.view = infoTemplate.view;
			//Render from back to front (elements inserted last in the container probably have the biggest depth; early depth testing should work better using this method
			for (auto it = sceneRenderData->Renderables.rbegin(); it != sceneRenderData->Renderables.rend(); it++)
			{
				//float uiDepth = static_cast<float>(it->get().GetUIDepth());
				//info.view = (uiDepth !=	0.0f) ? (glm::translate(maxDepthMat, Vec3f(0.0f, 0.0f, -uiDepth))) : (maxDepthMat);
				info.view = glm::translate(info.view, Vec3f(0.0f, 0.0f, 1.0f));
				info.CalculateVP();
				(*it)->Render(info, shader.get());
			}
		}
	}

	void RenderEngine::RenderBoundInDebug(RenderInfo& info, GLenum mode, GLint first, GLint count, Vec3f color)
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


	void RenderEngine::FullSceneRender(RenderInfo& info, GameSceneRenderData* sceneRenderData, GEE_FB::Framebuffer* framebuffer, Viewport viewport, bool clearMainFB, bool modifyForwardsDepthForUI)
	{
		const GameSettings::VideoSettings& settings = info.TbCollection.Settings;
		bool debugPhysics = GameHandle->GetInputRetriever().IsKeyPressed(Key::F2);
		bool debugComponents = true;
		bool useLightingAlgorithms = sceneRenderData->ContainsLights() || sceneRenderData->ContainsLightProbes();
		Mat4f currentFrameView = info.view;
		info.previousFrameView = PreviousFrameView;

		const GEE_FB::Framebuffer& target = ((framebuffer) ? (*framebuffer) : (GEE_FB::getDefaultFramebuffer(GameHandle->GetGameSettings()->WindowSize)));
		if (viewport.GetSize() == Vec2u(0))
			viewport = Viewport(Vec2u(0), target.GetSize());

		//sceneRenderData->LightsBuffer.SubData4fv(info.camPos, 16); TO BYLO TU...

		MainFramebufferToolbox* mainTb = info.TbCollection.GetTb<MainFramebufferToolbox>();
		GEE_FB::Framebuffer& MainFramebuffer = *mainTb->MainFb;

		glDepthFunc(GL_LEQUAL);

		////////////////////2. Geometry pass
		if (useLightingAlgorithms)
		{
			if (sceneRenderData->LightsBuffer.HasBeenGenerated())
				sceneRenderData->LightsBuffer.SubData4fv(info.camPos, 16); //TODO: BUGCHECK A TERAZ JEST TU.
			DeferredShadingToolbox* deferredTb = info.TbCollection.GetTb<DeferredShadingToolbox>();
			GEE_FB::Framebuffer& GFramebuffer = *deferredTb->GFb;

			{
				Viewport onlySizeViewport(Vec2u(0), viewport.GetSize());
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

			glClearBufferfv(GL_COLOR, 0, Math::GetDataPtr(Vec3f(0.0f, 0.0f, 0.0f)));
			if (info.TbCollection.GetSettings().bBloom)
				glClearBufferfv(GL_COLOR, 1, Math::GetDataPtr(Vec3f(0.0f)));

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
				LightProbeComponent* probe = sceneRenderData->LightProbes[i];
				Shader* shader = probe->GetRenderShader(info.TbCollection);
				shader->Uniform1f("lightProbes[" + std::to_string(i) + "].intensity", probe->GetProbeIntensity());

				if (probe->GetShape() == EngineBasicShape::QUAD)
					continue;

				shader->Uniform3fv("lightProbes[" + std::to_string(i) + "].position", probe->GetTransform().GetWorldTransform().Pos());
			}

			std::vector<std::unique_ptr<RenderableVolume>> probeVolumes;
			probeVolumes.resize(sceneRenderData->LightProbes.size());
			std::transform(sceneRenderData->LightProbes.begin(), sceneRenderData->LightProbes.end(), probeVolumes.begin(), [](LightProbeComponent* probe) {return std::make_unique<LightProbeVolume>(LightProbeVolume(*probe)); });
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
				Viewport onlySizeViewport(Vec2u(0), viewport.GetSize());
				MainFramebuffer.Bind(true, &viewport);
			}

			if (clearMainFB)
			{
				glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			}
		}

		////////////////////3.25 Render global cubemap
		info.MainPass = false;
		info.CareAboutShader = false;

		if (sceneRenderData->ContainsLightProbes())
			TestRenderCubemap(info, sceneRenderData);


		////////////////////3.5 Forward rendering pass
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		info.MainPass = true;
		info.CareAboutShader = true;

		if (modifyForwardsDepthForUI)
			RenderRawSceneUI(info, sceneRenderData);
		else
			for (unsigned int i = 0; i < ForwardShaders.size(); i++)
				RenderRawScene(info, sceneRenderData, ForwardShaders[i].get());

		FindShader("Forward_NoLight")->Use();
		FindShader("Forward_NoLight")->Uniform2fv("atlasData", Vec2f(0.0f));
		FindShader("Forward_NoLight")->Uniform2fv("atlasTexOffset", Vec2f(0.0f));


		info.MainPass = false;
		info.CareAboutShader = false;

		glDisable(GL_DEPTH_TEST);


		if (debugPhysics && GameHandle->GetMainScene()->GetRenderData() == sceneRenderData)
			GameHandle->GetPhysicsHandle()->DebugRender(*GameHandle->GetMainScene()->GetPhysicsData(), *this, info);

		////////////////////4. Postprocessing pass (Blur + Tonemapping & Gamma Correction)
		Postprocessing.Render(info.TbCollection, target, &viewport, MainFramebuffer.GetColorBuffer(0).get(), (settings.bBloom) ? (MainFramebuffer.GetColorBuffer(1).get()) : (nullptr), MainFramebuffer.DepthBuffer.get(), (settings.IsVelocityBufferNeeded()) ? (MainFramebuffer.GetColorBuffer("velocityTex").get()) : (nullptr));

		PreviousFrameView = currentFrameView;
	}

	void RenderEngine::PrepareFrame()
	{
		if (GameHandle->GetMainScene())
			BindSkeletonBatch(GameHandle->GetMainScene()->GetRenderData(), static_cast<unsigned int>(0));

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
		info.view = Mat4f(1.0f);
		info.projection = Mat4f(1.0f);
		info.VP = Mat4f(1.0f);

		glViewport(0, 0, 800, 600);
		info.MainPass = true;
		info.CareAboutShader = true;
		RenderRawScene(info, ScenesRenderData[1], FindShader("Forward_NoLight"));
		RenderRawScene(info, ScenesRenderData[1], FindShader("TextShader"));

		PostFrame();
	}

	void RenderEngine::TestRenderCubemap(RenderInfo& info, GameSceneRenderData* sceneRenderData)
	{
		sceneRenderData->LightProbes.back()->GetEnvironmentMap().Bind(0);
		//sceneRenderData->ProbeTexArrays->PrefilterMapArr.Bind(0);
		//LightProbes[1]->EnvironmentMap.Bind(0);
		FindShader("Cubemap")->Use();
		FindShader("Cubemap")->Uniform1f("mipLevel", (float)std::fmod(glfwGetTime(), 4.0));

		bool bCalcVelocity = GameHandle->GetGameSettings()->Video.IsVelocityBufferNeeded() && info.MainPass;
		bool jitter = info.MainPass && GameHandle->GetGameSettings()->Video.IsTemporalReprojectionEnabled();

		Mat4f jitteredVP = (jitter) ? (Postprocessing.GetJitterMat(info.TbCollection.GetSettings(), Postprocessing.GetFrameIndex()) * info.VP) : (info.VP);

		if (bCalcVelocity)
		{
			FindShader("Cubemap")->UniformMatrix4fv("prevVP", info.projection * info.previousFrameView);
			FindShader("Cubemap")->UniformMatrix4fv("flickerMat", Postprocessing.GetJitterMat(info.TbCollection.GetSettings(), Postprocessing.GetFrameIndex()));
			FindShader("Cubemap")->UniformMatrix4fv("prevFlickerMat", Postprocessing.GetJitterMat(info.TbCollection.GetSettings(), (Postprocessing.GetFrameIndex() + 1) % 2));
		}
		else
		{
			FindShader("Cubemap")->UniformMatrix4fv("prevVP", Mat4f(1.0f));
			FindShader("Cubemap")->UniformMatrix4fv("flickerMat", Mat4f(1.0f));
			FindShader("Cubemap")->UniformMatrix4fv("prevFlickerMat", Mat4f(1.0f));
		}


		RenderStaticMesh(info, GetBasicShapeMesh(EngineBasicShape::CUBE), Transform(), FindShader("Cubemap"));
	}

	void RenderEngine::RenderCubemapFromTexture(Texture targetTex, Texture tex, Vec2u size, Shader& shader, int* layer, int mipLevel)
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
			default:						std::cerr << "ERROR: Can't render a 2D texture to cubemap texture, when the (cubemap) texture's type is " << targetTex.GetType() << ".\n"; return;
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

		Mat4f viewTranslation = info.view;

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

	void RenderEngine::RenderText(const RenderInfo& infoPreConvert, const Font& font, std::string content, Transform t, Vec3f color, Shader* shader, bool convertFromPx, const std::pair<TextAlignment, TextAlignment>& alignment)
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

		Vec2f resolution(GameHandle->GetGameSettings()->WindowSize);// (GameHandle->GetGameSettings()->ViewportData.z, GameHandle->GetGameSettings()->ViewportData.w);s

		RenderInfo info = infoPreConvert;
		if (convertFromPx)
		{
			Mat4f pxConvertMatrix = glm::ortho(0.0f, resolution.x, 0.0f, resolution.y);
			const Mat4f& proj = info.projection;
			Mat4f vp = info.CalculateVP() * pxConvertMatrix;
			info.projection = proj;
			info.VP = vp;

			info.UseMaterials = true;
			info.CareAboutShader = true;
			info.MainPass = false;
		}

		Vec2f halfExtent = static_cast<Vec2f>(t.Scale());
		//	halfExtent.y *= 1.0f - font.GetBaselineHeight() / 4.0f;	//Account for baseline height (we move the character quads by the height in the next line, so we have to shrink them a bit so that the text fits within halfExtent)
		t.Move(Vec3f(0.0f, -t.Scale().y * 2.0f + font.GetBaselineHeight() * t.Scale().y * 2.0f, 0.0f));	//align to bottom (-t.ScaleRef.y), move down to the bottom of it (-t.ScaleRef.y), and then move up to baseline height (-t.ScaleRef.y * 2.0f + font.GetBaselineHeight() * halfExtent.y * 2.0f)

		if (alignment.first != TextAlignment::LEFT)
		{
			float advancesSum = 0.0f;
			for (int i = 0; i < static_cast<int>(content.length()); i++)
				advancesSum += (font.GetCharacter(content[i]).Advance);

			t.Move(t.Rot() * Vec3f(-advancesSum * halfExtent.x * (static_cast<float>(alignment.first) - static_cast<float>(TextAlignment::LEFT)), 0.0f, 0.0f));
		}

		if (alignment.second != TextAlignment::BOTTOM)
			t.Move(t.Rot() * Vec3f(0.0f, -t.Scale().y * (static_cast<float>(alignment.second) - static_cast<float>(TextAlignment::BOTTOM)), 0.0f));

		//t.Move(Vec3f(0.0f, -64.0f, 0.0f));
		//t.Move(Vec3f(0.0f, 11.0f, 0.0f) / scale);

		const Mat3f& textRot = t.GetRotationMatrix();

		Material textMaterial("TextMaterial", 0.0f, FindShader("TextShader"));

		Transform initialT = t;

		for (int i = 0; i < static_cast<int>(content.length()); i++)
		{
			if (content[i] == '\n')
			{
				initialT.Move(Vec2f(0.0f, -halfExtent.y * 2.0f));
				t = initialT;
				continue;
			}
			shader->Uniform1i("glyphNr", content[i]);
			const Character& c = font.GetCharacter(content[i]);

			t.Move(textRot * Vec3f(static_cast<Vec2f>(c.Bearing) * halfExtent, 0.0f) * 2.0f);
			t.SetScale(halfExtent);
			//printVector(vp * t.GetWorldTransformMatrix() * Vec4f(0.0f, 0.0f, 0.0f, 1.0f), "Letter " + std::to_string(i));
			RenderStaticMesh(info, GetBasicShapeMesh(EngineBasicShape::QUAD), t, shader, nullptr, &textMaterial);
			t.Move(textRot * -Vec3f(static_cast<Vec2f>(c.Bearing) * halfExtent, 0.0f) * 2.0f);

			t.Move(textRot * Vec3f(c.Advance * halfExtent.x, 0.0f, 0.0f) * 2.0f);
		}

		glDisable(GL_BLEND);
	}

	void RenderEngine::RenderStaticMesh(const RenderInfo& info, const MeshInstance& mesh, const Transform& transform, Shader* shader, Mat4f* lastFrameMVP, Material* material, bool billboard)
	{
		std::vector<std::unique_ptr<MeshInstance>> vec;
		vec.push_back(std::make_unique<MeshInstance>(MeshInstance(mesh)));
		RenderStaticMeshes(info, vec, transform, shader, lastFrameMVP, material, billboard);
	}

	void RenderEngine::RenderStaticMeshes(const RenderInfo& info, const std::vector<std::unique_ptr<MeshInstance>>& meshes, const Transform& transform, Shader* shader, Mat4f* lastFrameMVP, Material* overrideMaterial, bool billboard)
	{
		if (meshes.empty())
			return;

		std::unique_ptr<MaterialInstance> createdInstance = ((overrideMaterial) ? (std::make_unique<MaterialInstance>(MaterialInstance(*overrideMaterial))) : (nullptr));

		bool handledShader = false;
		for (int i = 0; i < static_cast<int>(meshes.size()); i++)
		{
			const MeshInstance& meshInst = *meshes[i];
			const Mesh& mesh = meshInst.GetMesh();
			MaterialInstance* materialInst = ((overrideMaterial) ? (createdInstance.get()) : (meshInst.GetMaterialInst()));
			const Material* material = ((overrideMaterial) ? (overrideMaterial) : (meshInst.GetMaterialPtr()));

			if ((info.CareAboutShader && material && shader->GetName() != material->GetRenderShaderName()) || (info.OnlyShadowCasters && !mesh.CanCastShadow()) || (materialInst && !materialInst->ShouldBeDrawn()))
				continue;

			if (!handledShader)
			{
				handledShader = true;

				Mat4f modelMat = transform.GetWorldTransformMatrix();	//the ComponentTransform's world transform is cached
				if (billboard)
					modelMat = modelMat * Mat4f(glm::inverse(transform.GetWorldTransform().GetRotationMatrix()) * glm::inverse(Mat3f(info.view)));

				bool bCalcVelocity = GameHandle->GetGameSettings()->Video.IsVelocityBufferNeeded() && info.MainPass;
				bool jitter = info.MainPass && GameHandle->GetGameSettings()->Video.IsTemporalReprojectionEnabled();

				Mat4f jitteredVP = (jitter) ? (Postprocessing.GetJitterMat(info.TbCollection.GetSettings(), (Postprocessing.GetFrameIndex() + 1) % 2) * info.VP) : (info.VP);
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
					materialInst->UpdateWholeUBOData(shader, EmptyTexture);
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

				Mat4f modelMat = transform.GetWorldTransformMatrix();	//the ComponentTransform's world transform is cached
				bool bCalcVelocity = GameHandle->GetGameSettings()->Video.IsVelocityBufferNeeded() && info.MainPass;
				bool jitter = info.MainPass && GameHandle->GetGameSettings()->Video.IsTemporalReprojectionEnabled();

				Mat4f jitteredVP = (jitter) ? (Postprocessing.GetJitterMat(info.TbCollection.GetSettings()) * info.VP) : (info.VP);
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
					materialInst->UpdateWholeUBOData(shader, EmptyTexture);
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

		for (auto i = Shaders.begin(); i != Shaders.end(); i++)
			if (std::find(ForwardShaders.begin(), ForwardShaders.end(), *i) == ForwardShaders.end())
			{
				(*i)->Dispose();
				Shaders.erase(i);
				i--;
			}

		LightShaders.clear();
	}

	Vec3f GetCubemapFront(unsigned int index)
	{
		GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + (index % 6);
		Vec3f front(0.0f);

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

	Vec3f GetCubemapUp(unsigned int index)
	{
		GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + (index % 6);

		if (face == GL_TEXTURE_CUBE_MAP_POSITIVE_Y)
			return Vec3f(0.0f, 0.0f, 1.0f);
		else if (face == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y)
			return Vec3f(0.0f, 0.0f, -1.0f);
		return Vec3f(0.0f, -1.0f, 0.0f);
	}
}