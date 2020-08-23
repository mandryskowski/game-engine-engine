#include "Postprocess.h"
#include "SMAA/AreaTex.h"
#include "SMAA/SearchTex.h"
#include "Mesh.h"
#include <random>

using namespace GEE_FB;

void Postprocess::RenderQuad(unsigned int tex, unsigned int texSlot, bool bound, bool defaultFBO) const
{
	if (!bound)
		glBindVertexArray(QuadVAO);
	if (tex > 0)
	{
		glActiveTexture(GL_TEXTURE0 + texSlot);
		glBindTexture(GL_TEXTURE_2D, tex);
	}
	if (defaultFBO)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	QuadShader->Use();
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

Postprocess::Postprocess():
	Settings(nullptr),
	QuadVAO(0),
	QuadVBO(0),
	SSAOShader(nullptr),
	SSAOFramebuffer(nullptr),
	SSAONoiseTex(nullptr),
	StorageFramebuffer(nullptr),
	FrameIndex(0)
{
}

void Postprocess::Init(GameManager* gameHandle, glm::uvec2 resolution)
{
	RenderEngineManager* renderHandle = gameHandle->GetRenderEngineHandle();
	Resolution = resolution;
	Settings = gameHandle->GetGameSettings();
	float data[] = {
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		 1.0f,  1.0f,
		 1.0f,  1.0f,
		-1.0f,  1.0f,
		-1.0f, -1.0f,

		 0.0f,  0.0f,
		 1.0f,  0.0f,
		 1.0f,  1.0f,
		 1.0f,  1.0f,
		 0.0f,  1.0f,
		 0.0f,  0.0f,
	};
	QuadVAO = generateVAO(QuadVBO, std::vector<unsigned int> {2, 2}, 6, data);

	QuadShader = renderHandle->AddShader(ShaderLoader::LoadShaders("Quad", "Shaders/quad.vs", "Shaders/quad.fs"));
	QuadShader->Use();
	QuadShader->Uniform1i("tex", 0);

	TonemapGammaShader = renderHandle->AddShader(ShaderLoader::LoadShaders("TonemapGamma", "Shaders/tonemap_gamma.vs", "Shaders/tonemap_gamma.fs"));
	TonemapGammaShader->Use();
	TonemapGammaShader->Uniform1i("HDRbuffer", 0);
	TonemapGammaShader->Uniform1i("brightnessBuffer", 1);
	TonemapGammaShader->Uniform1f("gamma", Settings->MonitorGamma);

	GaussianBlurShader = renderHandle->AddShader(ShaderLoader::LoadShaders("GaussianBlur", "Shaders/gaussianblur.vs", "Shaders/gaussianblur.fs"));
	GaussianBlurShader->Use();
	GaussianBlurShader->Uniform1i("tex", 0);

	std::string smaaDefines = "#define SCR_WIDTH " + std::to_string(resolution.x) + ".0 \n" +
							  "#define SCR_HEIGHT " + std::to_string(resolution.y) + ".0 \n";
	switch (Settings->AALevel)
	{
		case SettingLevel::SETTING_LOW: smaaDefines += "#define SMAA_PRESET_LOW 1\n"; break;
		case SettingLevel::SETTING_MEDIUM: smaaDefines += "#define SMAA_PRESET_MEDIUM 1\n"; break;
		case SettingLevel::SETTING_HIGH: smaaDefines += "#define SMAA_PRESET_HIGH 1\n"; break;
		case SettingLevel::SETTING_ULTRA: smaaDefines += "#define SMAA_PRESET_ULTRA 1\n"; break;
	}
	if (Settings->AAType == AntiAliasingType::AA_SMAAT2X)
		smaaDefines += "#define SMAA_REPROJECTION 1\n";

	if (Settings->AAType == AntiAliasingType::AA_SMAA1X || Settings->AAType == AntiAliasingType::AA_SMAAT2X)
	{
		SMAAShaders[0] = renderHandle->AddShader(ShaderLoader::LoadShadersWithInclData("SMAAEdgeDetection", smaaDefines, "SMAA/edge.vs", "SMAA/edge.fs"));
		SMAAShaders[0]->Use();
		SMAAShaders[0]->Uniform1i("colorTex", 0);
		SMAAShaders[0]->Uniform1i("depthTex", 1);

		SMAAShaders[1] = renderHandle->AddShader(ShaderLoader::LoadShadersWithInclData("SMAABlendingWeight", smaaDefines, "SMAA/blend.vs", "SMAA/blend.fs"));
		SMAAShaders[1]->Use();
		SMAAShaders[1]->Uniform1i("edgesTex", 0);
		SMAAShaders[1]->Uniform1i("areaTex", 1);
		SMAAShaders[1]->Uniform1i("searchTex", 2);
		SMAAShaders[1]->Uniform4fv("ssIndices", glm::vec4(0.0f));

		SMAAAreaTex = textureFromBuffer(areaTexBytes, AREATEX_WIDTH, AREATEX_HEIGHT, GL_RG, GL_RG, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);
		SMAASearchTex = textureFromBuffer(searchTexBytes, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, GL_R8, GL_RED, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);

		SMAAShaders[2] = renderHandle->AddShader(ShaderLoader::LoadShadersWithInclData("SMAANeighborhoodBlending", smaaDefines, "SMAA/neighborhood.vs", "SMAA/neighborhood.fs"));
		SMAAShaders[2]->Use();
		SMAAShaders[2]->Uniform1i("colorTex", 0);
		SMAAShaders[2]->Uniform1i("blendTex", 1);
		SMAAShaders[2]->Uniform1i("velocityTex", 2);

		SMAAFramebuffer = std::make_unique<Framebuffer>();
		SMAAFramebuffer->SetAttachments(resolution, std::vector<std::shared_ptr<FramebufferAttachment>>{ GEE_FB::reserveColorBuffer(resolution, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR), GEE_FB::reserveColorBuffer(resolution, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR), GEE_FB::reserveColorBuffer(resolution, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR) });
		if (Settings->AAType == AntiAliasingType::AA_SMAAT2X)
		{
			SMAAShaders[3] = renderHandle->AddShader(ShaderLoader::LoadShadersWithInclData("SMAAReprojection", smaaDefines, "SMAA/reproject.vs", "SMAA/reproject.fs"));
			SMAAShaders[3]->Use();
			SMAAShaders[3]->Uniform1i("colorTex", 0);
			SMAAShaders[3]->Uniform1i("prevColorTex", 1);
			SMAAShaders[3]->Uniform1i("velocityTex", 2);

			StorageFramebuffer = std::make_unique<Framebuffer>();
			StorageFramebuffer->SetAttachments(resolution, std::vector<std::shared_ptr<FramebufferAttachment>>{ GEE_FB::reserveColorBuffer(resolution, GL_RGBA, GL_UNSIGNED_BYTE), GEE_FB::reserveColorBuffer(resolution, GL_RGBA, GL_UNSIGNED_BYTE) });
		}
	}

	if (Settings->AmbientOcclusionSamples > 0)
	{
		SSAOShader = renderHandle->AddShader(ShaderLoader::LoadShadersWithInclData("SSAO", "#define SSAO_SAMPLES " + std::to_string(Settings->AmbientOcclusionSamples) + "\n", "Shaders/ssao.vs", "Shaders/ssao.fs"));
		SSAOShader->SetExpectedMatrices(std::vector<MatrixType> {MatrixType::PROJECTION});
		SSAOShader->Use();
		SSAOShader->Uniform1f("radius", 0.5f);
		SSAOShader->Uniform1i("gPosition", 0);
		SSAOShader->Uniform1i("gNormal", 1);
		SSAOShader->Uniform1i("noiseTex", 2);
		SSAOShader->Uniform2fv("resolution", resolution);

		SSAOFramebuffer = std::make_unique<Framebuffer>();
		SSAOFramebuffer->SetAttachments(resolution, GEE_FB::reserveColorBuffer(resolution, GL_RED, GL_FLOAT, GL_LINEAR, GL_LINEAR));

		std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
		std::default_random_engine generator;

		std::vector<glm::vec3> ssaoSamples;
		ssaoSamples.reserve(Settings->AmbientOcclusionSamples);
		for (unsigned int i = 0; i < Settings->AmbientOcclusionSamples; i++)
		{
			glm::vec3 sample(
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator)
			);

			sample = glm::normalize(sample) * randomFloats(generator);

			float scale = (float)i / (float)Settings->AmbientOcclusionSamples;
			scale = glm::mix(0.1f, 1.0f, scale * scale);
			sample *= scale;

			SSAOShader->Uniform3fv("samples[" + std::to_string(i) + "]", sample);
			ssaoSamples.push_back(sample);
		}

		std::vector<glm::vec3> ssaoNoise;
		ssaoNoise.reserve(16);
		for (unsigned int i = 0; i < 16; i++)
		{
			glm::vec3 noise(
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator) * 2.0f - 1.0f,
				0.0f
			);

			ssaoNoise.push_back(noise);
		}

		SSAONoiseTex = std::make_unique<Texture>();
		SSAONoiseTex->GenerateID();
		SSAONoiseTex->Bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	TonemapGammaFramebuffer.SetAttachments(resolution, GEE_FB::reserveColorBuffer(resolution, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR));

	for (int i = 0; i < 2; i++)
		BlurFramebuffers[i].SetAttachments(resolution, GEE_FB::reserveColorBuffer(resolution, GL_RGB16F, GL_FLOAT, GL_LINEAR, GL_LINEAR));
}

