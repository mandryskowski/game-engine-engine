#include "Postprocess.h"

ColorBufferData::ColorBufferData(GLenum texEnum, GLenum internalFormat, GLenum type, GLenum magFilter, GLenum minFilter)
{
	OpenGLBuffer = 0;
	TextureEnum = texEnum;
	InternalFormat = internalFormat;
	Type = type;
	MagFilter = magFilter;
	MinFilter = minFilter;
}

bool ColorBufferData::ContainsAlphaChannel()
{
	switch (InternalFormat)
	{
	case GL_RGB16F:
	case GL_RGB32F:
	case GL_RGBA:
		return true;
	}

	return false;
}

void ColorBufferData::CreateOpenGLBuffer(glm::uvec2 size, unsigned int samples)
{
	glGenTextures(1, &OpenGLBuffer);
	glBindTexture(TextureEnum, OpenGLBuffer);
	GLenum format = (ContainsAlphaChannel()) ? (GL_RGBA) : (GL_RGB);
	if (TextureEnum == GL_TEXTURE_2D_MULTISAMPLE || TextureEnum == GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
	{
		//glTexImage2D(GL_TEXTURE_2D, 0, (bAlpha) ? (GL_RGBA16F) : (GL_RGB16F), size.x, size.y, 0, (bAlpha) ? (GL_RGBA) : (GL_RGB), GL_FLOAT, (void*)(nullptr));
		glTexImage2DMultisample(TextureEnum, samples, InternalFormat, size.x, size.y, GL_TRUE);
	}
	else
	{
		glTexImage2D(TextureEnum, 0, InternalFormat, size.x, size.y, 0, format, Type, (void*)(nullptr));
		glTexParameteri(TextureEnum, GL_TEXTURE_MAG_FILTER, MagFilter);
		glTexParameteri(TextureEnum, GL_TEXTURE_MIN_FILTER, MinFilter);
		glTexParameteri(TextureEnum, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(TextureEnum, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
}
void ColorBufferData::BindToFramebuffer(GLenum target, GLenum attachment)
{
	glBindTexture(TextureEnum, OpenGLBuffer);
	glFramebufferTexture2D(target, attachment, TextureEnum, OpenGLBuffer, 0);
}


DepthBufferData::DepthBufferData(GLenum texEnum, GLenum internalFormat, GLenum magFilter, GLenum minFilter)
{
	OpenGLBuffer = 0;
	TextureEnum = texEnum;
	InternalFormat = internalFormat;
	MagFilter = magFilter;
	MinFilter = minFilter;
}

bool DepthBufferData::ContainsStencil()
{
	if (InternalFormat == GL_DEPTH24_STENCIL8 || InternalFormat == GL_DEPTH32F_STENCIL8)
		return true;
	return false;
}

void DepthBufferData::CreateOpenGLBuffer(glm::uvec2 size, unsigned int samples)
{
	bool bCubemap = (TextureEnum == GL_TEXTURE_CUBE_MAP || TextureEnum == GL_TEXTURE_CUBE_MAP_ARRAY) ? (true) : (false);
	bool bMultisample = (TextureEnum == GL_TEXTURE_2D_MULTISAMPLE || TextureEnum == GL_TEXTURE_2D_MULTISAMPLE_ARRAY) ? (true) : (false);
	
	glGenTextures(1, &OpenGLBuffer);
	glBindTexture(TextureEnum, OpenGLBuffer);
	
	GLenum format = ((this->ContainsStencil()) ? (GL_DEPTH_STENCIL) : (GL_DEPTH_COMPONENT));
	GLenum type = ((this->ContainsStencil() ? (GL_UNSIGNED_INT_24_8) : (GL_FLOAT)));
	if (bMultisample)
		glTexImage2DMultisample(TextureEnum, samples, InternalFormat, size.x, size.y, GL_TRUE);
	else
	{
		GLenum target = (bCubemap) ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X) : (TextureEnum);
		for (int i = 0; i < ((bCubemap) ? (6) : (1)); i++)
			glTexImage2D(target + i, 0, InternalFormat, size.x, size.y, 0, format, type, (void*)(nullptr));
	}
	if (!bMultisample)
	{
		glTexParameteri(TextureEnum, GL_TEXTURE_MAG_FILTER, MagFilter);
		glTexParameteri(TextureEnum, GL_TEXTURE_MIN_FILTER, MinFilter);
	}
}
void DepthBufferData::BindToFramebuffer(GLenum target, GLenum attachment)
{
	glBindTexture(TextureEnum, OpenGLBuffer);
	if (TextureEnum == GL_TEXTURE_CUBE_MAP)
		glFramebufferTexture(target, attachment, OpenGLBuffer, 0);
	else
		glFramebufferTexture2D(target, attachment, TextureEnum, OpenGLBuffer, 0);
}

Framebuffer::Framebuffer()
{
	FBO = 0;
	RenderSize = glm::uvec2(0, 0);
	DepthBuffer = nullptr;
}

bool Framebuffer::isLoaded()
{
	if (FBO == 0)
		return false;
	return true;
}

unsigned int Framebuffer::GetFBO()
{
	return FBO;
}

ColorBufferData Framebuffer::GetColorBuffer(unsigned int index)
{
	if (index >= ColorBuffers.size())
	{
		std::cerr << "Can't find colorbuffer " << index << '\n';
		return ColorBufferData();
	}
	return ColorBuffers[index];
}

void Framebuffer::Load(glm::uvec2 size, ColorBufferData colorBuffer, DepthBufferData* depthBuffer, unsigned int samples)
{
	Load(size, std::vector<ColorBufferData>{colorBuffer}, depthBuffer, samples);
}
void Framebuffer::Load(glm::uvec2 size, std::vector <ColorBufferData> colorBuffers, DepthBufferData* depthBuffer, unsigned int samples)
{
	RenderSize = size;

	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	std::vector <GLenum> attachments;
	ColorBuffers.assign(colorBuffers.size(), ColorBufferData());

	for (unsigned int i = 0; i < colorBuffers.size(); i++)
	{
		ColorBuffers[i] = colorBuffers[i];
		ColorBuffers[i].CreateOpenGLBuffer(RenderSize, samples);
		attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
		ColorBuffers[i].BindToFramebuffer(GL_FRAMEBUFFER, attachments[i]);
	}
	
	if (attachments.empty())
	{
		glReadBuffer(GL_NONE);
		glDrawBuffer(GL_NONE);
	}
	else
	{
		glDrawBuffers(attachments.size(), &attachments[0]);
	}

	DepthBuffer = depthBuffer;
	if (DepthBuffer)
	{
		if (DepthBuffer->OpenGLBuffer == 0)
			DepthBuffer->CreateOpenGLBuffer(RenderSize, samples);
		else
			std::cerr << "INFO: A texture is bound to two framebuffers at once.\n";

		DepthBuffer->BindToFramebuffer(GL_FRAMEBUFFER, (DepthBuffer->ContainsStencil()) ? (GL_DEPTH_STENCIL_ATTACHMENT) : (GL_DEPTH_ATTACHMENT));
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "FBO has not been loaded properly: " << framebufferStatusToString(glCheckFramebufferStatus(GL_FRAMEBUFFER)) << '\n';

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::BlitToFBO(unsigned int fbo2, int bufferCount)
{
	for (int i = 0; i < 2; i++)
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
		glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo2);
		glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);

		glBlitFramebuffer(0, 0, RenderSize.x, RenderSize.y, 0, 0, RenderSize.x, RenderSize.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void Framebuffer::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

Postprocess::Postprocess()
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


	PPShader.LoadShaders("postprocess.vs", "postprocess.fs");
	PPShader.Use();
	PPShader.Uniform1i("HDRbuffer", 0);
	PPShader.Uniform1i("brightnessBuffer", 1);

	GaussianBlurShader.LoadShaders("gaussianblur.vs", "gaussianblur.fs");
	GaussianBlurShader.Use();
	GaussianBlurShader.Uniform1i("tex", 0);
}

void Postprocess::LoadSettings(const GameSettings* settings)
{
	//load gamma
	PPShader.Use();
	PPShader.Uniform1f("gamma", settings->MonitorGamma);

	//load framebuffers
	for (int i = 0; i < 2; i++)
		PingPongFramebuffers[i].Load(settings->WindowSize, ColorBufferData(GL_TEXTURE_2D, GL_RGB16F, GL_FLOAT, GL_LINEAR, GL_LINEAR));
}

void Postprocess::Render(unsigned int colorBuffer, unsigned int brightnessBuffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	PPShader.Use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorBuffer);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, brightnessBuffer);
	PPShader.Uniform1i("HDRbuffer", 0);

	glBindVertexArray(QuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

unsigned int Postprocess::GaussianBlur(unsigned int tex, int passes)
{
	if (passes == 0)
		return tex;

	GaussianBlurShader.Use();
	glBindVertexArray(QuadVAO);
	glActiveTexture(GL_TEXTURE0);

	bool horizontal = true;

	for (int i = 0; i < passes; i++)
	{
		PingPongFramebuffers[horizontal].Bind();
		GaussianBlurShader.Uniform1i("horizontal", horizontal);
		glBindTexture(GL_TEXTURE_2D, (i == 0) ? (tex) : (PingPongFramebuffers[!horizontal].GetColorBuffer(0).OpenGLBuffer));
		glDrawArrays(GL_TRIANGLES, 0, 6);

		horizontal = !horizontal;
	}
	return PingPongFramebuffers[!horizontal].GetColorBuffer(0).OpenGLBuffer;
}