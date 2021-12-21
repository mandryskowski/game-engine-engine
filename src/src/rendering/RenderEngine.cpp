#include <rendering/RenderEngine.h>
#include <game/GameScene.h>
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
#include <scene/TextComponent.h>

#include <input/InputDevicesStateRetriever.h>

#include <rendering/OutlineRenderer.h>

namespace GEE
{
	RenderEngine::RenderEngine(GameManager* gameHandle) :
		GameHandle(gameHandle),
		Resolution(),
		PreviousFrameView(Mat4f(1.0f)),
		CubemapData(),
		BoundSkeletonBatch(nullptr),
		CurrentTbCollection(nullptr),
		SimpleShader(nullptr)
	{
		//configure some openGL settings 
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		glFrontFace(GL_CCW);
		glCullFace(GL_BACK);

		//generate engine's empty texture
		EmptyTexture = Texture::Loader<>::FromBuffer2D(Vec2u(1, 1), Math::GetDataPtr(Vec3f(0.5f, 0.5f, 1.0f)),
		                                               Texture::Format::RGBA(), 3);
		EmptyTexture.SetMinFilter(Texture::MinFilter::Nearest(), true, true);
		EmptyTexture.SetMagFilter(Texture::MagFilter::Nearest(), true);
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
			if (!(sceneRenderData->LightProbes.empty() && !sceneRenderData->bIsAnUIScene && GameHandle->GetGameSettings()->Video.Shading == ShadingAlgorithm::SHADING_PBR_COOK_TORRANCE))
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
		if (GameHandle->HasStarted() && sceneRenderData.LightProbes.empty() && !sceneRenderData.bIsAnUIScene && GameHandle->GetGameSettings()->Video.Shading == ShadingAlgorithm::SHADING_PBR_COOK_TORRANCE)
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

	SharedPtr<Shader> RenderEngine::AddNonCustomShader(SharedPtr<Shader> shader)
	{
		Shaders.push_back(shader);
		return Shaders.back();
	}

	void RenderEngine::GenerateEngineObjects()
	{
		//Add the engine objects to the tree collection. This way models loaded from the level file can simply use the internal engine meshes for stuff like particles
		//Preferably, the engine objects should be added to the tree collection first, so searching for them takes less time as they'll possibly be used often.

		GameScene& engineObjScene = GameHandle->CreateScene("GEE_Engine_Objects");

		EngineDataLoader::LoadHierarchyTree(engineObjScene, "Assets/EngineObjects/quad.obj", &engineObjScene.CreateHierarchyTree("ENG_QUAD"));
		EngineDataLoader::LoadHierarchyTree(engineObjScene, "Assets/EngineObjects/cube.obj", &engineObjScene.CreateHierarchyTree("ENG_CUBE"));
		EngineDataLoader::LoadHierarchyTree(engineObjScene, "Assets/EngineObjects/sphere.obj", &engineObjScene.CreateHierarchyTree("ENG_SPHERE"));
		EngineDataLoader::LoadHierarchyTree(engineObjScene, "Assets/EngineObjects/cone.obj", &engineObjScene.CreateHierarchyTree("ENG_CONE"));

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

		SimpleShader = AddNonCustomShader(ShaderLoader::LoadShaders("Forward_NoLight", "Shaders/forward_nolight.vs", "Shaders/forward_nolight.fs")).get();
		Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MVP});
		Shaders.back()->AddTextureUnit(0, "albedo1");
		Shaders.back()->SetOnMaterialWholeDataUpdateFunc([](Shader& shader, const Material& mat) {
			MaterialUtil::DisableColorIfAlbedoTextureDetected(shader, mat);
		});

