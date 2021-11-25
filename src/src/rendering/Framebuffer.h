#pragma once
#include "Texture.h"
#include "Viewport.h"
#include <vector>
#include <string> 

namespace GEE
{
	namespace GEE_FB
	{

		enum class Axis
		{
			XPositive,
			XNegative,
			YPositive,
			YNegative,
			ZPositive,
			ZNegative,

			X = XPositive,
			Y = YPositive,
			Z = ZPositive
		};

		struct AttachmentSlot
		{
			/**
			 * @param colorSlot: the index of the color slot to be returned. Must not be negative.
			 * @return the color slot at the passed index.
			*/
			static AttachmentSlot Color(unsigned int colorSlot) { return AttachmentSlot(GL_COLOR_ATTACHMENT0 + colorSlot, true); }

			static AttachmentSlot Depth() { return GL_DEPTH_ATTACHMENT; }
			static AttachmentSlot Stencil() { return GL_STENCIL_ATTACHMENT; }
			static AttachmentSlot DepthStencil() { return GL_DEPTH_STENCIL_ATTACHMENT; }

			static AttachmentSlot None() { return GL_ZERO; }

			GLenum GetEnumGL() const { return SlotGL; }
			bool IsColorAttachment() const { return bColorAttachment; }

			bool operator==(const AttachmentSlot& rhs) const { return SlotGL == rhs.SlotGL; }
			bool operator!=(const AttachmentSlot& rhs) const { return SlotGL != rhs.SlotGL; }
		private:
			AttachmentSlot(GLenum slotGL, bool color = false) : SlotGL(slotGL), bColorAttachment(color) {}
			GLenum SlotGL;
			bool bColorAttachment;
		};


		class FramebufferAttachment : public NamedTexture
		{
		public:


			FramebufferAttachment(const NamedTexture& tex, AttachmentSlot specifySlot, unsigned int mipLevel = 0);
			FramebufferAttachment(const NamedTexture& tex, Axis cubemapSide, AttachmentSlot specifySlot, unsigned int mipLevel = 0);
			FramebufferAttachment(const NamedTexture& tex, unsigned int layer, AttachmentSlot specifySlot, unsigned int mipLevel = 0);

			AttachmentSlot GetAttachmentSlot() const;
			void SetAttachmentSlot(AttachmentSlot attachmentSlot);

			/**
			 * @brief Returns the texture type that is used to attach this FramebufferAttachment to a Framebuffer. Likely to be identical with Texture::GetType(), but may not be the same when only a specific side of a cubemap is attached, for example.
			 * @return a GLenum which is the attached texture type
			*/
			GLenum GetAttachedType() const;

			unsigned int GetTextureLayer() const;

			unsigned int GetMipLevel() const;
			void SetMipLevel(unsigned int);

		private:
			AttachmentSlot SpecifiedSlot;
			unsigned int TextureLayer, MipLevel;
			GLenum OverrideTexType;
		};

		class Framebuffer
		{
			unsigned int FBO;
			friend Framebuffer getDefaultFramebuffer(Vec2u);
		public:
			std::vector <FramebufferAttachment> Attachments;

			std::vector <AttachmentSlot> ExcludedSlots;

			friend Framebuffer getDefaultFramebuffer(Vec2u);
		public:
			Framebuffer();

