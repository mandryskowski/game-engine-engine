#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

Texture::Texture(GLenum type, unsigned int id, std::string path):
	Type(type),
	ID(id),
	Path(path)
{
}

unsigned int Texture::GenerateID(GLenum type)
{
	glGenTextures(1, &ID);
	if (type != GL_ZERO)
		Type = type;
	return ID;
}

GLenum Texture::GetType() const
{
	return Type;
}

unsigned int Texture::GetID() const
{
	return ID;
}

std::string Texture::GetPath() const
{
	return Path;
}

void Texture::Bind(int texSlot) const
{
	if (texSlot >= 0)
		glActiveTexture(GL_TEXTURE0 + texSlot);
	glBindTexture(Type, ID);
}

void Texture::Dispose()
{
	if (ID > 0)
		glDeleteTextures(1, &ID);
	ID = 0;
}

NamedTexture::NamedTexture(Texture tex, std::string name) :
	Texture(tex),
	ShaderName(name)
{
}

std::string NamedTexture::GetShaderName()
{
	return ShaderName;
}

/*
========================================================================================================================
========================================================================================================================
========================================================================================================================
*/

GLenum internalFormatToAlpha(GLenum internalformat)
{
	switch (internalformat)
	{
	case GL_RGB: return GL_RGBA;
	case GL_SRGB: return GL_SRGB_ALPHA;
	case GL_RGB16F: return GL_RGBA16F;
	case GL_RGB32F: return GL_RGBA32F;
	}

	return GL_RGBA;
}

template <class T> Texture textureFromFile(std::string path, GLenum internalformat, GLenum magFilter, GLenum minFilter, bool flip)
{
	if (flip)
		stbi_set_flip_vertically_on_load(true);

	unsigned int tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	int width, height, nrChannels;
	GLenum type = GL_UNSIGNED_BYTE;

	void* data;
	if (std::is_same<T, unsigned char>::value)
		data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
	else if (std::is_same<T, float>::value)
	{
		data = stbi_loadf(path.c_str(), &width, &height, &nrChannels, 0);
		type = GL_FLOAT;
	}

	if (!data)
	{
		std::cerr << "Can't load texture from " << path << '\n';
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, glm::value_ptr(glm::vec3(1.0f, 0.0f, 1.0f)));	//set the texture's color to pink so its obvious that this texture is missing
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		return Texture(GL_TEXTURE_2D, tex, path);
	}
	GLenum format = GL_RGB;
	switch (nrChannels)
	{
	case 1: format = GL_RED; break;
	case 3: format = GL_RGB; break;
	case 4:
		format = GL_RGBA;
		internalformat = internalFormatToAlpha(internalformat);
		break;
	default:
		format = GL_RGB;
		std::cout << "Info: Channel count " << nrChannels << " of " << path << " is not supported. Texture will be loaded as GL_RGB.\n";
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, data);

	if (minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST || minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR)
		glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);

	if (flip)
		stbi_set_flip_vertically_on_load(false);

	stbi_image_free(data);

	return Texture(GL_TEXTURE_2D, tex, path);
}

Texture textureFromBuffer(const void* buffer, unsigned int width, unsigned int height, GLenum internalformat, GLenum format, GLenum type, GLenum magFilter, GLenum minFilter)
{
	unsigned int tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, buffer);

	if (minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST || minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR)
		glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);

	return Texture(GL_TEXTURE_2D, tex);
}

template Texture textureFromFile<unsigned char>(std::string, GLenum, GLenum, GLenum, bool);
template Texture textureFromFile<float>(std::string, GLenum, GLenum, GLenum, bool);