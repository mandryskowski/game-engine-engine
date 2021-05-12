#include <rendering/LightProbe.h>
#include <rendering/Framebuffer.h>
#include <rendering/Shader.h>
#include <rendering/RenderInfo.h>
#include <rendering/Mesh.h>
#include <rendering/RenderToolbox.h>
#include <scene/LightProbeComponent.h>

LightProbe::LightProbe(GameSceneRenderData* sceneRenderData)
{
	SceneRenderData = sceneRenderData;
	ProbeNr = sceneRenderData->GetProbeTexArrays()->NextLightProbeNr++;
	EnvironmentMap = *GEE_FB::reserveColorBuffer(glm::uvec2(1024, 1024), GL_RGB16F, GL_FLOAT, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_TEXTURE_CUBE_MAP, 0, "Environment cubemap");
	RenderHandle = sceneRenderData->GetRenderHandle();
}

EngineBasicShape LightProbe::GetShape() const
{
	return EngineBasicShape::QUAD;
}

Transform LightProbe::GetTransform() const
{
	return Transform();
}

Shader* LightProbe::GetRenderShader(const RenderToolboxCollection& renderCol) const
{
	return renderCol.FindShader("CookTorranceIBL");
}

void LightProbeLoader::LoadLightProbeFromFile(LightProbeComponent& probe, std::string filepath)
{
	RenderEngineManager* renderHandle = probe.GetScene().GetGameHandle()->GetRenderEngineHandle();
	probe.Shape = EngineBasicShape::QUAD;

	Texture erTexture = textureFromFile<float>(filepath, GL_RGBA16F, GL_LINEAR, GL_LINEAR, 1);	//load equirectangular hdr texture
	int layer = probe.GetProbeIndex() * 6;

	renderHandle->RenderCubemapFromTexture(probe.GetEnvironmentMap(), erTexture, glm::uvec2(1024), *renderHandle->FindShader("ErToCubemap"));
	probe.GetEnvironmentMap().Bind();
	probe.OptionalFilepath = filepath;
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	ConvoluteLightProbe(probe, &probe.GetEnvironmentMap());
}

void LightProbeLoader::ConvoluteLightProbe(const LightProbeComponent& probe, Texture* envMap)
{
	RenderEngineManager* renderHandle = probe.GetScene().GetGameHandle()->GetRenderEngineHandle();
	LightProbeTextureArrays* probeTexArrays = probe.GetScene().GetRenderData()->GetProbeTexArrays();

	envMap->Bind();
	if (PrimitiveDebugger::bDebugProbeLoading)
	{
		std::cout << "Convoluting probe " << probe.GetProbeIndex() << ".\n";
		std::cout << "Irradiancing\n";
	}
	renderHandle->FindShader("CubemapToIrradiance")->Use();
	int layer = probe.GetProbeIndex() * 6;

	renderHandle->RenderCubemapFromTexture(probeTexArrays->IrradianceMapArr, *envMap, glm::uvec2(16), *renderHandle->FindShader("CubemapToIrradiance"), &layer);

	if (PrimitiveDebugger::bDebugProbeLoading)
		std::cout << "Prefiltering\n";
	renderHandle->FindShader("CubemapToPrefilter")->Use();
	renderHandle->FindShader("CubemapToPrefilter")->Uniform1f("cubemapNr", static_cast<float>(probe.GetProbeIndex()));
	for (int mipmap = 0; mipmap < 5; mipmap++)
	{
		layer = probe.GetProbeIndex()* 6;
		renderHandle->FindShader("CubemapToPrefilter")->Uniform1f("roughness", static_cast<float>(mipmap) / 5.0f);
		renderHandle->RenderCubemapFromTexture(probeTexArrays->PrefilterMapArr, *envMap, glm::vec2(256.0f) / std::pow(2.0f, static_cast<float>(mipmap)), *renderHandle->FindShader("CubemapToPrefilter"), &layer, mipmap);
	}
	if (PrimitiveDebugger::bDebugProbeLoading)
		std::cout << "Ending\n";
}

void LightProbeLoader::LoadLightProbeTextureArrays(GameSceneRenderData* sceneRenderData)
{
	RenderEngineManager* renderHandle = sceneRenderData->GetRenderHandle();
	LightProbeTextureArrays* probeTexArrays = sceneRenderData->GetProbeTexArrays();
	probeTexArrays->IrradianceMapArr = *GEE_FB::reserveColorBuffer(glm::uvec3(16, 16, 8), GL_RGB16F, GL_FLOAT, GL_LINEAR, GL_LINEAR, GL_TEXTURE_CUBE_MAP_ARRAY, 0, "Irradiance cubemap array");
	probeTexArrays->PrefilterMapArr = *GEE_FB::reserveColorBuffer(glm::uvec3(256, 256, 8), GL_RGB16F, GL_FLOAT, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_TEXTURE_CUBE_MAP_ARRAY, 0, "Prefilter cubemap array");
	probeTexArrays->PrefilterMapArr.Bind();
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP_ARRAY);

	probeTexArrays->BRDFLut = *GEE_FB::reserveColorBuffer(glm::uvec2(512), GL_RGB16F, GL_FLOAT, GL_LINEAR, GL_LINEAR, GL_TEXTURE_2D);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	GEE_FB::Framebuffer framebuffer;
	framebuffer.SetAttachments(glm::uvec2(512), GEE_FB::FramebufferAttachment(probeTexArrays->BRDFLut));
	framebuffer.Bind(true);
	renderHandle->FindShader("BRDFLutGeneration")->Use();
	renderHandle->RenderStaticMesh(RenderInfo(*renderHandle->GetCurrentTbCollection()), renderHandle->GetBasicShapeMesh(EngineBasicShape::QUAD), Transform(), renderHandle->FindShader("BRDFLutGeneration"));

	framebuffer.Dispose();
}

LocalLightProbe::LocalLightProbe(GameSceneRenderData* sceneRenderData, Transform transform, EngineBasicShape shape):
	LightProbe(sceneRenderData),
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

LightProbeVolume::LightProbeVolume(const LightProbeComponent& probe):
	ProbePtr(&probe)
{
}

EngineBasicShape LightProbeVolume::GetShape() const
{
	return ProbePtr->GetShape();
}

Transform LightProbeVolume::GetRenderTransform() const
{
	return ProbePtr->GetTransform().GetWorldTransform();
}

Shader* LightProbeVolume::GetRenderShader(const RenderToolboxCollection& renderCol) const
{
	return ProbePtr->GetRenderShader(renderCol);
}

void LightProbeVolume::SetupRenderUniforms(const Shader& shader) const
{
	shader.Uniform1f("lightProbeNr", ProbePtr->GetProbeIndex());
}

LightProbeTextureArrays::LightProbeTextureArrays():
	NextLightProbeNr(0)
{
}
