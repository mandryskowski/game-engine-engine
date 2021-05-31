#pragma once
#include <animation/Animation.h>
#include <stb/stb_image.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <rendering/Texture.h> 

#include <math/Vec.h>
#include <cereal/access.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

namespace GEE
{
	struct MaterialLoadingData;	//note: it's legally incomplete in class Material declaration (it's safe to use incomplete types in functions'/methods' declarations!)

	class Material
	{
	public:
		struct MaterialLoc : public HTreeObjectLoc
		{
			std::string Name;
			MaterialLoc(HTreeObjectLoc treeObjectLoc, const std::string& name = std::string()) : HTreeObjectLoc(treeObjectLoc), Name(name) {}
			MaterialLoc(const char* name) : HTreeObjectLoc(), Name(name) {}	//allows implicit conversion from const char*
			MaterialLoc(const std::string& name) : HTreeObjectLoc(), Name(name) {}	//allows implicit conversion from const std::string&
			std::string GetFullStr() const
			{
				return (!GetTreeName().empty()) ? (GetTreeName() + ":" + Name) : (Name);
			}
		};
	public:
		Material(MaterialLoc, float depthScale = 0.0f, Shader* shader = nullptr);
		const MaterialLoc& GetLocalization() const;
		std::string GetName() const;
		const std::string& GetRenderShaderName() const;
		Vec4f GetColor() const;
		void SetDepthScale(float);
		void SetShininess(float);
		void SetRenderShaderName(const std::string&);
		void SetColor(const glm::vec3& color);
		void SetColor(const glm::vec4& color); //Think twice before you use colours; you need a corresponding shader to make it work (one that has uniform vec4 color in it)
		void SetRoughnessColor(float roughness);
		void SetMetallicColor(float metallic);
		void SetAoColor(float ao);
		void AddTexture(std::shared_ptr<NamedTexture> tex);

		void LoadFromAiMaterial(const aiScene* scene, aiMaterial*, const std::string&, MaterialLoadingData*);
		void LoadAiTexturesOfType(const aiScene* scene, aiMaterial*, const std::string&, aiTextureType, std::string, MaterialLoadingData*);

		virtual void InterpolateInAnimation(InterpolatorBase*) const {}	//some Materials can be animated. That's why we declare these two virtual methods - objects of some child classes interpolate their animated values in here...
		virtual void UpdateInstanceUBOData(Shader* shader, bool setValuesToDefault = false) const { shader->Uniform2fv("atlasData", glm::vec2(0.0f)); }	//...and pass the interpolated values to shader in here. I separated these functions for flexibility - you don't always want to interpolate the values each time you use the material for rendering
		virtual void UpdateWholeUBOData(Shader*, Texture& emptyTexture) const;

		template <typename Archive> void Save(Archive& archive) const
		{
			archive(cereal::make_nvp("Name", GetLocalization().Name), cereal::make_nvp("Textures", Textures), CEREAL_NVP(Color), CEREAL_NVP(Shininess), CEREAL_NVP(DepthScale), CEREAL_NVP(RenderShaderName));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			archive(cereal::make_nvp("Name", Localization.Name), cereal::make_nvp("Textures", Textures), cereal::make_nvp("Color", Color), cereal::make_nvp("Shininess", Shininess), cereal::make_nvp("DepthScale", DepthScale), cereal::make_nvp("RenderShaderName", RenderShaderName));
			if (RenderShaderName.empty())
				RenderShaderName = "Geometry";
		}
		template <typename Archive> static void load_and_construct(Archive& archive, cereal::construct<GEE::Material>& construct)
		{
			construct("if_you_see_this_serializing_error_ocurred");
			construct->Load(archive);
		}

		virtual void GetEditorDescription(EditorDescriptionBuilder);


	public:
		MaterialLoc Localization;
		std::vector <std::shared_ptr<NamedTexture>> Textures;	//Often used; NamedTextures (NamedTexture is a child class of Texture) are bound to the samplers which names correspond to ShaderName of a NamedTexture.
		Vec4f Color;	//Probably not often used; Used for some materials that only need a colour, e.g. for button materials - they can be monocolour
		float Shininess;
		float DepthScale;	//used in parallax mapping
		std::string RenderShaderName;

		float RoughnessColor, MetallicColor, AoColor;
	};

	struct MaterialLoadingData
	{
		std::vector <std::shared_ptr<Material>> LoadedMaterials;	//note: LoadedMaterials and LoadedAiMaterials will ALWAYS be the same size
		std::vector <aiMaterial*> LoadedAiMaterials;
		std::vector <std::shared_ptr<NamedTexture>> LoadedTextures;

		std::shared_ptr<NamedTexture> FindTexture(const std::string& path) const;	//Looks for a texture loaded from the given path.
		void AddTexture(std::shared_ptr<NamedTexture>);		//Adds the given texture to the LoadedTextures vector. Doesn't check for duplicates - do it yourself using FindTexture()
	};

	/*
	========================================================================================================================
	========================================================================================================================
	========================================================================================================================
	*/

	class AtlasMaterial : public Material
	{
		Vec2f AtlasSize;
		mutable float TextureID;	//texture ID from 0 -> AtlasSize.x * AtlasSize.y
									//if it isn't an integer, the shader will blend between two nearest textures in the atlas (f.e. 1.5 mixes texture 1 and 2 equally)

	public:
		AtlasMaterial(Material&& mat, glm::ivec2 atlasSize = glm::ivec2(0));
		AtlasMaterial(MaterialLoc loc, glm::ivec2 atlasSize = glm::ivec2(0));
		float GetMaxTextureID() const;
		virtual void UpdateInstanceUBOData(Shader* shader, bool setValuesToDefault = false) const override;
		virtual void UpdateWholeUBOData(Shader* shader, Texture& emptyTexture) const override;