		AddShader(ShaderLoader::LoadShaders("TextShader", "Shaders/text.vs", "Shaders/text.fs"));
		Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MVP});

		//load debug shaders
		AddNonCustomShader(ShaderLoader::LoadShaders("Debug", "Shaders/debug.vs", "Shaders/debug.fs"))->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MVP});
	}

	void RenderEngine::Resize(Vec2u resolution)
	{
		Resolution = resolution;
		const GameSettings* settings = GameHandle->GetGameSettings();
	}

	const ShadingAlgorithm& RenderEngine::GetShadingModel()
	{
		return GameHandle->GetGameSettings()->Video.Shading;
	}

	Mesh& RenderEngine::GetBasicShapeMesh(EngineBasicShape type)
	{
		switch (type)
		{
		case EngineBasicShape::QUAD:
			return *GameHandle->FindHierarchyTree("ENG_QUAD")->FindMesh("Quad");
		case EngineBasicShape::CUBE:
			return *GameHandle->FindHierarchyTree("ENG_CUBE")->FindMesh("Cube");
		case EngineBasicShape::SPHERE:
			return *GameHandle->FindHierarchyTree("ENG_SPHERE")->FindMesh("Sphere");
		case EngineBasicShape::CONE:
			return *GameHandle->FindHierarchyTree("ENG_CONE")->FindMesh("Cone");
		default:
			std::cout << "ERROR! Enum value " << static_cast<int>(type) << " does not represent a basic shape mesh.\n";
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

	Texture RenderEngine::GetEmptyTexture()
	{
		return EmptyTexture;
	}

	SkeletonBatch* RenderEngine::GetBoundSkeletonBatch()
	{
		return BoundSkeletonBatch;
	}

	Shader* RenderEngine::GetSimpleShader()
	{
		return SimpleShader;
	}

	std::vector<Shader*> RenderEngine::GetCustomShaders()
	{
		std::vector<Shader*> shaders;
		shaders.reserve(CustomShaders.size());
		for (auto& it : CustomShaders)
			shaders.push_back(it.get());

		return shaders;
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
		RenderTbCollections.push_back(MakeUnique<RenderToolboxCollection>(tbCollection));
		if (setupToolboxesAccordingToSettings)
			RenderTbCollections.back()->AddTbsRequiredBySettings();

		CurrentTbCollection = RenderTbCollections.back().get();

		return *RenderTbCollections.back();
	}

	Material* RenderEngine::AddMaterial(SharedPtr<Material>  material)
	{
		std::cout << "Adding material " << Materials.size() << " (" << material << ")";
		if (material)
			std::cout << " (" << material->GetLocalization().Name << ")\n";
		Materials.push_back(material);
		return Materials.back().get();
	}

	SharedPtr<Shader> RenderEngine::AddShader(SharedPtr<Shader> shader)
	{
		Shaders.push_back(shader);
		CustomShaders.push_back(shader);

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
		RenderTbCollections.erase(std::remove_if(RenderTbCollections.begin(), RenderTbCollections.end(), [&tbCollection](UniquePtr<RenderToolboxCollection>& tbColVec) {return tbColVec.get() == &tbCollection; }), RenderTbCollections.end());
	}

	void RenderEngine::EraseMaterial(Material& mat)
	{
		Materials.erase(std::remove_if(Materials.begin(), Materials.end(), [&mat](SharedPtr<Material>& matVec) {return matVec.get() == &mat; }), Materials.end());
	}

	SharedPtr<Material> RenderEngine::FindMaterial(std::string name)
	{
		if (name.empty())
			return nullptr;

		if (auto materialIt = std::find_if(Materials.begin(), Materials.end(), [name](const SharedPtr<Material>& material) {	return material->GetLocalization().Name == name; }); materialIt != Materials.end())
			return *materialIt;

		std::cerr << "INFO: Can't find material " << name << "!\n";
		return nullptr;
	}

	Shader* RenderEngine::FindShader(std::string name)
	{

		if (auto shaderIt = std::find_if(Shaders.begin(), Shaders.end(), [name](SharedPtr<Shader> shader) { return shader->GetName() == name; }); shaderIt != Shaders.end())
			return shaderIt->get();

		std::cerr << "ERROR! Can't find a shader named " << name << "!\n";
		return nullptr;
	}

	void RenderEngine::PrepareScene(RenderingContextID contextID, RenderToolboxCollection& tbCollection, GameSceneRenderData* sceneRenderData)
	{
		if (sceneRenderData->ContainsLights() && (GameHandle->GetGameSettings()->Video.ShadowLevel > SettingLevel::SETTING_MEDIUM || sceneRenderData->HasLightWithoutShadowMap()))
			ShadowMapRenderer(*this).ShadowMaps(contextID, tbCollection, *sceneRenderData, sceneRenderData->Lights);
	}


	void RenderEngine::PrepareFrame()
	{
		if (GameHandle->GetMainScene())
		{
			for (auto& it : GameHandle->GetMainScene()->GetRenderData()->SkeletonBatches)
				it->SwapBoneUBOs();

			BindSkeletonBatch(GameHandle->GetMainScene()->GetRenderData(), static_cast<unsigned int>(0));
		}

		//std::cout << "***Shaders deubg***\n";
		//for (auto& it : Shaders)
		//	std::cout << it->GetName() << '\n';

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDrawBuffer(GL_BACK);
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void RenderEngine::RenderText(const SceneMatrixInfo& infoPreConvert, const Font& font, std::string content, Transform t, Vec3f color, Shader* shader, bool convertFromPx, const std::pair<TextAlignment, TextAlignment>& alignment)
	{
		if (!Material::ShaderInfo(MaterialShaderHint::None).MatchesRequiredInfo(infoPreConvert.GetRequiredShaderInfo()) && shader && shader != FindShader("TextShader"))	///TODO: CHANGE IT SO TEXTS USE MATERIALS SO THEY CAN BE RENDERED USING DIFFERENT SHADERS!!!!
			return;

		if (!shader)
		{
			shader = FindShader("TextShader");
			shader->Use();
		}

		if (infoPreConvert.GetAllowBlending())
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBlendEquation(GL_FUNC_ADD);
		}


		if (infoPreConvert.GetUseMaterials())	// TODO: Przetestuj - sprawdz czy nic nie zniknelo i czy moge usunac  info.UseMaterials = true;  z linijki 969
			shader->Uniform4fv("material.color", Vec4f(color, 1.0f));
		font.GetBitmapsArray().Bind(0);

		Vec2f resolution(infoPreConvert.GetTbCollection().GetSettings().Resolution);

		SceneMatrixInfo info = infoPreConvert;
		if (convertFromPx)
		{
			Mat4f pxConvertMatrix = glm::ortho(0.0f, resolution.x, 0.0f, resolution.y);
			const Mat4f& proj = info.GetProjection();
			info.CalculateVP();
			Mat4f vp = info.GetVP() * pxConvertMatrix;
			info.SetVPArtificially(vp);
		}

		Vec2f halfExtent = t.GetScale();
		float textHeight = t.GetScale().y;// textUtil::GetTextHeight(content, t.GetScale(), font);
		//	halfExtent.y *= 1.0f - font.GetBaselineHeight() / 4.0f;	//Account for baseline height (we move the character quads by the height in the next line, so we have to shrink them a bit so that the text fits within halfExtent)
		t.Move(Vec3f(0.0f, -t.GetScale().y * 2.0f + font.GetBaselineHeight() * t.GetScale().y * 2.0f, 0.0f));	//align to bottom (-t.ScaleRef.y), move down to the bottom of it (-t.ScaleRef.y), and then move up to baseline height (-t.ScaleRef.y * 2.0f + font.GetBaselineHeight() * halfExtent.y * 2.0f)

		// Align horizontally
		if (alignment.first != TextAlignment::LEFT)
		{
			float textLength = textUtil::GetTextLength(content, t.GetScale(), font);

			t.Move(t.GetRot() * Vec3f(-textLength / 2.0f * (static_cast<float>(alignment.first) - static_cast<float>(TextAlignment::LEFT)), 0.0f, 0.0f));
		}

		// Align vertically
		if (alignment.second != TextAlignment::BOTTOM)
			t.Move(t.GetRot() * Vec3f(0.0f, -textHeight * (static_cast<float>(alignment.second) - static_cast<float>(TextAlignment::BOTTOM)), 0.0f));

		// Account for multiple lines if the text is not aligned to TOP. Each line (excluding the first one, that's why we cancel out the first line) moves the text up by its scale if it is aligned to BOTTOM and by 1/2 of its scale if it is aligned to CENTER.
		if (auto firstLineBreak = content.find_first_of('\n'); alignment.second != TextAlignment::TOP && firstLineBreak != std::string::npos)
			;// t.Move(t.GetRot() * Vec3f(0.0f, (textHeight - textUtil::GetTextHeight(content.substr(0, firstLineBreak), t.GetScale(), font)) / 2.0f * (static_cast<float>(TextAlignment::TOP) - static_cast<float>(alignment.second)), 0.0f));

		//t.Move(Vec3f(0.0f, -64.0f, 0.0f));
		//t.Move(Vec3f(0.0f, 11.0f, 0.0f) / scale);

		const Mat3f& textRot = t.GetRotationMatrix();

		Material textMaterial("TextMaterial", *FindShader("TextShader"));

		Transform initialT = t;

		info.SetUseMaterials(false);	// do not bind materials before rendering quads

		for (int i = 0; i < static_cast<int>(content.length()); i++)
		{
			if (content[i] == '\n')
			{
				initialT.Move(Vec2f(0.0f, -halfExtent.y * 2.0f));
				t = initialT;
				continue;
			}
			shader->Uniform1i("material.glyphNr", content[i]);
			const Character& c = font.GetCharacter(content[i]);

			t.Move(textRot * Vec3f(c.Bearing * halfExtent, 0.0f) * 2.0f);
			t.SetScale(halfExtent);
			//printVector(vp * t.GetWorldTransformMatrix() * Vec4f(0.0f, 0.0f, 0.0f, 1.0f), "Letter " + std::to_string(i));

			Renderer(*this).StaticMeshInstances(info, { MeshInstance(GetBasicShapeMesh(EngineBasicShape::QUAD), &textMaterial) }, t, *shader);
			t.Move(textRot * -Vec3f(c.Bearing * halfExtent, 0.0f) * 2.0f);

			t.Move(textRot * Vec3f(c.Advance * halfExtent.x, 0.0f, 0.0f) * 2.0f);
		}
		
		glDisable(GL_BLEND);
	}

	void RenderEngine::Dispose()
	{
		RenderTbCollections.clear();
		CubemapData.DefaultFramebuffer.Dispose(false);

		Postprocessing.Dispose();

		for (auto i = Shaders.begin(); i != Shaders.end(); i++)
			if (std::find_if(CustomShaders.begin(), CustomShaders.end(), [&](SharedPtr<Shader>& forwardShader) { return forwardShader.get() == i->get(); }) == CustomShaders.end())
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