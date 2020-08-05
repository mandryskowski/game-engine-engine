#pragma once
#include "Texture.h"
#include <glm/glm.hpp>
#include <vector>
#include <string> 
#include <memory>

namespace GEE_FB
{
	class FramebufferAttachment : public Texture
	{
	protected:
		GLenum TextureEnum;
		GLenum InternalFormat;
		GLenum Type;
		GLenum MagFilter;
		GLenum MinFilter;

		//TODO: Implement this class as a base class for ColorBuffers and DepthBuffers.
	public:
		FramebufferAttachment(GLenum texEnum = GL_TEXTURE_2D, GLenum internalFormat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST);
		virtual bool WasCreated();
		virtual void CreateOpenGLBuffer(glm::uvec2 size, unsigned int samples = 0);
		void BindToFramebuffer(GLenum target, GLenum attachment);
	};

	class ColorBuffer : public FramebufferAttachment
	{
	public:
		ColorBuffer(GLenum texEnum = GL_TEXTURE_2D, GLenum internalFormat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST);
		bool ContainsAlphaChannel();
		void CreateOpenGLBuffer(glm::uvec2 size, unsigned int samples = 0);
	};

	class DepthStencilBuffer : public FramebufferAttachment
	{
	public:
		DepthStencilBuffer(GLenum texEnum = GL_TEXTURE_2D, GLenum internalFormat = GL_DEPTH_COMPONENT, GLenum type = GL_DEPTH_COMPONENT, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST);
		bool ContainsStencil();
		void CreateOpenGLBuffer(glm::uvec2 size, unsigned int samples = 0);
	};

	class Framebuffer
	{
		unsigned int FBO;
		glm::uvec2 RenderSize;
	public:
		std::vector <std::shared_ptr<ColorBuffer>> ColorBuffers;
		std::shared_ptr<DepthStencilBuffer> DepthBuffer;
	public:
		Framebuffer();

		bool IsLoaded() const;
		unsigned int GetFBO() const;
		glm::uvec2 GetSize() const;
		std::shared_ptr<ColorBuffer> GetColorBuffer(unsigned int index) const;
		unsigned int GetNumberOfColorBuffers() const;
		void Load(glm::uvec2 size, std::shared_ptr<ColorBuffer> colorBuffer = nullptr, std::shared_ptr<DepthStencilBuffer> depthBuffer = nullptr, unsigned int samples = 0);
		void Load(glm::uvec2 size, std::vector<std::shared_ptr<ColorBuffer>> colorBuffers, std::shared_ptr<DepthStencilBuffer> depthBuffer = nullptr, unsigned int samples = 0);
		void Load(glm::uvec2 size, const ColorBuffer& colorBuffer, DepthStencilBuffer* depthBuffer = nullptr, unsigned int samples = 0);
		void Load(glm::uvec2 size, const std::vector<ColorBuffer>& colorBuffers, DepthStencilBuffer* depthBuffer = nullptr, unsigned int samples = 0);
		void BlitToFBO(unsigned int, int = 1);
		void Bind(bool changeViewportSize = true) const;
	};
}
std::string framebufferStatusToString(GLenum);