#include "Material.h"

Texture::Texture(std::string path, std::string name, bool sRGB)
{
	ID = textureFromFile(path, GL_NEAREST, GL_LINEAR_MIPMAP_LINEAR, sRGB);
	ShaderName = name;
	Path = path;
}
Texture::Texture(unsigned int id, std::string name)
{
	ID = id;
	ShaderName = name;
	Path = "";
}

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

void Material::AddTexture(Texture* tex)
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
	std::vector <Texture*>* prevLoadedTextures = nullptr;
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
				if ((*prevLoadedTextures)[j]->Path == pathStr)
				{
					Textures.push_back((*prevLoadedTextures)[j]);
					bWasLoadedPreviously = true;
					break;
				}
			}

			if (bWasLoadedPreviously)	//skip the costly loading if we already loaded this file
				continue;
		}

		Texture* tex = new Texture(pathStr, shaderName + std::to_string(i + 1), sRGB);	//create a new Texture and pass the file path, the shader name (for example diffuse1, diffuse2, ...) and the sRGB info
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
			if (Textures[j]->ShaderName == unitPair->second)
			{
				glBindTexture(GL_TEXTURE_2D, Textures[j]->ID);
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

/*
========================================================================================================================
========================================================================================================================
========================================================================================================================
*/

unsigned int textureFromFile(std::string path, GLenum magFilter, GLenum minFilter, bool sRGB, bool flip)
{
	if (flip)
		stbi_set_flip_vertically_on_load(true);

	unsigned int tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	int width, height, nrChannels;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

	if (!data)
	{
		std::cerr << "nie laduje mi " << path << '\n';
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, glm::value_ptr(glm::vec3(1.0f, 0.0f, 1.0f)));	//set the texture's color to pink so its obvious that this texture is missing
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		return tex;
	}
	GLenum format = GL_RGB;
	GLenum internalformat = (sRGB) ? (GL_SRGB) : (GL_RGB);
	switch (nrChannels)
	{
	case 1: format = GL_RED; break;
	case 3: format = GL_RGB; break;
	case 4: format = GL_RGBA;
			internalformat = (sRGB) ? (GL_SRGB_ALPHA) : (GL_RGBA); break;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, GL_UNSIGNED_BYTE, data);

	if (minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST || minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR)
		glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);

	if (flip)
		stbi_set_flip_vertically_on_load(false);

	return tex;
}

unsigned int textureFromBuffer(const unsigned char& buffer, unsigned int width, unsigned int height, GLenum internalformat, GLenum format, GLenum magFilter, GLenum minFilter)
{
	unsigned int tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, GL_UNSIGNED_BYTE, &buffer);

	if (minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST || minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR)
		glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);

	return tex;
}
