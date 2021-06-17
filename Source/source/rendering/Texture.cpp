#include <rendering/Texture.h>
#include <stb/stb_image.h>
#include <iostream>
#include <assimp/texture.h>
#include <vector>

namespace GEE
{
	Texture::Texture(GLenum type, GLenum internalFormat, unsigned int id, const std::string& path) :
		Type(type),
		InternalFormat(internalFormat),
		ID(id),
		Path(path)
	{
	}

	Texture::Texture():
		Texture(GL_TEXTURE_2D)
	{
	}

	unsigned int Texture::GenerateID(GLenum type)
	{
		glGenTextures(1, &ID);
		if (type != GL_ZERO)
			Type = type;
		return ID;
	}

	void Texture::Bind(int texSlot) const
	{
		if (texSlot >= 0)
			glActiveTexture(GL_TEXTURE0 + texSlot);
		glBindTexture(Type, ID);
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

	void Texture::SetWrap(GLenum wrapS, GLenum wrapT, GLenum wrapR, bool isAlreadyBound)
	{
		if (!isAlreadyBound) Bind();
		
		if (wrapS != 0) glTexParameteri(GetType(), GL_TEXTURE_WRAP_S, wrapS);
		if (wrapT != 0) glTexParameteri(GetType(), GL_TEXTURE_WRAP_T, wrapT);
		if (wrapR != 0) glTexParameteri(GetType(), GL_TEXTURE_WRAP_R, wrapR);
	}

	void Texture::GenerateMipmap(bool isAlreadyBound)
	{
		if (!isAlreadyBound) Bind();
		glGenerateMipmap(Type);
	}

	void Texture::SetMinFilter(MinTextureFilter minFilter, bool generateMipmapIfPossible, bool isAlreadyBound)
	{
		if (!isAlreadyBound) Bind();
		glTexParameteri(Type, GL_TEXTURE_MIN_FILTER, Impl::GetTextureFilterGL(minFilter.Filter));

		if (generateMipmapIfPossible)
			switch (minFilter.Filter)
			{
				case TextureFilter::NearestClosestMipmap:
				case TextureFilter::BilinearClosestMipmap:
				case TextureFilter::NearestInterpolateMipmap:
				case TextureFilter::Trilinear:
					GenerateMipmap(isAlreadyBound);
			}
	}

	void Texture::SetMagFilter(MagTextureFilter magFilter, bool isAlreadyBound)
	{
		glTexParameteri(Type, GL_TEXTURE_MAG_FILTER, Impl::GetTextureFilterGL(magFilter.Filter));
	}

	void Texture::Dispose()
	{
		if (ID > 0)
			glDeleteTextures(1, &ID);
		ID = 0;
	}

	Texture Texture::FromGeneratedGlId(GLenum type, unsigned int glID, GLenum internalFormat)
	{
		return Texture(type, internalFormat, glID);
	}

	GLenum Texture::Impl::GetTextureFilterGL(TextureFilter filter)
	{
		switch (filter)
		{
			case TextureFilter::Nearest: default: return GL_NEAREST;
			case TextureFilter::Bilinear: return GL_LINEAR;
			case TextureFilter::NearestClosestMipmap: return GL_NEAREST_MIPMAP_NEAREST;
			case TextureFilter::BilinearClosestMipmap: return GL_LINEAR_MIPMAP_NEAREST;
			case TextureFilter::NearestInterpolateMipmap: return GL_NEAREST_MIPMAP_LINEAR;
			case TextureFilter::Trilinear: return GL_LINEAR_MIPMAP_LINEAR;
		}
	}

	template <typename PixelChannelType>
	Texture Texture::Loader::FromFile2D(const std::string& filepath, bool flip, MinTextureFilter minFilter, MagTextureFilter magFilter, GLenum internalFormat)
	{
		if (flip)
			stbi_set_flip_vertically_on_load(true);

		int width, height, nrChannels;
		void* data = nullptr;
		{
			if (std::is_same<PixelChannelType, unsigned char>::value)
				data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 0);
			else if (std::is_same<PixelChannelType, float>::value)
				data = stbi_loadf(filepath.c_str(), &width, &height, &nrChannels, 0);
			else
				std::cerr << "ERROR! Unrecognized type of T for file " + filepath + "\n";
		}

		if (!data)
		{
			std::cerr << "Can't load texture from " << filepath << '\n';
			Texture tex = FromBuffer2D<float>(Vec2u(1, 1), Math::GetDataPtr(Vec3f(1.0f, 0.0f, 1.0f)), 3, GL_RGB);	//set the texture's color to pink so its obvious that this texture is missing
			tex.SetMinFilter(MinTextureFilter::Nearest(), true, true);	//disable mipmaps
			return tex;
		}

		Texture tex = FromBuffer2D<PixelChannelType>(Vec2u(width, height), data, nrChannels, internalFormat);
		tex.SetPath(filepath);

		tex.SetMinFilter(minFilter, true, true);
		tex.SetMagFilter(magFilter, true);

		tex.SetWrap(GL_REPEAT, GL_REPEAT, 0, true);

		if (flip)
			stbi_set_flip_vertically_on_load(false);

		stbi_image_free(data);

		return tex;
	}

