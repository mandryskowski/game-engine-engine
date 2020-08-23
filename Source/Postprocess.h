#pragma once
#include "GameSettings.h"
#include "GameManager.h"
#include "Framebuffer.h"
#include "Texture.h"

class RenderInfo;

class Postprocess: public PostprocessManager
{
	const GameSettings* Settings;
	glm::uvec2 Resolution;

	friend class RenderEngine;	//usun to

	unsigned int QuadVAO;
	unsigned int QuadVBO;

	std::shared_ptr<Shader> QuadShader;
	std::shared_ptr<Shader> GaussianBlurShader;
	std::shared_ptr<Shader> SSAOShader;
	std::shared_ptr<Shader> TonemapGammaShader;
	std::shared_ptr<Shader> SMAAShaders[4];

	Texture SMAAAreaTex, SMAASearchTex;
	std::unique_ptr<Texture> SSAONoiseTex;

	std::unique_ptr<GEE_FB::Framebuffer> SSAOFramebuffer;
	std::unique_ptr<GEE_FB::Framebuffer> StorageFramebuffer;	//two color buffers; before rendering to FBO 0 we render to one of its color buffers (we switch them each frame) so as to gain access to the final color buffer in the next frame (for temporal AA)
	mutable unsigned int FrameIndex;
	std::unique_ptr<GEE_FB::Framebuffer> SMAAFramebuffer;	//three color buffers; we use the first two color buffers as edgeTex&blendTex in SMAA pass and optionally the last one to store the neighborhood blend result for reprojection
	GEE_FB::Framebuffer TonemapGammaFramebuffer;
	GEE_FB::Framebuffer BlurFramebuffers[2];	//only color buffer; we use them as ping pong framebuffers in gaussian blur pass.
										//Side note: I wonder which method is quicker - switching a framebuffer or switching an attachment or switching a buffer to draw to via glDrawBuffer.
										//Side note 2: I've heard that they are similar in terms of performance. So now I wonder why do we use ping-pong framebuffers and not ping-pong color buffers. They should work the same, shouldn't they?

	void RenderQuad(unsigned int tex = 0, unsigned int texSlot = 0, bool bound = false, bool defaultFBO = true) const;
public:
	Postprocess();
	void Init(GameManager* gameHandle, glm::uvec2 resolution);

	virtual unsigned int GetFrameIndex() override;
	virtual glm::mat4 GetJitterMat(int optionalIndex = -1) override;	//dont pass anything to receive the current jitter matrix

	const Texture* GaussianBlur(const Texture* tex, int passes, GEE_FB::Framebuffer* writeFramebuffer = nullptr, unsigned int writeColorBuffer = 0) const;
	const Texture* SSAOPass(RenderInfo&, const Texture* gPosition, const Texture* gNormal);
	const Texture* SMAAPass(const Texture* colorTex, const Texture* depthTex, const Texture* previousColorTex = nullptr, const Texture* velocityTex = nullptr, GEE_FB::Framebuffer* writeFramebuffer = nullptr, unsigned int writeColorBuffer = 0, bool bT2x = false) const;
	const Texture* TonemapGammaPass(const Texture* colorTex, const Texture* blurTex, GEE_FB::Framebuffer* writeFramebuffer = nullptr, bool bWriteToDefault = false) const;	//converts from linear to gamma and from HDR data to LDR
	void Render(const Texture* colorTex, const Texture* blurTex = nullptr, const Texture* depthTex = nullptr, const Texture* velocityTex = nullptr, GEE_FB::Framebuffer* finalFramebuffer = nullptr) const;

	void Dispose();
};