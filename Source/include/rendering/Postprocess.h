#pragma once
#include <game/GameSettings.h>
#include <game/GameManager.h>
#include "Framebuffer.h"
#include "Texture.h"

class RenderInfo;
class GaussianBlurToolbox;
class SMAAToolbox;
class ComposedImageStorageToolbox;

/*class SSAOToolbox
{
	SSAOToolbox(RenderToolboxCollection* col)
	{
		//col->Add(this);
		//col->GetTb<SSAOToolbox>();
	}
};*/

class Postprocess: public PostprocessManager
{
	RenderEngineManager* RenderHandle;

	friend class RenderEngine;	//usun to

	unsigned int QuadVAO;
	unsigned int QuadVBO;

	std::shared_ptr<Shader> QuadShader;

	mutable unsigned int FrameIndex;

	void RenderQuad(unsigned int tex = 0, unsigned int texSlot = 0, bool bound = false, bool defaultFBO = true) const;
public:
	Postprocess();
	void Init(GameManager* gameHandle, glm::uvec2 resolution);

	virtual unsigned int GetFrameIndex() override;
	virtual glm::mat4 GetJitterMat(const GameSettings::VideoSettings& usedSettings, int optionalIndex = -1) override;	//dont pass anything to receive the current jitter matrix

	const Texture* GaussianBlur(GaussianBlurToolbox& tb, const GEE_FB::Framebuffer& writeFramebuffer, const Viewport* viewport, const Texture* tex, int passes, unsigned int writeColorBuffer = 0) const;
	const Texture* SSAOPass(RenderInfo&, const Texture* gPosition, const Texture* gNormal);
	const Texture* SMAAPass(SMAAToolbox& tb, const GEE_FB::Framebuffer& writeFramebuffer, const Viewport* viewport, const Texture* colorTex, const Texture* depthTex, const Texture* previousColorTex = nullptr, const Texture* velocityTex = nullptr, unsigned int writeColorBuffer = 0, bool bT2x = false) const;
	const Texture* TonemapGammaPass(ComposedImageStorageToolbox& tbCollection, const GEE_FB::Framebuffer& writeFramebuffer, const Viewport* viewport, const Texture* colorTex, const Texture* blurTex) const;	//converts from linear to gamma and from HDR data to LDR
	void Render(RenderToolboxCollection& tbCollection, const GEE_FB::Framebuffer& finalFramebuffer, const Viewport* viewport, const Texture* colorTex, const Texture* blurTex = nullptr, const Texture* depthTex = nullptr, const Texture* velocityTex = nullptr) const;

	void Dispose();
};