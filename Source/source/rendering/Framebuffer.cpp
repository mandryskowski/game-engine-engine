#include <rendering/Framebuffer.h>
#include <iostream>
#include <algorithm>
#include <game/GameManager.h> //delete after PrimitiveDebugging is done



namespace GEE
{
	using namespace GEE_FB;
	FramebufferAttachment::FramebufferAttachment(NamedTexture tex, GLenum attachmentEnum) :
		NamedTexture(tex),
		AttachmentEnum(attachmentEnum)
	{
	}

	GLenum FramebufferAttachment::GetAttachmentEnum() const
	{
		return AttachmentEnum;
	}

	void FramebufferAttachment::SetAttachmentEnum(GLenum attachmentType)
	{
		AttachmentEnum = attachmentType;
	}

	Framebuffer::Framebuffer():
		FBO(0),
		DepthBuffer(NamedTexture(), GL_ZERO)
	{
	}

	bool Framebuffer::HasBeenGenerated() const
	{
		if (FBO == 0)
			return false;
		return true;
	}

	unsigned int Framebuffer::GetFBO() const
	{
		return FBO;
	}

	const FramebufferAttachment Framebuffer::GetColorTexture(unsigned int index) const
	{
		if (index >= ColorBuffers.size())
		{
			if (FBO != 0 || index != 0)
				;// std::cerr << "Can't find colorbuffer " << index << '\n';
			return FramebufferAttachment(NamedTexture(), GL_ZERO);
		}
		return ColorBuffers[index];
	}

	const FramebufferAttachment Framebuffer::GetColorTexture(std::string name) const
	{
		auto found = std::find_if(ColorBuffers.begin(), ColorBuffers.end(), [name](const FramebufferAttachment& colorBuffer) {  return colorBuffer.GetShaderName() == name; });
		if (found == ColorBuffers.end())
		{
			std::cerr << "Can't find colorbuffer " << name << '\n';
			return FramebufferAttachment(NamedTexture(), GL_ZERO);
		}
		return *found;
	}

	unsigned int Framebuffer::GetColorTextureCount() const
	{
		return ColorBuffers.size();
	}

	Vec2u GEE_FB::Framebuffer::GetSize() const
	{
		if (!ColorBuffers.empty()) return ColorBuffers.front().GetSize();
		if (DepthBuffer.HasBeenGenerated()) return DepthBuffer.GetSize();

		return Vec2u(0);
	}

	void Framebuffer::Generate()
	{
		if (HasBeenGenerated())
		{
			std::cout << "ERROR: Cannot generate a Framebuffer that has been generated already. Call the Dispose function before you generate it again.\n";
			return;
		}
		glGenFramebuffers(1, &FBO);
	}

	void Framebuffer::AttachTexture(const NamedTexture& colorBuffer, const FramebufferAttachment& depthAttachment)
	{
		std::vector<NamedTexture> colorBuffers;
		if (colorBuffer.HasBeenGenerated())
			colorBuffers.push_back(colorBuffer);

		AttachTexture(colorBuffers, depthAttachment);
	}
	void Framebuffer::AttachTexture(std::vector<NamedTexture> colorBuffers, const FramebufferAttachment& depthAttachment)
	{
		if (!HasBeenGenerated())
			Generate();
		Bind(false);

		for (unsigned int i = 0; i < ColorBuffers.size(); i++)
		{
			ColorBuffers.push_back(FramebufferAttachment(colorBuffers[i], GL_COLOR_ATTACHMENT0 + i));
			AttachTextureGL(ColorBuffers[i]);
			if (PrimitiveDebugger::bDebugFramebuffers)
				std::cout << "Adding color buffer " + std::to_string(ColorBuffers[i].GetID()) + " " + ColorBuffers[i].GetShaderName() + " to slot " + std::to_string(i) + ".\n";
		}

		SetDrawTextures();

		DepthBuffer = depthAttachment;
		AttachTextureGL(DepthBuffer);
				

		if (PrimitiveDebugger::bDebugFramebuffers)
			debugFramebuffer();

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

			const Vec2u size = GetSize();
			glBlitFramebuffer(0, 0, size.x, size.y, 0, 0, size.x, size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

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
			if (viewport->GetSize().x <= GetSize().x && viewport->GetSize().y <= GetSize().y)
			{
				viewport->SetOpenGLState();
				return;
			}

			std::cerr << "ERROR! Cannot resize viewport to " << viewport->GetSize().x << ", " << viewport->GetSize().y << ". Framebuffer is too small.\n";
		}

		glViewport(0, 0, GetSize().x, GetSize().y);
	}

	void Framebuffer::SetDrawTexture(unsigned int index) const
	{
		if (FBO == 0)
			glDrawBuffer(GL_BACK);
		else if (index < ColorBuffers.size() && !IsBufferExcluded(ColorBuffers[index].GetAttachmentEnum()))
			glDrawBuffer(GL_COLOR_ATTACHMENT0 + index);
		else
			glDrawBuffer(GL_NONE);
	}

