#pragma once
#include <string>
#include <glad/glad.h>

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
};

class MaterialTexture: public Texture
{
	std::string ShaderName;

public:
	MaterialTexture(Texture tex, std::string name = "diffuse");

	std::string GetShaderName();
};


GLenum internalFormatToAlpha(GLenum);
template <class T = unsigned char> Texture textureFromFile(std::string path, GLenum internalformat, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR, bool flip = false);
Texture textureFromBuffer(const void* buffer, unsigned int width, unsigned int height, GLenum internalformat, GLenum format, GLenum type, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR);