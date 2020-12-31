#pragma once
#include "Component.h"
#include "Animation.h"
#include <stb/stb_image.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

struct MaterialLoadingData;	//note: it's legally incomplete in class Material declaration (it's safe to use incomplete types in functions'/methods' declarations!)

class NamedTexture;

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
	void SetColor(glm::vec4 color); //Think twice before you use colours; you need a corresponding shader to make it work (one that has uniform vec4 color in it)
	void AddTexture(NamedTexture* tex);

	void LoadFromAiMaterial(const aiScene* scene, aiMaterial*, std::string, MaterialLoadingData*);
	void LoadAiTexturesOfType(const aiScene* scene, aiMaterial*, std::string, aiTextureType, std::string, MaterialLoadingData*);

	virtual void InterpolateInAnimation(Interpolation*) const {}	//some Materials can be animated. That's why we declare these two virtual methods - objects of some child classes interpolate their animated values in here...
	virtual void UpdateInstanceUBOData(Shader*) const {}	//...and pass the interpolated values to shader in here. I separated these functions for flexibility - you don't always want to interpolate the values each time you use the material for rendering
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
	AtlasMaterial(std::string name, glm::vec2 atlasSize, float shine = 64.0f, float depthScale = 0.0f, Shader* shader = nullptr);
	virtual void InterpolateInAnimation(Interpolation* interp) const override;
	virtual void UpdateInstanceUBOData(Shader* shader) const override;
	virtual void UpdateWholeUBOData(Shader* shader, Texture& emptyTexture) const override;
};

/*
========================================================================================================================
========================================================================================================================
========================================================================================================================
*/

struct MaterialInstance
{
	const Material& MaterialRef;
	Interpolation* AnimationInterp;	//optional; use if you animate the material
	bool DrawBeforeAnim, DrawAfterAnim;

public:
	MaterialInstance(const Material&);
	MaterialInstance(const Material&, Interpolation&, bool drawBefore = false, bool drawAfter = false);
	const Material& GetMaterialRef() const;
	bool ShouldBeDrawn() const;
	void SetInterp(Interpolation*);
	void ResetAnimation();

	void Update(float deltaTime);
	void UpdateInstanceUBOData(Shader* shader) const;
	void UpdateWholeUBOData(Shader* shader, Texture& emptyTexture) const;
};