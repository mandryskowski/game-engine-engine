#include "Material.h"
#include "Texture.h"
#include <assimp/pbrmaterial.h>

Material::Material(std::string name, float shine, float depthScale, Shader* shader)
{
	Name = name;
	Shininess = shine;
	DepthScale = depthScale;
	Color = glm::vec4(0.0f);
	if (shader)
		RenderShaderName = shader->GetName();
}

std::string Material::GetName()
{
	return Name;
}

std::string Material::GetRenderShaderName()
{
	return RenderShaderName;
}

void Material::SetDepthScale(float scale)
{
	DepthScale = scale;
}
void Material::SetShininess(float shine)
{
	Shininess = shine;
}

void Material::SetRenderShaderName(std::string name)
{
	RenderShaderName = name;
}

void Material::SetColor(glm::vec4 color)
{
	Color = color;
}

void Material::AddTexture(NamedTexture* tex)
{
	Textures.push_back(tex);
}

void Material::LoadFromAiMaterial(aiMaterial* material, std::string directory, MaterialLoadingData* matLoadingData)
{
	//Load material's name
	aiString name;
	float shininess;
	material->Get(AI_MATKEY_NAME, name);
	material->Get(AI_MATKEY_SHININESS, shininess);
	Name = name.C_Str();
	Shininess = shininess;

	//This vector should contain all of the texture types that the engine can process (or they will not be loaded)
	std::vector<std::pair<aiTextureType, std::string>> materialTypes =
	{
		std::pair<aiTextureType, std::string>(aiTextureType_DIFFUSE, "albedo"),
		std::pair<aiTextureType, std::string>(aiTextureType_SPECULAR, "specular"),
		std::pair<aiTextureType, std::string>(aiTextureType_HEIGHT, "normal"),
		std::pair<aiTextureType, std::string>(aiTextureType_DISPLACEMENT, "depth"),
		std::pair<aiTextureType, std::string>(aiTextureType_SHININESS, "roughness"),
		std::pair<aiTextureType, std::string>(aiTextureType_METALNESS, "metallic"),
		std::pair<aiTextureType, std::string>(aiTextureType_UNKNOWN, "aaa"),
		std::pair<aiTextureType, std::string>(aiTextureType_LIGHTMAP, "aaa"),
	};
	
	//Load textures by type
	for (unsigned int i = 0; i < materialTypes.size(); i++)
		LoadAiTexturesOfType(material, directory, materialTypes[i].first, materialTypes[i].second, matLoadingData);
}

void Material::LoadAiTexturesOfType(aiMaterial* material, std::string directory, aiTextureType type, std::string shaderName, MaterialLoadingData* matLoadingData)
{
	std::vector <NamedTexture*>* prevLoadedTextures = nullptr;
	if (matLoadingData)
		prevLoadedTextures = &matLoadingData->LoadedTextures;

	std::string name;
	switch (type)
	{
	case aiTextureType_DIFFUSE:
		name = "aiTextureType_DIFFUSE"; break;
	case aiTextureType_SPECULAR:
		name = "aiTextureType_SPECULAR"; break;
	case aiTextureType_HEIGHT:
		name = "aiTextureType_HEIGHT"; break;
	case aiTextureType_DISPLACEMENT:
		name = "aiTextureType_DISPLACEMENT"; break;
	case aiTextureType_SHININESS:
		name = "aiTextureType_SHININESS"; break;
	}

	bool sRGB = false;
	if (type == aiTextureType_DIFFUSE)
		sRGB = true;

	for (unsigned int i = 0; i < material->GetTextureCount(type); i++)	//for each texture of this type (there can be more than one)
	{
		aiString path;
		std::string pathStr;
		aiTextureMapping texMapping;	//TODO: maybe use this later?

		material->GetTexture(type, i, &path, &texMapping);
		pathStr = directory + path.C_Str();

		if (prevLoadedTextures)
		{
			bool bWasLoadedPreviously = false;
			for (unsigned int j = 0; j < prevLoadedTextures->size(); j++)	//check if the texture was loaded in the past (saves a lot of time)
			{
				if ((*prevLoadedTextures)[j]->GetPath() == pathStr)
				{
					Textures.push_back((*prevLoadedTextures)[j]);
					bWasLoadedPreviously = true;
					break;
				}
			}

			if (bWasLoadedPreviously)	//skip the costly loading if we already loaded this file
				continue;
		}

		NamedTexture* tex = new NamedTexture(textureFromFile(pathStr, (sRGB) ? (GL_SRGB) : (GL_RGB)), shaderName + std::to_string(i + 1));	//create a new Texture and pass the file path, the shader name (for example albedo1, roughness1, ...) and the sRGB info
		
		Textures.push_back(tex);
		if (prevLoadedTextures)
			prevLoadedTextures->push_back(tex);
	}
}