unsigned int Postprocess::GetFrameIndex()
{
	return FrameIndex;
}

glm::mat4 Postprocess::GetJitterMat(int optionalIndex)
{
	glm::vec2 jitter(0.0f);

	switch (Settings->AAType)
	{
		case AA_NONE:
		case AA_SMAA1X:
		default:
			return glm::mat4(1.0f);
		case AA_SMAAT2X:
			glm::vec2 jitters[] = {
				glm::vec2(0.25f, -0.25f),
				glm::vec2(-0.25f, 0.25f)
			};

			/*glm::vec2 jitters[] = {
	glm::vec2(10.0f, -10.0f),
	glm::vec2(-10.0f, 10.0f)
};*/
			jitter = jitters[(optionalIndex >= 0) ? (optionalIndex) : (FrameIndex)];
			break;
	}

	jitter /= static_cast<glm::vec2>(Resolution);
	return glm::translate(glm::mat4(1.0f), glm::vec3(jitter, 0.0f));
}

const Texture* Postprocess::GaussianBlur(const Texture* tex, int passes, Framebuffer* writeFramebuffer, unsigned int writeColorBuffer) const	//Implemented using ping-pong framebuffers (one gaussian blur pass is separable into two lower-cost passes) and linear filtering for performance
{
	if (passes == 0)
		return tex;

	GaussianBlurShader->Use();
	glBindVertexArray(QuadVAO);
	glActiveTexture(GL_TEXTURE0);

	bool horizontal = true;

	for (int i = 0; i < passes; i++)
	{
		if (i == passes - 1 && writeFramebuffer)	//If the calling function specified the framebuffer to write the final result to, bind the given framebuffer for the last pass.
		{
			writeFramebuffer->Bind();
			glDrawBuffer(GL_COLOR_ATTACHMENT0 + writeColorBuffer);
		}
		else	//For any pass that doesn't match given criteria, just switch the ping pong fbos
		{
			BlurFramebuffers[horizontal].Bind();
			const Texture* bindTex = (i == 0) ? (tex) : (BlurFramebuffers[!horizontal].GetColorBuffer(0).get());
			bindTex->Bind();
		}
		GaussianBlurShader->Uniform1i("horizontal", horizontal);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		horizontal = !horizontal;
	}

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	return (writeFramebuffer) ? (writeFramebuffer->GetColorBuffer(0).get()) : (BlurFramebuffers[!horizontal].GetColorBuffer(0).get());
}

