#include "Material.h"
#include "Texture.h"

Material::Material(std::string name, float shine, float depthScale, Shader* shader)
{ 
	Name = name;
	Shininess = shine;
	DepthScale = depthScale;
	RenderShader = shader;
}

std::string Material::GetName()
{
	return Name;
}

Shader* Material::GetRenderShader()
{
	return RenderShader;
}

void Material::SetDepthScale(float scale)
{
	DepthScale = scale;
}
void Material::SetShininess(float shine)
{
	Shininess = shine;
}

void Material::SetRenderShader(Shader* shader)
{
	RenderShader = shader;
}

void Material::AddTexture(MaterialTexture* tex)
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
	std::cout << "swieconko: " << shininess << '\n';

	//This vector should contain all of the texture types that the engine can process (or they will not be loaded)
	std::vector<std::pair<aiTextureType, std::string>> materialTypes =
	{
		std::pair<aiTextureType, std::string>(aiTextureType_DIFFUSE, "diffuse"),
		std::pair<aiTextureType, std::string>(aiTextureType_SPECULAR, "specular"),
		std::pair<aiTextureType, std::string>(aiTextureType_HEIGHT, "normal"),
		std::pair<aiTextureType, std::string>(aiTextureType_DISPLACEMENT, "depth")
	};
	
	//Load textures by type
	for (unsigned int i = 0; i < materialTypes.size(); i++)
		LoadAiTexturesOfType(material, directory, materialTypes[i].first, materialTypes[i].second, matLoadingData);
}

void Material::LoadAiTexturesOfType(aiMaterial* material, std::string directory, aiTextureType type, std::string shaderName, MaterialLoadingData* matLoadingData)
{
	std::vector <MaterialTexture*>* prevLoadedTextures = nullptr;
	if (matLoadingData)
		prevLoadedTextures = &matLoadingData->LoadedTextures;

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

		MaterialTexture* tex = new MaterialTexture(textureFromFile(pathStr, (sRGB) ? (GL_SRGB) : (GL_RGB)), shaderName + std::to_string(i + 1));	//create a new Texture and pass the file path, the shader name (for example diffuse1, diffuse2, ...) and the sRGB info
		Textures.push_back(tex);
		if (prevLoadedTextures)
			prevLoadedTextures->push_back(tex);
	}
}

void Material::UpdateWholeUBOData(Shader* shader, unsigned int emptyTexture)
{
	shader->Uniform1f("material.shininess", Shininess);
	shader->Uniform1f("material.depthScale", DepthScale);

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
			glBindTexture(GL_TEXTURE_2D, emptyTexture);
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

void AtlasMaterial::UpdateWholeUBOData(Shader* shader, unsigned int emptyTexture)
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

void MaterialInstance::UpdateWholeUBOData(Shader* shader, unsigned int emptyTexture)
{
	UpdateInstanceUBOData(shader);
	MaterialPtr->UpdateWholeUBOData(shader, emptyTexture);
}