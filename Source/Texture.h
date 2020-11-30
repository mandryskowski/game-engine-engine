#pragma once
#include <string>
#include <glad4/glad.h>
#include <glm/glm.hpp>
#include <memory>

class Texture
{
protected:
	GLenum Type;
	unsigned int ID;
	std::string Path;

public:
	Texture(GLenum type = GL_TEXTURE_2D, unsigned int id = 0, std::string path = "");
	unsigned int GenerateID(GLenum type = GL_ZERO);

	GLenum GetType() const;
	unsigned int GetID() const;
	std::string GetPath() const;

	void Bind(int texSlot = -1) const;

	virtual void Dispose();
};

class NamedTexture: public Texture
{
	std::string ShaderName;

public:
	NamedTexture(Texture tex, std::string name = "diffuse");

	std::string GetShaderName();
	void SetShaderName(std::string);
};

bool containsAlphaChannel(GLenum internalformat);

GLenum internalFormatToAlpha(GLenum);
template <class T = unsigned char> Texture textureFromFile(std::string path, GLenum internalformat, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR, bool flip = false);
Texture textureFromBuffer(const void* buffer, unsigned int width, unsigned int height, GLenum internalformat, GLenum format, GLenum type, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR);
std::shared_ptr<Texture> reserveTexture(glm::uvec2 size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefined2DTexture", GLenum format = GL_ZERO);	//Pass the last argument to override the default internalformat->format conversion
std::shared_ptr<Texture> reserveTexture(glm::uvec3 size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefined3DTexture", GLenum format = GL_ZERO);