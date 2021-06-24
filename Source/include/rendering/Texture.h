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
		GLenum Type;
		unsigned int ID;
	public:	Vec3u Size; protected:
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

		/**
		 * @brief Returns the 2D size of this Texture. For 2D textures and texture arrays it returns the size of a single texture, for cubemaps and cubemap arrays the size of a single side.
		 * @return Vec2u: x - width of single texture, y - height of single texture
		*/
		Vec2u GetSize2D() const;
		/**
		 * @brief Returns the 3D size of this Texture. For 2D textures and cubemaps the z component should be ignored, for 2d texture arrays and cubemap arrays the z component contains the amount of textures in the array.
		 * @return Vec3u: x - width of single texture, y - height of single texture, z - number of textures in the array
		*/
		Vec3u GetSize3D() const;

	private:
		void SetSize(const Vec2u& size);
		void SetSize(const Vec3u& size);
	public:

		bool HasBeenGenerated() const;

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

		class TextureFormat
		{
		public:
			static TextureFormat Red() { return GL_RED; }
			static TextureFormat Green() { return GL_GREEN; }
			static TextureFormat Blue() { return GL_BLUE; }
			static TextureFormat RG() { return GL_RG; }
			static TextureFormat RGB() { return GL_RGB; }
			static TextureFormat SRGB() { return GL_SRGB; }
			static TextureFormat RGBA() { return GL_RGBA; }
			static TextureFormat SRGBA() { return GL_SRGB_ALPHA; }
			
			static TextureFormat FromNrChannels(unsigned int nrChannels)
			{
				switch (nrChannels)
				{
				case 1: return Red();
				case 2: return RG();
				case 3: return RGB();
				case 4: return RGBA();
				}
			}

			struct Float16
			{
				static TextureFormat Red() { return GL_R16F; }
				static TextureFormat RG() { return GL_RG16F; }
				static TextureFormat RGB() { return GL_RGB16F; }
				static TextureFormat RGBA() { return GL_RGBA16F; }

				static TextureFormat FromNrChannels(unsigned int nrChannels)
				{
					switch (nrChannels)
					{
					case 1: return Red();
					case 2: return RG();
					case 3: return RGB();
					case 4: return RGBA();
					}
				}
			};
			struct Float32
			{
				static TextureFormat Red() { return GL_R32F; }
				static TextureFormat RG() { return GL_RG32F; }
				static TextureFormat RGB() { return GL_RGB32F; }
				static TextureFormat RGBA() { return GL_RGBA32F; }

				static TextureFormat FromNrChannels(unsigned int nrChannels)
				{
					switch (nrChannels)
					{
					case 1: return Red();
					case 2: return RG();
					case 3: return RGB();
					case 4: return RGBA();
					}
				}
			};
			GLenum GetFormatGl() const { return FormatGl; }
			bool operator==(const TextureFormat& rhs) { return rhs.FormatGl == FormatGl; }
		private:
			friend class Loader;

			friend class Texture;
			TextureFormat(GLenum formatGl) : FormatGl(formatGl) {}
			GLenum FormatGl;
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
			archive(cereal::make_nvp("Type", Type), cereal::make_nvp("InternalFormat", InternalFormat.GetFormatGl()), cereal::make_nvp("Path", Path));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			GLenum internalFormat;
			archive(cereal::make_nvp("Type", Type), cereal::make_nvp("InternalFormat", internalFormat), cereal::make_nvp("Path", Path));
			//if (!Path.empty() && Path.front() == '*')
			//else
			*this = Loader<>::FromFile2D(Path, internalFormat, false, MinTextureFilter::Trilinear(), MagTextureFilter::Bilinear());
			SetWrap(GL_REPEAT, GL_REPEAT, 0, true);
		}

		virtual void Dispose();
		template <typename PixelChannelType = unsigned char>
		struct Loader
		{
			static Texture FromFile2D(const std::string&, TextureFormat internalFormat = TextureFormat::RGBA(), bool flip = false, MinTextureFilter = MinTextureFilter::Trilinear(), MagTextureFilter = MagTextureFilter::Bilinear());
			static Texture FromFileEquirectangularCubemap(const std::string&);

			static Texture FromBuffer2D(unsigned int width, const void* buffer, TextureFormat internalFormat = TextureFormat::RGBA(), int desiredChannels = 0);
			static Texture FromBuffer2D(const Vec2u& size, const void* buffer, int nrChannels = 3, TextureFormat internalFormat = TextureFormat::RGBA());
			static Texture FromBuffer2DArray(const Vec3u& size, const void* buffer, int nrChannels = 3, TextureFormat internalFormat = TextureFormat::RGBA());
			static Texture FromBuffersCubemap(const Vec2u& oneSideSize, const void* buffers[6] = { }, int nrChannels = 3, TextureFormat internalFormat = TextureFormat::RGBA());

			static Texture ReserveEmpty2D(const Vec2u& size, TextureFormat internalFormat = TextureFormat::RGBA());
			static Texture ReserveEmpty2DArray(const Vec3u& size, TextureFormat internalFormat = TextureFormat::RGBA());
			static Texture ReserveEmptyCubemap(const Vec2u& oneSideSize, TextureFormat internalFormat = TextureFormat::RGB());
			static Texture ReserveEmptyCubemapArray(const Vec4u& size);

			struct Assimp
			{
				static Texture FromAssimpEmbedded(const aiTexture&, bool sRGB, MinTextureFilter = MinTextureFilter::Trilinear(), MagTextureFilter = MagTextureFilter::Bilinear());
			};

			struct Impl
			{
				static Texture GenerateEmpty(GLenum texType, TextureFormat internalFormat);
				static GLenum GetChannelTypeEnum();
			};
		};
		static Texture FromGeneratedGlId(const Vec2u& size, GLenum type, unsigned int glID, TextureFormat internalFormat);

		protected:
		Texture(const Vec2u& size, GLenum type, TextureFormat internalFormat = TextureFormat::RGBA(), unsigned int id = 0, const std::string& path = "");
		Texture(const Vec3u& size, GLenum type, TextureFormat internalFormat = TextureFormat::RGBA(), unsigned int id = 0, const std::string& path = "");

		TextureFormat InternalFormat;
	};

	class NamedTexture : public Texture
	{
		std::string ShaderName;

	public:
		NamedTexture(const Texture& tex = Texture(), const std::string& name = "diffuse");

		std::string GetShaderName() const;
		void SetShaderName(std::string);
		template <typename Archive> void Serialize(Archive& archive)
		{
			archive(cereal::make_nvp("ShaderName", ShaderName), cereal::make_nvp("Texture", cereal::base_class<Texture>(this)));
		}
	};


	bool containsAlphaChannel(GLenum internalformat);
	GLenum nrChannelsToFormat(int nrChannels, bool bBGRA = false, GLenum* internalFormat = nullptr);

	GLenum internalFormatToAlpha(GLenum);
	NamedTexture reserveTexture(Vec2u size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefined2DTexture", GLenum format = GL_ZERO);	//Pass the last argument to override the default internalformat->format conversion
	NamedTexture reserveTexture(Vec3u size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefined3DTexture", GLenum format = GL_ZERO);
}

namespace cereal
{
	template <typename Archive>
	struct specialize<Archive, GEE::NamedTexture, cereal::specialization::member_serialize> {};
}