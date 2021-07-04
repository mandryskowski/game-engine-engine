#include <rendering/Postprocess.h>
#include "../SMAA/AreaTex.h"
#include "../SMAA/SearchTex.h"
#include <rendering/Mesh.h>
#include <rendering/RenderInfo.h>
#include <rendering/RenderToolbox.h>
#include <random>

namespace GEE
{
	using namespace GEE_FB;

	Postprocess::Postprocess() :
		FrameIndex(0),
		RenderHandle(nullptr)
	{
	}

	void Postprocess::Init(GameManager* gameHandle, Vec2u resolution)
	{
		RenderHandle = gameHandle->GetRenderEngineHandle();

		QuadShader = RenderHandle->AddShader(ShaderLoader::LoadShaders("Quad", "Shaders/quad.vs", "Shaders/quad.fs"));
		QuadShader->Use();
		QuadShader->Uniform1i("tex", 0);
	}

	unsigned int Postprocess::GetFrameIndex()
	{
		return FrameIndex;
	}

	Mat4f Postprocess::GetJitterMat(const GameSettings::VideoSettings& usedSettings, int optionalIndex)
	{
		Vec2f jitter(0.0f);

		switch (usedSettings.AAType)
		{
		case AA_NONE:
		case AA_SMAA1X:
		default:
			return Mat4f(1.0f);
		case AA_SMAAT2X:
			Vec2f jitters[] = {
				Vec2f(-0.25f, 0.25f),
				Vec2f(0.25f, -0.25f)
			};
			jitter = jitters[((optionalIndex >= 0) ? (optionalIndex) : (FrameIndex))];
			break;
		}

		jitter /= static_cast<Vec2f>(usedSettings.Resolution) * 0.5f;
		return glm::translate(Mat4f(1.0f), Vec3f(jitter, 0.0f));
	}

	const Texture Postprocess::GaussianBlur(PPToolbox<GaussianBlurToolbox> ppTb, const GEE_FB::Framebuffer& writeFramebuffer, const Viewport* viewport, const Texture& tex, int passes, unsigned int writeColorBuffer) const	//Implemented using ping-pong framebuffers (one gaussian blur pass is separable into two lower-cost passes) and linear filtering for performance
	{
		if (passes == 0)
			return tex;

		GaussianBlurToolbox& tb = ppTb.GetTb();

		tb.GaussianBlurShader->Use();
		glActiveTexture(GL_TEXTURE0);

		bool horizontal = true;

		for (int i = 0; i < passes; i++)
		{
			if (i == passes - 1)	//If the calling function specified the framebuffer to write the final result to, bind the given framebuffer for the last pass.
			{
				writeFramebuffer.Bind();
				writeFramebuffer.SetDrawSlot(writeColorBuffer);
			}
			else	//For any pass that doesn't match given criteria, just switch the ping pong fbos
			{
				tb.BlurFramebuffers[horizontal]->Bind();
				const Texture& bindTex = (i == 0) ? (tex) : (tb.BlurFramebuffers[!horizontal]->GetColorTexture(0));
				bindTex.Bind();
			}
			tb.GaussianBlurShader->Uniform1i("horizontal", horizontal);
			ppTb.RenderFullscreenQuad(tb.GaussianBlurShader, false);

			horizontal = !horizontal;
		}

		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		tb.BlurFramebuffers[!horizontal]->GetColorTexture(0);
		return writeFramebuffer.GetColorTexture(writeColorBuffer);
	}