		Interpolator<float>& GetTextureIDInterpolatorTemplate(float constantTextureID);
		Interpolator<float>& GetTextureIDInterpolatorTemplate(const Interpolation&, float min = 0.0f, float max = 0.0f);

		template <typename Archive> void Serialize(Archive& archive)
		{
			archive(CEREAL_NVP(AtlasSize), CEREAL_NVP(TextureID), cereal::make_nvp("Material", cereal::base_class<Material>(this)));
		}
		template <typename Archive> static void load_and_construct(Archive& archive, cereal::construct<AtlasMaterial>& construct)
		{
			construct("if_you_see_this_serializing_error_ocurred");
			construct->Serialize(archive);
		}

		virtual void GetEditorDescription(EditorDescriptionBuilder) override;
	};

	/*
	========================================================================================================================
	========================================================================================================================
	========================================================================================================================
	*/

	struct MaterialInstance
	{
		Material& MaterialRef;
		InterpolatorBase* AnimationInterp;	//optional; use if you animate the material
		bool DrawBeforeAnim, DrawAfterAnim;

	public:
		MaterialInstance(Material&);
		MaterialInstance(Material&, InterpolatorBase&, bool drawBefore = true, bool drawAfter = true);
		MaterialInstance(const MaterialInstance&) = delete;
		MaterialInstance(MaterialInstance&&);
		Material& GetMaterialRef();
		const Material& GetMaterialRef() const;
		bool ShouldBeDrawn() const;
		void SetInterp(InterpolatorBase*);
		void SetDrawBeforeAnim(bool);
		void SetDrawAfterAnim(bool);
		void ResetAnimation();

		void Update(float deltaTime);
		void UpdateInstanceUBOData(Shader* shader) const;
		void UpdateWholeUBOData(Shader* shader, Texture& emptyTexture) const;

		MaterialInstance& operator=(const MaterialInstance&) = delete;
		MaterialInstance& operator=(MaterialInstance&&);

		template <typename Archive> void Save(Archive& archive) const
		{
			std::shared_ptr<Material> mat = GameManager::Get().GetRenderEngineHandle()->FindMaterial(MaterialRef.GetLocalization().Name);
			if (!mat)
			{
				std::cout << "ERROR! Cannot save materialinstance of " << MaterialRef.GetLocalization().Name << ". The most likely cause of this is that the material has not been added to the RenderEngine, and thus cannot be found.\n";
				return;
			}
			std::string name = mat->Localization.Name, treeName = mat->Localization.GetTreeName();
			archive(cereal::make_nvp("MaterialName", name), cereal::make_nvp("MaterialOptionalPath", treeName));
			if (treeName.empty())
				archive(cereal::make_nvp("ExternalMaterial", mat));
			archive(CEREAL_NVP(DrawBeforeAnim), CEREAL_NVP(DrawAfterAnim));
		}
		template <typename Archive> static void load_and_construct(Archive& archive, cereal::construct<MaterialInstance>& construct)
		{
			std::string materialName, materialPath;
			archive(cereal::make_nvp("MaterialName", materialName), cereal::make_nvp("MaterialOptionalPath", materialPath));
			std::shared_ptr<Material> mat = GameManager::Get().GetRenderEngineHandle()->FindMaterial(materialName);
			if (!mat)
			{
				if (materialPath.empty())
				{
					archive(cereal::make_nvp("ExternalMaterial", mat));
					GameManager::Get().GetRenderEngineHandle()->AddMaterial(mat);
				}
				else
				{
					std::cout << "ERROR: Could not find material " << materialName << " (path - " + materialPath + ")\n";
					exit(42069);
				}
			}

			bool drawBeforeAnim, drawAfterAnim;
			archive(cereal::make_nvp("DrawBeforeAnim", drawBeforeAnim), cereal::make_nvp("DrawAfterAnim", drawAfterAnim));


			construct(*mat);// , drawBeforeAnim, drawAfterAnim);

			//GameManager::DefaultScene->AddPostLoadLambda([mat]() { std::cout << "uwaga robie " << mat << '\n'; GameManager::Get().GetRenderEngineHandle()->AddMaterial(mat); });
		}
	};

	constexpr Vec3f hsvToRgb(Vec3f hsvColor)
	{
		float C = hsvColor.z * hsvColor.y;	//V * S
		float X = C * (1.0f - glm::abs(glm::mod(hsvColor.x / 60.0f, 2.0f) - 1.0f));	// C * ( 1 - | (H / 60) % 2 - 1| )
		float m = hsvColor.z - C;	//V * C

		Vec3f rgbPrime(0.0f);
		int hueCirclePart = floorConstexpr(hsvColor.x / 60.0f);	// floor( H / 60 )

		switch (hueCirclePart)
		{
		case 0: rgbPrime = Vec3f(C, X, 0.0f); break;	// 0 <= H < 60
		case 1: rgbPrime = Vec3f(X, C, 0.0f); break;	// 60 <= H < 120
		case 2: rgbPrime = Vec3f(0.0f, C, X); break;	// 120 <= H < 180
		case 3: rgbPrime = Vec3f(0.0f, X, C); break;	// 180 <= H < 240
		case 4: rgbPrime = Vec3f(X, 0.0f, C); break;	// 240 <= H < 300
		case 5: rgbPrime = Vec3f(C, 0.0f, X); break;	// 300 <= H < 360
		}

		return rgbPrime + m;
	}
}

namespace cereal
{
	template <class Archive>
	struct specialize<Archive, GEE::AtlasMaterial, cereal::specialization::member_serialize> {};
}

CEREAL_REGISTER_TYPE(GEE::Material)
CEREAL_REGISTER_TYPE(GEE::AtlasMaterial)