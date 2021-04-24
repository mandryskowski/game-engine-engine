#pragma once
#include <scene/Component.h>
#include <animation/Animation.h>
#include <stb/stb_image.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

struct MaterialLoadingData;	//note: it's legally incomplete in class Material declaration (it's safe to use incomplete types in functions'/methods' declarations!)

class NamedTexture;

enum MaterialData
{

};

class Material
{
public:
	struct MaterialLoc
	{
		std::string Name;
		std::string OptionalPath;
		MaterialLoc(const std::string& name, const std::string& optionalPath = std::string()) : Name(name), OptionalPath(optionalPath) {}
		std::string GetStr() const
		{
			return (!OptionalPath.empty()) ? (OptionalPath + ":" + Name) : (Name);
		}
	} Localization;
	std::vector <std::shared_ptr<NamedTexture>> Textures;	//Often used; NamedTextures (NamedTexture is a child class of Texture) are bound to the samplers which names correspond to ShaderName of a NamedTexture.
	Vec4f Color;	//Probably not often used; Used for some materials that only need a colour, e.g. for button materials - they can be monocolour
	float Shininess;
	float DepthScale;	//used in parallax mapping
	std::string RenderShaderName;

public:
	Material(MaterialLoc, float depthScale = 0.0f, Shader* shader = nullptr);
	Material(const std::string& name, float depthScale = 0.0f, Shader* shader = nullptr);
	const MaterialLoc& GetLocalization() const;
	const std::string& GetRenderShaderName() const;
	void SetDepthScale(float);
	void SetShininess(float);
	void SetRenderShaderName(const std::string&);
	void SetColor(const glm::vec3& color);
	void SetColor(const glm::vec4& color); //Think twice before you use colours; you need a corresponding shader to make it work (one that has uniform vec4 color in it)
	void AddTexture(std::shared_ptr<NamedTexture> tex);

	void LoadFromAiMaterial(const aiScene* scene, aiMaterial*, const std::string&, MaterialLoadingData*);
	void LoadAiTexturesOfType(const aiScene* scene, aiMaterial*, const std::string&, aiTextureType, std::string, MaterialLoadingData*);

	virtual void InterpolateInAnimation(InterpolatorBase*) const {}	//some Materials can be animated. That's why we declare these two virtual methods - objects of some child classes interpolate their animated values in here...
	virtual void UpdateInstanceUBOData(Shader* shader, bool setValuesToDefault = false) const {shader->Uniform2fv("atlasData", glm::vec2(0.0f));}	//...and pass the interpolated values to shader in here. I separated these functions for flexibility - you don't always want to interpolate the values each time you use the material for rendering
	virtual void UpdateWholeUBOData(Shader*, Texture& emptyTexture) const;

	template <typename Archive> void Save(Archive& archive) const
	{
		archive(cereal::make_nvp("Name", Localization.Name), cereal::make_nvp("OptionalPath", Localization.OptionalPath), CEREAL_NVP(Textures), CEREAL_NVP(Color), CEREAL_NVP(Shininess), CEREAL_NVP(DepthScale), CEREAL_NVP(RenderShaderName));
	}
	template <typename Archive> void Load(Archive& archive)
	{
		archive(cereal::make_nvp("Name", Localization.Name), cereal::make_nvp("Textures", Textures), cereal::make_nvp("Color", Color), cereal::make_nvp("Shininess", Shininess), cereal::make_nvp("DepthScale", DepthScale), cereal::make_nvp("RenderShaderName", RenderShaderName));
		if (RenderShaderName.empty())
			RenderShaderName = "Geometry";
	}
	template <typename Archive> static void load_and_construct(Archive& archive, cereal::construct<Material>& construct)
	{
		construct("if_you_see_this_serializing_error_ocurred");
		construct->Load(archive);
	}
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
	AtlasMaterial(const std::string& name, glm::ivec2 atlasSize = glm::ivec2(0));
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
};

namespace cereal
{
	template <class Archive>
	struct specialize<Archive, AtlasMaterial, cereal::specialization::member_serialize> {};
}

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
		std::shared_ptr<Material> mat = GameManager::DefaultScene->GetGameHandle()->GetRenderEngineHandle()->FindMaterial(MaterialRef.GetLocalization().Name);
		archive(cereal::make_nvp("MaterialRef", mat), CEREAL_NVP(DrawBeforeAnim), CEREAL_NVP(DrawAfterAnim));
	}
	template <typename Archive> static void load_and_construct(Archive& archive, cereal::construct<MaterialInstance>& construct)
	{
		std::shared_ptr<Material> mat;
		bool drawBeforeAnim, drawAfterAnim;
		archive(cereal::make_nvp("MaterialRef", mat), cereal::make_nvp("DrawBeforeAnim", drawBeforeAnim), cereal::make_nvp("DrawAfterAnim", drawAfterAnim));

		GameManager::DefaultScene->GetGameHandle()->GetRenderEngineHandle()->AddMaterial(mat);
		

		construct(*mat);// , drawBeforeAnim, drawAfterAnim);
	}
};