const Texture* Postprocess::SSAOPass(RenderInfo& info, const Texture* gPosition, const Texture* gNormal)
{
	if (!SSAOFramebuffer)
	{
		std::cerr << "ERROR! Attempted to render SSAO image without calling the setup\n";
		return 0;
	}

	SSAOFramebuffer->Bind();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);


	gPosition->Bind(0);
	gNormal->Bind(1);
	SSAONoiseTex->Bind(2);

	SSAOShader->Use();
	SSAOShader->UniformMatrix4fv("view", *info.view);
	SSAOShader->UniformMatrix4fv("projection", *info.projection);

	glBindVertexArray(QuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	GaussianBlur(SSAOFramebuffer->GetColorBuffer(0).get(), 4, SSAOFramebuffer.get());
	return SSAOFramebuffer->GetColorBuffer(0).get();
}

const Texture* Postprocess::SMAAPass(const Texture* colorTex, const Texture* depthTex, const Texture* previousColorTex, const Texture* velocityTex, Framebuffer* writeFramebuffer, unsigned int writeColorBuffer, bool bT2x) const
{
	SMAAFramebuffer->Bind();

	/////////////////1. Edge detection
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	SMAAShaders[0]->Use();
	colorTex->Bind(0);
	depthTex->Bind(1);

	glBindVertexArray(QuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	///////////////2. Weight blending
	glDrawBuffer(GL_COLOR_ATTACHMENT1);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	SMAAShaders[1]->Use();
	if (bT2x)
		SMAAShaders[1]->Uniform4fv("ssIndices", (FrameIndex) ? (glm::vec4(1, 1, 1, 0)) : (glm::vec4(2, 2, 2, 0)));

	SMAAFramebuffer->GetColorBuffer(0)->Bind(0);	//bind edges texture to slot 0
	SMAAAreaTex.Bind(1);
	SMAASearchTex.Bind(2);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	///////////////3. Neighborhood blending
	if (bT2x)
	{
		glDrawBuffer(GL_COLOR_ATTACHMENT2);
	}
	else
	{
		((writeFramebuffer) ? (writeFramebuffer->Bind(true)) : (glBindFramebuffer(GL_FRAMEBUFFER, 0)));
		if (!writeFramebuffer)
			glDrawBuffer(GL_BACK);
		else
			glDrawBuffer(GL_COLOR_ATTACHMENT0 + writeColorBuffer);
	}

	glEnable(GL_FRAMEBUFFER_SRGB);

	SMAAShaders[2]->Use();

	colorTex->Bind(0);
	SMAAFramebuffer->GetColorBuffer(1)->Bind(1); //bind weights texture to slot 0

	if (velocityTex)
	{
		velocityTex->Bind(2);
	}

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisable(GL_FRAMEBUFFER_SRGB);

	if (!bT2x)
		return 0;

	///////////////4. Temporal resolve (optional)
	writeFramebuffer->Bind(true);
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + writeColorBuffer);
	
	SMAAShaders[3]->Use();

	SMAAFramebuffer->GetColorBuffer(2)->Bind(0);
	previousColorTex->Bind(1);
	velocityTex->Bind(2);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	return writeFramebuffer->GetColorBuffer(writeColorBuffer).get();
}

