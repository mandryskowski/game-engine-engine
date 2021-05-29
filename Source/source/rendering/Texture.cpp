#include <rendering/Texture.h>
#include <stb/stb_image.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/texture.h>
#include <vector>

namespace GEE
{
	Texture::Texture(GLenum type, GLenum internalFormat, unsigned int id, std::string path) :
		Type(type),
		InternalFormat(internalFormat),
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

	void Texture::SetPath(const std::string& path)
	{
		Path = path;
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

	void NamedTexture::SetShaderName(std::string name)
	{
		ShaderName = name;
	}

	/*
	========================================================================================================================
	========================================================================================================================
	========================================================================================================================
	*/

	bool containsAlphaChannel(GLenum internalformat)
	{
		switch (internalformat)
		{
		case GL_RGB16F:
		case GL_RGB32F:
		case GL_RGBA:
			return true;
		}

		return false;
	}

	GLenum nrChannelsToFormat(int nrChannels, bool bBGRA, GLenum* internalFormat)
	{
		switch (nrChannels)
		{
		case 1: return ((bBGRA) ? (GL_BLUE) : (GL_RED));
		default:
			std::cout << "Info: Channel count " << nrChannels << " is not supported. Texture will be loaded as GL_RGB.\n";
		case 3:
			return ((bBGRA) ? (GL_BGR) : (GL_RGB));
		case 4:
			if (internalFormat)
				*internalFormat = internalFormatToAlpha(*internalFormat);
			return ((bBGRA) ? (GL_BGRA) : (GL_RGBA));
		}
	}

	GLenum internalFormatToAlpha(GLenum internalformat)
	{
		switch (internalformat)
		{
		case GL_RGB: return GL_RGBA;
		case GL_SRGB: return GL_SRGB_ALPHA;
		case GL_RGB16F: return GL_RGBA16F;
		case GL_RGB32F: return GL_RGBA32F;

		case GL_BGR: return GL_BGRA;
		case GL_BGR_INTEGER: return GL_BGRA_INTEGER;
		}

		return internalformat;
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
		else
			std::cerr << "ERROR! Unrecognized type of T for file " + path + "\n";

		if (!data)
		{
			std::cerr << "Can't load texture from " << path << '\n';
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, glm::value_ptr(glm::vec3(1.0f, 0.0f, 1.0f)));	//set the texture's color to pink so its obvious that this texture is missing
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			return Texture(GL_TEXTURE_2D, internalformat, tex, path);
		}
		GLenum format = nrChannelsToFormat(nrChannels, false, &internalformat);

		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, data);
		if (path == "nanosuit/glass_dif.png")
			std::cout << "***Crashing texture***\n Internalformat: " << internalformat << "\n Width: " << width << "\n Height:" << height << "\n Format: " << format << "\n Min filter: " << minFilter << "\n Is type GL_UNSIGNED_BYTE?: " << (type == GL_UNSIGNED_BYTE) << "\n Data pointer: " << data << "\n";

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		if (path == "nanosuit/glass_dif.png")
			std::cout << glGetError() << "\n";
		if (minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST || minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR)
			glGenerateMipmap(GL_TEXTURE_2D);

		if (path == "nanosuit/glass_dif.png")
			std::cout << "TEX2\n";

		if (flip)
			stbi_set_flip_vertically_on_load(false);

		stbi_image_free(data);

		return Texture(GL_TEXTURE_2D, internalformat, tex, path);
	}

	Texture textureFromAiEmbedded(const aiTexture& tex, bool bSRGB)
	{
		std::cout << tex.mWidth << "   " << tex.mHeight << "!!!\n";
		int width, height, nrChannels;

		stbi_uc* data = stbi_load_from_memory(&tex.pcData[0].b, tex.mWidth, &width, &height, &nrChannels, 0);
		std::cout << tex.mFilename.C_Str() << " Niby " << width << " " << height << "  |  " << nrChannels << "\n";

		Texture result = textureFromBuffer(data, width, height, nrChannelsToFormat(nrChannels, false), nrChannelsToFormat(nrChannels, false), GL_UNSIGNED_BYTE);
		result.SetPath(tex.mFilename.C_Str());

		stbi_image_free(data);

		return result;
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

		return Texture(GL_TEXTURE_2D, internalformat, tex);
	}

	std::shared_ptr<Texture> reserveTexture(glm::uvec2 size, GLenum internalformat, GLenum type, GLenum magFilter, GLenum minFilter, GLenum texType, unsigned int samples, std::string texName, GLenum format)
	{
		std::shared_ptr<Texture> tex = std::make_shared<NamedTexture>(NamedTexture(Texture(), texName));
		tex->GenerateID(texType);
		tex->Bind();

		bool bCubemap = texType == GL_TEXTURE_CUBE_MAP || texType == GL_TEXTURE_CUBE_MAP_ARRAY;
		bool bMultisample = texType == GL_TEXTURE_2D_MULTISAMPLE || texType == GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

		if (bMultisample)
		{
			glTexImage2DMultisample(texType, samples, internalformat, size.x, size.y, GL_TRUE);
			return tex;
		}

		GLenum texImageType = (bCubemap) ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X) : (texType);
		if (format == GL_ZERO)
			format = (containsAlphaChannel(internalformat)) ? (GL_RGBA) : (GL_RGB);

		std::cout << "Reserving " << size.x << '\n';

		if (size.x == 546 && type == GL_UNSIGNED_BYTE)
		{
			std::vector<GLubyte> pixels(size.x * size.y * 4, 128);
			for (int i = 0; i < 1; i++)
				glTexImage2D(texImageType + i, 0, internalformat, size.x, size.y, 0, format, type, &pixels[0]);
			std::cout << "pomalowalem\n";
		}
		else
		{
			for (int i = 0; i < ((bCubemap) ? (6) : (1)); i++)
				glTexImage2D(texImageType + i, 0, internalformat, size.x, size.y, 0, format, type, (void*)(nullptr));
		}

		glTexParameteri(texType, GL_TEXTURE_MAG_FILTER, magFilter);
		glTexParameteri(texType, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(texType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(texType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		return tex;
	}

	std::shared_ptr<Texture> reserveTexture(glm::uvec3 size, GLenum internalformat, GLenum type, GLenum magFilter, GLenum minFilter, GLenum texType, unsigned int samples, std::string texName, GLenum format)
	{
		std::shared_ptr<Texture> tex = std::make_shared<NamedTexture>(NamedTexture(Texture(), texName));
		tex->GenerateID(texType);
		tex->Bind();

		bool bCubemap = texType == GL_TEXTURE_CUBE_MAP || texType == GL_TEXTURE_CUBE_MAP_ARRAY;
		bool bMultisample = texType == GL_TEXTURE_2D_MULTISAMPLE || texType == GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

		if (bMultisample)
		{
			glTexImage3DMultisample(texType, samples, internalformat, size.x, size.y, size.z * (bCubemap) ? (6) : (1), GL_TRUE);
			return tex;
		}

		if (format == GL_ZERO)
			format = (containsAlphaChannel(internalformat)) ? (GL_RGBA) : (GL_RGB);

		glTexImage3D(texType, 0, internalformat, size.x, size.y, size.z * ((bCubemap) ? (6) : (1)), 0, format, type, (void*)(nullptr));

		glTexParameteri(texType, GL_TEXTURE_MAG_FILTER, magFilter);
		glTexParameteri(texType, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(texType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(texType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		return tex;
	}


	template Texture textureFromFile<unsigned char>(std::string, GLenum, GLenum, GLenum, bool);
	template Texture textureFromFile<float>(std::string, GLenum, GLenum, GLenum, bool);
}