	const Texture Postprocess::SSAOPass(RenderInfo& info, const Texture& gPosition, const Texture& gNormal)
	{
		SSAOToolbox* tb;
		if (!(tb = info.TbCollection.GetTb<SSAOToolbox>()))
		{
			std::cerr << "ERROR! Attempted to render SSAO image without calling the setup\n";
			return Texture();
		}

		tb->SSAOFb->Bind();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		gPosition.Bind(0);
		gNormal.Bind(1);
		tb->SSAONoiseTex->Bind(2);

		tb->SSAOShader->Use();
		tb->SSAOShader->UniformMatrix4fv("view", info.view);
		tb->SSAOShader->UniformMatrix4fv("projection", info.projection);

		RenderFullscreenQuad(info, tb->SSAOShader, false);

		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		GaussianBlur(GetPPToolbox<GaussianBlurToolbox>(info.TbCollection), *tb->SSAOFb, nullptr, tb->SSAOFb->GetColorTexture(0), 4);
		return tb->SSAOFb->GetColorTexture(0);
	}

	const Texture Postprocess::SMAAPass(PPToolbox<SMAAToolbox> ppTb, const GEE_FB::Framebuffer& writeFramebuffer, const Viewport* viewport, const Texture& colorTex, const Texture& depthTex, const Texture& previousColorTex, const Texture& velocityTex, unsigned int writeColorBuffer, bool bT2x) const
	{
		SMAAToolbox& tb = ppTb.GetTb();
		tb.SMAAFb->Bind();

		/////////////////1. Edge detection
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		tb.SMAAShaders[0]->Use();
		colorTex.Bind(0);
		depthTex.Bind(1);

		ppTb.RenderFullscreenQuad(tb.SMAAShaders[0]);

		///////////////2. Weight blending
		glDrawBuffer(GL_COLOR_ATTACHMENT1);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		tb.SMAAShaders[1]->Use();
		if (bT2x)
			tb.SMAAShaders[1]->Uniform4fv("ssIndices", (FrameIndex == 0) ? (Vec4f(1, 1, 1, 0)) : (Vec4f(2, 2, 2, 0)));

		tb.SMAAFb->GetColorTexture(0).Bind(0);	//bind edges texture to slot 0
		tb.SMAAAreaTex->Bind(1);
		tb.SMAASearchTex->Bind(2);

		ppTb.RenderFullscreenQuad(tb.SMAAShaders[1]);

		///////////////3. Neighborhood blending
		if (bT2x)
		{
			glDrawBuffer(GL_COLOR_ATTACHMENT2);
		}
		else
		{
			if (viewport) writeFramebuffer.Bind(*viewport);
			else writeFramebuffer.Bind(true);
			writeFramebuffer.SetDrawSlot(writeColorBuffer);
		}

		//glEnable(GL_FRAMEBUFFER_SRGB);

		tb.SMAAShaders[2]->Use();

		colorTex.Bind(0);
		tb.SMAAFb->GetColorTexture(1).Bind(1); //bind weights texture to slot 0

		velocityTex.Bind(2);

		ppTb.RenderFullscreenQuad(tb.SMAAShaders[2]);
		//glDisable(GL_FRAMEBUFFER_SRGB);

		if (!bT2x)
			return Texture();
		if (!velocityTex.HasBeenGenerated())
			return Texture();

		///////////////4. Temporal resolve (optional)
		if (viewport) writeFramebuffer.Bind(*viewport);
		else writeFramebuffer.Bind(true);
		writeFramebuffer.SetDrawSlot(writeColorBuffer);

		tb.SMAAShaders[3]->Use();

		tb.SMAAFb->GetColorTexture(2).Bind(0);
		previousColorTex.Bind(1);
		velocityTex.Bind(2);

		ppTb.RenderFullscreenQuad(tb.SMAAShaders[3]);

		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		return writeFramebuffer.GetColorTexture(writeColorBuffer);
	}

