#include <rendering/LightProbe.h>
#include <rendering/Framebuffer.h>
#include <rendering/Shader.h>
#include <rendering/RenderInfo.h>
#include <rendering/RenderToolbox.h>
#include <scene/LightProbeComponent.h>

namespace GEE
{
	void LightProbeLoader::LoadLightProbeFromFile(LightProbeComponent& probe, std::string filepath)
	{
		RenderEngineManager* renderHandle = probe.GetScene().GetGameHandle()->GetRenderEngineHandle();
		probe.Shape = EngineBasicShape::QUAD;

		Texture erTexture = Texture::Loader<float>::FromFile2D(filepath, Texture::Format::Float16::RGBA(), true, Texture::MinFilter::Bilinear(), Texture::MagFilter::Bilinear());	//load equirectangular hdr texture
		int layer = probe.GetProbeIndex() * 6;

		renderHandle->RenderCubemapFromTexture(probe.GetEnvironmentMap(), erTexture, Vec2u(1024), *renderHandle->FindShader("ErToCubemap"));
		probe.GetEnvironmentMap().Bind();
		probe.OptionalFilepath = filepath;
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		ConvoluteLightProbe(probe, probe.GetEnvironmentMap());
	}

	void LightProbeLoader::ConvoluteLightProbe(const LightProbeComponent& probe, const Texture& envMap)
	{
		RenderEngineManager* renderHandle = probe.GetScene().GetGameHandle()->GetRenderEngineHandle();
		LightProbeTextureArrays* probeTexArrays = probe.GetScene().GetRenderData()->GetProbeTexArrays();

		envMap.Bind();
		if (PrimitiveDebugger::bDebugProbeLoading)
		{
			std::cout << "Convoluting probe " << probe.GetProbeIndex() << ".\n";
			std::cout << "Irradiancing\n";
		}
		renderHandle->FindShader("CubemapToIrradiance")->Use();
		int layer = probe.GetProbeIndex() * 6;

		renderHandle->RenderCubemapFromTexture(probeTexArrays->IrradianceMapArr, envMap, Vec2u(16), *renderHandle->FindShader("CubemapToIrradiance"), &layer);

		if (PrimitiveDebugger::bDebugProbeLoading)
			std::cout << "Prefiltering\n";
		renderHandle->FindShader("CubemapToPrefilter")->Use();
		renderHandle->FindShader("CubemapToPrefilter")->Uniform1f("cubemapNr", static_cast<float>(probe.GetProbeIndex()));
		for (int mipmap = 0; mipmap < 5; mipmap++)
		{
			layer = probe.GetProbeIndex() * 6;
			renderHandle->FindShader("CubemapToPrefilter")->Uniform1f("roughness", static_cast<float>(mipmap) / 5.0f);
			renderHandle->RenderCubemapFromTexture(probeTexArrays->PrefilterMapArr, envMap, Vec2f(256.0f) / std::pow(2.0f, static_cast<float>(mipmap)), *renderHandle->FindShader("CubemapToPrefilter"), &layer, mipmap);
		}
		if (PrimitiveDebugger::bDebugProbeLoading)
			std::cout << "Ending\n";
	}

	void LightProbeLoader::LoadLightProbeTextureArrays(GameSceneRenderData* sceneRenderData)
	{
		RenderEngineManager* renderHandle = sceneRenderData->GetRenderHandle();
		LightProbeTextureArrays* probeTexArrays = sceneRenderData->GetProbeTexArrays();
		probeTexArrays->IrradianceMapArr = NamedTexture(Texture::Loader<float>::ReserveEmptyCubemapArray(Vec3u(16, 16, 8), Texture::Format::Float16::RGB()), "Irradiance cubemap array");
		probeTexArrays->PrefilterMapArr = NamedTexture(Texture::Loader<float>::ReserveEmptyCubemapArray(Vec3u(256, 256, 8), Texture::Format::Float16::RGB()), "Prefilter cubemap array");
		probeTexArrays->PrefilterMapArr.SetMinFilter(Texture::MinFilter::Trilinear(), true, true);

		probeTexArrays->BRDFLut = NamedTexture(Texture::Loader<float>::ReserveEmpty2D(Vec2u(512), Texture::Format::Float16::RGB()), "brdfLutTex");
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		GEE_FB::Framebuffer framebuffer;
		framebuffer.AttachTextures(probeTexArrays->BRDFLut);
		framebuffer.Bind(true);
		renderHandle->FindShader("BRDFLutGeneration")->Use();
		renderHandle->RenderStaticMesh(RenderInfo(*renderHandle->GetCurrentTbCollection()), renderHandle->GetBasicShapeMesh(EngineBasicShape::QUAD), Transform(), renderHandle->FindShader("BRDFLutGeneration"));

		framebuffer.Dispose();
	}
	/*
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
	}*/

	LightProbeVolume::LightProbeVolume(const LightProbeComponent& probe) :
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

	LightProbeTextureArrays::LightProbeTextureArrays() :
		NextLightProbeNr(0)
	{
	}

}