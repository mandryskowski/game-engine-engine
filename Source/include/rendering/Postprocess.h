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

		std::shared_ptr<Shader> QuadShader;

		mutable unsigned int FrameIndex;

	public:
		Postprocess();
		void Init(GameManager* gameHandle, glm::uvec2 resolution);

		virtual unsigned int GetFrameIndex() override;
		virtual glm::mat4 GetJitterMat(const GameSettings::VideoSettings& usedSettings, int optionalIndex = -1) override;	//dont pass anything to receive the current jitter matrix
		template <typename ToolboxType> PPToolbox<ToolboxType> GetPPToolbox(RenderToolboxCollection& toolboxCol) const
		{
			return PPToolbox<ToolboxType>(*this, toolboxCol);
		}

		const Texture* GaussianBlur(PPToolbox<GaussianBlurToolbox> tb, const GEE_FB::Framebuffer& writeFramebuffer, const Viewport* viewport, const Texture* tex, int passes, unsigned int writeColorBuffer = 0) const;
		const Texture* SSAOPass(RenderInfo&, const Texture* gPosition, const Texture* gNormal);
		const Texture* SMAAPass(PPToolbox<SMAAToolbox> tb, const GEE_FB::Framebuffer& writeFramebuffer, const Viewport* viewport, const Texture* colorTex, const Texture* depthTex, const Texture* previousColorTex = nullptr, const Texture* velocityTex = nullptr, unsigned int writeColorBuffer = 0, bool bT2x = false) const;
		const Texture* TonemapGammaPass(PPToolbox<ComposedImageStorageToolbox> tbCollection, const GEE_FB::Framebuffer& writeFramebuffer, const Viewport* viewport, const Texture* colorTex, const Texture* blurTex) const;	//converts from linear to gamma and from HDR data to LDR
		void Render(RenderToolboxCollection& tbCollection, const GEE_FB::Framebuffer& finalFramebuffer, const Viewport* viewport, const Texture* colorTex, const Texture* blurTex = nullptr, const Texture* depthTex = nullptr, const Texture* velocityTex = nullptr) const;
		void RenderFullscreenQuad(RenderToolboxCollection& tbCollection, Shader* shader = nullptr, bool useShader = true) const {
			RenderFullscreenQuad(RenderInfo(tbCollection, glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::vec3(0.0f), false), shader, useShader);
		}
		void RenderFullscreenQuad(RenderInfo& info, Shader* shader = nullptr, bool useShader = true) const {
			if (!shader) shader = RenderHandle->FindShader("Quad");
			if (useShader)	shader->Use();
			RenderHandle->RenderStaticMesh(info, MeshInstance(RenderHandle->GetBasicShapeMesh(EngineBasicShape::QUAD), nullptr), Transform(), shader);
		}

		void Dispose();
	};
}