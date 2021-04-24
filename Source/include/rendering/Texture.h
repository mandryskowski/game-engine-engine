#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include <cereal/access.hpp>
#include <cereal/macros.hpp>

struct aiTexture;

class Texture
{
protected:
	GLenum Type, InternalFormat;
	unsigned int ID;
	std::string Path;

public:
	Texture(GLenum type = GL_TEXTURE_2D, GLenum internalFormat = GL_RGB, unsigned int id = 0, std::string path = "");
	unsigned int GenerateID(GLenum type = GL_ZERO);

	GLenum GetType() const;
	unsigned int GetID() const;
	std::string GetPath() const;

	void SetPath(const std::string&);

	void Bind(int texSlot = -1) const;

	virtual void Dispose();
	template <typename Archive> void Save(Archive& archive) const
	{
		archive(cereal::make_nvp("Type", Type), cereal::make_nvp("InternalFormat", InternalFormat), cereal::make_nvp("Path", Path));
	}
	template <typename Archive> void Load(Archive& archive)
	{
		archive(cereal::make_nvp("Type", Type), cereal::make_nvp("InternalFormat", InternalFormat), cereal::make_nvp("Path", Path));
		//if (!Path.empty() && Path.front() == '*')
		//else
		*this = textureFromFile(Path, InternalFormat);
	}
};

class NamedTexture: public Texture
{
	std::string ShaderName;

public:
	NamedTexture(Texture tex = Texture(), std::string name = "diffuse");

	std::string GetShaderName();
	void SetShaderName(std::string);
	template <typename Archive> void Serialize(Archive& archive)
	{
		archive(cereal::make_nvp("ShaderName", ShaderName), cereal::make_nvp("Texture", cereal::base_class<Texture>(this)));
	}
};

namespace cereal
{
	template <class Archive>
	struct specialize<Archive, NamedTexture, cereal::specialization::member_serialize> {};
}

bool containsAlphaChannel(GLenum internalformat);
GLenum nrChannelsToFormat(int nrChannels, bool bBGRA = false, GLenum* internalFormat = nullptr);

GLenum internalFormatToAlpha(GLenum);
template <class T = unsigned char> Texture textureFromFile(std::string path, GLenum internalformat, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR, bool flip = false);
Texture textureFromAiEmbedded(const aiTexture&, bool bSRGB);
Texture textureFromBuffer(const void* buffer, unsigned int width, unsigned int height, GLenum internalformat, GLenum format, GLenum type, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR);
std::shared_ptr<Texture> reserveTexture(glm::uvec2 size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefined2DTexture", GLenum format = GL_ZERO);	//Pass the last argument to override the default internalformat->format conversion
std::shared_ptr<Texture> reserveTexture(glm::uvec3 size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefined3DTexture", GLenum format = GL_ZERO);