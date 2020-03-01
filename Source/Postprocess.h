#pragma once
#include "GameSettings.h"
#include "MeshComponent.h"

struct ColorBufferData
{
	unsigned int OpenGLBuffer;
	bool bHDR;
	bool bAlpha;
	GLenum TextureEnum;
	GLenum MagFilter;
	GLenum MinFilter;

	ColorBufferData(bool hdr = true, bool alpha = true, GLenum texEnum = GL_TEXTURE_2D, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST)
	{
		OpenGLBuffer = 0;
		bHDR = hdr;
		bAlpha = alpha;
		TextureEnum = texEnum;
		MagFilter = magFilter;
		MinFilter = minFilter;
	}
	GLenum GetInternalFormat()
	{
		if (bHDR)
		{
			if (bAlpha)	return GL_RGBA16F;
			return GL_RGB16F;
		}
		else
		{
			if (bAlpha)	return GL_RGBA;
			return GL_RGB;
		}
	}
	void CreateOpenGLBuffer(glm::uvec2 size, unsigned int samples = 0)
	{
		glGenTextures(1, &OpenGLBuffer);
		glBindTexture(TextureEnum, OpenGLBuffer);

		GLenum internalformat = GetInternalFormat();
		GLenum format = (bAlpha) ? (GL_RGBA) : (GL_RGB);

		if (TextureEnum == GL_TEXTURE_2D_MULTISAMPLE || TextureEnum == GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
		{
			//glTexImage2D(GL_TEXTURE_2D, 0, (bAlpha) ? (GL_RGBA16F) : (GL_RGB16F), size.x, size.y, 0, (bAlpha) ? (GL_RGBA) : (GL_RGB), GL_FLOAT, (void*)(nullptr));
			glTexImage2DMultisample(TextureEnum, samples, internalformat, size.x, size.y, GL_TRUE);
		}
		else
		{
			glTexImage2D(TextureEnum, 0, internalformat, size.x, size.y, 0, format, (bHDR) ? (GL_FLOAT) : (GL_UNSIGNED_BYTE), (void*)(nullptr));

			glTexParameteri(TextureEnum, GL_TEXTURE_MAG_FILTER, MagFilter);
			glTexParameteri(TextureEnum, GL_TEXTURE_MIN_FILTER, MinFilter);
			glTexParameteri(TextureEnum, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(TextureEnum, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}
	void BindToFramebuffer(GLenum target, GLenum attachment)
	{
		glBindTexture(TextureEnum, OpenGLBuffer);
		glFramebufferTexture2D(target, attachment, TextureEnum, OpenGLBuffer, 0);
	}
};

struct DepthBufferData
{
	unsigned int OpenGLBuffer;
	GLenum TextureEnum;
	GLenum MagFilter;
	GLenum MinFilter;

	DepthBufferData(GLenum texEnum = GL_TEXTURE_2D, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST)
	{
		OpenGLBuffer = 0;
		TextureEnum = texEnum;
		MagFilter = magFilter;
		MinFilter = minFilter;
	}
	void CreateOpenGLBuffer(glm::uvec2 size, unsigned int samples = 0)
	{
		bool bCubemap = (TextureEnum == GL_TEXTURE_CUBE_MAP || TextureEnum == GL_TEXTURE_CUBE_MAP_ARRAY) ? (true) : (false);
		bool bMultisample = (TextureEnum == GL_TEXTURE_2D_MULTISAMPLE || TextureEnum == GL_TEXTURE_2D_MULTISAMPLE_ARRAY) ? (true) : (false);

		glGenTextures(1, &OpenGLBuffer);
		glBindTexture(TextureEnum, OpenGLBuffer);

		if (bMultisample)
			glTexImage2DMultisample(TextureEnum, samples, GL_DEPTH_COMPONENT, size.x, size.y, GL_TRUE);
		else
		{
			GLenum target = (bCubemap) ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X) : (TextureEnum);
			for (int i = 0; i < ((bCubemap) ? (6) : (1)); i++)
				glTexImage2D(target + i, 0, GL_DEPTH_COMPONENT, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, (void*)(nullptr));
		}

		if (!bMultisample)
		{
			glTexParameteri(TextureEnum, GL_TEXTURE_MAG_FILTER, MagFilter);
			glTexParameteri(TextureEnum, GL_TEXTURE_MIN_FILTER, MinFilter);
		}
	}
	void BindToFramebuffer(GLenum target, GLenum attachment)
	{
		glBindTexture(TextureEnum, OpenGLBuffer);
		if (TextureEnum == GL_TEXTURE_CUBE_MAP)
			glFramebufferTexture(target, attachment, OpenGLBuffer, 0);
		else
			glFramebufferTexture2D(target, attachment, TextureEnum, OpenGLBuffer, 0);
	}
};

class Framebuffer
{
	unsigned int FBO;
	glm::uvec2 RenderSize;
public:
	std::vector <ColorBufferData> ColorBuffers;
	DepthBufferData* DepthBuffer;
public:
	Framebuffer()
	{
		FBO = 0;
		RenderSize = glm::uvec2(0, 0);
		DepthBuffer = nullptr;
	}
	
	bool isLoaded()
	{
		if (FBO == 0)
			return false;
		return true;
	}
	unsigned int GetFBO()
	{
		return FBO;
	}
	ColorBufferData GetColorBuffer(unsigned int index)
	{
		if (index >= ColorBuffers.size())
		{
			std::cerr << "Can't find colorbuffer " << index << '\n';
			return ColorBufferData();
		}
		return ColorBuffers[index];
	}
	void Load(glm::uvec2 size, ColorBufferData colorBuffer, DepthBufferData* depthBuffer = nullptr, unsigned int samples = 0)
	{
		Load(size, std::vector<ColorBufferData>{colorBuffer}, depthBuffer, samples);
	}
	void Load(glm::uvec2 size, std::vector <ColorBufferData> colorBuffers, DepthBufferData* depthBuffer = nullptr, unsigned int samples = 0)
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
			glDrawBuffers(attachments.size(), &attachments[0]);

		DepthBuffer = depthBuffer;
		if (DepthBuffer)
		{
			DepthBuffer->CreateOpenGLBuffer(RenderSize, samples);
			DepthBuffer->BindToFramebuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT);
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cerr << "FBO has not been loaded properly: " << framebufferStatusToString(glCheckFramebufferStatus(GL_FRAMEBUFFER)) << '\n';
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void BlitToFBO(unsigned int fbo2)
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo2);

		glBlitFramebuffer(0, 0, RenderSize.x, RenderSize.y, 0, 0, RenderSize.x, RenderSize.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void Bind() { glBindFramebuffer(GL_FRAMEBUFFER, FBO); }
};

class Postprocess
{
	unsigned int QuadVAO;
	unsigned int QuadVBO;

	Shader PPShader;
	Shader GaussianBlurShader;

	Framebuffer PingPongFramebuffers[2];

public:
	Postprocess();
	void LoadSettings(const GameSettings*);
	void Render(unsigned int, unsigned int);
	unsigned int GaussianBlur(unsigned int, int);
};