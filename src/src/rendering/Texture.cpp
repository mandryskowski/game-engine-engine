#include <rendering/Texture.h>
#include <stb/stb_image.h>
#include <iostream>
#include <assimp/texture.h>


namespace GEE
{
	Texture::Texture(const Vec2u& size, GLenum type, Format internalFormat, unsigned int id, const std::string& path) :
		Texture(Vec3u(size, 1.0f), type, internalFormat, id, path)
	{}
	Texture::Texture(const Vec3u& size, GLenum type, Format internalFormat, unsigned int id, const std::string& path) :
		Type(type),
		InternalFormat(internalFormat),
		ID(id),
		Size(size),
		Path(path)
	{
	}

	Texture::Texture() :
		Texture(Vec2u(0), GL_TEXTURE_2D)
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

	Vec2u Texture::GetSize2D() const
	{
		return Size;
	}

	Vec3u Texture::GetSize3D() const
	{
		return Size;
	}

	bool Texture::HasBeenGenerated() const
	{
		return ID != 0;
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

	void Texture::SetBorderColor(const Vec4f& color)
	{
		glTexParameterfv(Type, GL_TEXTURE_BORDER_COLOR, Math::GetDataPtr(color));
	}

	void Texture::GenerateMipmap(bool isAlreadyBound)
	{
		if (!isAlreadyBound) Bind();
		glGenerateMipmap(Type);
	}

	void Texture::SetMinFilter(MinFilter minFilter, bool isAlreadyBound, bool generateMipmapIfPossible)
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
				default: break;
			}
	}

	void Texture::SetMagFilter(MagFilter magFilter, bool isAlreadyBound)
	{
		glTexParameteri(Type, GL_TEXTURE_MAG_FILTER, Impl::GetTextureFilterGL(magFilter.Filter));
	}

	void Texture::Dispose()
	{
		if (HasBeenGenerated())
			glDeleteTextures(1, &ID);
		ID = 0;
	}

	Texture Texture::FromGeneratedGlId(const Vec2u& size, GLenum type, unsigned int glID, Format internalFormat)
	{
		return Texture(size, type, internalFormat, glID);
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
	Texture Texture::Loader<PixelChannelType>::FromFile2D(const std::string& filepath, Format internalFormat, bool flip, MinFilter minFilter, MagFilter magFilter)
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
			if (stbi_failure_reason())
				std::cout << "Stbi failure: " << stbi_failure_reason() << "\n";
			Texture tex = Texture::Loader<float>::FromBuffer2D(Vec2u(1, 1), Math::GetDataPtr(Vec3f(1.0f, 0.0f, 1.0f)), Format::RGB(), 3);	//set the texture's color to pink so its obvious that this texture is missing
			tex.SetMinFilter(MinFilter::Nearest(), true, true);	//disable mipmaps
			return tex;
		}

		Texture tex = FromBuffer2D(Vec2u(width, height), data, internalFormat, nrChannels);
		tex.SetPath(filepath);

		tex.SetMinFilter(minFilter, true, true);
		tex.SetMagFilter(magFilter, true);

		if (flip)
			stbi_set_flip_vertically_on_load(false);

		stbi_image_free(data);

		return tex;
	}


	template<typename PixelChannelType>
	Texture Texture::Loader<PixelChannelType>::FromBuffer2D(unsigned int rowWidth, const void* buffer, Format internalFormat, int desiredChannels)
	{
		int width = 0, height = 0, nrChannels = 0;

		void* data = nullptr;
		if (std::is_same<PixelChannelType, unsigned char>::value) data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(buffer), rowWidth, &width, &height, &nrChannels, desiredChannels);
		else if (std::is_same<PixelChannelType, float>::value) data = stbi_loadf_from_memory(reinterpret_cast<const stbi_uc*>(buffer), rowWidth, &width, &height, &nrChannels, desiredChannels);
		else std::cout << "ERROR: Cannot recognize type passed to FromBuffer2D.\n";
		
		if (!data)
		{
			std::cout << "ERROR: Cannot load pixels from memory in function FromBuffer2D.\n";
			if (stbi_failure_reason())
				std::cout << "Stbi failure: " << stbi_failure_reason() << "\n";
		}

		std::cout << "Deduced buffer width: " << width << ", and height: " << height << ". Nr of channels: " << nrChannels << "\n";
		Texture result = FromBuffer2D(Vec2u(width, height), data, internalFormat, nrChannels);
		stbi_image_free(data);

		return result;
	}

	template<typename PixelChannelType>
	Texture Texture::Loader<PixelChannelType>::FromBuffer2D(const Vec2u& size, const void* buffer, Format internalFormat, int nrChannels)
	{
		return FromBuffer2D(size, buffer, Impl::AssertCorrectChannelsForInternalFormat(internalFormat, nrChannels), Format::FromNrChannels(nrChannels));
	}

	template<typename PixelChannelType>
	Texture Texture::Loader<PixelChannelType>::FromBuffer2D(const Vec2u& size, const void* buffer, Format internalFormat, Format format)
	{
		Texture tex = Impl::GenerateEmpty(GL_TEXTURE_2D, internalFormat);

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat.GetEnumGL(), size.x, size.y, 0, format.GetEnumGL(), Impl::GetChannelTypeEnum(), buffer);
		tex.SetSize(size);

		tex.SetMinFilter(MinFilter::Bilinear(), true, true);	//disable mipmaps by default
		tex.SetWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, true);

		return tex;
	}

	template<typename PixelChannelType>
	Texture Texture::Loader<PixelChannelType>::FromBuffer2DArray(const Vec3u& size, const void* buffer, Format internalFormat, Format format)
	{
		Texture tex = Impl::GenerateEmpty(GL_TEXTURE_2D_ARRAY, internalFormat);

		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat.GetEnumGL(), size.x, size.y, size.z, 0, format.GetEnumGL(), Impl::GetChannelTypeEnum(), buffer);
		tex.SetSize(size);

		tex.SetMinFilter(MinFilter::Bilinear(), true, true);	//disable mipmaps by default
		tex.SetWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, true);

		return tex;
	}


	template<typename PixelChannelType>
	Texture Texture::Loader<PixelChannelType>::FromBuffer2DArray(const Vec3u& size, const void* buffer, Format internalFormat, int nrChannels)
	{
		return FromBuffer2DArray(size, buffer, internalFormat, Format::FromNrChannels(nrChannels));
	}

	template<typename PixelChannelType>
	Texture Texture::Loader<PixelChannelType>::FromBuffersCubemap(const Vec2u& oneSideSize, std::array<const void*, 6> buffers, Format internalFormat, int nrChannels)
	{
		Texture tex = Impl::GenerateEmpty(GL_TEXTURE_CUBE_MAP, internalFormat);
		const Format format = Format::FromNrChannels(nrChannels);
		
		for (int i = 0; i < 6; i++)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat.GetEnumGL(), oneSideSize.x, oneSideSize.y, 0, format.GetEnumGL(), Impl::GetChannelTypeEnum(), buffers[i]);
		tex.SetSize(oneSideSize);

		tex.SetMinFilter(MinFilter::Bilinear(), true, true);
		tex.SetWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, true);

		return tex;
	}

	template<typename PixelChannelType>
	Texture Texture::Loader<PixelChannelType>::FromBuffersCubemapArray(const Vec3u& size, const void* buffer, Format internalFormat, Format format)
	{
		Texture tex = Impl::GenerateEmpty(GL_TEXTURE_CUBE_MAP_ARRAY, internalFormat);

		glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, internalFormat.GetEnumGL(), size.x, size.y, size.z * 6, 0, format.GetEnumGL(), Impl::GetChannelTypeEnum(), buffer);
		tex.SetSize(size);

		tex.SetMinFilter(MinFilter::Bilinear(), true, true);
		tex.SetWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, true);

		return tex;
	}

	template<typename PixelChannelType>
	Texture Texture::Loader<PixelChannelType>::FromBuffersCubemapArray(const Vec3u& size, const void* buffer, Format internalFormat, int nrChannels)
	{
		return FromBuffersCubemapArray(size, buffer, internalFormat, Format::FromNrChannels(nrChannels));
	}

	template <typename PixelChannelType>
	Texture Texture::Loader<PixelChannelType>::Impl::GenerateEmpty(GLenum texType, Format internalFormat)
	{
		Texture tex;
		tex.GenerateID(texType);
		tex.Bind();
		tex.SetPath("buffer");		
		tex.InternalFormat = internalFormat;

		return tex;
	}

	template <> GLenum Texture::Loader<unsigned char>::Impl::GetChannelTypeEnum() { return GL_UNSIGNED_BYTE; }

	template<typename PixelChannelType>
	Texture::Format Texture::Loader<PixelChannelType>::Impl::AssertCorrectChannelsForInternalFormat(Format internalFormat, unsigned int nrChannels)
	{
		switch (internalFormat.GetEnumGL())
		{
			case GL_RGBA: if (nrChannels < 4) return Format::FromNrChannels(nrChannels); break;
			case GL_RGB: if (nrChannels < 3) return Format::FromNrChannels(nrChannels); break;
			case GL_RG: if (nrChannels < 2) return Format::FromNrChannels(nrChannels); break;
		}

		return internalFormat;

	}
	template <> GLenum Texture::Loader<unsigned int>::Impl::GetChannelTypeEnum() { return GL_UNSIGNED_INT; }
	template <> GLenum Texture::Loader<float>::Impl::GetChannelTypeEnum() { return GL_FLOAT; }
	template <> GLenum Texture::Loader<Texture::LoaderArtificialType::Uint24_8>::Impl::GetChannelTypeEnum() { return GL_UNSIGNED_INT_24_8; }

	template <typename PixelChannelType>
	Texture Texture::Loader<PixelChannelType>::ReserveEmpty2D(const Vec2u& size, Format internalFormat, Format format)
	{
		return FromBuffer2D(size, nullptr, internalFormat, format);
	}

	template<typename PixelChannelType>
	Texture Texture::Loader<PixelChannelType>::ReserveEmpty2DArray(const Vec3u& size, Format internalFormat, Format format)
	{
		return FromBuffer2DArray(size, nullptr, internalFormat, format);
	}

	template<typename PixelChannelType>
	Texture Texture::Loader<PixelChannelType>::ReserveEmptyCubemap(const Vec2u& oneSideSize, Format internalFormat)
	{
		return FromBuffersCubemap(oneSideSize, { }, internalFormat, 3);
	}

	template<typename PixelChannelType>
	Texture Texture::Loader<PixelChannelType>::ReserveEmptyCubemapArray(const Vec3u& size, Format internalFormat, Format format)
	{
		return FromBuffersCubemapArray(size, nullptr, internalFormat, format);
	}

	template <typename PixelChannelType>
	Texture Texture::Loader<PixelChannelType>::Assimp::FromAssimpEmbedded(const aiTexture& assimpTex, bool sRGB, MinFilter minFilter, MagFilter magFilter)
	{
		Texture tex = FromBuffer2D(assimpTex.mWidth, &assimpTex.pcData[0].b, (sRGB) ? (Format::SRGBA()) : (Format::RGBA()));
		tex.SetMinFilter(minFilter, true, true);
		tex.SetMagFilter(magFilter, true);
		return tex;
	}

	void Texture::SetSize(const Vec2u& size)
	{
		Size = Vec3u(size, 1.0f);
	}

	void Texture::SetSize(const Vec3u& size)
	{
		Size = size;
	}


	template Texture::Loader<unsigned char>;
	template Texture::Loader<unsigned int>;
	template Texture::Loader<float>;
	template Texture::Loader<Texture::LoaderArtificialType::Uint24_8>;

	NamedTexture::NamedTexture(const Texture& tex, const std::string& name) :
		Texture(tex),
		ShaderName(name)
	{
	}

	std::string NamedTexture::GetShaderName() const
	{
		return ShaderName;
	}

	void NamedTexture::SetShaderName(const std::string& name)
	{
		ShaderName = name;
	}
}