const Texture* Postprocess::TonemapGammaPass(const Texture* colorTex, const Texture* blurTex, GEE_FB::Framebuffer* writeFramebuffer, bool bWriteToDefault) const
{
	if (bWriteToDefault)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDrawBuffer(GL_BACK);
	}
	else if (writeFramebuffer)
		writeFramebuffer->Bind();
	else
		TonemapGammaFramebuffer.Bind();

	glDisable(GL_DEPTH_TEST);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	TonemapGammaShader->Use();

	colorTex->Bind(0);
	if (blurTex)
		blurTex->Bind(1);
	TonemapGammaShader->Uniform1i("HDRbuffer", 0);

	glBindVertexArray(QuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	return (writeFramebuffer || bWriteToDefault) ? (nullptr) : (TonemapGammaFramebuffer.GetColorBuffer(0).get());
}

void Postprocess::Render(const Texture* colorTex, const Texture* blurTex, const Texture* depthTex, const Texture* velocityTex, GEE_FB::Framebuffer* finalFramebuffer) const
{
	if (Settings->bBloom && blurTex != 0)
		blurTex = GaussianBlur(blurTex, 10);

	if (Settings->AAType == AntiAliasingType::AA_SMAA1X)
	{
		const Texture* compositedTex = TonemapGammaPass(colorTex, blurTex);
		SMAAPass(compositedTex, depthTex, nullptr, nullptr, finalFramebuffer);
	}
	else if (Settings->AAType == AntiAliasingType::AA_SMAAT2X && velocityTex != 0)
	{
		const Texture* compositedTex = TonemapGammaPass(colorTex, blurTex);
		unsigned int previousFrameIndex = (static_cast<int>(FrameIndex) - 1) % StorageFramebuffer->GetNumberOfColorBuffers();
		const Texture* SMAAresult = SMAAPass(compositedTex, depthTex, StorageFramebuffer->GetColorBuffer(previousFrameIndex).get(), velocityTex, StorageFramebuffer.get(), FrameIndex, true);

		if (finalFramebuffer)
			finalFramebuffer->Bind();
		else
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDrawBuffer(GL_BACK);
		}
		glEnable(GL_FRAMEBUFFER_SRGB);
		QuadShader->Use();

		SMAAresult->Bind(0);

		glBindVertexArray(QuadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisable(GL_FRAMEBUFFER_SRGB);
		
		FrameIndex = (FrameIndex + 1) % StorageFramebuffer->GetNumberOfColorBuffers();
	}
	else
		TonemapGammaPass(colorTex, blurTex, finalFramebuffer, (finalFramebuffer == nullptr));
}

void Postprocess::Dispose()
{
	if (SSAOFramebuffer)
		SSAOFramebuffer->Dispose(true);
	if (StorageFramebuffer)
		StorageFramebuffer->Dispose(true);
	if (SMAAFramebuffer)
		SMAAFramebuffer->Dispose(true);
	TonemapGammaFramebuffer.Dispose(true);
	for (int i = 0; i < 2; i++)
		BlurFramebuffers[i].Dispose(true);
}
