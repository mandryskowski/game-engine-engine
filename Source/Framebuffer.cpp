#include "Framebuffer.h"
#include <iostream>
#include <algorithm>

using namespace GEE_FB;

FramebufferAttachment::FramebufferAttachment(GLenum texEnum, GLenum internalFormat, GLenum type, GLenum magFilter, GLenum minFilter)
{
	ID = 0;
	TextureEnum = texEnum;
	InternalFormat = internalFormat;
	Type = type;
	MagFilter = magFilter;
	MinFilter = minFilter;
}

bool FramebufferAttachment::WasCreated()
{
	return ID != 0;
}

void FramebufferAttachment::CreateOpenGLBuffer(glm::uvec2 size, unsigned int samples)
{
	glGenTextures(1, &ID);
	glBindTexture(TextureEnum, ID);
}

void FramebufferAttachment::BindToFramebuffer(GLenum target, GLenum attachment)
{
	glBindTexture(GL_TEXTURE_2D, ID);
	if (TextureEnum == GL_TEXTURE_CUBE_MAP)
		glFramebufferTexture(target, attachment, ID, 0);
	else
		glFramebufferTexture2D(target, attachment, TextureEnum, ID, 0);
}


ColorBuffer::ColorBuffer(GLenum texEnum, GLenum internalFormat, GLenum type, GLenum magFilter, GLenum minFilter) :
	FramebufferAttachment(texEnum, internalFormat, type, magFilter, minFilter)
{
}

bool ColorBuffer::ContainsAlphaChannel()
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

void ColorBuffer::CreateOpenGLBuffer(glm::uvec2 size, unsigned int samples)
{
	FramebufferAttachment::CreateOpenGLBuffer(size, samples);
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


DepthStencilBuffer::DepthStencilBuffer(GLenum texEnum, GLenum internalFormat, GLenum type, GLenum magFilter, GLenum minFilter):
	FramebufferAttachment(texEnum, internalFormat, type, magFilter, minFilter)
{
}

bool DepthStencilBuffer::ContainsStencil()
{
	if (InternalFormat == GL_DEPTH24_STENCIL8 || InternalFormat == GL_DEPTH32F_STENCIL8)
		return true;
	return false;
}

void DepthStencilBuffer::CreateOpenGLBuffer(glm::uvec2 size, unsigned int samples)
{
	FramebufferAttachment::CreateOpenGLBuffer(size, samples);
	bool bCubemap = (TextureEnum == GL_TEXTURE_CUBE_MAP || TextureEnum == GL_TEXTURE_CUBE_MAP_ARRAY) ? (true) : (false);
	bool bMultisample = (TextureEnum == GL_TEXTURE_2D_MULTISAMPLE || TextureEnum == GL_TEXTURE_2D_MULTISAMPLE_ARRAY) ? (true) : (false);

	GLenum format = ((this->ContainsStencil()) ? (GL_DEPTH_STENCIL) : (GL_DEPTH_COMPONENT));
	if (bMultisample)
		glTexImage2DMultisample(TextureEnum, samples, InternalFormat, size.x, size.y, GL_TRUE);
	else
	{
		GLenum target = (bCubemap) ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X) : (TextureEnum);
		for (int i = 0; i < ((bCubemap) ? (6) : (1)); i++)
			glTexImage2D(target + i, 0, InternalFormat, size.x, size.y, 0, format, Type, (void*)(nullptr));
	}
	if (!bMultisample)
	{
		glTexParameteri(TextureEnum, GL_TEXTURE_MAG_FILTER, MagFilter);
		glTexParameteri(TextureEnum, GL_TEXTURE_MIN_FILTER, MinFilter);
	}
}

Framebuffer::Framebuffer()
{
	FBO = 0;
	RenderSize = glm::uvec2(0, 0);
	DepthBuffer = nullptr;
}

bool Framebuffer::IsLoaded() const
{
	if (FBO == 0)
		return false;
	return true;
}

unsigned int Framebuffer::GetFBO() const
{
	return FBO;
}

