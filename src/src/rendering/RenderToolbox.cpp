#include <rendering/RenderToolbox.h>
#include <rendering/Framebuffer.h>
#include <game/GameSettings.h>
#include <scene/LightComponent.h>
#include <SMAA/AreaTex.h>
#include <SMAA/SearchTex.h>
#include <rendering/Shader.h>
#include <random>

namespace GEE
{
	using namespace GEE_FB;

	ShaderFromHintGetter::ShaderFromHintGetter(RenderEngineManager& renderHandle)
	{
		ShaderHintMap[MaterialShaderHint::Simple] = renderHandle.GetSimpleShader();
	}

	Shader* ShaderFromHintGetter::GetShader(MaterialShaderHint hint)
	{
		return ShaderHintMap[hint];
	}

	void ShaderFromHintGetter::SetShader(MaterialShaderHint hint, Shader* shader)
	{
		ShaderHintMap[hint] = shader;
	}

	bool RenderToolbox::IsSetup()
	{
		return !(Shaders.empty() && Fbs.empty());
	}

	Shader* RenderToolbox::FindShader(std::string name)
	{
		auto found = std::find_if(Shaders.begin(), Shaders.end(), [name](SharedPtr<Shader>& shader) { return shader->GetName() == name; });
		if (found != Shaders.end())
			return found->get();

		return nullptr;
	}

	void RenderToolbox::Dispose()
	{
		for (int i = 0; i < static_cast<int>(Fbs.size()); i++)
			Fbs[i]->Dispose(true);

		for (int i = 0; i < static_cast<int>(Shaders.size()); i++)
			Shaders[i]->Dispose();

		for (int i = 0; i < static_cast<int>(Textures.size()); i++)
			Textures[i]->Dispose();

		Fbs.clear();
		Shaders.clear();
		Textures.clear();
	}

	GEE_FB::Framebuffer* RenderToolbox::AddFramebuffer()
	{
		Fbs.push_back(MakeShared<GEE_FB::Framebuffer>());
		return Fbs.back().get();
	}

	Shader* RenderToolbox::AddShader(const SharedPtr<Shader>& shader)
	{
		Shaders.push_back(shader);
		return Shaders.back().get();
	}

	Texture* RenderToolbox::AddTexture(const Texture& tex)
	{
		Textures.push_back(MakeShared<Texture>(Texture(tex)));
		return Textures.back().get();
	}

	void RenderToolbox::SetShaderHint(MaterialShaderHint hint, Shader* shader)
	{
		ShaderGetter->SetShader(hint, shader);
	}

	ForwardShadingToolbox::ForwardShadingToolbox(ShaderFromHintGetter& getter, const GameSettings::VideoSettings& settings):
		RenderToolbox(getter)
	{
		Setup(settings);
	}

