#pragma once
#include "GameSettings.h"
#include "MeshComponent.h"

struct ColorBufferData
{
	unsigned int OpenGLBuffer;
	bool bHDR;
	bool bAlpha;
	GLenum MagFilter;
	GLenum MinFilter;

	ColorBufferData(bool hdr = true, bool alpha = true, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST)
	{
		OpenGLBuffer = 0;
		bHDR = hdr;
		bAlpha = alpha;
		MagFilter = magFilter;
		MinFilter = minFilter;
	}
	void CreateOpenGLBuffer(glm::uvec2 size)
	{
		glGenTextures(1, &OpenGLBuffer);
		glBindTexture(GL_TEXTURE_2D, OpenGLBuffer);
		if (bHDR)
			glTexImage2D(GL_TEXTURE_2D, 0, (bAlpha) ? (GL_RGBA16F) : (GL_RGB16F), size.x, size.y, 0, (bAlpha) ? (GL_RGBA) : (GL_RGB), GL_FLOAT, (void*)(nullptr));
		else
			glTexImage2D(GL_TEXTURE_2D, 0, (bAlpha) ? (GL_RGBA) : (GL_RGB), size.x, size.y, 0, (bAlpha) ? (GL_RGBA) : (GL_RGB), GL_UNSIGNED_BYTE, (void*)(nullptr));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MagFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MinFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	void BindToFramebuffer(GLenum target, GLenum attachment)
	{
		glBindTexture(GL_TEXTURE_2D, OpenGLBuffer);
		glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, OpenGLBuffer, 0);
	}
};

struct DepthBufferData
{
	unsigned int OpenGLBuffer;
	bool bCubemap;
	GLenum MagFilter;
	GLenum MinFilter;

	DepthBufferData(bool cubemap = false, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST)
	{
		OpenGLBuffer = 0;
		bCubemap = cubemap;
		MagFilter = magFilter;
		MinFilter = minFilter;
	}
	void CreateOpenGLBuffer(glm::uvec2 size)
	{
		GLenum BindName = GL_TEXTURE_2D;
		GLenum texImageTarget = GL_TEXTURE_2D;
		if (bCubemap)
		{
			BindName = GL_TEXTURE_CUBE_MAP;
			texImageTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		}
		glGenTextures(1, &OpenGLBuffer);
		glBindTexture(BindName, OpenGLBuffer);

		for (int i = 0; i < ((bCubemap) ? (6) : (1)); i++)
			glTexImage2D(texImageTarget + i, 0, GL_DEPTH_COMPONENT, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, (void*)(nullptr));

		glTexParameteri(BindName, GL_TEXTURE_MAG_FILTER, MagFilter);
		glTexParameteri(BindName, GL_TEXTURE_MIN_FILTER, MinFilter);
	}
	void BindToFramebuffer(GLenum target, GLenum attachment)
	{
		if (bCubemap)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, OpenGLBuffer);
			glFramebufferTexture(GL_TEXTURE_CUBE_MAP, GL_DEPTH_ATTACHMENT, OpenGLBuffer, 0);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, OpenGLBuffer);
			glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, OpenGLBuffer, 0);
		}
	}
};

class Framebuffer
{
	unsigned int FBO;
public:
	std::vector <ColorBufferData> ColorBuffers;
	DepthBufferData* DepthBuffer;
public:
	Framebuffer()
	{
		FBO = -1;
		DepthBuffer = nullptr;
	}
	
	bool isLoaded()
	{
		if (FBO == -1)
			return false;
		return true;
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
	void Load(glm::uvec2 size, ColorBufferData colorBuffer, DepthBufferData* depthBuffer = nullptr)
	{
		Load(size, std::vector<ColorBufferData>{ColorBufferData()}, depthBuffer);
	}
	void Load(glm::uvec2 size, std::vector <ColorBufferData> colorBuffers, DepthBufferData* depthBuffer = nullptr)
	{
		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);

		std::vector <GLenum> attachments;
		ColorBuffers.assign(colorBuffers.size(), ColorBufferData());

		for (unsigned int i = 0; i < colorBuffers.size(); i++)
		{
			ColorBuffers[i] = colorBuffers[i];
			ColorBuffers[i].CreateOpenGLBuffer(size);
			attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
			ColorBuffers[i].BindToFramebuffer(GL_FRAMEBUFFER, attachments[i]);
		}
		if (attachments.empty())
			glDrawBuffer(GL_NONE);
		else
			glDrawBuffers(attachments.size(), &attachments[0]);

		DepthBuffer = depthBuffer;
		if (DepthBuffer)
		{
			DepthBuffer->CreateOpenGLBuffer(size);
			DepthBuffer->BindToFramebuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT);
		}


		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cerr << "FBO has not been loaded properly.\n";
		
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