glm::uvec2 Framebuffer::GetSize() const
{
	return RenderSize;
}

std::shared_ptr<ColorBuffer> Framebuffer::GetColorBuffer(unsigned int index) const
{
	if (index >= ColorBuffers.size())
	{
		std::cerr << "Can't find colorbuffer " << index << '\n';
		return nullptr;
	}
	return ColorBuffers[index];
}

unsigned int Framebuffer::GetNumberOfColorBuffers() const
{
	return ColorBuffers.size();
}

void Framebuffer::Load(glm::uvec2 size, std::shared_ptr<ColorBuffer> colorBuffer, std::shared_ptr<DepthStencilBuffer> depthBuffer, unsigned int samples)
{
	std::vector<std::shared_ptr<ColorBuffer>> colorBuffers;
	if (colorBuffer)
		colorBuffers.push_back(colorBuffer);

	Load(size, colorBuffers, depthBuffer, samples);
}
void Framebuffer::Load(glm::uvec2 size, std::vector <std::shared_ptr<ColorBuffer>> colorBuffers, std::shared_ptr<DepthStencilBuffer> depthBuffer, unsigned int samples)
{
	RenderSize = size;

	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	std::vector <GLenum> attachments;
	ColorBuffers = colorBuffers;

	for (unsigned int i = 0; i < colorBuffers.size(); i++)
	{
		if (!ColorBuffers[i]->WasCreated())
			ColorBuffers[i]->CreateOpenGLBuffer(RenderSize, samples);
		attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
		ColorBuffers[i]->BindToFramebuffer(GL_FRAMEBUFFER, attachments[i]);
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
		if (!DepthBuffer->WasCreated())
			DepthBuffer->CreateOpenGLBuffer(RenderSize, samples);
		else
			std::cerr << "INFO: A texture is bound to two framebuffers at once.\n";

		DepthBuffer->BindToFramebuffer(GL_FRAMEBUFFER, (DepthBuffer->ContainsStencil()) ? (GL_DEPTH_STENCIL_ATTACHMENT) : (GL_DEPTH_ATTACHMENT));
	}

	if ((colorBuffers.size() > 0 || depthBuffer) && glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "FBO has not been loaded properly: " << framebufferStatusToString(glCheckFramebufferStatus(GL_FRAMEBUFFER)) << '\n';

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Load(glm::uvec2 size, const ColorBuffer& colorBuffer, DepthStencilBuffer* depthBuffer, unsigned int samples)
{
	Load(size, std::make_shared<ColorBuffer>(colorBuffer), (depthBuffer) ? (std::make_shared<DepthStencilBuffer>(*depthBuffer)) : (nullptr), samples);
}
void Framebuffer::Load(glm::uvec2 size, const std::vector<ColorBuffer>& colorBuffers, DepthStencilBuffer* depthBuffer, unsigned int samples)
{
	std::vector<std::shared_ptr<ColorBuffer>> createdColorBuffers;
	createdColorBuffers.resize(colorBuffers.size());
	std::transform(colorBuffers.begin(), colorBuffers.end(), createdColorBuffers.begin(), [](const ColorBuffer& colorBuffer) { return std::make_shared<ColorBuffer>(colorBuffer); });
	
	Load(size, createdColorBuffers, (depthBuffer) ? (std::make_shared<DepthStencilBuffer>(*depthBuffer)) : (nullptr), samples);
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

void Framebuffer::Bind(bool changeViewportSize) const
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	if (changeViewportSize)
		glViewport(0, 0, RenderSize.x, RenderSize.y);
}

std::string framebufferStatusToString(GLenum status)
{
	switch (status)
	{
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "Incomplete attachment.\n";
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "Incomplete draw buffer.\n";
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: return "Incomplete layer targets.\n";
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "Missing attachments.\n";
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "Incomplete multisample.\n";
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "Incomplete read buffer.\n";
	case GL_FRAMEBUFFER_COMPLETE: return "Framebuffer is complete.\n";
	}

	return "Error " + std::to_string(status) + " is not known by the program.\n";
}

