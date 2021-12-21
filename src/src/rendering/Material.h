#pragma once
#include <animation/Animation.h>
#include <stb/stb_image.h>
#include <assimp/scene.h>
#include <rendering/Texture.h>
#include <rendering/Shader.h>

#include <cereal/access.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

namespace GEE
{
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
			MaterialLoc(const std::string& name) : HTreeObjectLoc(), Name(name) //allows implicit conversion from const std::string&
			{
			}
			std::string GetFullStr() const
			{
				return (!GetTreeName().empty()) ? (GetTreeName() + ":" + Name) : (Name);
			}
		};

		struct ShaderInfo
		{
			ShaderInfo(MaterialShaderHint hint) : ShaderHint(hint), CustomShader(nullptr) {}
			ShaderInfo(Shader& customShader) : ShaderHint(MaterialShaderHint::None), CustomShader(&customShader) {}

			bool IsValid() const { return UsesShaderHint() || CustomShader; }
			bool UsesShaderHint() const { return ShaderHint != MaterialShaderHint::None; }
			bool MatchesRequiredInfo(const ShaderInfo& requiredInfo)
			{
				if (requiredInfo.UsesShaderHint())
					return requiredInfo.GetShaderHint() == GetShaderHint();

				return (!requiredInfo.GetCustomShader() || requiredInfo.GetCustomShader() == GetCustomShader());
			}

			MaterialShaderHint GetShaderHint() const { return ShaderHint; }
			Shader* GetCustomShader() { return CustomShader; }
			const Shader* GetCustomShader() const { return CustomShader; }

			/**
			 * @brief If this ShaderInfo refers to a custom shader, the shader will be returned. If this ShaderInfo contains a hint, the correct shader will be retrieved from the render toolbox collection.
			 * @return a pointer to a Shader useful in rendering. Can be nullptr if this Shader info is not valid.
			*/
			Shader* RetrieveShaderForRendering(RenderToolboxCollection&);

			template <typename Archive>
			void Save(Archive& archive) const
			{
				archive(CEREAL_NVP(ShaderHint), cereal::make_nvp("CustomShaderName", (CustomShader) ? (CustomShader->GetName()) : (std::string())));
			}

			template <typename Archive>
			void Load(Archive& archive)
			{
				std::string customShaderName;
				archive(CEREAL_NVP(ShaderHint), customShaderName);

				if (!customShaderName.empty())
					CustomShader = GameManager::Get().GetRenderEngineHandle()->FindShader(customShaderName);
			}

			template <typename Archive>
			static void load_and_construct(Archive& archive, cereal::construct<ShaderInfo>& construct)
			{
				construct(MaterialShaderHint::None);
				construct->Load(archive);
			}

		private:
			/*
			*	Shader hints are useful if our material dose not use a specific type of shader
			*	If they do, use the optional pointer
			*/
			MaterialShaderHint ShaderHint;
			Shader* CustomShader;
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

		virtual void InterpolateInAnimation(InterpolatorBase*) const //some Materials can be animated. That's why we declare these two virtual methods - objects of some child classes interpolate their animated values in here...
		{
		}
		//...and pass the interpolated values to shader in here. I separated these functions for flexibility - you don't always want to interpolate the values each time you use the material for rendering
		virtual void UpdateInstanceUBOData(Shader* shader, bool setValuesToDefault = false) const
		{
			shader->Uniform2fv("atlasData", Vec2f(0.0f));
		}
		virtual void UpdateWholeUBOData(Shader*, Texture& emptyTexture) const;

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
		static void load_and_construct(Archive& archive, cereal::construct<GEE::Material>& construct)
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

		SharedPtr<NamedTexture> FindTexture(const std::string& path) const;
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
		virtual void UpdateInstanceUBOData(Shader* shader, bool setValuesToDefault = false) const override;
		virtual void UpdateWholeUBOData(Shader* shader, Texture& emptyTexture) const override;

		Interpolator<float>& GetTextureIDInterpolatorTemplate(float constantTextureID);
		Interpolator<float>& GetTextureIDInterpolatorTemplate(const Interpolation&, float min = 0.0f, float max = 0.0f);

		template <typename Archive>
		void Serialize(Archive& archive)
		{
			archive(CEREAL_NVP(AtlasSize), CEREAL_NVP(TextureID),
			        cereal::make_nvp("Material", cereal::base_class<Material>(this)));
		}

		template <typename Archive>
		static void load_and_construct(Archive& archive, cereal::construct<AtlasMaterial>& construct)
		{
			construct(MaterialLoc("if_you_see_this_serializing_error_ocurred"));
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
	public:
		MaterialInstance(Material&);
		MaterialInstance(Material&, InterpolatorBase&, bool drawBefore = true, bool drawAfter = true);
		MaterialInstance(const MaterialInstance&) = delete;
		MaterialInstance(MaterialInstance&&);

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

		void Update(float deltaTime);
		void UpdateInstanceUBOData(Shader* shader) const;
		void UpdateWholeUBOData(Shader* shader, Texture& emptyTexture) const;

		MaterialInstance& operator=(const MaterialInstance&) = delete;
		MaterialInstance& operator=(MaterialInstance&&);

		template <typename Archive>
		void Save(Archive& archive) const
		{
			SharedPtr<Material> mat = GameManager::Get().GetRenderEngineHandle()->FindMaterial(
				MaterialRef.GetLocalization().Name);
			if (!mat)
			{
				std::cout << "ERROR! Cannot save materialinstance of " << MaterialRef.GetLocalization().Name <<
					". The most likely cause of this is that the material has not been added to the RenderEngine, and thus cannot be found.\n";
				return;
			}
			std::string name = mat->Localization.Name, treeName = mat->Localization.GetTreeName();
			archive(cereal::make_nvp("MaterialName", name), cereal::make_nvp("MaterialOptionalPath", treeName));
			if (treeName.empty())
				archive(cereal::make_nvp("ExternalMaterial", mat));
			archive(CEREAL_NVP(DrawBeforeAnim), CEREAL_NVP(DrawAfterAnim));
		}

		template <typename Archive>
		static void load_and_construct(Archive& archive, cereal::construct<MaterialInstance>& construct)
		{
			std::string materialName, materialPath;
			archive(cereal::make_nvp("MaterialName", materialName),
			        cereal::make_nvp("MaterialOptionalPath", materialPath));
			SharedPtr<Material> mat = GameManager::Get().GetRenderEngineHandle()->FindMaterial(materialName);
			if (!mat)
			{
				if (materialPath.empty())
				{
					archive(cereal::make_nvp("ExternalMaterial", mat));
					GameManager::Get().GetRenderEngineHandle()->AddMaterial(mat);
				}
				else
				{
					std::cout << "ERROR: Could not find material " << materialName << " (path - " + materialPath +
						")\n";
					exit(42069);
				}
			}

			bool drawBeforeAnim, drawAfterAnim;
			archive(cereal::make_nvp("DrawBeforeAnim", drawBeforeAnim),
			        cereal::make_nvp("DrawAfterAnim", drawAfterAnim));


			construct(*mat); // , drawBeforeAnim, drawAfterAnim);

			//GameManager::DefaultScene->AddPostLoadLambda([mat]() { std::cout << "uwaga robie " << mat << '\n'; GameManager::Get().GetRenderEngineHandle()->AddMaterial(mat); });
		}
	private:
		Material& MaterialRef;
		InterpolatorBase* AnimationInterp; //optional; use if you animate the material
		bool DrawBeforeAnim, DrawAfterAnim;
	};

	constexpr Vec3f hsvToRgb(Vec3f hsvColor)
	{
		float C = hsvColor.z * hsvColor.y; //V * S
		float X = C * (1.0f - glm::abs(glm::mod(hsvColor.x / 60.0f, 2.0f) - 1.0f)); // C * ( 1 - | (H / 60) % 2 - 1| )
		float m = hsvColor.z - C; //V * C

		Vec3f rgbPrime(0.0f);
		int hueCirclePart = floorConstexpr(hsvColor.x / 60.0f); // floor( H / 60 )

		switch (hueCirclePart)
		{
		case 0: rgbPrime = Vec3f(C, X, 0.0f);  // 0 <= H < 60
			break;
		case 1: rgbPrime = Vec3f(X, C, 0.0f); // 60 <= H < 120
			break;
		case 2: rgbPrime = Vec3f(0.0f, C, X); // 120 <= H < 180
			break; 
		case 3: rgbPrime = Vec3f(0.0f, X, C); // 180 <= H < 240
			break; 
		case 4: rgbPrime = Vec3f(X, 0.0f, C); // 240 <= H < 300
			break; 
		case 5: rgbPrime = Vec3f(C, 0.0f, X); // 300 <= H < 360
			break;
		}

		return rgbPrime + m;
	}
}

namespace cereal
{
	template <class Archive>
	struct specialize<Archive, GEE::AtlasMaterial, cereal::specialization::member_serialize>
	{
	};
}

GEE_REGISTER_TYPE(GEE::Material)

GEE_REGISTER_TYPE(GEE::AtlasMaterial)