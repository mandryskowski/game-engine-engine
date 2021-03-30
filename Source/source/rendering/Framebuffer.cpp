#include <rendering/Framebuffer.h>
#include <iostream>
#include <algorithm>
#include <game/GameManager.h> //delete after PrimitiveDebugging is done

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
		if (FBO != 0 || index != 0)
			;// std::cerr << "Can't find colorbuffer " << index << '\n';
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

void Framebuffer::Bind(bool changeViewportSize, const Viewport* viewport) const
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	if (!changeViewportSize)
		return;

	if (viewport)
	{
		if (viewport->GetSize().x <= RenderSize.x && viewport->GetSize().y <= RenderSize.y)
		{
			viewport->SetOpenGLState();
			return;
		}

		std::cerr << "ERROR! Cannot resize viewport to " << viewport->GetSize().x << ", " << viewport->GetSize().y << ". Framebuffer is too small.\n";
	}

	glViewport(0, 0, RenderSize.x, RenderSize.y);
}

void Framebuffer::SetDrawBuffer(unsigned int index) const
{
	if (FBO == 0)
		glDrawBuffer(GL_BACK);
	else if (ColorBuffers.empty() || ColorBuffers.size() <= index)
		glDrawBuffer(GL_NONE);
	else
		glDrawBuffer(GL_COLOR_ATTACHMENT0 + index);
}

void GEE_FB::Framebuffer::SetDrawBuffers() const
{
	if (FBO == 0)
		glDrawBuffer(GL_BACK);
	else if (ColorBuffers.empty())
		glDrawBuffer(GL_NONE);
	else
	{
		std::vector<GLenum> attachments;
		for (unsigned int i = 0; i < ColorBuffers.size(); i++)
			attachments.push_back(GL_COLOR_ATTACHMENT0 + i);

		glDrawBuffers(attachments.size(), &attachments[0]);
	}
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

bool GEE_FB::containsStencil(GLenum internalformat)
{
	//attachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
	if (internalformat == GL_DEPTH24_STENCIL8 || internalformat == GL_DEPTH32F_STENCIL8)
		return true;

	//attachmentType = GL_DEPTH_ATTACHMENT;
	return false;
}

Framebuffer GEE_FB::getDefaultFramebuffer(glm::uvec2 windowRes)
{
	Framebuffer fb;
	fb.FBO = 0;
	fb.RenderSize = windowRes;
	
	return fb;
}

std::shared_ptr<FramebufferAttachment> GEE_FB::reserveColorBuffer(glm::uvec2 size, GLenum internalformat, GLenum type, GLenum magFilter, GLenum minFilter, GLenum texType, unsigned int samples, std::string texName, GLenum format)
{
	std::shared_ptr<FramebufferAttachment> tex = std::make_shared<FramebufferAttachment>(FramebufferAttachment(NamedTexture(*reserveTexture(size, internalformat, type, magFilter, minFilter, texType, samples, texName, format), texName)));
	tex->SetAttachmentEnum(GL_COLOR_ATTACHMENT0);

	return tex;
}

std::shared_ptr<FramebufferAttachment> GEE_FB::reserveColorBuffer(glm::uvec3 size, GLenum internalformat, GLenum type, GLenum magFilter, GLenum minFilter, GLenum texType, unsigned int samples, std::string texName, GLenum format)
{
	std::shared_ptr<FramebufferAttachment> tex = std::make_shared<FramebufferAttachment>(FramebufferAttachment(NamedTexture(*reserveTexture(size, internalformat, type, magFilter, minFilter, texType, samples, texName, format), texName)));
	tex->SetAttachmentEnum(GL_COLOR_ATTACHMENT0);

	return tex;
}

std::shared_ptr<FramebufferAttachment> GEE_FB::reserveDepthBuffer(glm::uvec2 size, GLenum internalformat, GLenum type, GLenum magFilter, GLenum minFilter, GLenum texType, unsigned int samples, std::string texName, GLenum format)
{
	if (format == GL_ZERO)
		format = ((GEE_FB::containsStencil(internalformat)) ? (GL_DEPTH_STENCIL) : (GL_DEPTH_COMPONENT));

	std::shared_ptr<FramebufferAttachment> tex = std::make_shared<FramebufferAttachment>(FramebufferAttachment(NamedTexture(*reserveTexture(size, internalformat, type, magFilter, minFilter, texType, samples, texName, format), texName)));
	tex->SetAttachmentEnum((GEE_FB::containsStencil(internalformat)) ? (GL_DEPTH_STENCIL_ATTACHMENT) : (GL_DEPTH_ATTACHMENT));

	return tex;
}
