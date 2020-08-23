#include "LightProbe.h"
#include "Framebuffer.h"
#include "Shader.h"
#include "Mesh.h"

Texture LightProbeTextureArrays::IrradianceMapArr = Texture();
Texture LightProbeTextureArrays::PrefilterMapArr = Texture();
Texture LightProbeTextureArrays::BRDFLut = Texture();
unsigned int LightProbeTextureArrays::NextLightProbeNr = 0;
RenderEngineManager* LightProbeLoader::RenderHandle = nullptr;

LightProbe::LightProbe()
{
	ProbeNr = LightProbeTextureArrays::NextLightProbeNr++;
	EnvironmentMap = *GEE_FB::reserveColorBuffer(glm::uvec2(1024, 1024), GL_RGB16F, GL_FLOAT, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_TEXTURE_CUBE_MAP, 0, "Environment cubemap");
	RenderHandle = LightProbeLoader::RenderHandle;
}

EngineBasicShape LightProbe::GetShape() const
{
	return EngineBasicShape::QUAD;
}

Transform LightProbe::GetTransform() const
{
	return Transform();
}

Shader* LightProbe::GetRenderShader() const
{
	return RenderHandle->FindShader("CookTorranceIBL");
}

std::shared_ptr<LightProbe> LightProbeLoader::LightProbeFromFile(std::string path)
{
	std::shared_ptr<LightProbe> probe = std::make_shared<LightProbe>();
	Texture erTexture = textureFromFile<float>(path, GL_RGBA16F, GL_LINEAR, GL_LINEAR, 1);	//load equirectangular hdr texture
	int layer = probe->ProbeNr * 6;

	RenderHandle->RenderCubemapFromTexture(probe->EnvironmentMap, erTexture, glm::uvec2(1024), *RenderHandle->FindShader("ErToCubemap"));
	probe->EnvironmentMap.Bind();
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	ConvoluteLightProbe(*probe, &probe->EnvironmentMap);
	

	return probe;
}

void LightProbeLoader::ConvoluteLightProbe(const LightProbe& probe, Texture* envMap)
{
	envMap->Bind();
	if (PrimitiveDebugger::bDebugProbeLoading)
	{
		std::cout << "Convoluting probe " << probe.ProbeNr << ".\n";
		std::cout << "Irradiancing\n";
	}
	RenderHandle->FindShader("CubemapToIrradiance")->Use();
	int layer = probe.ProbeNr * 6;
	RenderHandle->RenderCubemapFromTexture(LightProbeTextureArrays::IrradianceMapArr, *envMap, glm::uvec2(16), *RenderHandle->FindShader("CubemapToIrradiance"), &layer);

	if (PrimitiveDebugger::bDebugProbeLoading)
		std::cout << "Prefiltering\n";
	RenderHandle->FindShader("CubemapToPrefilter")->Use();
	RenderHandle->FindShader("CubemapToPrefilter")->Uniform1f("cubemapNr", static_cast<float>(probe.ProbeNr));
	for (int mipmap = 0; mipmap < 5; mipmap++)
	{
		layer = probe.ProbeNr * 6;
		RenderHandle->FindShader("CubemapToPrefilter")->Uniform1f("roughness", static_cast<float>(mipmap) / 5.0f);
		RenderHandle->RenderCubemapFromTexture(LightProbeTextureArrays::PrefilterMapArr, *envMap, glm::vec2(256.0f) / std::pow(2.0f, static_cast<float>(mipmap)), *RenderHandle->FindShader("CubemapToPrefilter"), &layer, mipmap);
	}
	if (PrimitiveDebugger::bDebugProbeLoading)
		std::cout << "Ending\n";
}

void LightProbeLoader::LoadLightProbeTextureArrays()
{
	LightProbeTextureArrays::IrradianceMapArr = *GEE_FB::reserveColorBuffer(glm::uvec3(16, 16, 8), GL_RGB16F, GL_FLOAT, GL_LINEAR, GL_LINEAR, GL_TEXTURE_CUBE_MAP_ARRAY, 0, "Irradiance cubemap array");
	LightProbeTextureArrays::PrefilterMapArr = *GEE_FB::reserveColorBuffer(glm::uvec3(256, 256, 8), GL_RGB16F, GL_FLOAT, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_TEXTURE_CUBE_MAP_ARRAY, 0, "Prefilter cubemap array");
	LightProbeTextureArrays::PrefilterMapArr.Bind();
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP_ARRAY);

	LightProbeTextureArrays::BRDFLut = *GEE_FB::reserveColorBuffer(glm::uvec2(512), GL_RGB16F, GL_FLOAT, GL_LINEAR, GL_LINEAR, GL_TEXTURE_2D);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	GEE_FB::Framebuffer framebuffer;
	framebuffer.SetAttachments(glm::uvec2(512), GEE_FB::FramebufferAttachment(LightProbeTextureArrays::BRDFLut));
	framebuffer.Bind(true);
	LightProbeLoader::RenderHandle->FindShader("BRDFLutGeneration")->Use();
	LightProbeLoader::RenderHandle->Render(RenderInfo(), LightProbeLoader::RenderHandle->GetBasicShapeMesh(EngineBasicShape::QUAD), Transform(), LightProbeLoader::RenderHandle->FindShader("BRDFLutGeneration"));

	framebuffer.Dispose();
}

LocalLightProbe::LocalLightProbe(Transform transform, EngineBasicShape shape):
	LightProbe(),
	Shape(shape),
	ProbeTransform(transform)
{
}

EngineBasicShape LocalLightProbe::GetShape() const
{
	return Shape;
}

Transform LocalLightProbe::GetTransform() const
{
	return ProbeTransform;
}

LightProbeVolume::LightProbeVolume(const LightProbe& probe):
	ProbePtr(&probe)
{
}

EngineBasicShape LightProbeVolume::GetShape() const
{
	return ProbePtr->GetShape();
}

Transform LightProbeVolume::GetRenderTransform() const
{
	return ProbePtr->GetTransform();
}

Shader* LightProbeVolume::GetRenderShader() const
{
	return ProbePtr->GetRenderShader();
}

void LightProbeVolume::SetupRenderUniforms(const Shader& shader) const
{
	shader.Uniform1f("lightProbeNr", ProbePtr->ProbeNr);
}