#pragma once
#include "GameSettings.h"
#include "Mesh.h"
#include "Framebuffer.h"

class Postprocess
{
	unsigned int QuadVAO;
	unsigned int QuadVBO;

	Shader GaussianBlurShader;
	std::unique_ptr<Shader> SSAOShader;
	Shader TonemapGammaShader;
	Shader SMAAShaders[3];

	unsigned int SMAAAreaTex, SMAASearchTex;
	std::unique_ptr<unsigned int> SSAONoiseTex;

	std::unique_ptr<Framebuffer> SSAOFramebuffer;
	Framebuffer TonemapGammaFramebuffer;
	Framebuffer SMAAFramebuffer;		//two color buffers; we use the color buffers as edgeTex&blendTex in SMAA pass.
	Framebuffer BlurFramebuffers[2];	//only color buffer; we use them as ping pong framebuffers in gaussian blur pass.
										//Side note: I wonder which method is quicker - switching a framebuffer or switching an attachment or switching a buffer to draw to via glDrawBuffer.
										//Side note 2: I've heard that they are similar in terms of performance. So now I wonder why do we use ping-pong framebuffers and not ping-pong color buffers. They should work the same, shouldn't they?

public:
	Postprocess();
	void Init(const GameSettings*);
	unsigned int GaussianBlur(unsigned int tex, int passes, Framebuffer* writeFramebuffer = nullptr) const;
	unsigned int SSAOPass(RenderInfo&, unsigned int gPosition, unsigned int gNormal);
	unsigned int SMAAPass(unsigned int colorTex, unsigned int depthTex) const;
	unsigned int TonemapGammaPass(unsigned int colorTex, unsigned int blurTex, bool bFinal = false) const;	//converts from linear to gamma and from HDR data to LDR
	void Render(unsigned int colorTex, unsigned int blurTex, unsigned int depthTex, const GameSettings*) const;
};