	template Texture Texture::Loader::FromFile2D<unsigned char>(const std::string&, bool, MinTextureFilter, MagTextureFilter, GLenum);
	template Texture Texture::Loader::FromFile2D<float>(const std::string&, bool, MinTextureFilter, MagTextureFilter, GLenum);

	template<typename PixelChannelType>
	Texture Texture::Loader::FromBuffer2D(unsigned int rowWidth, const void* buffer, GLenum internalFormat, int desiredChannels)
	{
		int width = 0, height = 0, nrChannels = 0;

		void* data = nullptr;
		if (std::is_same<PixelChannelType, unsigned char>::value) data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(buffer), rowWidth, &width, &height, &nrChannels, desiredChannels);
		else if (std::is_same<PixelChannelType, float>::value) data = stbi_loadf_from_memory(reinterpret_cast<const stbi_uc*>(buffer), rowWidth, &width, &height, &nrChannels, desiredChannels);
		else std::cout << "ERROR: Cannot recognize type passed to FromBuffer2D.\n";

		if (!data)
			std::cerr << "ERROR: Cannot load pixels from memory in function FromBuffer2D.\n";

		Texture result = FromBuffer2D<PixelChannelType>(Vec2u(width, height), data, nrChannels, internalFormat);

		stbi_image_free(data);

