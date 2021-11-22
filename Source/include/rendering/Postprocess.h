#pragma once
#include <game/GameSettings.h>
#include <game/GameManager.h>
#include <rendering/RenderInfo.h>
#include <rendering/Mesh.h>
#include "Framebuffer.h"
#include "Texture.h"

namespace GEE
{
	class RenderInfo;
	class GaussianBlurToolbox;
	class SMAAToolbox;
	class ComposedImageStorageToolbox;
	class Postprocess;

	template<typename ToolboxType> class PPToolbox
	{
	public:
		PPToolbox(const Postprocess& postprocessRef, RenderToolboxCollection& collection) : PostprocessRef(postprocessRef), TbCollection(collection), Tb(*collection.GetTb<ToolboxType>()) {}
		ToolboxType& GetTb() { return Tb; }
		void RenderFullscreenQuad(Shader* shader, bool useShader = true) { PostprocessRef.RenderFullscreenQuad(TbCollection, shader); }	//Pass nullptr to render with default Quad shader (which just renders the texture at slot 0)

	private:
		const Postprocess& PostprocessRef;
		RenderToolboxCollection& TbCollection;	//for rendering quad
		ToolboxType& Tb;
	};

	class Postprocess : public PostprocessManager
	{
		RenderEngineManager* RenderHandle;

		friend class RenderEngine;	//usun to

		SharedPtr<Shader> QuadShader;

		mutable unsigned int FrameIndex;

	public:
		Postprocess();
		void Init(GameManager* gameHandle, Vec2u resolution);

		virtual unsigned int GetFrameIndex() override;
		virtual Mat4f GetJitterMat(const GameSettings::VideoSettings& usedSettings, int optionalIndex = -1) override;	//dont pass anything to receive the current jitter matrix
		template <typename ToolboxType> PPToolbox<ToolboxType> GetPPToolbox(RenderToolboxCollection& toolboxCol) const
		{
			return PPToolbox<ToolboxType>(*this, toolboxCol);
		}

	/*	static Texture GaussianBlur(PPToolbox<GaussianBlurToolbox> tb, const GEE_FB::Framebuffer& writeFramebuffer,
		                            const Viewport* viewport, const Texture& tex, int passes,
		                            unsigned int writeColorBuffer = 0);
		Texture SSAOPass(RenderInfo&, const Texture& gPosition, const Texture& gNormal) const;
		Texture SMAAPass(PPToolbox<SMAAToolbox> tb, const GEE_FB::Framebuffer& writeFramebuffer,
		                 const Viewport* viewport, const Texture& colorTex, const Texture& depthTex,
		                 const Texture& previousColorTex = Texture(), const Texture& velocityTex = Texture(),
		                 unsigned int writeColorBuffer = 0, bool bT2x = false) const;
		static Texture TonemapGammaPass(PPToolbox<ComposedImageStorageToolbox> tbCollection,
		                                const GEE_FB::Framebuffer& writeFramebuffer, const Viewport* viewport,
		                                const Texture& colorTex, const Texture& blurTex);	//converts from linear to gamma and from HDR data to LDR
		void Render(RenderToolboxCollection& tbCollection, const GEE_FB::Framebuffer& finalFramebuffer, const Viewport* viewport, const Texture& colorTex, Texture blurTex = Texture(), const Texture& depthTex = Texture(), const Texture& velocityTex = Texture()) const;
		void RenderFullscreenQuad(RenderToolboxCollection& tbCollection, Shader* shader = nullptr, bool useShader = true) const
		{
			TbInfo<MatrixInfoExt> info(tbCollection, Mat4f(1.0f), Mat4f(1.0f), Vec3f(0.0f));
			info.SetUseMaterials(false);
			RenderFullscreenQuad(info, shader, useShader);
		}
		void RenderFullscreenQuad(TbInfo<MatrixInfoExt> info, Shader* shader = nullptr, bool useShader = true) const {
			if (!shader) shader = RenderHandle->FindShader("Quad");
			if (useShader)	shader->Use();
			Renderer(*RenderHandle).StaticMeshInstances(info, { MeshInstance(RenderHandle->GetBasicShapeMesh(EngineBasicShape::QUAD) , nullptr) }, Transform(), *shader);
		}*/

		static void Dispose();
	};
}