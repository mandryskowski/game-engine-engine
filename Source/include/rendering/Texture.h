#pragma once
#include <string>
#include <glad/glad.h>
#include <math/Vec.h>
#include <memory>
#include <cereal/access.hpp>
#include <cereal/macros.hpp>
#include <math/Vec.h>
#include <array>

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

		void SetBorderColor(const Vec4f& color);

		/**
		 * @brief Generate mipmap for this texture. Only valid if MinFilter was set to NearestClosestMipmap, BilinearClosestMipamp, NearestInterpolateMipmap or Trilinear.
		 * @param isAlreadyBound: a flag indicating whether the function can skip binding the texture (purely for optimization)a flag indicatin
		*/
		void GenerateMipmap(bool isAlreadyBound = false);

		class MinFilter
		{
		public:
			static MinFilter Nearest() { return MinFilter(TextureFilter::Nearest); }
			static MinFilter Bilinear() { return MinFilter(TextureFilter::Bilinear); }
			static MinFilter NearestClosestMipmap() { return MinFilter(TextureFilter::NearestClosestMipmap); }
			static MinFilter BilinearClosestMipmap() { return MinFilter(TextureFilter::BilinearClosestMipmap); }
			static MinFilter NearestInterpolateMipmap() { return MinFilter(TextureFilter::NearestInterpolateMipmap); }
			static MinFilter Trilinear() { return MinFilter(TextureFilter::Trilinear); }
			bool operator==(const MinFilter& rhs) { return Filter == rhs.Filter; }
		private:
			TextureFilter Filter;
			friend class Texture;
			MinFilter(TextureFilter filter) : Filter(filter) {}
		};

		class MagFilter
		{
		public:
			static MagFilter Nearest() { return MagFilter(TextureFilter::Nearest); }
			static MagFilter Bilinear() { return MagFilter(TextureFilter::Bilinear); }
			bool operator==(const MagFilter& rhs) { return Filter == rhs.Filter; }
		private:
			TextureFilter Filter;
			friend class Texture;
			MagFilter(TextureFilter filter) : Filter(filter) {}
		};

		class Format
		{
		public:
			static Format None() { return GL_ZERO; }
			static Format Red() { return GL_RED; }
			static Format Green() { return GL_GREEN; }
			static Format Blue() { return GL_BLUE; }
			static Format RG() { return GL_RG; }
			static Format RGB() { return GL_RGB; }
			static Format SRGB() { return GL_SRGB; }
			static Format RGBA() { return GL_RGBA; }
			static Format SRGBA() { return GL_SRGB_ALPHA; }

			static Format Depth() { return GL_DEPTH_COMPONENT; }
			static Format DepthStencil() { return GL_DEPTH_STENCIL; }
			
			static Format FromNrChannels(unsigned int nrChannels)
			{
				switch (nrChannels)
				{
				case 1: return Red();
				case 2: return RG();
				case 3: return RGB();
				case 4: return RGBA();
				}
			}
			static Format FromGLConst(GLenum formatGlConst)
			{
				return formatGlConst;
			}

			struct Float16
			{
				static Format Red() { return GL_R16F; }
				static Format RG() { return GL_RG16F; }
				static Format RGB() { return GL_RGB16F; }
				static Format RGBA() { return GL_RGBA16F; }

				static Format FromNrChannels(unsigned int nrChannels)
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
				static Format Red() { return GL_R32F; }
				static Format RG() { return GL_RG32F; }
				static Format RGB() { return GL_RGB32F; }
				static Format RGBA() { return GL_RGBA32F; }

				static Format FromNrChannels(unsigned int nrChannels)
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

			struct Uint32
			{
				static Format Depth24Stencil8() { return GL_DEPTH24_STENCIL8; }
			};

			GLenum GetFormatGl() const { return FormatGl; }
			bool operator==(const Format& rhs) { return rhs.FormatGl == FormatGl; }
		private:
			Format(GLenum formatGl) : FormatGl(formatGl) {}	//implicit construction for methods of Texture::Format
			GLenum FormatGl;
		};

		/**
		 * @brief 
		 * @param minFilter: a Texture::MinFilter object specifying the filtering used when the texture is minified.
		 * @param generateMipmapIfPossible: a flag indicating whether whether the function should call Texture::GenerateMipmap if minFilter is suitable for mipmaps.
		 * @param isAlreadyBound: a flag indicating whether the function can skip binding the texture (purely for optimization)
		*/
		void SetMinFilter(MinFilter minFilter = MinFilter::Trilinear(), bool isAlreadyBound = false, bool generateMipmapIfPossible = true);
		/**
		 * @brief 
		 * @param magFilter: a TextureFilter enum specifying the filtering used when the texture is magnified. Valid values: Nearest and Bilinear.
		 * @param isAlreadyBound: a flag indicating whether the function can skip binding the texture (purely for optimization)
		*/
		void SetMagFilter(MagFilter magFilter = MagFilter::Bilinear(), bool isAlreadyBound = false);

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

			*this = Loader<>::FromFile2D(Path, Format::FromGLConst(internalFormat), false, MinFilter::Trilinear(), MagFilter::Bilinear());
			SetWrap(GL_REPEAT, GL_REPEAT, 0, true);
		}

		virtual void Dispose();

		struct LoaderArtificialType
		{
			struct Uint24_8 {};
		};
		template <typename PixelChannelType = unsigned char>
		struct Loader
		{
			static Texture FromFile2D(const std::string&, Format internalFormat = Format::RGBA(), bool flip = false, MinFilter = MinFilter::Trilinear(), MagFilter = MagFilter::Bilinear());
			static Texture FromFileEquirectangularCubemap(const std::string&);

			static Texture FromBuffer2D(unsigned int width, const void* buffer, Format internalFormat = Format::RGBA(), int desiredChannels = 0);
			static Texture FromBuffer2D(const Vec2u& size, const void* buffer, Format internalFormat, int nrChannels);
			static Texture FromBuffer2D(const Vec2u& size, const void* buffer, Format internalFormat = Format::RGBA(), Format format = Format::RGB());
			static Texture FromBuffer2DArray(const Vec3u& size, const void* buffer, Format internalFormat = Format::RGBA(), Format format = Format::RGB());
			static Texture FromBuffer2DArray(const Vec3u& size, const void* buffer, Format internalFormat = Format::RGBA(), int nrChannels = 3);
			static Texture FromBuffersCubemap(const Vec2u& oneSideSize, std::array<const void*, 6> buffers = { }, Format internalFormat = Format::RGB(), int nrChannels = 3);
			static Texture FromBuffersCubemapArray(const Vec3u& size, const void* buffer, Format internalFormat = Format::RGB(), Format format = Format::RGB());
			static Texture FromBuffersCubemapArray(const Vec3u& size, const void* buffer, Format internalFormat = Format::RGB(), int nrChannels = 3);

			static Texture ReserveEmpty2D(const Vec2u& size, Format internalFormat = Format::RGBA(), Format format = Format::RGB());
			static Texture ReserveEmpty2DArray(const Vec3u& size, Format internalFormat = Format::RGBA(), Format format = Format::RGB());
			static Texture ReserveEmptyCubemap(const Vec2u& oneSideSize, Format internalFormat = Format::RGB());
			static Texture ReserveEmptyCubemapArray(const Vec3u& size, Format internalFormat = Format::RGB(), Format format = Format::RGB());

			struct Assimp
			{
				static Texture FromAssimpEmbedded(const aiTexture&, bool sRGB, MinFilter = MinFilter::Trilinear(), MagFilter = MagFilter::Bilinear());
			};

			struct Impl
			{
				static Texture GenerateEmpty(GLenum texType, Format internalFormat);
				static GLenum GetChannelTypeEnum();
			};
		};
		static Texture FromGeneratedGlId(const Vec2u& size, GLenum type, unsigned int glID, Format internalFormat);

		protected:
		Texture(const Vec2u& size, GLenum type, Format internalFormat = Format::RGBA(), unsigned int id = 0, const std::string& path = "");
		Texture(const Vec3u& size, GLenum type, Format internalFormat = Format::RGBA(), unsigned int id = 0, const std::string& path = "");

		Format InternalFormat;
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

	GLenum internalFormatToAlpha(GLenum);
	NamedTexture reserveTexture(Vec2u size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefined2DTexture", GLenum format = GL_ZERO);	//Pass the last argument to override the default internalformat->format conversion
	NamedTexture reserveTexture(Vec3u size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefined3DTexture", GLenum format = GL_ZERO);
}

namespace cereal
{
	template <typename Archive>
	struct specialize<Archive, GEE::NamedTexture, cereal::specialization::member_serialize> {};
}