		return result;
	}
	template Texture Texture::Loader::FromBuffer2D<unsigned char>(unsigned int, const void*, GLenum, int);
	template Texture Texture::Loader::FromBuffer2D<float>(unsigned int, const void*, GLenum, int);

	template<typename PixelChannelType>
	Texture Texture::Loader::FromBuffer2D(const Vec2u& size, const void* buffer, int nrChannels, GLenum internalFormat)
	{
		Texture tex = Impl::GenerateEmpty(GL_TEXTURE_2D, internalFormat);
		GLenum format = nrChannelsToFormat(nrChannels, false, &internalFormat);

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.x, size.y, 0, format, Impl::GetChannelTypeEnum<PixelChannelType>(), buffer);

		tex.SetMinFilter(MinTextureFilter::Bilinear(), true, true);	//disable mipmaps by default

		return tex;
	}
	template Texture Texture::Loader::FromBuffer2D<unsigned char>(const Vec2u&, const void*, int, GLenum);
	template Texture Texture::Loader::FromBuffer2D<float>(const Vec2u&, const void*, int, GLenum);


	template<typename PixelChannelType>
	Texture Texture::Loader::FromBuffer2DArray(const Vec3u& size, const void* buffer, int nrChannels, GLenum internalFormat)
	{
		Texture tex = Impl::GenerateEmpty(GL_TEXTURE_2D_ARRAY, internalFormat);
		GLenum format = nrChannelsToFormat(nrChannels, false, &internalFormat);

		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, size.x, size.y, size.z, 0, format, Impl::GetChannelTypeEnum<PixelChannelType>(), buffer);

		tex.SetMinFilter(MinTextureFilter::Bilinear(), true, true);	//disable mipmaps by default

		return tex;
	}
	template Texture Texture::Loader::FromBuffer2DArray<unsigned char>(const Vec3u&, const void*, int, GLenum);

	template<typename PixelChannelType>
	Texture Texture::Loader::FromBuffersCubemap(const Vec2u& oneSideSize, const void* buffers[6], int nrChannels, GLenum internalFormat)
	{
		Texture tex = Impl::GenerateEmpty(GL_TEXTURE_CUBE_MAP, internalFormat);

		GLenum format = nrChannelsToFormat(nrChannels, false, &internalFormat);

		for (int i = 0; i < ((bCubemap) ? (6) : (1)); i++)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalformat, oneSideSize.x, oneSideSize.y, 0, format, Impl::GetChannelTypeEnum<PixelChannelType>(), buffers[i]);

		tex.SetMinFilter(MinTextureFilter::Bilinear(), true, true);

		return tex;
	}

	template <> GLenum Texture::Loader::Impl::GetChannelTypeEnum<unsigned char>() { return GL_UNSIGNED_BYTE; }
	template <> GLenum Texture::Loader::Impl::GetChannelTypeEnum<float>() { return GL_FLOAT; }

	template GLenum Texture::Loader::Impl::GetChannelTypeEnum<unsigned char>();
	template GLenum Texture::Loader::Impl::GetChannelTypeEnum<float>();

	Texture Texture::Loader::Impl::GenerateEmpty(GLenum texType, GLenum internalFormat)
	{
		Texture tex;
		tex.GenerateID(texType);
		tex.Bind();
		tex.SetPath("buffer");		
		tex.InternalFormat = internalFormat;

		return tex;
	}

	template <typename PixelChannelType>
	Texture Texture::Loader::ReserveEmpty2D(const Vec2u& size, GLenum internalFormat)
	{
		return FromBuffer2D<PixelChannelType>(size, nullptr, 3, internalFormat);
	}

	template<typename PixelChannelType>
	Texture Texture::Loader::ReserveEmpty2DArray(const Vec3u& size, GLenum internalFormat)
	{
		return FromBuffer2DArray<PixelChannelType>(size, nullptr, 1, internalFormat);
	}
	template Texture Texture::Loader::ReserveEmpty2DArray<unsigned char>(const Vec3u&, GLenum);

	template<typename PixelChannelType>
	Texture Texture::Loader::ReserveEmptyCubemap(const Vec2u& oneSideSize, GLenum internalFormat)
	{
		return FromBuffersCubemap<PixelChannelType>(oneSideSize, { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }, 3, internalFormat);
	}

	Texture Texture::Loader::Assimp::FromAssimpEmbedded(const aiTexture& assimpTex, bool sRGB, MinTextureFilter minFilter, MagTextureFilter magFilter)
	{
		Texture tex = FromBuffer2D(assimpTex.mWidth, &assimpTex.pcData[0].b, (sRGB) ? (GL_SRGB_ALPHA) : (GL_RGBA));
		tex.SetMinFilter(minFilter, true, true);
		tex.SetMagFilter(magFilter, true);
		return tex;
	}

	NamedTexture::NamedTexture(const Texture& tex, const std::string& name) :
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
		case 2: if (bBGRA) std::cout << "ERROR: in nrChannelsToFormat: No support for GL_BG format. Texture will be loaded as GL_RG\n"; return GL_RG;
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

	std::shared_ptr<Texture> reserveTexture(Vec2u size, GLenum internalformat, GLenum type, GLenum magFilter, GLenum minFilter, GLenum texType, unsigned int samples, std::string texName, GLenum format)
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

	std::shared_ptr<Texture> reserveTexture(Vec3u size, GLenum internalformat, GLenum type, GLenum magFilter, GLenum minFilter, GLenum texType, unsigned int samples, std::string texName, GLenum format)
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
}