	void ForwardShadingToolbox::Setup(const GameSettings::VideoSettings& settings)
	{
		//////////////////////////////SHADER LOADING//////////////////////////////
		std::vector<std::string> lightShadersNames;
		std::vector<std::string> lightShadersDefines;
		std::pair<std::string, std::string> lightShadersPath;
		std::string settingsDefines = settings.GetShaderDefines(settings.Resolution);

		switch (settings.Shading)
		{
		case ShadingAlgorithm::SHADING_PHONG:
			settingsDefines += "#define PHONG_SHADING 1\n";
			lightShadersNames = { "PhongDirectional", "PhongPoint", "PhongSpot" };
			lightShadersDefines = { "#define DIRECTIONAL_LIGHT 1\n", "#define POINT_LIGHT 1\n", "#define SPOT_LIGHT 1\n" };
			lightShadersPath = std::pair<std::string, std::string>("Shaders/phong.vs", "Shaders/phong.fs");
			break;
		case ShadingAlgorithm::SHADING_PBR_COOK_TORRANCE:
			settingsDefines += "#define PBR_SHADING 1\n";
			//lightShadersNames = { "CookTorranceDirectional", "CookTorrancePoint", "CookTorranceSpot" };
			lightShadersNames = { "Geometry", "Geometry", "Geometry" };
			lightShadersDefines = { "#define DIRECTIONAL_LIGHT 1\n", "#define POINT_LIGHT 1\n", "#define SPOT_LIGHT 1\n"};
			lightShadersPath = std::pair<std::string, std::string>("Shaders/forward_pbrCookTorrance.vs", "Shaders/forward_pbrCookTorrance.fs");
			break;
		}

		//1. Geometry shader
		std::vector<std::pair<unsigned int, std::string>> gShaderTextureUnits = {
			std::pair<unsigned int, std::string>(0, "albedo1"),
			std::pair<unsigned int, std::string>(1, "specular1"),
			std::pair<unsigned int, std::string>(2, "normal1"),
			std::pair<unsigned int, std::string>(3, "depth1"),
		};
		if (settings.Shading == ShadingAlgorithm::SHADING_PBR_COOK_TORRANCE)
		{
			std::vector<std::pair<unsigned int, std::string>> pbrTextures = {
				std::pair<unsigned int, std::string>(4, "roughness1"),
				std::pair<unsigned int, std::string>(5, "metallic1"),
				std::pair<unsigned int, std::string>(6, "ao1"),
				std::pair<unsigned int, std::string>(7, "combined1") };
			gShaderTextureUnits.insert(gShaderTextureUnits.begin(), pbrTextures.begin(), pbrTextures.end());
		}

		//2. Light shaders (directional, point, spot)
		for (int i = 0; i < static_cast<int>(lightShadersNames.size()); i++)
		{
			Shaders.push_back(ShaderLoader::LoadShadersWithInclData(lightShadersNames[i], settingsDefines + lightShadersDefines[i], lightShadersPath.first, lightShadersPath.second));
			Shaders.back()->Use();

			Shaders.back()->Uniform<int>("shadowMaps", 10);
			Shaders.back()->Uniform<int>("shadowCubemaps", 11);

			Shaders.back()->UniformBlockBinding("BoneMatrices", 10);

			Shaders.back()->SetTextureUnitNames(gShaderTextureUnits);
			Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MODEL, MatrixType::MVP, MatrixType::NORMAL});
			Shaders.back()->SetOnMaterialWholeDataUpdateFunc([](Shader& shader, const Material& mat) {
				MaterialUtil::DisableColorIfAlbedoTextureDetected(shader, mat);
				});