	void GEE_FB::Framebuffer::SetDrawTextures() const
	{
		if (FBO == 0)
			glDrawBuffer(GL_BACK);
		else if (ColorBuffers.empty())
			glDrawBuffer(GL_NONE);
		else
		{
			std::vector<GLenum> attachments;
			for (unsigned int i = 0; i < ColorBuffers.size(); i++)
				if (IsBufferExcluded(ColorBuffers[i].GetAttachmentEnum()))
					continue;
				else
					attachments.push_back(GL_COLOR_ATTACHMENT0 + i);

			glDrawBuffers(attachments.size(), &attachments[0]);
		}
	}

	bool GEE_FB::Framebuffer::ExcludeColorTexture(const std::string& name)
	{
		if (std::find_if(ExcludedBuffers.begin(), ExcludedBuffers.end(), [&](FramebufferAttachment* buffer) { return buffer->GetShaderName() == name; }) != ExcludedBuffers.end())
			return false;

		for (auto& it : ColorBuffers)
			if (it.GetShaderName() == name)
			{
				ExcludedBuffers.push_back(it.GetAttachmentEnum());
				return true;
			}
		return false;
	}

	bool GEE_FB::Framebuffer::ReincludeColorTexture(const std::string& name)
	{
		bool found = false;
		ExcludedBuffers.erase(std::remove_if(ExcludedBuffers.begin(), ExcludedBuffers.end(), [&](FramebufferAttachment* bufferVec) { if (bufferVec->GetShaderName() == name); return (found = true); return false; }), ExcludedBuffers.end());
		return found;
	}

	bool GEE_FB::Framebuffer::IsBufferExcluded(GLenum buffer) const
	{
		for (auto& it : ExcludedBuffers)
			if (it == buffer)
				return true;
		return false;
	}

	bool GEE_FB::Framebuffer::AttachTextureGL(const FramebufferAttachment& texture)
	{
		if (!texture.HasBeenGenerated())
		{
			std::cout << "ERROR: Cannot attach texture " + texture.GetShaderName() << " to a framebuffer, because the texture has not been generated.\n";
			return false;
		}
		else if (texture.GetSize() != GetSize() && GetSize() != Vec2u(0))
		{
			std::cout << "ERROR: Cannot attach texture " + texture.GetShaderName() << " of size " << texture.GetSize() << " to a framebuffer of size " << GetSize() << '\n';
			return false;
		}

		texture.Bind();

		if (texture.GetType() == GL_TEXTURE_CUBE_MAP)
			glFramebufferTexture(GL_FRAMEBUFFER, texture.GetAttachmentEnum(), texture.GetID(), 0);
		else
			glFramebufferTexture2D(GL_FRAMEBUFFER, texture.GetAttachmentEnum(), texture.GetType(), texture.GetID(), 0);

		

		return true;
	}

	void Framebuffer::Dispose(bool disposeTextures)
	{
		if (disposeTextures)
		{
			for (int i = 0; i < static_cast<int>(ColorBuffers.size()); i++)
				ColorBuffers[i].Dispose();
			DepthBuffer.Dispose();
		}

		if (HasBeenGenerated())
		{
			glDeleteFramebuffers(1, &FBO);
			FBO = 0;
		}
		ColorBuffers.clear();
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

	Framebuffer GEE_FB::getDefaultFramebuffer(Vec2u windowRes)
	{
		Framebuffer fb;
		fb.FBO = 0;
		fb.RenderSize = windowRes;

		return fb;
	}

	NamedTexture GEE_FB::reserveColorBuffer(Vec2u size, GLenum internalformat, GLenum type, GLenum magFilter, GLenum minFilter, GLenum texType, unsigned int samples, std::string texName, GLenum format)
	{
		return NamedTexture(*reserveTexture(size, internalformat, type, magFilter, minFilter, texType, samples, texName, format), texName);
	}

	NamedTexture GEE_FB::reserveColorBuffer(Vec3u size, GLenum internalformat, GLenum type, GLenum magFilter, GLenum minFilter, GLenum texType, unsigned int samples, std::string texName, GLenum format)
	{
		return NamedTexture(*reserveTexture(size, internalformat, type, magFilter, minFilter, texType, samples, texName, format), texName);
	}

	FramebufferAttachment GEE_FB::reserveDepthBuffer(Vec2u size, GLenum internalformat, GLenum type, GLenum magFilter, GLenum minFilter, GLenum texType, unsigned int samples, std::string texName, GLenum format)
	{
		if (format == GL_ZERO)
			format = ((GEE_FB::containsStencil(internalformat)) ? (GL_DEPTH_STENCIL) : (GL_DEPTH_COMPONENT));

		FramebufferAttachment attachment(FramebufferAttachment(
										NamedTexture(*reserveTexture(size, internalformat, type, magFilter, minFilter, texType, samples, texName, format), texName),
										(GEE_FB::containsStencil(internalformat)) ? (GL_DEPTH_STENCIL_ATTACHMENT) : (GL_DEPTH_ATTACHMENT)));

		return attachment;
	}

}