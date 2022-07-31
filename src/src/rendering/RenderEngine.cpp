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

		GameScene& engineObjScene = GameHandle->CreateScene("GEE_Engine_Objects");

		BasicShapes[EngineBasicShape::Quad] = EngineDataLoader::LoadHierarchyTree(engineObjScene, "Assets/EngineObjects/quad.obj", &engineObjScene.CreateHierarchyTree("ENG_QUAD"))->FindMesh("Quad");
		BasicShapes[EngineBasicShape::Cube] = EngineDataLoader::LoadHierarchyTree(engineObjScene, "Assets/EngineObjects/cube.obj", &engineObjScene.CreateHierarchyTree("ENG_CUBE"))->FindMesh("Cube");
		BasicShapes[EngineBasicShape::Sphere] = EngineDataLoader::LoadHierarchyTree(engineObjScene, "Assets/EngineObjects/sphere.obj", &engineObjScene.CreateHierarchyTree("ENG_SPHERE"))->FindMesh("Sphere");
		BasicShapes[EngineBasicShape::Cone] = EngineDataLoader::LoadHierarchyTree(engineObjScene, "Assets/EngineObjects/cone.obj", &engineObjScene.CreateHierarchyTree("ENG_CONE"))->FindMesh("Cone");

		for (auto it : BasicShapes)
			GEE_CORE_ASSERT(it.second != nullptr, "One of the basic shapes could not be located on the disk. Engine files have likely been corrupted.");
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

		for (int i = 0; i < 2; i++)
		{
			Shaders.push_back(AddShader(ShaderLoader::LoadShadersWithInclData("ShadowMapVisualisation_" + ((i == 0) ? (std::string("2D")) : (std::string("3D"))), "#define LIGHT_2D\n", "Shaders/shadowMapVisualisation.vs", "Shaders/shadowMapVisualisation.fs")));
			Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MODEL, MatrixType::MVP});
			Shaders.back()->UniformBlockBinding("BoneMatrices", 10);
			Shaders.back()->Use();
			Shaders.back()->Uniform1i("shadowMaps", 10);
			Shaders.back()->Uniform1i("shadowCubemaps", 11);
		}

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
		return *BasicShapes[type];
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

	std::vector<SharedPtr<Material>> RenderEngine::GetMaterials()
	{
		return Materials;
	}

	RenderToolboxCollection& GEE::RenderEngine::AddRenderTbCollection(UniquePtr<RenderToolboxCollection> tbCollection, bool setupToolboxesAccordingToSettings)
	{
		RenderTbCollections.push_back(std::move(tbCollection));
		if (setupToolboxesAccordingToSettings)
			RenderTbCollections.back()->AddTbsRequiredBySettings();

		CurrentTbCollection = RenderTbCollections.back().get();

		return *RenderTbCollections.back();
	}

	SharedPtr<Material> RenderEngine::AddMaterial(SharedPtr<Material>  material)
	{
		std::cout << "Adding material " << Materials.size() << " (" << material << ")";
		if (material)
			std::cout << " (" << material->GetLocalization().Name << ")\n";
		Materials.push_back(material);
		return Materials.back();
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

	SharedPtr<Material> RenderEngine::FindMaterialImpl(const std::string& name)
	{
		if (name.empty())
			return nullptr;

		if (auto materialIt = std::find_if(Materials.begin(), Materials.end(), [name](const SharedPtr<Material>& material) {	return material->GetLocalization().Name == name; }); materialIt != Materials.end())
			return *materialIt;

		std::cerr << "INFO: Can't find material " << name << "!\n";
		return nullptr;
	}

	Shader* RenderEngine::FindShader(const std::string& name)
	{

		if (auto shaderIt = std::find_if(Shaders.begin(), Shaders.end(), [name](SharedPtr<Shader> shader) { return shader->GetName() == name; }); shaderIt != Shaders.end())
			return shaderIt->get();

		std::cerr << "ERROR! Can't find a shader named " << name << "!\n";
		return nullptr;
	}

	RenderToolboxCollection* GEE::RenderEngine::FindTbCollection(const std::string& name)
	{
		auto found = std::find_if(RenderTbCollections.begin(), RenderTbCollections.end(), [&name](UniquePtr<RenderToolboxCollection>& tbCol) { return tbCol->GetName() == name; });
		if (found == RenderTbCollections.end())
			return nullptr;

		return found->get();
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