#include <rendering/RenderToolbox.h>
#include <rendering/Framebuffer.h>
#include <game/GameSettings.h>
#include <scene/LightComponent.h>
#include "../SMAA/AreaTex.h"
#include "../SMAA/SearchTex.h"
#include <rendering/Shader.h>
#include <random>

namespace GEE
{
	using namespace GEE_FB;

	DeferredShadingToolbox::DeferredShadingToolbox() :
		GFb(nullptr),
		GeometryShader(nullptr)
	{
	}

	DeferredShadingToolbox::DeferredShadingToolbox(const GameSettings::VideoSettings& settings) :
		DeferredShadingToolbox()
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
			NamedTexture(Texture::Loader<float>::ReserveEmpty2D(settings.Resolution, Texture::Format::Float32::RGB()), "gPosition"),	//gPosition texture
			NamedTexture(Texture::Loader<float>::ReserveEmpty2D(settings.Resolution, Texture::Format::Float32::RGB()), "gNormal"),	//gNormal texture
			NamedTexture(Texture::Loader<>::ReserveEmpty2D(settings.Resolution, Texture::Format::RGB()), "gAlbedoSpec")				//gAlbedoSpec texture
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
		case ShadingModel::SHADING_PHONG:
			settingsDefines += "#define PHONG_SHADING 1\n";
			lightShadersNames = { "PhongDirectional", "PhongPoint", "PhongSpot" };
			lightShadersDefines = { "#define DIRECTIONAL_LIGHT 1\n", "#define POINT_LIGHT 1\n", "#define SPOT_LIGHT 1\n" };
			lightShadersPath = std::pair<std::string, std::string>("Shaders/phong.vs", "Shaders/phong.fs");
			break;
		case ShadingModel::SHADING_PBR_COOK_TORRANCE:
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
		if (settings.Shading == ShadingModel::SHADING_PBR_COOK_TORRANCE)
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
			Shaders.back()->Uniform1i("gPosition", 0);
			Shaders.back()->Uniform1i("gNormal", 1);
			Shaders.back()->Uniform1i("gAlbedoSpec", 2);
			Shaders.back()->Uniform1i("gAlphaMetalAo", 3);
			if (settings.AmbientOcclusionSamples > 0)
				Shaders.back()->Uniform1i("ssaoTex", 4);

			Shaders.back()->Uniform1i("shadowMaps", 10);
			Shaders.back()->Uniform1i("shadowCubemaps", 11);
			Shaders.back()->Uniform1i("irradianceCubemaps", 12);
			Shaders.back()->Uniform1i("prefilterCubemaps", 13);
			Shaders.back()->Uniform1i("BRDFLutTex", 14);

			Shaders.back()->Uniform1f("lightProbeNr", 3.0f);

