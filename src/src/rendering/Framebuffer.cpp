#include <rendering/Framebuffer.h>
#include <iostream>
#include <algorithm>
#include <game/GameManager.h> //delete after PrimitiveDebugging is done



namespace GEE
{
	using namespace GEE_FB;

	FramebufferAttachment::FramebufferAttachment(const NamedTexture& tex, AttachmentSlot specifySlot, unsigned int mipLevel):
		FramebufferAttachment(tex, std::numeric_limits<unsigned int>::max(), specifySlot, mipLevel)
	{
	}

	FramebufferAttachment::FramebufferAttachment(const NamedTexture& tex, Axis cubemapSide, AttachmentSlot specifySlot, unsigned int mipLevel):
		NamedTexture(tex),
		SpecifiedSlot(specifySlot),
		TextureLayer(std::numeric_limits<unsigned int>::max()),
		MipLevel(mipLevel),
		OverrideTexType(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<unsigned int>(cubemapSide))
	{
	}

	FramebufferAttachment::FramebufferAttachment(const NamedTexture& tex, unsigned int layer, AttachmentSlot specifySlot, unsigned int mipLevel):
		NamedTexture(tex),
		SpecifiedSlot(specifySlot),
		TextureLayer(layer),
		MipLevel(mipLevel),
		OverrideTexType(GL_ZERO)
	{
	}

	AttachmentSlot FramebufferAttachment::GetAttachmentSlot() const
	{
		return SpecifiedSlot;
	}

	void FramebufferAttachment::SetAttachmentSlot(AttachmentSlot slot)
	{
		SpecifiedSlot = slot;
	}

	GLenum FramebufferAttachment::GetAttachedType() const
	{
		if (OverrideTexType != GL_ZERO)
			return OverrideTexType;

		return GetType();
	}

	unsigned int GEE_FB::FramebufferAttachment::GetTextureLayer() const
	{
		return TextureLayer;
	}

	unsigned int GEE_FB::FramebufferAttachment::GetMipLevel() const
	{
		return MipLevel;
	}

	void GEE_FB::FramebufferAttachment::SetMipLevel(unsigned int mipLevel)
	{
		MipLevel = mipLevel;
	}

