#pragma once
#include <string>
#include <glad/glad.h>
#include <math/Vec.h>
#include <memory>
#include <cereal/access.hpp>
#include <cereal/macros.hpp>
#include <math/Vec.h>

struct aiTexture;

namespace GEE
{
	class Texture
	{
	protected:
		GLenum Type, InternalFormat;
		unsigned int ID;
		std::string Path;

		enum class TextureFilter
		{
			Nearest,
			Bilinear,
			NearestClosestMipmap,
			BilinearClosestMipmap,
			NearestInterpolateMipmap,
			Trilinear
		};

		friend class Material;
	private:
		Texture(GLenum type, GLenum internalFormat = GL_RGB, unsigned int id = 0, const std::string& path = "");
	public:
		/**
		 * @brief Constructs undefined texture.
		*/
		Texture();
		unsigned int GenerateID(GLenum type = GL_ZERO);

		void Bind(int texSlot = -1) const;

		GLenum GetType() const;
		unsigned int GetID() const;
		std::string GetPath() const;

		void SetPath(const std::string&);
		void SetWrap(GLenum wrapS = 0, GLenum wrapT = 0, GLenum wrapR = 0, bool isAlreadyBound = false);

		/**
		 * @brief Generate mipmap for this texture. Only valid if MinFilter was set to NearestClosestMipmap, BilinearClosestMipamp, NearestInterpolateMipmap or Trilinear.
		 * @param isAlreadyBound: a flag indicating whether the function can skip binding the texture (purely for optimization)a flag indicatin
		*/
		void GenerateMipmap(bool isAlreadyBound = false);

		class MinTextureFilter
		{
		public:
			static MinTextureFilter Nearest() { return MinTextureFilter(TextureFilter::Nearest); }
			static MinTextureFilter Bilinear() { return MinTextureFilter(TextureFilter::Bilinear); }
			static MinTextureFilter NearestClosestMipmap() { return MinTextureFilter(TextureFilter::NearestClosestMipmap); }
			static MinTextureFilter BilinearClosestMipmap() { return MinTextureFilter(TextureFilter::BilinearClosestMipmap); }
			static MinTextureFilter NearestInterpolateMipmap() { return MinTextureFilter(TextureFilter::NearestInterpolateMipmap); }
			static MinTextureFilter Trilinear() { return MinTextureFilter(TextureFilter::Trilinear); }
			bool operator==(const MinTextureFilter& rhs) { return Filter == rhs.Filter; }
		private:
			TextureFilter Filter;
			friend class Texture;
			MinTextureFilter(TextureFilter filter) : Filter(filter) {}
		};

		class MagTextureFilter
		{
		public:
			static MagTextureFilter Nearest() { return MagTextureFilter(TextureFilter::Nearest); }
			static MagTextureFilter Bilinear() { return MagTextureFilter(TextureFilter::Bilinear); }
			bool operator==(const MagTextureFilter& rhs) { return Filter == rhs.Filter; }
		private:
			TextureFilter Filter;
			friend class Texture;
			MagTextureFilter(TextureFilter filter) : Filter(filter) {}
		};

		/**
		 * @brief 
		 * @param minFilter: a MinTextureFilter object specifying the filtering used when the texture is minified.
		 * @param generateMipmapIfPossible: a flag indicating whether whether the function should call Texture::GenerateMipmap if minFilter is suitable for mipmaps.
		 * @param isAlreadyBound: a flag indicating whether the function can skip binding the texture (purely for optimization)
		*/
		void SetMinFilter(MinTextureFilter minFilter = MinTextureFilter::Trilinear(), bool generateMipmapIfPossible = true, bool isAlreadyBound = false);
		/**
		 * @brief 
		 * @param magFilter: a TextureFilter enum specifying the filtering used when the texture is magnified. Valid values: Nearest and Bilinear.
		 * @param isAlreadyBound: a flag indicating whether the function can skip binding the texture (purely for optimization)
		*/
		void SetMagFilter(MagTextureFilter magFilter = MagTextureFilter::Bilinear(), bool isAlreadyBound = false);

		struct Impl
		{
			static GLenum GetTextureFilterGL(TextureFilter);
		};

