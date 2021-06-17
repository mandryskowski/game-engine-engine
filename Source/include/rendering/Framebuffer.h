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
			FramebufferAttachment(NamedTexture tex = NamedTexture(Texture()), GLenum attachmentEnum = GL_ZERO);
			bool WasCreated();
			GLenum GetAttachmentEnum();
			void SetAttachmentEnum(GLenum);
			void BindToFramebuffer(GLenum target, GLenum attachment);
		};

		class Framebuffer
		{
			unsigned int FBO;
			Vec2u RenderSize;
		public:
			std::vector <std::shared_ptr<FramebufferAttachment>> ColorBuffers;
			std::shared_ptr<FramebufferAttachment> DepthBuffer;

			std::vector<FramebufferAttachment*> ExcludedBuffers;

			friend Framebuffer getDefaultFramebuffer(Vec2u);
		public:
			Framebuffer();

			bool IsLoaded() const;
			unsigned int GetFBO() const;
			Vec2u GetSize() const;
			std::shared_ptr<FramebufferAttachment> GetColorBuffer(unsigned int index) const;
			std::shared_ptr<FramebufferAttachment> GetColorBuffer(std::string name) const;
			unsigned int GetNumberOfColorBuffers() const;
			void SetAttachments(Vec2u size = Vec2u(0), std::shared_ptr<FramebufferAttachment> colorBuffer = nullptr, std::shared_ptr<FramebufferAttachment> depthBuffer = nullptr, unsigned int samples = 0);
			void SetAttachments(Vec2u size, std::vector<std::shared_ptr<FramebufferAttachment>> colorBuffers, std::shared_ptr<FramebufferAttachment> depthBuffer = nullptr, unsigned int samples = 0);
			void SetAttachments(Vec2u size, const FramebufferAttachment& colorBuffer, FramebufferAttachment* depthBuffer = nullptr, unsigned int samples = 0);
			void SetAttachments(Vec2u size, const std::vector<FramebufferAttachment>& colorBuffers, FramebufferAttachment* depthBuffer = nullptr, unsigned int samples = 0);
			void BlitToFBO(unsigned int, int = 1);
			void Bind(bool changeViewportSize = true, const Viewport* = nullptr) const;
			void SetDrawBuffer(unsigned int index) const;	//Sets the buffer COLOR_ATTACHMENT0 + index, if available
			void SetDrawBuffers() const;

			bool ExcludeDrawBuffer(const std::string&);
			bool DeexcludeDrawBuffer(const std::string&);
		private:
			bool IsBufferExcluded(FramebufferAttachment&) const;
		public:

			void Dispose(bool disposeBuffers = false);
		};
		bool containsStencil(GLenum internalformat);

		Framebuffer getDefaultFramebuffer(Vec2u windowRes);

		std::shared_ptr<FramebufferAttachment> reserveColorBuffer(Vec2u size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefinedColorBuffer", GLenum format = GL_ZERO);
		std::shared_ptr<FramebufferAttachment> reserveColorBuffer(Vec3u size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefinedColorBuffer", GLenum format = GL_ZERO);
		std::shared_ptr<FramebufferAttachment> reserveDepthBuffer(Vec2u size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefinedDepthBuffer", GLenum format = GL_ZERO);
	}
	std::string debugFramebuffer();
}