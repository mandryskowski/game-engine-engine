#pragma once
#include <string>
#include <glad/glad.h>
#include <math/Vec.h>
#include <cereal/access.hpp>
#include <array>

struct aiTexture;

namespace GEE
{
	class Texture
	{
		enum class TextureFilter;
		class Format;
		class MinFilter;
		class MagFilter;

		friend class Material;
	public:
		/**
		 * @brief Constructs undefined texture, which does not refer to any ID of a texture on the GPU.
		*/
		Texture();
		unsigned int GenerateID(GLenum type = GL_ZERO);

		/**
		 * @brief Binds this Texture, which allows to make changes to its OpenGL state. Many methods which do that will call Bind automatically, unless you pass an argument that disables it - to save performance.
		 * @param texSlot: pass this argunent to bind this texture to a specific slot (for use in shaders). Ignore if it doesn't matter - again, to save performance.
		*/
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

		bool HasBeenGenerated() const;

		void SetPath(const std::string&);
		void SetWrap(GLenum wrapS = 0, GLenum wrapT = 0, GLenum wrapR = 0, bool isAlreadyBound = false);

		void SetBorderColor(const Vec4f& color);

		/**
		 * @brief Generate mipmaps for this texture. Only valid if MinFilter was set to NearestClosestMipmap, BilinearClosestMipamp, NearestInterpolateMipmap or Trilinear.
		 * @param isAlreadyBound: a flag indicating whether the function can skip binding the texture (purely for optimization)a flag indicatin
		*/
		void GenerateMipmap(bool isAlreadyBound = false);

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

		/**
		 * @brief Disposes of the texture data contained on the GPU. Any other Texture objects that refer to the same ID will also be disposed. Has no effect if HasBeenGenerated() is not true. ID becomes invalid.
		*/
		void Dispose();

		// Loading
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
				static Format RGBA() { return GL_RGBA32UI; }
			};
			
			struct Integer
			{
				static Format RGBA() { return GL_RGBA_INTEGER; }
			};

			GLenum GetEnumGL() const { return FormatGl; }
			bool operator==(const Format& rhs) const { return rhs.FormatGl == FormatGl; }
		private:
			Format(GLenum formatGl) : FormatGl(formatGl) {}	//implicit construction for methods of Texture::Format
			GLenum FormatGl;
		};

		template <typename Archive> void Save(Archive& archive) const
		{
			archive(cereal::make_nvp("Type", Type), cereal::make_nvp("InternalFormat", InternalFormat.GetEnumGL()), cereal::make_nvp("Path", Path));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			GLenum internalFormat;
			archive(cereal::make_nvp("Type", Type), cereal::make_nvp("InternalFormat", internalFormat), cereal::make_nvp("Path", Path));

			*this = Loader<>::FromFile2D(Path, Format::FromGLConst(internalFormat), false, MinFilter::Trilinear(), MagFilter::Bilinear());
			SetWrap(GL_REPEAT, GL_REPEAT, 0, true);
		}

		protected:
		Texture(const Vec2u& size, GLenum type, Format internalFormat = Format::RGBA(), unsigned int id = 0, const std::string& path = "");
		Texture(const Vec3u& size, GLenum type, Format internalFormat = Format::RGBA(), unsigned int id = 0, const std::string& path = "");

		void SetSize(const Vec2u& size);
		void SetSize(const Vec3u& size);

		enum class TextureFilter
		{
			Nearest,
			Bilinear,
			NearestClosestMipmap,
			BilinearClosestMipmap,
			NearestInterpolateMipmap,
			Trilinear
		};

		struct Impl
		{
			static GLenum GetTextureFilterGL(TextureFilter);
		};

		GLenum Type;
		Format InternalFormat;
		unsigned int ID;
		Vec3u Size;
		std::string Path;
	};

	class NamedTexture : public Texture
	{
		std::string ShaderName;

	public:
		NamedTexture(const Texture& tex = Texture(), const std::string& name = "albedo1");

		std::string GetShaderName() const;
		void SetShaderName(const std::string&);
		template <typename Archive> void Serialize(Archive& archive)
		{
			archive(cereal::make_nvp("ShaderName", ShaderName), cereal::make_nvp("Texture", cereal::base_class<Texture>(this)));
		}
	};


}

namespace cereal
{
	template <typename Archive>
	struct specialize<Archive, GEE::NamedTexture, cereal::specialization::member_serialize> {};
}