		template <typename Archive> void Save(Archive& archive) const
		{
			archive(cereal::make_nvp("Type", Type), cereal::make_nvp("InternalFormat", InternalFormat), cereal::make_nvp("Path", Path));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			archive(cereal::make_nvp("Type", Type), cereal::make_nvp("InternalFormat", InternalFormat), cereal::make_nvp("Path", Path));
			//if (!Path.empty() && Path.front() == '*')
			//else
			*this = Loader::FromFile2D(Path, false, MinTextureFilter::Trilinear(), MagTextureFilter::Bilinear(), InternalFormat);
		}

		virtual void Dispose();
		struct Loader
		{
			template <typename PixelChannelType = unsigned char> static Texture FromFile2D(const std::string&, bool flip = false, MinTextureFilter = MinTextureFilter::Trilinear(), MagTextureFilter = MagTextureFilter::Bilinear(), GLenum internalFormat = GL_RGBA);
			template <typename PixelChannelType> static Texture FromFileEquirectangularCubemap(const std::string&);

			template <typename PixelChannelType = unsigned char> static Texture FromBuffer2D(unsigned int width, const void* buffer, GLenum internalFormat = GL_RGBA, int desiredChannels = 0);
			template <typename PixelChannelType = unsigned char> static Texture FromBuffer2D(const Vec2u& size, const void* buffer, int nrChannels = 3, GLenum internalFormat = GL_RGBA);
			template <typename PixelChannelType = unsigned char> static Texture FromBuffer2DArray(const Vec3u& size, const void* buffer, int nrChannels = 3, GLenum internalFormat = GL_RGBA);
			template <typename PixelChannelType = unsigned char> static Texture FromBuffersCubemap(const Vec2u& oneSideSize, const void* buffers[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }, int nrChannels = 3, GLenum internalFormat = GL_RGBA);

			template <typename PixelChannelType = unsigned char> static Texture ReserveEmpty2D(const Vec2u& size, GLenum internalFormat = GL_RGBA);
			template <typename PixelChannelType = unsigned char> static Texture ReserveEmpty2DArray(const Vec3u& size, GLenum internalFormat = GL_RGBA);
			template <typename PixelChannelType = unsigned char> static Texture ReserveEmptyCubemap(const Vec2u& oneSideSize, GLenum internalFormat = GL_RGB);
			template <typename PixelChannelType> static Texture ReserveEmptyCubemapArray(const Vec4u& size);

			struct Assimp
			{
				static Texture FromAssimpEmbedded(const aiTexture&, bool sRGB, MinTextureFilter = MinTextureFilter::Trilinear(), MagTextureFilter = MagTextureFilter::Bilinear());
			};

			struct Impl
			{
				template <typename PixelChannelType> static GLenum GetChannelTypeEnum();
				static Texture GenerateEmpty(GLenum texType, GLenum internalFormat);
			};
		};
		static Texture FromGeneratedGlId(GLenum type, unsigned int glID, GLenum internalFormat);
	};

	class NamedTexture : public Texture
	{
		std::string ShaderName;

	public:
		NamedTexture(const Texture& tex = Texture(), const std::string& name = "diffuse");

		std::string GetShaderName();
		void SetShaderName(std::string);
		template <typename Archive> void Serialize(Archive& archive)
		{
			archive(cereal::make_nvp("ShaderName", ShaderName), cereal::make_nvp("Texture", cereal::base_class<Texture>(this)));
		}
	};


	bool containsAlphaChannel(GLenum internalformat);
	GLenum nrChannelsToFormat(int nrChannels, bool bBGRA = false, GLenum* internalFormat = nullptr);

	GLenum internalFormatToAlpha(GLenum);
	std::shared_ptr<Texture> reserveTexture(Vec2u size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefined2DTexture", GLenum format = GL_ZERO);	//Pass the last argument to override the default internalformat->format conversion
	std::shared_ptr<Texture> reserveTexture(Vec3u size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefined3DTexture", GLenum format = GL_ZERO);
}

namespace cereal
{
	template <typename Archive>
	struct specialize<Archive, GEE::NamedTexture, cereal::specialization::member_serialize> {};
}