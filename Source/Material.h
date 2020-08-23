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
	std::vector <NamedTexture*> Textures;
	float Shininess;
	float DepthScale;	//used in parallax mapping
	std::string Name;
	std::string RenderShaderName;

public:
	Material(std::string name, float shine = 64.0f, float depthScale = 0.0f, Shader* shader = nullptr);
	std::string GetName();
	std::string GetRenderShaderName();
	void SetDepthScale(float);
	void SetShininess(float);
	void SetRenderShaderName(std::string);
	void AddTexture(NamedTexture* tex);

	void LoadFromAiMaterial(aiMaterial*, std::string, MaterialLoadingData*);
	void LoadAiTexturesOfType(aiMaterial*, std::string, aiTextureType, std::string, MaterialLoadingData*);

	virtual void InterpolateInAnimation(Interpolation*) {}	//some Materials can be animated. That's why we declare these two virtual methods - objects of some child classes interpolate their animated values in here...
	virtual void UpdateInstanceUBOData(Shader*) {}	//...and pass the interpolated values to shader in here. I separated these functions for flexibility - you don't always want to interpolate the values each time you use the material for rendering
	virtual void UpdateWholeUBOData(Shader*, Texture& emptyTexture);
};

struct MaterialLoadingData
{
	std::vector <Material*> LoadedMaterials;	//note: LoadedMaterials and LoadedAiMaterials will ALWAYS be the same size
	std::vector <aiMaterial*> LoadedAiMaterials;
	std::vector <NamedTexture*> LoadedTextures;
};

/*
========================================================================================================================
========================================================================================================================
========================================================================================================================
*/

class AtlasMaterial : public Material
{
	glm::vec2 AtlasSize;
	float TextureID;	//texture ID from 0 -> AtlasSize.x * AtlasSize.y
						//if it isn't an integer, the shader will blend between two nearest textures in the atlas (f.e. 1.5 mixes texture 1 and 2 equally)

public:
	AtlasMaterial(std::string name, glm::vec2 atlasSize, float shine = 64.0f, float depthScale = 0.0f, Shader* shader = nullptr);
	virtual void InterpolateInAnimation(Interpolation* interp) override;
	virtual void UpdateInstanceUBOData(Shader* shader) override;
	virtual void UpdateWholeUBOData(Shader* shader, Texture& emptyTexture) override;
};

/*
========================================================================================================================
========================================================================================================================
========================================================================================================================
*/

struct MaterialInstance
{
	Material* MaterialPtr;
	Interpolation* AnimationInterp;	//optional; use if you animate the material
	bool DrawBeforeAnim, DrawAfterAnim;

public:
	MaterialInstance(Material*, Interpolation* = nullptr, bool drawBefore = false, bool drawAfter = false);
	Material* GetMaterialPtr();
	bool ShouldBeDrawn();
	void SetInterp(Interpolation*);
	void ResetAnimation();

	void Update(float deltaTime);
	void UpdateInstanceUBOData(Shader* shader);
	void UpdateWholeUBOData(Shader* shader, Texture& emptyTexture);
};