			bool HasBeenGenerated() const;
			unsigned int GetFBO() const;
			const FramebufferAttachment GetColorTexture(unsigned int index) const;
			const FramebufferAttachment GetAttachment(std::string name) const;
			const FramebufferAttachment GetAttachment(AttachmentSlot) const;
			const FramebufferAttachment GetAnyDepthAttachment() const;
			unsigned int GetColorTextureCount() const;
			Vec2u GetSize() const;
			/**
			 * @brief Generates the Framebuffer Object. If it is already generated, call the Dispose method first.
			*/
			void Generate();
			/**
			 * @brief Attach any FramebufferAttachment to use it for rendering. Calls SetDrawSlots() automatically - call SetDrawSlot(unsigned int) if you want to use only one color texture for drawing.
			 * @warning Make sure that the slot to be used for attaching it is not occupied by any other FramebufferAttachment. This function does not check it. Two FramebufferAttachment at the same slot will only cause trouble. Also do not pass a texture that has not been generated (loaded).
			 * @param attachment: the FramebufferAttachment to attach to this Framebuffer. Can be a color texture, a depth, stencil, or depth and stencil buffer.
			 * @param bindFramebuffer: a boolean indicating whether this Framebuffer should be bound (which is neccessary to attach the attachment). Pass false if you're sure it is bound to reduce the performance overhead.
			 * @param autoSetDrawTextures: a boolean indicating whether draw textures should be automatically set in this method. To reduce overhead, pass false if attachment is a depth/stencil buffer or if you will call SetDrawSlot/SetDrawSlots later.
			*/
			void Attach(const FramebufferAttachment& attachment, bool bindFramebuffer = true, bool autoSetDrawTextures = true);
			/**
			 * @brief Converts the passed color texture to FramebufferAttachment and attaches it (and optionally depthAttachment as well) to this Framebuffer.
			 * @param colorTexture: the one color texture to be attached at color slot 0 (!).
			 * @param depthAttachment: optional argument; if passed, this method calls Attach(depthAttachment).
			*/
			void AttachTextures(const NamedTexture& colorTexture, const FramebufferAttachment& depthAttachment = FramebufferAttachment(NamedTexture(), AttachmentSlot::None()));
			/**
			 * @brief Converts one or many passed color textures to FramebufferAttachments and attaches it to this Framebuffer.
			 * @param colorTextures: a vector of textures to be attached at color slots (0) to (colorTextures.size() - 1) in the same order as they are in the passed vector.
			 * @param depthAttachment: optional argument; if passed, this method calls Attach(depthAttachment).
			*/
			void AttachTextures(std::vector<NamedTexture> colorTextures, const FramebufferAttachment& depthAttachment = FramebufferAttachment(NamedTexture(), AttachmentSlot::None()));

			void Detach(AttachmentSlot, bool dispose = false);

			void BlitToFBO(unsigned int, int = 1);

			/**
			 * @brief Bind this Framebuffer to use it in rendering.
			 * @param changeViewportSize: resize the viewport to the size of this Framebuffer.
			*/
			void Bind(bool resizeViewport = false) const;
			/**
			 * @brief Bind this Framebuffer to use it in rendering.
			 * @param viewport: resize the rendering viewport to the size of the passed Viewport object.
			*/
			void Bind(const Viewport& viewport) const;

			/**
			 * @brief Set the only color slot to be used in rendering from now on.
			 * @param index: the index of the color slot
			 * @warning This function DOES NOT behave like glDrawBuffer. Instead, every fragment color is assigned to GL_NONE except of the one at slot index. Game Engine Engine assumes that fragment color locations are the same as the indices of color attachment slots, just to avoid confusion.
			*/
			void SetDrawSlot(unsigned int index) const;	//Sets the buffer COLOR_ATTACHMENT0 + index, if available
			/**
			 * @brief Set the color slots of all color buffers of this Framebuffer, except of those which are excluded, to be used in rendering from now on.
			 * @see Framebuffer::ExcludeDrawSlot(AttachmentSlot).
			 * @warning This function DOES NOT assign draw slots like glDrawBuffers. Be mindful of that when writing your own shaders.
			 * @see Framebuffer::SetDrawSlot(unsigned int).
			*/
			void SetDrawSlots() const;

			/**
			 * @brief Excludes any color texture attached to this Framebuffer at the passed color slot from being a render target. From now on, no texture at the passed slot will not be modified by any draw calls.
			 * @param excludedSlot: the color slot to be excluded from being used until reincluding it.
			*/
			void ExcludeDrawSlot(AttachmentSlot excludedSlot);

			/**
			 * @brief Reincluded the passed color slot if it is excluded from using as a draw slot.
			 * @param reincludedSlot: the color slot to be reincluded.
			 * @return a boolean indicating whether the color slot was excluded prior to calling this method.
			*/
			bool ReincludeDrawSlot(AttachmentSlot reincludedSlot);

		private:
			bool IsSlotExcluded(AttachmentSlot) const;

			bool AttachTextureGL(const FramebufferAttachment& texture);

		public:
			/**
			 * @brief Disposes of the Framebuffer Object. Do not dispose of the attached textures if you wish to use them in the future.
			 * @param disposeTextures: a boolean indicating whether all of the attached textures should be disposed as well. If you want only some of them to be disposed, detach the ones you wish to keep.
			*/
			void Dispose(bool disposeTextures = false);
		};

		Framebuffer getDefaultFramebuffer(Vec2u windowRes);
	}
	std::string debugFramebuffer();
}