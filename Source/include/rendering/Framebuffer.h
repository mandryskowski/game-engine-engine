#pragma once
#include "Texture.h"
#include "Viewport.h"
#include <vector>
#include <string> 
#include <memory>

namespace GEE
{
	namespace GEE_FB
	{

		/*enum class Axis
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
		};*/

		class FramebufferAttachment : public NamedTexture
		{
		protected:
			GLenum AttachmentEnum;

		public:
			FramebufferAttachment(NamedTexture tex, GLenum attachmentEnum);

			/*FramebufferAttachment(const NamedTexture& tex);
			FramebufferAttachment(const NamedTexture& tex, Axis cubemapSide);
			FramebufferAttachment(const NamedTexture& tex, unsigned int layer);*/

			unsigned int GetAttachmentSlot() const;
			void SetAttachmentSlot(unsigned int attachmentSlot);

			GLenum GetAttachmentEnum() const;
			void SetAttachmentEnum(GLenum);
		};

		class Framebuffer
		{
			unsigned int FBO;
			friend Framebuffer getDefaultFramebuffer(Vec2u);
		public:
			std::vector <FramebufferAttachment> ColorBuffers;
			FramebufferAttachment DepthBuffer;

			std::vector<GLenum> ExcludedBuffers;

			friend Framebuffer getDefaultFramebuffer(Vec2u);
		public:
			Framebuffer();

			bool HasBeenGenerated() const;
			unsigned int GetFBO() const;
			const FramebufferAttachment GetColorTexture(unsigned int index) const;
			const FramebufferAttachment GetColorTexture(std::string name) const;
			unsigned int GetColorTextureCount() const;
			Vec2u GetSize() const;
			/**
			 * @brief Generates the Framebuffer Object. If it is already generated, call the Dispose method first.
			*/
			void Generate();
			void AttachTextures(std::vector<NamedTexture> colorTexture, const FramebufferAttachment& depthAttachment = FramebufferAttachment(NamedTexture(), GL_ZERO));
			void AttachTexture(const NamedTexture& colorTexture, const FramebufferAttachment& depthAttachment = FramebufferAttachment(NamedTexture(), GL_ZERO));
			void AttachCubemapSide(const NamedTexture& cubemapTexture, GLenum cubemapSide);
			void Attach2DArrayLayer(const NamedTexture& cubemapTexture, int layer);

			void BlitToFBO(unsigned int, int = 1);
			void Bind(bool changeViewportSize = false, const Viewport* = nullptr) const;
			void SetDrawTexture(unsigned int index) const;	//Sets the buffer COLOR_ATTACHMENT0 + index, if available
			void SetDrawTextures() const;
			
			/**
			 * @brief Excludes a texture attached to this Framebuffer from being a render target. From now on, the texture will not be modified by any draw calls.
			 * @param texName: name of the texture
			 * @return bool: a boolean indicating if a texture named texName was found and succesfully excluded.
			*/
			bool ExcludeColorTexture(const std::string& texName);
			bool ReincludeColorTexture(const std::string& texName);
		private:
			bool IsBufferExcluded(GLenum) const;

			bool AttachTextureGL(const FramebufferAttachment& texture);

		public:
			/**
			 * @brief Disposes of the Framebuffer Object. Do not dispose of the attached textures if you wish to use them in the future.
			 * @param disposeBuffers: a boolean indicating whether all of the attached textures should be disposed as well. If you want only some of them to be disposed, detach the ones you wish to keep.
			*/
			void Dispose(bool disposeTextures = false);
		};

		Framebuffer getDefaultFramebuffer(Vec2u windowRes);
	}
	std::string debugFramebuffer();
}