	Framebuffer::Framebuffer():
		FBO(0)
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
		if (index >= Attachments.size())
		{
			if (FBO != 0 || index != 0)
				std::cerr << "Can't find colorbuffer " << index << '\n';
			return FramebufferAttachment(NamedTexture(), AttachmentSlot::None());
		}
		return Attachments[index];
	}

	const FramebufferAttachment Framebuffer::GetAttachment(std::string name) const
	{
		auto found = std::find_if(Attachments.begin(), Attachments.end(), [name](const FramebufferAttachment& colorBuffer) {  return colorBuffer.GetShaderName() == name; });
		if (found == Attachments.end())
		{
			std::cerr << "Can't find colorbuffer " << name << '\n';
			return FramebufferAttachment(NamedTexture(), AttachmentSlot::None());
		}
		return *found;
	}

	const FramebufferAttachment GEE_FB::Framebuffer::GetAttachment(AttachmentSlot slot) const
	{
		for (auto& it : Attachments)
			if (it.GetAttachmentSlot() == slot)
				return it;

		std::cout << "ERROR: Could not get framebuffer attachment. No attachment exists at the passed slot (GLenum of AttachmentSlot: " << slot.GetEnumGL() << ")\n";
		return FramebufferAttachment(NamedTexture(), AttachmentSlot::None());
	}

	const FramebufferAttachment GEE_FB::Framebuffer::GetAnyDepthAttachment() const
	{
		for (auto& it : Attachments)
			if (it.GetAttachmentSlot() == AttachmentSlot::Depth() || it.GetAttachmentSlot() == AttachmentSlot::DepthStencil())
				return it;

		std::cout << "ERROR: Could not get any framebuffer depth attachment. No Depth nor DepthStencil attachment exists.\n";
		return FramebufferAttachment(NamedTexture(), AttachmentSlot::None());
	}

	unsigned int Framebuffer::GetColorTextureCount() const
	{
		return static_cast<unsigned int>(Attachments.size());
	}

	Vec2u GEE_FB::Framebuffer::GetSize() const
	{
		if (Attachments.empty())
			return Vec2u(0); 
		
		return Attachments.front().GetSize2D();
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

	void Framebuffer::Attach(const FramebufferAttachment& attachment, bool bindFramebuffer, bool autoSetDrawTextures)
	{
		if (bindFramebuffer)	//if the caller assures that this Framebuffer is bound, it must have been generated as well
		{
			if (!HasBeenGenerated())
				Generate();
			Bind(false);
		}

		Attachments.push_back(attachment);
		AttachTextureGL(attachment);

		if (autoSetDrawTextures)
			SetDrawSlots();
	}

	void Framebuffer::AttachTextures(const NamedTexture& colorTexture, const FramebufferAttachment& depthAttachment)
	{
		AttachTextures(std::vector<NamedTexture>({ colorTexture }), depthAttachment);
	}

	void Framebuffer::AttachTextures(std::vector<NamedTexture> colorBuffers, const FramebufferAttachment& depthAttachment)
	{
		if (colorBuffers.empty() && !depthAttachment.HasBeenGenerated())
		{
			std::cout << "ERROR: Cannot attach texture to framebuffer in method AttachTexture; no textures are present in the vector nor the depthAttachment is valid.\n";
			return;
		}
		
		if (!HasBeenGenerated())
			Generate();
		Bind(false);

		for (unsigned int i = 0; i < colorBuffers.size(); i++)
		{
			Attach(FramebufferAttachment(colorBuffers[i], AttachmentSlot::Color(i)), false, false);
			if (PrimitiveDebugger::bDebugFramebuffers)
				std::cout << "Adding color buffer " + std::to_string(Attachments[i].GetID()) + " " + Attachments[i].GetShaderName() + " to slot " + std::to_string(i) + ".\n";
		}

		SetDrawSlots();

		if (depthAttachment.HasBeenGenerated())
			Attach(depthAttachment, false, false);
				

		if (PrimitiveDebugger::bDebugFramebuffers)
			debugFramebuffer();
	}

	void Framebuffer::Detach(AttachmentSlot slot, bool dispose)
	{
		Attachments.erase(std::remove_if(Attachments.begin(), Attachments.end(), [&](FramebufferAttachment& attachmentVec) { if (slot != attachmentVec.GetAttachmentSlot()) return false; if (dispose) attachmentVec.Dispose(); return true; }), Attachments.end());
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

	void Framebuffer::Bind(bool changeViewportSize) const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		if (!changeViewportSize)
			return;

		if (GetSize() == Vec2u(0))
			std::cout << "ERROR: Cannot change viewport size to the size of the bound Framebuffer, because its size is 0.\nNb of attachments: " << Attachments.size() << '\n';
		else
			Viewport(GetSize()).SetOpenGLState();

	}

	void GEE_FB::Framebuffer::Bind(const Viewport& viewport) const
	{
		Bind(false);

		viewport.SetOpenGLState();
		if (viewport.GetSize().x > GetSize().x && viewport.GetSize().y > GetSize().y)
			std::cerr << "INFO: Viewport is set to " << viewport.GetSize() << ", but bound Framebuffer is smaller, of size " << GetSize() << ".\n";
	}

	void Framebuffer::SetDrawSlot(unsigned int index) const
	{
		if (FBO == 0)
			glDrawBuffer(GL_BACK);
		else if (!IsSlotExcluded(AttachmentSlot::Color(index)))
		{
			std::vector<GLenum> attachmentsGL(index + 1, GL_NONE);
			attachmentsGL[index] = GL_COLOR_ATTACHMENT0 + index;
			glDrawBuffers(static_cast<GLsizei>(attachmentsGL.size()), &attachmentsGL[0]);
		}
	}

	void GEE_FB::Framebuffer::SetDrawSlots() const
	{
		if (FBO == 0)
			glDrawBuffer(GL_BACK);
		else if (Attachments.empty())
			glDrawBuffer(GL_NONE);
		else
		{
			std::vector<GLenum> attachmentsGL;
			attachmentsGL.reserve(Attachments.size());

			
			GLenum currentColorAttachment = GL_COLOR_ATTACHMENT0;
			for (auto& it: Attachments)
			{
				AttachmentSlot slot = it.GetAttachmentSlot();
				if (!slot.IsColorAttachment() || IsSlotExcluded(slot))
					continue;

				while (currentColorAttachment < slot.GetEnumGL())
				{
					attachmentsGL.push_back(GL_NONE);
					currentColorAttachment++;
				}

				attachmentsGL.push_back(slot.GetEnumGL());
				currentColorAttachment++;
			}

			if (!attachmentsGL.empty())
				glDrawBuffers(static_cast<GLsizei>(attachmentsGL.size()), &attachmentsGL[0]);
		}
	}

	void GEE_FB::Framebuffer::ExcludeDrawSlot(AttachmentSlot excludedSlot)
	{
		ExcludedSlots.push_back(excludedSlot);
	}

	bool GEE_FB::Framebuffer::ReincludeDrawSlot(AttachmentSlot reincludedSlot)
	{
		bool found = false;
		ExcludedSlots.erase(std::remove_if(ExcludedSlots.begin(), ExcludedSlots.end(), [&](AttachmentSlot slotVec) { if (reincludedSlot != slotVec) { return false; } found = true; return true; }), ExcludedSlots.end());

		return found;
	}

	bool GEE_FB::Framebuffer::IsSlotExcluded(AttachmentSlot slot) const
	{
		for (auto& it : ExcludedSlots)
			if (it == slot)
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
		else if (texture.GetSize2D() != GetSize() && GetSize() != Vec2u(0))
		{
			std::cout << "ERROR: Cannot attach texture " + texture.GetShaderName() << " of size " << texture.GetSize2D() << " to a framebuffer of size " << GetSize() << '\n';
			return false;
		}

		texture.Bind();


		if (texture.GetTextureLayer() != std::numeric_limits<unsigned int>::max())
			glFramebufferTextureLayer(GL_FRAMEBUFFER, texture.GetAttachmentSlot().GetEnumGL(), texture.GetID(), texture.GetMipLevel(), texture.GetTextureLayer());
		else if (texture.GetAttachedType() == GL_TEXTURE_CUBE_MAP)
			glFramebufferTexture(GL_FRAMEBUFFER, texture.GetAttachmentSlot().GetEnumGL(), texture.GetID(), texture.GetMipLevel());
		else
			glFramebufferTexture2D(GL_FRAMEBUFFER, texture.GetAttachmentSlot().GetEnumGL(), texture.GetAttachedType(), texture.GetID(), texture.GetMipLevel());


		return true;
	}

	void Framebuffer::Dispose(bool disposeTextures)
	{
		if (disposeTextures)
		{
			for (int i = 0; i < static_cast<int>(Attachments.size()); i++)
				Attachments[i].Dispose();
		}

		if (HasBeenGenerated())
		{
			glDeleteFramebuffers(1, &FBO);
			FBO = 0;
		}
		Attachments.clear();
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

	Framebuffer GEE_FB::getDefaultFramebuffer(Vec2u windowRes)
	{
		Framebuffer fb;
		fb.FBO = 0;
		fb.Attachments.push_back(FramebufferAttachment(NamedTexture(Texture::FromGeneratedGlId(windowRes, GL_TEXTURE_2D, 0, Texture::Format::RGB())), AttachmentSlot::Color(0)));

		return fb;
	}

}