#include "Framebuffer.h"
#include <iostream>
#include <algorithm>
#include "GameManager.h" //delete after PrimitiveDebugging is done

using namespace GEE_FB;

FramebufferAttachment::FramebufferAttachment(NamedTexture tex, GLenum attachmentEnum):
	NamedTexture(tex),
	AttachmentEnum(attachmentEnum)
{
}

bool FramebufferAttachment::WasCreated()
{
	return ID != 0;
}

GLenum FramebufferAttachment::GetAttachmentEnum()
{
	return AttachmentEnum;
}

void FramebufferAttachment::SetAttachmentEnum(GLenum attachmentType)
{
	AttachmentEnum = attachmentType;
}

void FramebufferAttachment::BindToFramebuffer(GLenum target, GLenum attachment)
{
	Bind();
	if (Type == GL_TEXTURE_CUBE_MAP)
		glFramebufferTexture(target, attachment, ID, 0);
	else
		glFramebufferTexture2D(target, attachment, Type, ID, 0);
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

std::shared_ptr<FramebufferAttachment> Framebuffer::GetColorBuffer(unsigned int index) const
{
	if (index >= ColorBuffers.size())
	{
		std::cerr << "Can't find colorbuffer " << index << '\n';
		return nullptr;
	}
	return ColorBuffers[index];
}

std::shared_ptr<FramebufferAttachment> Framebuffer::GetColorBuffer(std::string name) const
{
	auto found = std::find_if(ColorBuffers.begin(), ColorBuffers.end(), [name](const std::shared_ptr<FramebufferAttachment>& colorBuffer) {  return colorBuffer->GetShaderName() == name; });
	if (found == ColorBuffers.end())
	{
		std::cerr << "Can't find colorbuffer " << name << '\n';
		return nullptr;
	}
	return *found;
}

unsigned int Framebuffer::GetNumberOfColorBuffers() const
{
	return ColorBuffers.size();
}

void Framebuffer::SetAttachments(glm::uvec2 size, std::shared_ptr<FramebufferAttachment> colorBuffer, std::shared_ptr<FramebufferAttachment> depthBuffer, unsigned int samples)
{
	std::vector<std::shared_ptr<FramebufferAttachment>> colorBuffers;
	if (colorBuffer)
		colorBuffers.push_back(colorBuffer);

	SetAttachments(size, colorBuffers, depthBuffer, samples);
}
void Framebuffer::SetAttachments(glm::uvec2 size, std::vector <std::shared_ptr<FramebufferAttachment>> colorBuffers, std::shared_ptr<FramebufferAttachment> depthBuffer, unsigned int samples)
{
	RenderSize = size;

	if (!IsLoaded())
	{
		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	}

	std::vector <GLenum> attachments;
	ColorBuffers = colorBuffers;

	for (unsigned int i = 0; i < colorBuffers.size(); i++)
	{
		attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
		ColorBuffers[i]->BindToFramebuffer(GL_FRAMEBUFFER, attachments[i]);
		if (PrimitiveDebugger::bDebugFramebuffers)
			std::cout << "Adding color buffer " + std::to_string(ColorBuffers[i]->GetID()) + " " + ColorBuffers[i]->GetShaderName() + " to slot " + std::to_string(i) + ".\n";
	}

	SetDrawBuffers();

	DepthBuffer = depthBuffer;
	if (DepthBuffer)
		DepthBuffer->BindToFramebuffer(GL_FRAMEBUFFER, DepthBuffer->GetAttachmentEnum());/*(1DepthBuffer->ContainsStencil(internalformat) ? (GL_DEPTH_STENCIL_ATTACHMENT) : (GL_DEPTH_ATTACHMENT));*/

	if ((colorBuffers.size() > 0 || depthBuffer) && PrimitiveDebugger::bDebugFramebuffers)
		debugFramebuffer();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::SetAttachments(glm::uvec2 size, const FramebufferAttachment& colorBuffer, FramebufferAttachment* depthBuffer, unsigned int samples)
{
	SetAttachments(size, std::make_shared<FramebufferAttachment>(colorBuffer), (depthBuffer) ? (std::make_shared<FramebufferAttachment>(*depthBuffer)) : (nullptr), samples);
}
void Framebuffer::SetAttachments(glm::uvec2 size, const std::vector<FramebufferAttachment>& colorBuffers, FramebufferAttachment* depthBuffer, unsigned int samples)
{
	std::vector<std::shared_ptr<FramebufferAttachment>> createdColorBuffers;
	createdColorBuffers.resize(colorBuffers.size());
	std::transform(colorBuffers.begin(), colorBuffers.end(), createdColorBuffers.begin(), [](const FramebufferAttachment& colorBuffer) { return std::make_shared<FramebufferAttachment>(colorBuffer); });
	
	SetAttachments(size, createdColorBuffers, (depthBuffer) ? (std::make_shared<FramebufferAttachment>(*depthBuffer)) : (nullptr), samples);
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

void GEE_FB::Framebuffer::SetDrawBuffers() const
{
	std::vector<GLenum> attachments;
	for (unsigned int i = 0; i < ColorBuffers.size(); i++)
		attachments.push_back(GL_COLOR_ATTACHMENT0 + i);

	if (attachments.empty())
		glDrawBuffer(GL_NONE);
	else
		glDrawBuffers(attachments.size(), &attachments[0]);
}

void Framebuffer::Dispose(bool disposeBuffers)
{
	if (disposeBuffers)
	{
		for (int i = 0; i < static_cast<int>(ColorBuffers.size()); i++)
			ColorBuffers[i]->Dispose();
		if (DepthBuffer)
			DepthBuffer->Dispose();
	}

	glDeleteFramebuffers(1, &FBO);
	FBO = 0;
	ColorBuffers.clear();
	DepthBuffer = nullptr;
}

std::string debugFramebuffer()
{
	std::string message;
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch (status)
	{
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: message = "Incomplete attachment.\n"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: message = "Incomplete draw buffer.\n"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: message = "Incomplete layer targets.\n"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: message = "Missing attachments.\n"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: message = "Incomplete multisample.\n"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: message = "Incomplete read buffer.\n"; break;
		case GL_FRAMEBUFFER_COMPLETE: message = "Framebuffer is complete.\n"; break;
		default: message = "Error " + std::to_string(status) + " is not known by the program.\n";
	}

	std::cout << message;

	return message;
}

bool GEE_FB::ContainsAlphaChannel(GLenum internalformat)
{
	switch (internalformat)
	{
	case GL_RGB16F:
	case GL_RGB32F:
	case GL_RGBA:
		return true;
	}

	return false;
}

bool GEE_FB::ContainsStencil(GLenum internalformat)
{
	//attachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
	if (internalformat == GL_DEPTH24_STENCIL8 || internalformat == GL_DEPTH32F_STENCIL8)
		return true;

	//attachmentType = GL_DEPTH_ATTACHMENT;
	return false;
}

std::shared_ptr<FramebufferAttachment> GEE_FB::reserveColorBuffer(glm::uvec2 size, GLenum internalformat, GLenum type, GLenum magFilter, GLenum minFilter, GLenum texType, unsigned int samples, std::string texName)
{
	std::shared_ptr<FramebufferAttachment> tex = std::make_shared<FramebufferAttachment>(NamedTexture(Texture(), texName));
	tex->GenerateID(texType);
	tex->Bind();
	tex->SetAttachmentEnum(GL_COLOR_ATTACHMENT0);

	bool bCubemap = texType == GL_TEXTURE_CUBE_MAP;
	bool bMultisample = texType == GL_TEXTURE_2D_MULTISAMPLE;

	if (bMultisample)
	{
		glTexImage2DMultisample(texType, samples, internalformat, size.x, size.y, GL_TRUE);
		return tex;
	}

	GLenum texImageType = (bCubemap) ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X) : (texType);
	GLenum format = (GEE_FB::ContainsAlphaChannel(internalformat)) ? (GL_RGBA) : (GL_RGB);

	for (int i = 0; i < ((bCubemap) ? (6) : (1)); i++)
		glTexImage2D(texImageType + i, 0, internalformat, size.x, size.y, 0, format, type, (void*)(nullptr));

	glTexParameteri(texType, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(texType, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(texType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(texType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return tex;
}

std::shared_ptr<FramebufferAttachment> GEE_FB::reserveColorBuffer(glm::uvec3 size, GLenum internalformat, GLenum type, GLenum magFilter, GLenum minFilter, GLenum texType, unsigned int samples, std::string texName)
{
	std::shared_ptr<FramebufferAttachment> tex = std::make_shared<FramebufferAttachment>(NamedTexture(Texture(), texName));
	tex->GenerateID(texType);
	tex->Bind();
	tex->SetAttachmentEnum(GL_COLOR_ATTACHMENT0);

	bool bCubemap = texType == GL_TEXTURE_CUBE_MAP_ARRAY;
	bool bMultisample = texType == GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

	if (bMultisample)
	{
		glTexImage3DMultisample(texType, samples, internalformat, size.x, size.y, size.z * (bCubemap) ? (6) : (1), GL_TRUE);
		return tex;
	}

	GLenum format = (GEE_FB::ContainsAlphaChannel(internalformat)) ? (GL_RGBA) : (GL_RGB);

	glTexImage3D(texType, 0, internalformat, size.x, size.y, size.z * ((bCubemap) ? (6) : (1)), 0, format, type, (void*)(nullptr));

	glTexParameteri(texType, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(texType, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(texType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(texType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return tex;
}

std::shared_ptr<FramebufferAttachment> GEE_FB::reserveDepthBuffer(glm::uvec2 size, GLenum internalformat, GLenum type, GLenum magFilter, GLenum minFilter, GLenum texType, unsigned int samples, std::string texName)
{
	std::shared_ptr<FramebufferAttachment> tex = std::make_shared<FramebufferAttachment>(NamedTexture(Texture(), texName));
	tex->GenerateID(texType);
	tex->Bind();

	bool bCubemap = texType == GL_TEXTURE_CUBE_MAP || texType == GL_TEXTURE_CUBE_MAP_ARRAY;
	bool bMultisample = texType == GL_TEXTURE_2D_MULTISAMPLE || texType == GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

	GLenum format = ((GEE_FB::ContainsStencil(internalformat)) ? (GL_DEPTH_STENCIL) : (GL_DEPTH_COMPONENT));
	tex->SetAttachmentEnum((GEE_FB::ContainsStencil(internalformat)) ? (GL_DEPTH_STENCIL_ATTACHMENT) : (GL_DEPTH_ATTACHMENT));

	if (bMultisample)
	{
		glTexImage2DMultisample(texType, samples, internalformat, size.x, size.y, GL_TRUE);
		return tex;
	}

	GLenum texImageType = (bCubemap) ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X) : (texType);
	for (int i = 0; i < ((bCubemap) ? (6) : (1)); i++)
		glTexImage2D(texImageType + i, 0, internalformat, size.x, size.y, 0, format, type, (void*)(nullptr));
	
	glTexParameteri(texType, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(texType, GL_TEXTURE_MIN_FILTER, minFilter);

	return tex;
}
