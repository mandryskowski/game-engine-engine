#pragma once
#include <animation/Animation.h>
#include <game/GameManager.h>
#include <stb_image.h>
#include <assimp/scene.h>
#include <rendering/Texture.h>
#include <rendering/Shader.h>

#include <utility/CerealNames.h>
#include <cereal/access.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

#include "ShaderInfo.h"

namespace GEE
{
	struct ShaderInfo;
	struct MaterialLoadingData;

	class Material
	{
	public:
		struct MaterialLoc : public HTreeObjectLoc
		{
			std::string Name;

			MaterialLoc(HTreeObjectLoc treeObjectLoc,
			            const std::string& name = std::string()) : HTreeObjectLoc(treeObjectLoc), Name(name)
			{
			}

			MaterialLoc(const char* name) : HTreeObjectLoc(), Name(name) //allows implicit conversion from const char*
			{
			}

			MaterialLoc(const std::string& name) : HTreeObjectLoc(), Name(name)
			//allows implicit conversion from const std::string&
			{
			}

			std::string GetFullStr() const
			{
				return (!GetTreeName().empty()) ? (GetTreeName() + ":" + Name) : (Name);
			}
		};

	public:
		explicit Material(MaterialLoc, ShaderInfo = MaterialShaderHint::Simple);
		explicit Material(MaterialLoc, const Vec3f& color, ShaderInfo = MaterialShaderHint::Simple);
		explicit Material(MaterialLoc, const Vec4f& color, ShaderInfo = MaterialShaderHint::Simple);
		const MaterialLoc& GetLocalization() const;
		std::string GetName() const;
		ShaderInfo GetShaderInfo() const;
		Vec4f GetColor() const;
		unsigned int GetTextureCount() const;
		NamedTexture GetTexture(unsigned int i) const;
		void SetDepthScale(float);
		void SetShininess(float);
		void SetShaderInfo(ShaderInfo);

		//Think twice before you use colours; you need a corresponding shader to make it work (one that has uniform vec4 color in it)
		void SetColor(const Vec3f& color);
		void SetColor(const Vec4f& color);
		void SetRoughnessColor(float roughness);
		void SetMetallicColor(float metallic);
		void SetAoColor(float ao);

		void AddTexture(const NamedTexture& tex);
		void AddTexture(SharedPtr<NamedTexture> tex);
		void RemoveTexture(NamedTexture&);

		void LoadFromAiMaterial(const aiScene* scene, aiMaterial*, const std::string&, MaterialLoadingData*);
		void LoadAiTexturesOfType(const aiScene* scene, aiMaterial*, const std::string&, aiTextureType, std::string,
		                          MaterialLoadingData*);

		virtual void InterpolateInAnimation(InterpolatorBase*) const
		//some Materials can be animated. That's why we declare these two virtual methods - objects of some child classes interpolate their animated values in here...
		{
		}

		//...and pass the interpolated values to shader in here. I separated these functions for flexibility - you don't always want to interpolate the values each time you use the material for rendering
		virtual void UpdateInstanceUBOData(Shader* shader, bool setValuesToDefault = false) const;
		virtual void UpdateWholeUBOData(Shader*, const Texture& emptyTexture) const;

		template <typename Archive>
		void Save(Archive& archive) const
		{
			archive(cereal::make_nvp("Name", GetLocalization().Name), cereal::make_nvp("Textures", Textures),
			        CEREAL_NVP(Color), CEREAL_NVP(Shininess), CEREAL_NVP(RoughnessColor), CEREAL_NVP(MetallicColor),
			        CEREAL_NVP(AoColor), CEREAL_NVP(DepthScale), CEREAL_NVP(MatShaderInfo));
		}

		template <typename Archive>
		void Load(Archive& archive)
		{
			archive(cereal::make_nvp("Name", Localization.Name), cereal::make_nvp("Textures", Textures),
			        cereal::make_nvp("Color", Color), cereal::make_nvp("Shininess", Shininess),
			        CEREAL_NVP(RoughnessColor), CEREAL_NVP(MetallicColor), CEREAL_NVP(AoColor),
			        cereal::make_nvp("DepthScale", DepthScale), CEREAL_NVP(MatShaderInfo));
		}

		template <typename Archive>
		static void load_and_construct(Archive& archive, cereal::construct<Material>& construct)
		{
			construct("if_you_see_this_serializing_error_ocurred");
			construct->Load(archive);
		}

		virtual void GetEditorDescription(EditorDescriptionBuilder);

	public:
		MaterialLoc Localization;
		ShaderInfo MatShaderInfo;
		//Often used; NamedTextures (NamedTexture is a child class of Texture) are bound to the samplers which names correspond to ShaderName of a NamedTexture.
		std::vector<SharedPtr<NamedTexture>> Textures;
		//Used for some materials that only need a colour, e.g. for button materials - we can represent them as just a single color
		Vec4f Color;
		float Shininess;
		//used in parallax mapping
		float DepthScale;

