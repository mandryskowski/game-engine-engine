#include "Material.h"

std::string Material::GetName()
{
	return Name;
}

void Material::LoadFromAiMaterial(aiMaterial* material, MaterialLoadingData* matLoadingData)
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
		LoadAiTexturesOfType(material, materialTypes[i].first, materialTypes[i].second, matLoadingData);
}

void Material::LoadAiTexturesOfType(aiMaterial* material, aiTextureType type, std::string shaderName, MaterialLoadingData* matLoadingData)
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
		pathStr = path.C_Str();

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

void Material::UpdateUBOData(Shader* shader, unsigned int emptyTexture)
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

unsigned int textureFromFile(std::string path, bool sRGB)
{
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
	switch (nrChannels)
	{
	case 1: format = GL_RED; break;
	case 3: format = GL_RGB; break;
	case 4: format = GL_RGBA; break;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, ((sRGB) ? (GL_SRGB) : (GL_RGB)), width, height, 0, format, GL_UNSIGNED_BYTE, data);

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);


	return tex;
}