void Material::UpdateWholeUBOData(Shader* shader, Texture& emptyTexture)
{
	shader->Uniform1f("material.shininess", Shininess);
	shader->Uniform1f("material.depthScale", DepthScale);
	shader->Uniform4fv("material.color", Color);

	std::vector<std::pair<unsigned int, std::string>>* textureUnits = shader->GetMaterialTextureUnits();

	for (unsigned int i = 0; i < textureUnits->size(); i++)
	{
		std::pair<unsigned int, std::string>* unitPair = &(*textureUnits)[i];
		glActiveTexture(GL_TEXTURE0 + unitPair->first);

		bool found = false;
		for (unsigned int j = 0; j < Textures.size(); j++)
		{
			if (Textures[j]->GetShaderName() == unitPair->second)
			{
				glBindTexture(GL_TEXTURE_2D, Textures[j]->GetID());
				found = true;
				break;
			}
		}
		if (!found)
			emptyTexture.Bind();
	}
}

/*
========================================================================================================================
========================================================================================================================
========================================================================================================================
*/

AtlasMaterial::AtlasMaterial(std::string name, glm::vec2 atlasSize, float shine, float depthScale, Shader* shader) :
	Material(name, shine, depthScale, shader),
	AtlasSize(atlasSize),
	TextureID(0.0f)
{
}

void AtlasMaterial::InterpolateInAnimation(Interpolation* interp)
{
	if (!interp)
	{
		TextureID = 0.0f;
		return;
	}

	TextureID = interp->InterpolateValues(0.0f, AtlasSize.x * AtlasSize.y - 1.0f);
	//std::cout << "Uwaga: " << TextureID << '\n';
}

void AtlasMaterial::UpdateInstanceUBOData(Shader* shader)
{
	shader->Uniform2fv("atlasData", glm::vec2(TextureID, AtlasSize.x));
}

void AtlasMaterial::UpdateWholeUBOData(Shader* shader, Texture& emptyTexture)
{
	UpdateInstanceUBOData(shader);
	Material::UpdateWholeUBOData(shader, emptyTexture);

	shader->Uniform2fv("atlasTexOffset", glm::vec2(1.0f) / AtlasSize);
}

/*
========================================================================================================================
========================================================================================================================
========================================================================================================================
*/

MaterialInstance::MaterialInstance(Material* materialPtr,  Interpolation* interp, bool drawBefore, bool drawAfter):
	MaterialPtr(materialPtr),
	AnimationInterp(interp),
	DrawBeforeAnim(drawBefore),
	DrawAfterAnim(drawAfter)
{
}

Material* MaterialInstance::GetMaterialPtr()
{
	return MaterialPtr;
}

bool MaterialInstance::ShouldBeDrawn()
{
	if ((!AnimationInterp) ||
		((DrawBeforeAnim || AnimationInterp->GetT() > 0.0f) && (DrawAfterAnim || AnimationInterp->GetT() < 1.0f)))
		return true;

	return false;
}

void MaterialInstance::SetInterp(Interpolation* interp)
{
	AnimationInterp = interp;
}

void MaterialInstance::ResetAnimation()
{
	AnimationInterp->Reset();
}

void MaterialInstance::Update(float deltaTime)
{
	if (!AnimationInterp)	//if not animated, return
		return;

	AnimationInterp->UpdateT(deltaTime);
}

void MaterialInstance::UpdateInstanceUBOData(Shader* shader)
{
	MaterialPtr->InterpolateInAnimation(AnimationInterp);
	MaterialPtr->UpdateInstanceUBOData(shader);
}

void MaterialInstance::UpdateWholeUBOData(Shader* shader, Texture& emptyTexture)
{
	UpdateInstanceUBOData(shader);
	MaterialPtr->UpdateWholeUBOData(shader, emptyTexture);
}