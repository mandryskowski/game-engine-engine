#pragma once
#include "Texture.h"
#include "Viewport.h"
#include <vector>
#include <string> 
#include <memory>

namespace GEE
{
	//TODO: Remont kapitalny calego systemu framebufferow
	//****	Wyjeb te 3 klasy i rozbij je na funkcje
	//****	Wyjeb funkcje Load z Framebuffer i rozbij to na: SetColorBuffers(vec<Texture>), AttachColorBuffer(Texture), SetDepthBuffer(Texture*), SetAttachments(vec<Texture>, Texture*) (pamietaj o glDrawBuffers zalezne od FBO!!!), argumenty tych funkcji to Texture zamiast tych zjebanych spaghetti attachment ja pierdole

	namespace GEE_FB
	{
		class FramebufferAttachment : public NamedTexture
		{
		protected:
			GLenum AttachmentEnum;

		public:
			FramebufferAttachment(NamedTexture tex, GLenum attachmentEnum);
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
		bool containsStencil(GLenum internalformat);

		Framebuffer getDefaultFramebuffer(Vec2u windowRes);

		NamedTexture reserveColorBuffer(Vec2u size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefinedColorBuffer", GLenum format = GL_ZERO);
		NamedTexture reserveColorBuffer(Vec3u size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefinedColorBuffer", GLenum format = GL_ZERO);
		FramebufferAttachment reserveDepthBuffer(Vec2u size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefinedDepthBuffer", GLenum format = GL_ZERO);
	}
	std::string debugFramebuffer();
}