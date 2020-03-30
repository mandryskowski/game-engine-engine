#pragma once
#include "GameSettings.h"
#include "MeshComponent.h"

struct ColorBufferData
{
	unsigned int OpenGLBuffer;
	GLenum TextureEnum;
	GLenum InternalFormat;
	GLenum Type;
	GLenum MagFilter;
	GLenum MinFilter;

	ColorBufferData(GLenum texEnum = GL_TEXTURE_2D, GLenum internalFormat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST);
	bool ContainsAlphaChannel();
	void CreateOpenGLBuffer(glm::uvec2 size, unsigned int samples = 0);
	void BindToFramebuffer(GLenum target, GLenum attachment);
};

struct DepthBufferData
{
	unsigned int OpenGLBuffer;
	GLenum TextureEnum;
	GLenum InternalFormat;
	GLenum MagFilter;
	GLenum MinFilter;

	DepthBufferData(GLenum texEnum = GL_TEXTURE_2D, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum = GL_DEPTH_COMPONENT);
	bool ContainsStencil();
	void CreateOpenGLBuffer(glm::uvec2 size, unsigned int samples = 0);
	void BindToFramebuffer(GLenum target, GLenum attachment);
};

class Framebuffer
{
	unsigned int FBO;
	glm::uvec2 RenderSize;
public:
	std::vector <ColorBufferData> ColorBuffers;
	DepthBufferData* DepthBuffer;
public:
	Framebuffer();
	
	bool isLoaded();
	unsigned int GetFBO();
	ColorBufferData GetColorBuffer(unsigned int index);
	void Load(glm::uvec2 size, ColorBufferData colorBuffer, DepthBufferData* depthBuffer = nullptr, unsigned int samples = 0);	//If you pass a DeptBufferData which OpenGLBuffer is not one (was generated), then this one will be used (another one won't be automatically generated; first delete the GL buffer yourself and set it to 0)
	void Load(glm::uvec2 size, std::vector <ColorBufferData> colorBuffers, DepthBufferData* depthBuffer = nullptr, unsigned int samples = 0);
	void BlitToFBO(unsigned int, int = 1);
	void Bind();
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