			if (i == (int)LightType::POINT || i == (int)LightType::SPOT || lightShadersNames[i] == "CookTorranceIBL")
				Shaders.back()->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::VIEW, MatrixType::MVP});

			LightShaders.push_back(Shaders.back().get());
		}

		GeometryShader = AddShader(ShaderLoader::LoadShadersWithInclData("Geometry", settingsDefines, "Shaders/geometry.vs", "Shaders/geometry.fs"));
		GeometryShader->UniformBlockBinding("BoneMatrices", 10);
		GeometryShader->UniformBlockBinding("PreviousBoneMatrices", 11);
		GeometryShader->SetTextureUnitNames(gShaderTextureUnits);
		GeometryShader->SetExpectedMatrices(std::vector<MatrixType>{MatrixType::MODEL, MatrixType::MVP, MatrixType::NORMAL});
	}

	MainFramebufferToolbox::MainFramebufferToolbox() :
		MainFb(nullptr)
	{
	}

	MainFramebufferToolbox::MainFramebufferToolbox(const GameSettings::VideoSettings& settings, DeferredShadingToolbox* deferredTb) :
		MainFramebufferToolbox()
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
			NamedTexture velocityBuffer = ((deferredTb) ? (deferredTb->GFb->GetAttachment("velocityTex")) : (NamedTexture(Texture::Loader<float>::ReserveEmpty2D(settings.Resolution, Texture::Format::Float16::RGB()), "velocityTex")));
			colorBuffers.push_back(velocityBuffer);
		}

		MainFb = AddFramebuffer();
		MainFb->AttachTextures(colorBuffers, sharedDepthStencil);	//color and blur buffers
		if (settings.IsVelocityBufferNeeded())
			MainFb->ExcludeDrawSlot(MainFb->GetAttachment("velocityTex").GetAttachmentSlot());

		//////////////////////////////SHADER LOADING//////////////////////////////

	}

	SSAOToolbox::SSAOToolbox() :
		SSAOFb(nullptr),
		SSAOShader(nullptr),
		SSAONoiseTex(nullptr)
	{
	}

	SSAOToolbox::SSAOToolbox(const GameSettings::VideoSettings& settings)
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
		SSAOShader->Uniform1f("radius", 0.5f);
		SSAOShader->Uniform1i("gPosition", 0);
		SSAOShader->Uniform1i("gNormal", 1);
		SSAOShader->Uniform1i("noiseTex", 2);

		//////////////////////////////TEX GENERATION//////////////////////////////
		std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
		std::default_random_engine generator;

		std::vector<Vec3f> ssaoSamples;
		ssaoSamples.reserve(settings.AmbientOcclusionSamples);
		for (unsigned int i = 0; i < settings.AmbientOcclusionSamples; i++)
		{
			Vec3f sample(
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator)
			);

			sample = glm::normalize(sample) * randomFloats(generator);

			float scale = (float)i / (float)settings.AmbientOcclusionSamples;
			scale = glm::mix(0.1f, 1.0f, scale * scale);
			sample *= scale;

			SSAOShader->Uniform3fv("samples[" + std::to_string(i) + "]", sample);
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

	PrevFrameStorageToolbox::PrevFrameStorageToolbox() :
		StorageFb(nullptr)
	{
	}

	PrevFrameStorageToolbox::PrevFrameStorageToolbox(const GameSettings::VideoSettings& settings) :
		PrevFrameStorageToolbox()
	{
		Setup(settings);
	}

	void PrevFrameStorageToolbox::Setup(const GameSettings::VideoSettings& settings)
	{
		StorageFb = AddFramebuffer();

		StorageFb->AttachTextures(std::vector<NamedTexture>{ NamedTexture(Texture::Loader<>::ReserveEmpty2D(settings.Resolution, Texture::Format::RGBA()), "frame1"), NamedTexture(Texture::Loader<>::ReserveEmpty2D(settings.Resolution, Texture::Format::RGBA()), "frame2") });
	}

	SMAAToolbox::SMAAToolbox() :
		SMAAShaders{ nullptr, nullptr, nullptr, nullptr },
		SMAAAreaTex(nullptr),
		SMAASearchTex(nullptr)
	{
	}

	SMAAToolbox::SMAAToolbox(const GameSettings::VideoSettings& settings) :
		SMAAToolbox()
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
		std::string smaaDefines = "#define SCR_WIDTH " + std::to_string(static_cast<float>(settings.Resolution.x)) + "\n" + "#define SCR_HEIGHT " + std::to_string(static_cast<float>(settings.Resolution.y)) + "\n";
		switch (settings.AALevel)
		{
		case SettingLevel::SETTING_LOW: smaaDefines += "#define SMAA_PRESET_LOW 1\n"; break;
		case SettingLevel::SETTING_MEDIUM: smaaDefines += "#define SMAA_PRESET_MEDIUM 1\n"; break;
		case SettingLevel::SETTING_HIGH: smaaDefines += "#define SMAA_PRESET_HIGH 1\n"; break;
		case SettingLevel::SETTING_ULTRA: smaaDefines += "#define SMAA_PRESET_ULTRA 1\n"; break;
		}
		if (settings.AAType == AntiAliasingType::AA_SMAAT2X)
			smaaDefines += "#define SMAA_REPROJECTION 1\n";

		Shaders = { ShaderLoader::LoadShadersWithInclData("SMAAEdgeDetection", smaaDefines, "SMAA/edge.vs", "SMAA/edge.fs"),
					ShaderLoader::LoadShadersWithInclData("SMAABlendingWeight", smaaDefines, "SMAA/blend.vs", "SMAA/blend.fs"),
					ShaderLoader::LoadShadersWithInclData("SMAANeighborhoodBlending", smaaDefines, "SMAA/neighborhood.vs", "SMAA/neighborhood.fs") };

		for (int i = 0; i < 3; i++)
			SMAAShaders[i] = Shaders[i].get();
		SMAAShaders[3] = nullptr;

		SMAAShaders[0]->Use();
		SMAAShaders[0]->Uniform1i("colorTex", 0);
		SMAAShaders[0]->Uniform1i("depthTex", 1);

		SMAAShaders[1]->Use();
		SMAAShaders[1]->Uniform1i("edgesTex", 0);
		SMAAShaders[1]->Uniform1i("areaTex", 1);
		SMAAShaders[1]->Uniform1i("searchTex", 2);
		SMAAShaders[1]->Uniform4fv("ssIndices", Vec4f(0.0f));

		SMAAShaders[2]->Use();
		SMAAShaders[2]->Uniform1i("colorTex", 0);
		SMAAShaders[2]->Uniform1i("blendTex", 1);
		SMAAShaders[2]->Uniform1i("velocityTex", 2);

		if (settings.AAType == AntiAliasingType::AA_SMAAT2X)
		{
			SMAAShaders[3] = AddShader(ShaderLoader::LoadShadersWithInclData("SMAAReprojection", smaaDefines, "SMAA/reproject.vs", "SMAA/reproject.fs"));
			SMAAShaders[3]->Use();
			SMAAShaders[3]->Uniform1i("colorTex", 0);
			SMAAShaders[3]->Uniform1i("prevColorTex", 1);
			SMAAShaders[3]->Uniform1i("velocityTex", 2);
		}

		//////////////////////////////TEX GENERATION//////////////////////////////
		SMAAAreaTex = AddTexture(Texture::Loader<>::FromBuffer2D(Vec2u(AREATEX_WIDTH, AREATEX_HEIGHT), areaTexBytes, Texture::Format::RG(), 2));
		SMAAAreaTex->SetMinFilter(Texture::MinFilter::Nearest());
		SMAAAreaTex->SetMagFilter(Texture::MagFilter::Nearest());
		
		SMAASearchTex = AddTexture(Texture::Loader<>::FromBuffer2D(Vec2u(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT), searchTexBytes, Texture::Format::Red(), 1));
		SMAASearchTex->SetMinFilter(Texture::MinFilter::Nearest());
		SMAASearchTex->SetMagFilter(Texture::MagFilter::Nearest());
	}

	ComposedImageStorageToolbox::ComposedImageStorageToolbox() :
		ComposedImageFb(nullptr),
		TonemapGammaShader(nullptr)
	{
	}

	ComposedImageStorageToolbox::ComposedImageStorageToolbox(const GameSettings::VideoSettings& settings) :
		ComposedImageStorageToolbox()
	{
		Setup(settings);
	}

	void ComposedImageStorageToolbox::Setup(const GameSettings::VideoSettings& settings)
	{
		ComposedImageFb = AddFramebuffer();
		ComposedImageFb->AttachTextures(NamedTexture(Texture::Loader<>::ReserveEmpty2D(settings.Resolution, Texture::Format::RGB()), "composedImageBuffer"));
		TonemapGammaShader = AddShader(ShaderLoader::LoadShadersWithInclData("TonemapGamma", std::string((settings.TMType == ToneMappingType::TM_REINHARD) ? ("#define TM_REINHARD\n") : ("")) + settings.GetShaderDefines(settings.Resolution), "Shaders/tonemap_gamma.vs", "Shaders/tonemap_gamma.fs"));
		TonemapGammaShader->Use();
		TonemapGammaShader->Uniform1i("HDRbuffer", 0);
		TonemapGammaShader->Uniform1i("brightnessBuffer", 1);
		TonemapGammaShader->Uniform1f("gamma", settings.MonitorGamma);
	}

	bool RenderToolbox::IsSetup()
	{
		return !(Shaders.empty() && Fbs.empty());
	}

	Shader* RenderToolbox::FindShader(std::string name)
	{
		auto found = std::find_if(Shaders.begin(), Shaders.end(), [name](std::shared_ptr<Shader>& shader) { return shader->GetName() == name; });
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
		Fbs.push_back(std::make_shared<GEE_FB::Framebuffer>());
		return Fbs.back().get();
	}

	Shader* RenderToolbox::AddShader(const std::shared_ptr<Shader>& shader)
	{
		Shaders.push_back(shader);
		return Shaders.back().get();
	}

	Texture* RenderToolbox::AddTexture(const Texture& tex)
	{
		Textures.push_back(std::make_shared<Texture>(Texture(tex)));
		return Textures.back().get();
	}

	GaussianBlurToolbox::GaussianBlurToolbox() :
		BlurFramebuffers{ nullptr, nullptr },
		GaussianBlurShader(nullptr)
	{
	}

	GaussianBlurToolbox::GaussianBlurToolbox(const GameSettings::VideoSettings& settings) :
		GaussianBlurToolbox()
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
		GaussianBlurShader->Uniform1i("tex", 0);
	}

	Shader* RenderToolboxCollection::FindShader(std::string name) const
	{
		for (int i = 0; i < static_cast<int>(Tbs.size()); i++)
			if (Shader* shader = Tbs[i]->FindShader(name))
				return shader;

		return nullptr;
	}

	ShadowMappingToolbox::ShadowMappingToolbox() :
		ShadowFramebuffer(nullptr),
		ShadowMapArray(nullptr),
		ShadowCubemapArray(nullptr)
	{
	}

	ShadowMappingToolbox::ShadowMappingToolbox(const GameSettings::VideoSettings& settings) :
		ShadowMappingToolbox()
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


		ShadowMapArray = AddTexture(Texture::Loader<float>::ReserveEmpty2DArray(Vec3u(shadowMapSize.x, shadowMapSize.y, 16), Texture::Format::Depth(), Texture::Format::Depth()));
		ShadowMapArray->SetMagFilter(Texture::MagFilter::Nearest(), true);
		ShadowMapArray->SetMinFilter(Texture::MinFilter::Nearest(), true, true);
		ShadowMapArray->SetWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, 0, true);
		ShadowMapArray->SetBorderColor(Vec4f(1.0f));

		ShadowCubemapArray = AddTexture(Texture::Loader<float>::ReserveEmptyCubemapArray(Vec3u(shadowMapSize.x, shadowMapSize.y, 16), Texture::Format::Depth(), Texture::Format::Depth()));
		ShadowCubemapArray->SetMagFilter(Texture::MagFilter::Nearest(), true);
		ShadowCubemapArray->SetMinFilter(Texture::MinFilter::Nearest(), true, true);
	}

	FinalRenderTargetToolbox::FinalRenderTargetToolbox() :
		FinalFramebuffer(nullptr)
	{
	}

	FinalRenderTargetToolbox::FinalRenderTargetToolbox(const GameSettings::VideoSettings& settings) :
		FinalRenderTargetToolbox()
	{
		Setup(settings);
	}

	void FinalRenderTargetToolbox::Setup(const GameSettings::VideoSettings& settings)
	{

		FinalFramebuffer = AddFramebuffer();
		FinalFramebuffer->AttachTextures(NamedTexture(Texture::Loader<>::ReserveEmpty2D(settings.Resolution, Texture::Format::RGB()), "FinalColorBuffer"));
		RenderTarget = FinalFramebuffer->GetColorTexture(0);

		FinalFramebuffer->Bind(false);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Final loaded!\n";
		printVector(settings.Resolution, "REZOLUCJA monke");
	}

	GEE_FB::Framebuffer& FinalRenderTargetToolbox::GetFinalFramebuffer()
	{
		return *FinalFramebuffer;
	}

}