		float RoughnessColor, MetallicColor, AoColor;
	};


	namespace MaterialUtil
	{
		void DisableColorIfAlbedoTextureDetected(Shader&, const Material&);
	}

	struct MaterialLoadingData
	{
		std::vector<SharedPtr<Material>> LoadedMaterials;
		//note: LoadedMaterials and LoadedAiMaterials will ALWAYS be the same size
		std::vector<aiMaterial*> LoadedAiMaterials;
		std::vector<SharedPtr<NamedTexture>> LoadedTextures;

		SharedPtr<NamedTexture> FindTexture(const String& path) const;
		//Looks for a texture loaded from the given path.
		void AddTexture(SharedPtr<NamedTexture>);
		//Adds the given texture to the LoadedTextures vector. Doesn't check for duplicates - do it yourself using FindTexture()
	};

	/*
	========================================================================================================================
	========================================================================================================================
	========================================================================================================================
	*/

	class AtlasMaterial : public Material
	{
		Vec2f AtlasSize;
		mutable float TextureID; //texture ID from 0 -> AtlasSize.x * AtlasSize.y
		//if it isn't an integer, the shader will blend between two nearest textures in the atlas (f.e. 1.5 mixes texture 1 and 2 equally)

	public:
		AtlasMaterial(Material&& mat, Vec2i atlasSize = Vec2i(0), ShaderInfo = MaterialShaderHint::Simple);
		AtlasMaterial(MaterialLoc loc, Vec2i atlasSize = Vec2i(0), ShaderInfo = MaterialShaderHint::Simple);
		float GetMaxTextureID() const;
		void UpdateInstanceUBOData(Shader* shader, bool setValuesToDefault = false) const override;
		void UpdateWholeUBOData(Shader* shader, const Texture& emptyTexture) const override;

		Interpolator<float>& GetTextureIDInterpolatorTemplate(float constantTextureID);
		Interpolator<float>& GetTextureIDInterpolatorTemplate(const Interpolation&, float min = 0.0f, float max = 0.0f);

		template <typename Archive>
		void Serialize(Archive& archive)
		{
			archive(CEREAL_NVP(AtlasSize), CEREAL_NVP(TextureID),
			        make_nvp("Material", cereal::base_class<Material>(this)));
		}

		template <typename Archive>
		static void load_and_construct(Archive& archive, cereal::construct<AtlasMaterial>& construct)
		{
			construct(MaterialLoc("if_you_see_this_serializing_error_ocurred"));
			construct->Serialize(archive);
		}

		void GetEditorDescription(EditorDescriptionBuilder) override;
	};

	/*
	========================================================================================================================
	========================================================================================================================
	========================================================================================================================
	*/

	struct MaterialInstance
	{
	public:
		MaterialInstance(SharedPtr<Material>);
		MaterialInstance(SharedPtr<Material>, InterpolatorBase&, bool drawBefore = true, bool drawAfter = true);
		MaterialInstance(const MaterialInstance&) = delete;
		MaterialInstance(MaterialInstance&&);

		SharedPtr<Material> GetMaterialPtr();
		const SharedPtr<Material> GetMaterialPtr() const;
		Material& GetMaterialRef();
		const Material& GetMaterialRef() const;
		bool IsAnimated() const;
		bool ShouldBeDrawn() const;
		void SetInterp(InterpolatorBase*);
		void SetDrawBeforeAnim(bool);
		void SetDrawAfterAnim(bool);
		/**
		 * @brief Reset AnimationInterp of this MaterialInstance.
		*/
		void ResetAnimation();

		void Update(Time deltaTime);
		void UpdateInstanceUBOData(Shader* shader) const;
		void UpdateWholeUBOData(Shader* shader, const Texture& emptyTexture) const;

		MaterialInstance& operator=(const MaterialInstance&) = delete;
		MaterialInstance& operator=(MaterialInstance&&);

		template <typename Archive>
		void Save(Archive& archive) const;

		template <typename Archive>
		static void load_and_construct(Archive& archive, cereal::construct<MaterialInstance>& construct);

	private:
		SharedPtr<Material> MaterialPtr;
		InterpolatorBase* AnimationInterp; //optional; use if you animate the material
		bool DrawBeforeAnim, DrawAfterAnim;
	};

	

	// Should be constexpr but fmod isn't
	Vec3f hsvToRgb(Vec3f hsvColor);
}

namespace cereal
{
	template <class Archive>
	struct specialize<Archive, GEE::AtlasMaterial, specialization::member_serialize>
	{
	};
}

GEE_REGISTER_TYPE(GEE::Material)

GEE_REGISTER_TYPE(GEE::AtlasMaterial)