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
	std::vector <NamedTexture*> Textures;	//Often used; NamedTextures (NamedTexture is a child class of Texture) are bound to the samplers which names correspond to ShaderName of a NamedTexture.
	glm::vec4 Color;	//Probably not often used; Used for some materials that only need a colour, e.g. for button materials - they can be monocolour
	float Shininess;
	float DepthScale;	//used in parallax mapping
	std::string Name;
	std::string RenderShaderName;

public:
	Material(std::string name, float shine = 64.0f, float depthScale = 0.0f, Shader* shader = nullptr);
	const std::string& GetName() const;
	const std::string& GetRenderShaderName() const;
	void SetDepthScale(float);
	void SetShininess(float);
	void SetRenderShaderName(std::string);
	void SetColor(const glm::vec3& color);
	void SetColor(const glm::vec4& color); //Think twice before you use colours; you need a corresponding shader to make it work (one that has uniform vec4 color in it)
	void AddTexture(NamedTexture* tex);

	void LoadFromAiMaterial(const aiScene* scene, aiMaterial*, std::string, MaterialLoadingData*);
	void LoadAiTexturesOfType(const aiScene* scene, aiMaterial*, std::string, aiTextureType, std::string, MaterialLoadingData*);

	virtual void InterpolateInAnimation(InterpolatorBase*) const {}	//some Materials can be animated. That's why we declare these two virtual methods - objects of some child classes interpolate their animated values in here...
	virtual void UpdateInstanceUBOData(Shader* shader, bool setValuesToDefault = false) const {shader->Uniform2fv("atlasData", glm::vec2(0.0f));}	//...and pass the interpolated values to shader in here. I separated these functions for flexibility - you don't always want to interpolate the values each time you use the material for rendering
	virtual void UpdateWholeUBOData(Shader*, Texture& emptyTexture) const;
};

struct MaterialLoadingData
{
	std::vector <Material*> LoadedMaterials;	//note: LoadedMaterials and LoadedAiMaterials will ALWAYS be the same size
	std::vector <aiMaterial*> LoadedAiMaterials;
	std::vector <NamedTexture*> LoadedTextures;

	NamedTexture* FindTexture(const std::string& path) const;	//Looks for a texture loaded from the given path.
	void AddTexture(NamedTexture&);		//Adds the given texture to the LoadedTextures vector. Doesn't check for duplicates - do it yourself using FindTexture()
};

/*
========================================================================================================================
========================================================================================================================
========================================================================================================================
*/

class AtlasMaterial : public Material
{
	glm::vec2 AtlasSize;
	mutable float TextureID;	//texture ID from 0 -> AtlasSize.x * AtlasSize.y
								//if it isn't an integer, the shader will blend between two nearest textures in the atlas (f.e. 1.5 mixes texture 1 and 2 equally)

public:
	AtlasMaterial(std::string name, glm::ivec2 atlasSize, float shine = 64.0f, float depthScale = 0.0f, Shader* shader = nullptr);
	float GetMaxTextureID() const;
	virtual void UpdateInstanceUBOData(Shader* shader, bool setValuesToDefault = false) const override;
	virtual void UpdateWholeUBOData(Shader* shader, Texture& emptyTexture) const override;

	Interpolator<float>& GetTextureIDInterpolatorTemplate(float constantTextureID);
	Interpolator<float>& GetTextureIDInterpolatorTemplate(const Interpolation&, float min = 0.0f, float max = 0.0f);
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
};