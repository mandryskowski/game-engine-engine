#pragma once
#include "Component.h"
#include <stb/stb_image.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

struct Texture
{
	unsigned int ID;
	std::string ShaderName;
	std::string Path;
	Texture(std::string path, std::string name = "diffuse", bool sRGB = true)
	{
		ID = textureFromFile(path, sRGB);
		ShaderName = name;
		Path = path;
	}
	Texture(unsigned int id = -1, std::string name = "diffuse")
	{
		ID = id;
		ShaderName = name;
		Path = "";
	}
};

struct MaterialLoadingData;	//note: it's legally incomplete in class Material declaration (it's safe to use incomplete types in functions'/methods' declarations!)

class Material
{
	std::vector <Texture*> Textures;
	float Shininess;
	float DepthScale;	//used in parallax mapping
	std::string Name;
public:
	Material(std::string name, float shine = 64.0f, float depthScale = 0.0f) { Name = name; Shininess = shine; DepthScale = depthScale; }
	std::string GetName();
	void AddTexture(Texture* tex) { Textures.push_back(tex); }
	void LoadFromAiMaterial(aiMaterial*, MaterialLoadingData*);
	void LoadAiTexturesOfType(aiMaterial*, aiTextureType, std::string, MaterialLoadingData*);
	void UpdateUBOData(Shader*, unsigned int);
};

struct MaterialLoadingData
{
	std::vector <Material*> LoadedMaterials;	//note: LoadedMaterials and LoadedAiMaterials will ALWAYS be the same size
	std::vector <aiMaterial*> LoadedAiMaterials;
	std::vector <Texture*> LoadedTextures;
};

unsigned int textureFromFile(std::string, bool);