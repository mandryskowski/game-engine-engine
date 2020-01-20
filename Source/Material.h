#pragma once
#include "Component.h"
#include <stb/stb_image.h>

struct Texture
{
	unsigned int ID;
	std::string ShaderName;
	Texture(std::string path, std::string name = "diffuse", bool sRGB = true)
	{
		ID = textureFromFile(path, sRGB);
		ShaderName = name;
	}
	Texture(unsigned int id = -1, std::string name = "diffuse")
	{
		ID = id;
		ShaderName = name;
	}
};

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
	void UpdateUBOData(Shader*, unsigned int);
};

unsigned int textureFromFile(std::string, bool);