			LightShaders.push_back(Shaders.back().get());
		}
	}

	DeferredShadingToolbox::DeferredShadingToolbox(ShaderFromHintGetter& getter, const GameSettings::VideoSettings& settings) :
		RenderToolbox(getter),
		GFb(nullptr),
		GeometryShader(nullptr)
	{
		Setup(settings);
	}


	void DeferredShadingToolbox::Setup(const GameSettings::VideoSettings& settings)
	{
		//////////////////////////////FBS LOADING//////////////////////////////
		//1. Common attachments
		GEE_FB::FramebufferAttachment sharedDepthStencil(NamedTexture(Texture::Loader<Texture::LoaderArtificialType::Uint24_8>::ReserveEmpty2D(settings.Resolution, Texture::Format::Uint32::Depth24Stencil8(), Texture::Format::DepthStencil()), "depthStencilTex"), AttachmentSlot::DepthStencil());	//we share this buffer between the geometry and main framebuffer - we want deferred-rendered objects depth to influence light volumes&forward rendered objects
		NamedTexture velocityBuffer(Texture::Loader<float>::ReserveEmpty2D(settings.Resolution, Texture::Format::Float32::RGB()), "velocityTex");

		//2. Geometry framebuffer attachments
		std::vector<NamedTexture> gColorBuffers = {
			NamedTexture(Texture::Loader<>::ReserveEmpty2D(settings.Resolution, Texture::Format::RGB()), "gAlbedoSpec"),				//gAlbedoSpec texture
			NamedTexture(Texture::Loader<float>::ReserveEmpty2D(settings.Resolution, Texture::Format::Float32::RGB()), "gPosition"),	//gPosition texture
			NamedTexture(Texture::Loader<float>::ReserveEmpty2D(settings.Resolution, Texture::Format::Float32::RGB()), "gNormal")		//gNormal texture
		};

		gColorBuffers.push_back(NamedTexture(Texture::Loader<>::ReserveEmpty2D(settings.Resolution, Texture::Format::RGB()), "gAlphaMetalAo")); //alpha, metallic, ao texture
		if (settings.IsVelocityBufferNeeded())
			gColorBuffers.push_back(velocityBuffer);

		GFb = AddFramebuffer();

		GFb->AttachTextures(gColorBuffers, sharedDepthStencil);

		//////////////////////////////SHADER LOADING//////////////////////////////
		std::vector<std::string> lightShadersNames;
		std::vector<std::string> lightShadersDefines;
		std::pair<std::string, std::string> lightShadersPath;
		std::string settingsDefines = settings.GetShaderDefines(settings.Resolution);

		switch (settings.Shading)
		{
		case ShadingAlgorithm::SHADING_PHONG:
			settingsDefines += "#define PHONG_SHADING 1\n";
			lightShadersNames = { "PhongDirectional", "PhongPoint", "PhongSpot" };
			lightShadersDefines = { "#define DIRECTIONAL_LIGHT 1\n", "#define POINT_LIGHT 1\n", "#define SPOT_LIGHT 1\n" };
			lightShadersPath = std::pair<std::string, std::string>("Shaders/phong.vs", "Shaders/phong.fs");
			break;
		case ShadingAlgorithm::SHADING_PBR_COOK_TORRANCE:
			settingsDefines += "#define PBR_SHADING 1\n";
			lightShadersNames = { "CookTorranceDirectional", "CookTorrancePoint", "CookTorranceSpot", "CookTorranceIBL" };
			lightShadersDefines = { "#define DIRECTIONAL_LIGHT 1\n", "#define POINT_LIGHT 1\n", "#define SPOT_LIGHT 1\n", "#define IBL_PASS 1\n" };
			lightShadersPath = std::pair<std::string, std::string>("Shaders/pbrCookTorrance.vs", "Shaders/pbrCookTorrance.fs");
			break;
		}

		//1. Geometry shader
		std::vector<std::pair<unsigned int, std::string>> gShaderTextureUnits = {
			std::pair<unsigned int, std::string>(0, "albedo1"),
			std::pair<unsigned int, std::string>(1, "specular1"),
			std::pair<unsigned int, std::string>(2, "normal1"),
			std::pair<unsigned int, std::string>(3, "depth1"),
		};
		if (settings.Shading == ShadingAlgorithm::SHADING_PBR_COOK_TORRANCE)
		{
			std::vector<std::pair<unsigned int, std::string>> pbrTextures = {
				std::pair<unsigned int, std::string>(4, "roughness1"),
				std::pair<unsigned int, std::string>(5, "metallic1"),
				std::pair<unsigned int, std::string>(6, "ao1"),
				std::pair<unsigned int, std::string>(7, "combined1") };
			gShaderTextureUnits.insert(gShaderTextureUnits.begin(), pbrTextures.begin(), pbrTextures.end());
		}

		//2. Light shaders (directional, point, spot, ibl (optional))
		for (int i = 0; i < static_cast<int>(lightShadersNames.size()); i++)
		{
			Shaders.push_back(ShaderLoader::LoadShadersWithInclData(lightShadersNames[i], settingsDefines + lightShadersDefines[i], lightShadersPath.first, lightShadersPath.second));
			Shaders.back()->Use();
			Shaders.back()->Uniform<int>("gAlbedoSpec", 0);
			Shaders.back()->Uniform<int>("gPosition", 1);
			Shaders.back()->Uniform<int>("gNormal", 2);
			Shaders.back()->Uniform<int>("gAlphaMetalAo", 3);
			if (settings.AmbientOcclusionSamples > 0)
				Shaders.back()->Uniform<int>("ssaoTex", 4);

			Shaders.back()->Uniform<int>("shadowMaps", 10);
			Shaders.back()->Uniform<int>("shadowCubemaps", 11);
			Shaders.back()->Uniform<int>("irradianceCubemaps", 12);
			Shaders.back()->Uniform<int>("prefilterCubemaps", 13);
			Shaders.back()->Uniform<int>("BRDFLutTex", 14);

			Shaders.back()->Uniform<float>("lightProbeNr", 3.0f);

			if (i == (int)LightType::POINT || i == (int)LightType::SPOT || lightShadersNames[i] == "CookTorranceIBL")
				Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::VIEW, MatrixType::MVP});

			LightShaders.push_back(Shaders.back().get());
		}

		GeometryShader = AddShader(ShaderLoader::LoadShadersWithInclData("Geometry", settingsDefines, "Shaders/geometry.vs", "Shaders/geometry.fs"));
		GeometryShader->UniformBlockBinding("BoneMatrices", 10);
		GeometryShader->UniformBlockBinding("PreviousBoneMatrices", 11);
		GeometryShader->SetTextureUnitNames(gShaderTextureUnits);
		GeometryShader->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MODEL, MatrixType::MVP, MatrixType::NORMAL});
		GeometryShader->SetOnMaterialWholeDataUpdateFunc([](Shader& shader, const Material& mat) {
			MaterialUtil::DisableColorIfAlbedoTextureDetected(shader, mat);
		});

		SetShaderHint(MaterialShaderHint::Shaded, GeometryShader);
	}

	MainFramebufferToolbox::MainFramebufferToolbox(ShaderFromHintGetter& getter, const GameSettings::VideoSettings& settings, DeferredShadingToolbox* deferredTb) :
		RenderToolbox(getter),
		MainFb(nullptr)
	{
		Setup(settings, deferredTb);
	}

	void MainFramebufferToolbox::Setup(const GameSettings::VideoSettings& settings, DeferredShadingToolbox* deferredTb)
	{
		//////////////////////////////FBS LOADING//////////////////////////////
		if (deferredTb && !deferredTb->IsSetup())
			deferredTb = nullptr;
		
		GEE_FB::FramebufferAttachment sharedDepthStencil = ((deferredTb) ? (deferredTb->GFb->GetAttachment(GEE_FB::AttachmentSlot::DepthStencil())) : (GEE_FB::FramebufferAttachment(NamedTexture(Texture::Loader<Texture::LoaderArtificialType::Uint24_8>::ReserveEmpty2D(settings.Resolution, Texture::Format::Uint32::Depth24Stencil8(), Texture::Format::DepthStencil()), "depthStencilTex"), AttachmentSlot::DepthStencil())));
		std::vector<NamedTexture> colorBuffers;
		colorBuffers.push_back(NamedTexture(Texture::Loader<float>::ReserveEmpty2D(settings.Resolution, Texture::Format::Float16::RGBA()), "hdrBuffer"));	//add color buffer

		if (settings.bBloom)
		{
			colorBuffers.push_back(NamedTexture(Texture::Loader<float>::ReserveEmpty2D(settings.Resolution, Texture::Format::Float16::RGBA()), "blurBuffer"));	//add blur buffer
			colorBuffers.back().SetMinFilter(GEE::Texture::MinFilter::Bilinear(), true, false);
			colorBuffers.back().SetMagFilter(GEE::Texture::MagFilter::Bilinear(), true);
		}
		if (settings.IsVelocityBufferNeeded())
		{
			NamedTexture velocityBuffer = ((deferredTb) ? (static_cast<NamedTexture>(deferredTb->GFb->GetAttachment("velocityTex"))) : (NamedTexture(Texture::Loader<float>::ReserveEmpty2D(settings.Resolution, Texture::Format::Float16::RGB()), "velocityTex")));
			colorBuffers.push_back(velocityBuffer);
		}

		MainFb = AddFramebuffer();
		MainFb->AttachTextures(colorBuffers, sharedDepthStencil);	//color and blur buffers
		if (settings.IsVelocityBufferNeeded())
			MainFb->ExcludeDrawSlot(MainFb->GetAttachment("velocityTex").GetAttachmentSlot());

		//////////////////////////////SHADER LOADING//////////////////////////////

	}

	SSAOToolbox::SSAOToolbox(ShaderFromHintGetter& getter, const GameSettings::VideoSettings& settings):
		RenderToolbox(getter),
		SSAOFb(nullptr),
		SSAOShader(nullptr),
		SSAONoiseTex(nullptr)
	{
		Setup(settings);
	}

	void SSAOToolbox::Setup(const GameSettings::VideoSettings& settings)
	{
		//////////////////////////////FBS LOADING//////////////////////////////
		SSAOFb = AddFramebuffer();
		SSAOFb->AttachTextures(NamedTexture(Texture::Loader<float>::ReserveEmpty2D(settings.Resolution, Texture::Format::Red()), "ssaoColorBuffer"));

		//////////////////////////////SHADER LOADING//////////////////////////////
		Shaders.push_back(ShaderLoader::LoadShadersWithInclData("SSAO", settings.GetShaderDefines(), "Shaders/ssao.vs", "Shaders/ssao.fs"));
		SSAOShader = Shaders.back().get();

		SSAOShader->SetExpectedMatrices(std::vector<MatrixType> {MatrixType::VIEW, MatrixType::PROJECTION});
		SSAOShader->Use();
		SSAOShader->Uniform<float>("radius", 0.5f);
		SSAOShader->Uniform<int>("gPosition", 0);
		SSAOShader->Uniform<int>("gNormal", 1);
		SSAOShader->Uniform<int>("noiseTex", 2);

		//////////////////////////////TEX GENERATION//////////////////////////////
		std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
		std::default_random_engine generator;

		std::vector<Vec3f> ssaoSamples;
		ssaoSamples.reserve(settings.AmbientOcclusionSamples);

		// Use a treshold to discard samples parallel to the surface (and avoid depth resolution artifacts)	
		const float minDotTreshold = 0.0f;	

		for (unsigned int i = 0; i < settings.AmbientOcclusionSamples; i++)
		{
			Vec3f sample;
			do
			{
				sample = glm::normalize(Vec3f( randomFloats(generator) * 2.0f - 1.0f,
								randomFloats(generator) * 2.0f - 1.0f,
								randomFloats(generator)));
			} while (glm::dot(sample, glm::vec3(0.0f, 0.0f, 1.0f)) < minDotTreshold);

			sample *= randomFloats(generator); // modify length randomly between 0 and 1

			float scale = (float)i / (float)settings.AmbientOcclusionSamples;
			scale = glm::mix(0.1f, 1.0f, scale * scale);	// put more samples closer to the fragment
			sample *= scale;

			SSAOShader->Uniform<Vec3f>("samples[" + std::to_string(i) + "]", sample);
			ssaoSamples.push_back(sample);
		}

		std::vector<Vec3f> ssaoNoise;
		ssaoNoise.reserve(16);
		for (unsigned int i = 0; i < 16; i++)
		{
			Vec3f noise(
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator) * 2.0f - 1.0f,
				0.0f
			);

			ssaoNoise.push_back(noise);
		}

		SSAONoiseTex = AddTexture(Texture::Loader<float>::FromBuffer2D(Vec2u(4), &ssaoNoise[0], Texture::Format::Float16::RGB()));
		SSAONoiseTex->SetMinFilter(Texture::MinFilter::Nearest(), true, true);
		SSAONoiseTex->SetMagFilter(Texture::MagFilter::Nearest(), true);
		SSAONoiseTex->SetWrap(GL_REPEAT, GL_REPEAT, 0, true);
	}

	PrevFrameStorageToolbox::PrevFrameStorageToolbox(ShaderFromHintGetter& getter, const GameSettings::VideoSettings& settings) :
		RenderToolbox(getter),
		StorageFb(nullptr)
	{
		Setup(settings);
	}

	void PrevFrameStorageToolbox::Setup(const GameSettings::VideoSettings& settings)
	{
		StorageFb = AddFramebuffer();

		StorageFb->AttachTextures(std::vector<NamedTexture>{ NamedTexture(Texture::Loader<>::ReserveEmpty2D(settings.Resolution, Texture::Format::RGBA()), "frame1"), NamedTexture(Texture::Loader<>::ReserveEmpty2D(settings.Resolution, Texture::Format::RGBA()), "frame2") });
	}

	SMAAToolbox::SMAAToolbox(ShaderFromHintGetter& getter, const GameSettings::VideoSettings& settings) :
		RenderToolbox(getter),
		SMAAShaders{ nullptr, nullptr, nullptr, nullptr },
		SMAAAreaTex(nullptr),
		SMAASearchTex(nullptr)
	{
		Setup(settings);
	}

	void SMAAToolbox::Setup(const GameSettings::VideoSettings& settings)
	{
		if (settings.AAType == AntiAliasingType::AA_NONE)
			return;

		//////////////////////////////FBS LOADING//////////////////////////////
		SMAAFb = AddFramebuffer();
		SMAAFb->AttachTextures({ NamedTexture(Texture::Loader<>::ReserveEmpty2D(settings.Resolution, Texture::Format::RGB()), "smaaEdges"), NamedTexture(Texture::Loader<>::ReserveEmpty2D(settings.Resolution, Texture::Format::RGBA()), "smaaWeight"), NamedTexture(Texture::Loader<>::ReserveEmpty2D(settings.Resolution, Texture::Format::RGBA()), "smaaNeighborhood") });

		//////////////////////////////SHADER LOADING//////////////////////////////
		std::string smaaDefines = "#define SCR_WIDTH " + std::to_string(settings.Resolution.x) + "\n" + "#define SCR_HEIGHT " + std::to_string(settings.Resolution.y) + "\n";
		switch (settings.AALevel)
		{
		case SettingLevel::SETTING_LOW: smaaDefines += "#define SMAA_PRESET_LOW 1\n"; break;
		case SettingLevel::SETTING_MEDIUM: smaaDefines += "#define SMAA_PRESET_MEDIUM 1\n"; break;
		case SettingLevel::SETTING_HIGH: smaaDefines += "#define SMAA_PRESET_HIGH 1\n"; break;
		case SettingLevel::SETTING_ULTRA: smaaDefines += "#define SMAA_PRESET_ULTRA 1\n"; break;
		}
		if (settings.AAType == AntiAliasingType::AA_SMAAT2X)
			smaaDefines += "#define SMAA_REPROJECTION 1\n";

		Shaders = { ShaderLoader::LoadShadersWithInclData("SMAAEdgeDetection", smaaDefines, "Shaders/SMAA/edge.vs", "Shaders/SMAA/edge.fs"),
					ShaderLoader::LoadShadersWithInclData("SMAABlendingWeight", smaaDefines, "Shaders/SMAA/blend.vs", "Shaders/SMAA/blend.fs"),
					ShaderLoader::LoadShadersWithInclData("SMAANeighborhoodBlending", smaaDefines, "Shaders/SMAA/neighborhood.vs", "Shaders/SMAA/neighborhood.fs") };

		for (int i = 0; i < 3; i++)
			SMAAShaders[i] = Shaders[i].get();
		SMAAShaders[3] = nullptr;

		SMAAShaders[0]->Use();
		SMAAShaders[0]->Uniform<int>("colorTex", 0);
		SMAAShaders[0]->Uniform<int>("depthTex", 1);

		SMAAShaders[1]->Use();
		SMAAShaders[1]->Uniform<int>("edgesTex", 0);
		SMAAShaders[1]->Uniform<int>("areaTex", 1);
		SMAAShaders[1]->Uniform<int>("searchTex", 2);
		SMAAShaders[1]->Uniform<Vec4f>("ssIndices", Vec4f(0.0f));

		SMAAShaders[2]->Use();
		SMAAShaders[2]->Uniform<int>("colorTex", 0);
		SMAAShaders[2]->Uniform<int>("blendTex", 1);
		SMAAShaders[2]->Uniform<int>("velocityTex", 2);

		if (settings.AAType == AntiAliasingType::AA_SMAAT2X)
		{
			SMAAShaders[3] = AddShader(ShaderLoader::LoadShadersWithInclData("SMAAReprojection", smaaDefines, "SMAA/reproject.vs", "SMAA/reproject.fs"));
			SMAAShaders[3]->Use();
			SMAAShaders[3]->Uniform<int>("colorTex", 0);
			SMAAShaders[3]->Uniform<int>("prevColorTex", 1);
			SMAAShaders[3]->Uniform<int>("velocityTex", 2);
		}

		//////////////////////////////TEX GENERATION//////////////////////////////
		SMAAAreaTex = AddTexture(Texture::Loader<>::FromBuffer2D(Vec2u(AREATEX_WIDTH, AREATEX_HEIGHT), areaTexBytes, Texture::Format::RG(), 2));
		SMAAAreaTex->SetMinFilter(Texture::MinFilter::Nearest());
		SMAAAreaTex->SetMagFilter(Texture::MagFilter::Nearest());
		
		SMAASearchTex = AddTexture(Texture::Loader<>::FromBuffer2D(Vec2u(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT), searchTexBytes, Texture::Format::Red(), 1));
		SMAASearchTex->SetMinFilter(Texture::MinFilter::Nearest());
		SMAASearchTex->SetMagFilter(Texture::MagFilter::Nearest());
	}

	ComposedImageStorageToolbox::ComposedImageStorageToolbox(ShaderFromHintGetter& getter, const GameSettings::VideoSettings& settings) :
		RenderToolbox(getter),
		ComposedImageFb(nullptr),
		TonemapGammaShader(nullptr)
	{
		Setup(settings);
	}

	void ComposedImageStorageToolbox::Setup(const GameSettings::VideoSettings& settings)
	{
		ComposedImageFb = AddFramebuffer();
		ComposedImageFb->AttachTextures({ NamedTexture(Texture::Loader<>::ReserveEmpty2D(settings.Resolution, Texture::Format::RGB()), "composedImageBuffer1") });
										// NamedTexture(Texture::Loader<>::ReserveEmpty2D(settings.Resolution, Texture::Format::RGB()), "composedImageBuffer2") });
		TonemapGammaShader = AddShader(ShaderLoader::LoadShadersWithInclData("TonemapGamma", std::string((settings.TMType == ToneMappingType::TM_REINHARD) ? ("#define TM_REINHARD\n") : ("")) + settings.GetShaderDefines(settings.Resolution), "Shaders/tonemap_gamma.vs", "Shaders/tonemap_gamma.fs"));
		TonemapGammaShader->Use();
		TonemapGammaShader->Uniform<int>("HDRbuffer", 0);
		TonemapGammaShader->Uniform<int>("brightnessBuffer", 1);
		TonemapGammaShader->Uniform<float>("gamma", settings.MonitorGamma);
	}

	GaussianBlurToolbox::GaussianBlurToolbox(ShaderFromHintGetter& getter, const GameSettings::VideoSettings& settings) :
		RenderToolbox(getter),
		BlurFramebuffers{ nullptr, nullptr },
		GaussianBlurShader(nullptr)
	{
		Setup(settings);
	}

	void GaussianBlurToolbox::Setup(const GameSettings::VideoSettings& settings)
	{
		for (int i = 0; i < 2; i++)
		{
			BlurFramebuffers[i] = AddFramebuffer();

			NamedTexture buffer(Texture::Loader<float>::ReserveEmpty2D(settings.Resolution, Texture::Format::Float16::RGBA()), "gaussianBlur" + std::to_string(i));
			buffer.SetMinFilter(Texture::MinFilter::Bilinear(), true, true);
			buffer.SetMagFilter(Texture::MagFilter::Bilinear(), true);
			BlurFramebuffers[i]->AttachTextures(buffer);
		}

		GaussianBlurShader = AddShader(ShaderLoader::LoadShaders("GaussianBlur", "Shaders/gaussianblur.vs", "Shaders/gaussianblur.fs"));
		GaussianBlurShader->Use();
		GaussianBlurShader->Uniform<int>("tex", 0);
	}

	ShadowMappingToolbox::ShadowMappingToolbox(ShaderFromHintGetter& getter, const GameSettings::VideoSettings& settings) :
		RenderToolbox(getter),
		ShadowFramebuffer(nullptr),
		ShadowMapArray(nullptr),
		ShadowCubemapArray(nullptr)
	{
		Setup(settings);
	}

	void ShadowMappingToolbox::Setup(const GameSettings::VideoSettings& settings)
	{
		Vec2u shadowMapSize(512);
		switch (settings.ShadowLevel)
		{
		case SETTING_LOW: shadowMapSize = Vec2f(1024); break;
		case SETTING_MEDIUM: shadowMapSize = Vec2f(2048); break;
		case SETTING_HIGH: shadowMapSize = Vec2f(512); break;
		case SETTING_ULTRA: shadowMapSize = Vec2f(1024); break;
		}
		ShadowFramebuffer = AddFramebuffer();
		ShadowFramebuffer->Generate();


		ShadowMapArray = AddTexture(Texture::Loader<float>::ReserveEmpty2DArray(Vec3u(shadowMapSize.x, shadowMapSize.y, settings.Max2DShadows), Texture::Format::Depth(), Texture::Format::Depth()));
		ShadowMapArray->SetMagFilter(Texture::MagFilter::Nearest(), true);
		ShadowMapArray->SetMinFilter(Texture::MinFilter::Nearest(), true, true);
		ShadowMapArray->SetWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, 0, true);
		ShadowMapArray->SetBorderColor(Vec4f(1.0f));

		ShadowCubemapArray = AddTexture(Texture::Loader<float>::ReserveEmptyCubemapArray(Vec3u(shadowMapSize.x, shadowMapSize.y, settings.Max3DShadows), Texture::Format::Depth(), Texture::Format::Depth()));
		ShadowCubemapArray->SetMagFilter(Texture::MagFilter::Nearest(), true);
		ShadowCubemapArray->SetMinFilter(Texture::MinFilter::Nearest(), true, true);
	}

	FinalRenderTargetToolbox::FinalRenderTargetToolbox(ShaderFromHintGetter& getter, const GameSettings::VideoSettings& settings) :
		RenderToolbox(getter),
		FinalFramebuffer(nullptr)
	{
		Setup(settings);
	}

	void FinalRenderTargetToolbox::Setup(const GameSettings::VideoSettings& settings)
	{

		FinalFramebuffer = AddFramebuffer();
		FinalFramebuffer->AttachTextures(NamedTexture(Texture::Loader<>::ReserveEmpty2D(settings.Resolution, Texture::Format::RGB()), "FinalColorBuffer"));
		RenderTarget = FinalFramebuffer->GetColorTexture(0);

		FinalFramebuffer->Bind(false);
	}

	GEE_FB::Framebuffer& FinalRenderTargetToolbox::GetFinalFramebuffer()
	{
		return *FinalFramebuffer;
	}


	GEditorToolbox::GEditorToolbox(ShaderFromHintGetter& getter, const GameSettings::VideoSettings& settings) :
		RenderToolbox(getter),
		GridShader(nullptr)
	{
		Setup(settings);
	}

	void GEditorToolbox::Setup(const GameSettings::VideoSettings& settings)
	{
		GridShader = AddShader(ShaderLoader::LoadShaders("GEditor_Grid", "Shaders/UI/grid.vs", "Shaders/UI/grid.fs"));
	}
  
	/**
	* @brief Constructor of RenderToolboxCollection.
	* @param name: the name of this RenderToolboxCollection
	* @param settings: settings to be used for rendering. DO NOT pass a local variable that is about to be destroyed or you will encounter UB. (should be allocated on the heap)
	*/

	RenderToolboxCollection::RenderToolboxCollection(const std::string& name, const GameSettings::VideoSettings& settings, RenderEngineManager& renderHandle) :
		Name(name),
		Settings(settings),
		ShaderGetter(MakeUnique<ShaderFromHintGetter>(renderHandle))
	{

	}

	RenderToolboxCollection::RenderToolboxCollection(const RenderToolboxCollection& tbCol):
		Tbs(tbCol.Tbs),
		ShaderGetter(MakeUnique<ShaderFromHintGetter>(*tbCol.ShaderGetter)),
		Name(tbCol.Name),
		Settings(tbCol.Settings)
	{
		for (auto& it : Tbs)
			it->ShaderGetter = ShaderGetter.get();
	}

	std::string RenderToolboxCollection::GetName() const
	{
		return Name;
	}

	void RenderToolboxCollection::AddTbsRequiredBySettings()
	{
		Dispose();

		if (Settings.AAType == AA_SMAA1X || Settings.AAType == AA_SMAAT2X)
			AddTb<SMAAToolbox>();

		if (Settings.bBloom || Settings.AmbientOcclusionSamples > 0)
			AddTb<GaussianBlurToolbox>();

		if (Settings.AmbientOcclusionSamples > 0)
			AddTb<SSAOToolbox>();

		if (Settings.IsVelocityBufferNeeded())
			AddTb<PrevFrameStorageToolbox>();

		if (Settings.Shading != ShadingAlgorithm::SHADING_FULL_LIT)
			AddTb<DeferredShadingToolbox>();

		if (Settings.ShadowLevel > SettingLevel::SETTING_NONE)
			AddTb<ShadowMappingToolbox>();

		if (!Settings.bDrawToWindowFBO)
			AddTb<FinalRenderTargetToolbox>();

		AddTb<ForwardShadingToolbox>();
		AddTb<MainFramebufferToolbox>(GetTb<DeferredShadingToolbox>());
		AddTb<ComposedImageStorageToolbox>();
	}

	const GameSettings::VideoSettings& RenderToolboxCollection::GetVideoSettings() const
	{
		return Settings;
	}

	void RenderToolboxCollection::Dispose()
	{
		for (auto& tb : Tbs)
			tb->Dispose();

		Tbs.clear();
	}

	bool RenderToolboxCollection::ShouldLoadToolbox(RenderToolbox* toolbox, bool loadIfNotPresent)
	{
		if ((toolbox != nullptr && toolbox->IsSetup()) || (!loadIfNotPresent))
			return false;

		return true;
	}
}