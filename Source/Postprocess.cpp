#include "Postprocess.h"
#include "SMAA/AreaTex.h"
#include "SMAA/SearchTex.h"
#include <random>

Postprocess::Postprocess():
	QuadVAO(0),
	QuadVBO(0),
	SMAAAreaTex(0),
	SMAASearchTex(0),
	SSAOShader(nullptr),
	SSAOFramebuffer(nullptr),
	SSAONoiseTex(nullptr)
{
}

void Postprocess::Init(const GameSettings* settings)
{
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

	TonemapGammaShader.LoadShaders("Shaders/tonemap_gamma.vs", "Shaders/tonemap_gamma.fs");
	TonemapGammaShader.Use();
	TonemapGammaShader.Uniform1i("HDRbuffer", 0);
	TonemapGammaShader.Uniform1i("brightnessBuffer", 1);
	TonemapGammaShader.Uniform1f("gamma", settings->MonitorGamma);

	GaussianBlurShader.LoadShaders("Shaders/gaussianblur.vs", "Shaders/gaussianblur.fs");
	GaussianBlurShader.Use();
	GaussianBlurShader.Uniform1i("tex", 0);

	std::string smaaDefines =   "#define SCR_WIDTH " + std::to_string(settings->WindowSize.x) + ".0 \n" +
								"#define SCR_HEIGHT " + std::to_string(settings->WindowSize.y) + ".0 \n" +
								"#define SMAA_PRESET_ULTRA 1" + "\n";
	
	SMAAShaders[0].LoadShadersWithInclData(smaaDefines, "SMAA/edge.vs", "SMAA/edge.fs");
	SMAAShaders[0].Use();
	SMAAShaders[0].Uniform1i("colorTex", 0);
	SMAAShaders[0].Uniform1i("depthTex", 1);

	SMAAShaders[1].LoadShadersWithInclData(smaaDefines, "SMAA/blend.vs", "SMAA/blend.fs");
	SMAAShaders[1].Use();
	SMAAShaders[1].Uniform1i("edgesTex", 0);
	SMAAShaders[1].Uniform1i("areaTex", 1);
	SMAAShaders[1].Uniform1i("searchTex", 2);

	SMAAAreaTex = textureFromBuffer(*areaTexBytes, AREATEX_WIDTH, AREATEX_HEIGHT, GL_RG, GL_RG, GL_NEAREST, GL_NEAREST);
	SMAASearchTex = textureFromBuffer(*searchTexBytes, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, GL_R8, GL_RED, GL_NEAREST, GL_NEAREST);

	SMAAShaders[2].LoadShadersWithInclData(smaaDefines, "SMAA/neighborhood.vs", "SMAA/neighborhood.fs");
	SMAAShaders[2].Use();
	SMAAShaders[2].Uniform1i("colorTex", 0);
	SMAAShaders[2].Uniform1i("blendTex", 1);

	//load framebuffers
	SMAAFramebuffer.Load(settings->WindowSize, std::vector<ColorBufferData>{ ColorBufferData(GL_TEXTURE_2D, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR), ColorBufferData(GL_TEXTURE_2D, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR) });
	TonemapGammaFramebuffer.Load(settings->WindowSize, ColorBufferData(GL_TEXTURE_2D, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR));

	for (int i = 0; i < 2; i++)
		BlurFramebuffers[i].Load(settings->WindowSize, ColorBufferData(GL_TEXTURE_2D, GL_RGB16F, GL_FLOAT, GL_LINEAR, GL_LINEAR));

	if (settings->AmbientOcclusionSamples > 0)
	{
		SSAOShader = std::make_unique<Shader>();
		SSAOShader->LoadShadersWithInclData("#define SSAO_SAMPLES " + std::to_string(settings->AmbientOcclusionSamples) + "\n", "Shaders/ssao.vs", "Shaders/ssao.fs");
		SSAOShader->SetExpectedMatrices(std::vector<MatrixType> {MatrixType::PROJECTION});
		SSAOShader->Use();
		SSAOShader->Uniform1f("radius", 0.5f);
		SSAOShader->Uniform1i("gPosition", 0);
		SSAOShader->Uniform1i("gNormal", 1);
		SSAOShader->Uniform1i("noiseTex", 2);
		SSAOShader->Uniform2fv("resolution", settings->WindowSize);

		SSAOFramebuffer = std::make_unique<Framebuffer>();
		SSAOFramebuffer->Load(settings->WindowSize, ColorBufferData(GL_TEXTURE_2D, GL_RED, GL_FLOAT, GL_LINEAR, GL_LINEAR));

		std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
		std::default_random_engine generator;

		std::vector<glm::vec3> ssaoSamples;
		ssaoSamples.reserve(settings->AmbientOcclusionSamples);
		for (unsigned int i = 0; i < settings->AmbientOcclusionSamples; i++)
		{
			glm::vec3 sample(
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator)
			);

			sample = glm::normalize(sample) * randomFloats(generator);

			float scale = (float)i / (float)settings->AmbientOcclusionSamples;
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

		SSAONoiseTex = std::make_unique<unsigned int>();
		glGenTextures(1, SSAONoiseTex.get());
		glBindTexture(GL_TEXTURE_2D, *SSAONoiseTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
}

unsigned int Postprocess::GaussianBlur(unsigned int tex, int passes, Framebuffer* writeFramebuffer) const	//Implemented using ping-pong framebuffers (one gaussian blur pass is separable into two lower-cost passes) and linear filtering for performance
{
	if (passes == 0)
		return tex;

	GaussianBlurShader.Use();
	glBindVertexArray(QuadVAO);
	glActiveTexture(GL_TEXTURE0);

	bool horizontal = true;

	for (int i = 0; i < passes; i++)
	{
		if (i == passes - 1 && writeFramebuffer)	//If the calling function specified the framebuffer to write the final result to, bind the given framebuffer for the last pass.
			writeFramebuffer->Bind();
		else	//For any pass that doesn't match given criteria, just switch the ping pong fbos
		{
			BlurFramebuffers[horizontal].Bind();
			glBindTexture(GL_TEXTURE_2D, (i == 0) ? (tex) : (BlurFramebuffers[!horizontal].GetColorBuffer(0).OpenGLBuffer));
		}
		GaussianBlurShader.Uniform1i("horizontal", horizontal);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		horizontal = !horizontal;
	}

	return (writeFramebuffer) ? (writeFramebuffer->GetColorBuffer(0).OpenGLBuffer) : (BlurFramebuffers[!horizontal].GetColorBuffer(0).OpenGLBuffer);
}

unsigned int Postprocess::SSAOPass(RenderInfo& info, unsigned int gPosition, unsigned int gNormal)
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


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, *SSAONoiseTex);

	SSAOShader->Use();
	SSAOShader->UniformMatrix4fv("view", *info.view);
	SSAOShader->UniformMatrix4fv("projection", *info.projection);

	glBindVertexArray(QuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	GaussianBlur(SSAOFramebuffer->GetColorBuffer(0).OpenGLBuffer, 4, SSAOFramebuffer.get());
	return SSAOFramebuffer->GetColorBuffer(0).OpenGLBuffer;
}

unsigned int Postprocess::SMAAPass(unsigned int colorTex, unsigned int depthTex) const
{
	SMAAFramebuffer.Bind();

	/////////////////1. Edge detection
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	SMAAShaders[0].Use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthTex);

	glBindVertexArray(QuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	///////////////2. Weight blending
	glDrawBuffer(GL_COLOR_ATTACHMENT1);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	SMAAShaders[1].Use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, SMAAFramebuffer.GetColorBuffer(0).OpenGLBuffer);	//bind edges texture to slot 0
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, SMAAAreaTex);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, SMAASearchTex);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	///////////////3. Neighborhood blending
	glEnable(GL_FRAMEBUFFER_SRGB);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	SMAAShaders[2].Use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, SMAAFramebuffer.GetColorBuffer(1).OpenGLBuffer);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisable(GL_FRAMEBUFFER_SRGB);
	return 0;
}

unsigned int Postprocess::TonemapGammaPass(unsigned int colorTex, unsigned int blurTex, bool bFinal) const
{
	(bFinal) ? (glBindFramebuffer(GL_FRAMEBUFFER, 0)) : (TonemapGammaFramebuffer.Bind());
	glDisable(GL_DEPTH_TEST);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	TonemapGammaShader.Use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, blurTex);
	TonemapGammaShader.Uniform1i("HDRbuffer", 0);

	glBindVertexArray(QuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	return (bFinal) ? (0) : (TonemapGammaFramebuffer.GetColorBuffer(0).OpenGLBuffer);
}

void Postprocess::Render(unsigned int colorTex, unsigned int blurTex, unsigned int depthTex, const GameSettings* settings) const
{
	if (settings->bBloom)
		blurTex = GaussianBlur(blurTex, 10);

	if (settings->AAType == AntiAliasingType::AA_SMAA)
	{
		unsigned int compositedTex = TonemapGammaPass(colorTex, blurTex);
		SMAAPass(compositedTex, depthTex);
	}
	else
		TonemapGammaPass(colorTex, blurTex, true);
}