	const Texture Postprocess::TonemapGammaPass(PPToolbox<ComposedImageStorageToolbox> tb, const GEE_FB::Framebuffer& writeFramebuffer, const Viewport* viewport, const Texture& colorTex, const Texture& blurTex) const
	{
		if (viewport) writeFramebuffer.Bind(*viewport);
		else writeFramebuffer.Bind(true);
		writeFramebuffer.SetDrawSlot(0);

		glDisable(GL_DEPTH_TEST);
		//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT);

		tb.GetTb().TonemapGammaShader->Use();

		colorTex.Bind(0);
		if (blurTex.HasBeenGenerated())
			blurTex.Bind(1);
		tb.GetTb().TonemapGammaShader->Uniform1i("HDRbuffer", 0);

		tb.RenderFullscreenQuad(tb.GetTb().TonemapGammaShader);

		return writeFramebuffer.GetColorTexture(0);
	}

	void Postprocess::Render(RenderToolboxCollection& tbCollection, const GEE_FB::Framebuffer& finalFramebuffer, const Viewport* viewport, const Texture& colorTex, Texture blurTex, const Texture& depthTex, const Texture& velocityTex) const
	{
		const GameSettings::VideoSettings& settings = tbCollection.GetSettings();

		if (settings.bBloom && blurTex.HasBeenGenerated())
			blurTex = GaussianBlur(GetPPToolbox<GaussianBlurToolbox>(tbCollection), *tbCollection.GetTb<GaussianBlurToolbox>()->BlurFramebuffers[0], nullptr, blurTex, 10);

		if (settings.AAType == AntiAliasingType::AA_SMAA1X)
		{
			const Texture& compositedTex = TonemapGammaPass(GetPPToolbox<ComposedImageStorageToolbox>(tbCollection), *tbCollection.GetTb<ComposedImageStorageToolbox>()->ComposedImageFb, nullptr, colorTex, blurTex);
			SMAAPass(GetPPToolbox<SMAAToolbox>(tbCollection), finalFramebuffer, viewport, compositedTex, depthTex);
		}
		else if (settings.AAType == AntiAliasingType::AA_SMAAT2X && velocityTex.HasBeenGenerated())
		{
			PrevFrameStorageToolbox* storageTb = tbCollection.GetTb<PrevFrameStorageToolbox>();
			const Texture& compositedTex = TonemapGammaPass(GetPPToolbox<ComposedImageStorageToolbox>(tbCollection), *tbCollection.GetTb<ComposedImageStorageToolbox>()->ComposedImageFb, nullptr, colorTex, blurTex);
			unsigned int previousFrameIndex = (static_cast<int>(FrameIndex) - 1) % storageTb->StorageFb->GetColorTextureCount();
			const Texture& SMAAresult = SMAAPass(GetPPToolbox<SMAAToolbox>(tbCollection), *storageTb->StorageFb, nullptr, compositedTex, depthTex, storageTb->StorageFb->GetColorTexture(previousFrameIndex), velocityTex, FrameIndex, true);

			if (viewport) finalFramebuffer.Bind(*viewport);
			else finalFramebuffer.Bind(true);

			finalFramebuffer.SetDrawSlot(0);
			glEnable(GL_FRAMEBUFFER_SRGB);
			QuadShader->Use();

			SMAAresult.Bind(0);

			RenderFullscreenQuad(tbCollection);
			glDisable(GL_FRAMEBUFFER_SRGB);

			FrameIndex = (FrameIndex + 1) % storageTb->StorageFb->GetColorTextureCount();
		}
		else
			TonemapGammaPass(GetPPToolbox<ComposedImageStorageToolbox>(tbCollection), finalFramebuffer, viewport, colorTex, blurTex);
	}

	void Postprocess::Dispose()
	{
		/*if (SSAOFramebuffer)
			;// SSAOFramebuffer->Dispose(true);
		if (StorageFramebuffer)
			StorageFramebuffer->Dispose(true);
		if (SMAAFramebuffer)
			SMAAFramebuffer->Dispose(true);*/
			//TonemapGammaFramebuffer.Dispose(true);
		for (int i = 0; i < 2; i++)
			;//RenderHandle->GetCurrentTbCollection()->GetTb<GaussianBlurToolbox>()->BlurFramebuffers[i]->Dispose(true);
	}
}