#pragma once
#include <glad4/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum MatrixType
{
	MODEL,
	VIEW,
	PROJECTION,
	MV,
	VP,
	MVP,
	NORMAL,
	MATRICES_NB
};

class Shader
{
	struct UniformLocation
	{
		std::string Name;
		GLint Location;

		UniformLocation(std::string name, GLint loc)
		{
			Name = name;
			Location = loc;
		}
	};

	friend struct ShaderLoader;



	unsigned int Program;
	std::string Name;

	std::vector <std::pair<unsigned int, std::string>> MaterialTextureUnits;
	bool ExpectedMatrices[MATRICES_NB];

	void DebugShader(unsigned int);
	GLint FindLocation(std::string) const;
	friend class RenderEngine;
public:
	mutable std::vector <UniformLocation> Locations;
	std::array<std::string, 3> ShadersSource;
	Shader(std::string name = "undefinedShader");
	std::string GetName();
	std::vector<std::pair<unsigned int, std::string>>* GetMaterialTextureUnits();
	bool ExpectsMatrix(unsigned int);

	void SetName(std::string);
	void SetTextureUnitNames(std::vector<std::pair<unsigned int, std::string>>);
	void AddTextureUnit(unsigned int, std::string);
	void SetExpectedMatrices(std::vector<MatrixType>);
	void AddExpectedMatrix(std::string);

	void Uniform1i(std::string, int) const;
	void Uniform1f(std::string, float) const;
	void Uniform2fv(std::string, glm::vec2) const;
	void Uniform3fv(std::string, glm::vec3) const;
	void Uniform4fv(std::string, glm::vec4) const;
	void UniformMatrix3fv(std::string, glm::mat3) const;
	void UniformMatrix4fv(std::string, const glm::mat4&) const;
	void UniformBlockBinding(std::string, unsigned int) const;
	void UniformBlockBinding(unsigned int, unsigned int) const;

	unsigned int GetUniformBlockIndex(std::string) const;
	void Use() const;
	void BindMatrices(const glm::mat4& model, const glm::mat4* view, const glm::mat4* projection, const glm::mat4* VP) const;

	void Dispose();
};

struct ShaderLoader
{
private:
	static unsigned int LoadShader(GLenum type, std::string path, std::string additionalData = std::string(), std::string* shaderSourcePtr = nullptr);
	static void DebugShader(unsigned int shaderID, std::string path = std::string());

public:
	static std::shared_ptr<Shader> LoadShaders(std::string shaderName, std::string vShaderPath, std::string fShaderPath, std::string gShaderPath = std::string());
	static std::shared_ptr<Shader> LoadShadersWithInclData(std::string shaderName, std::string data, std::string vShaderPath, std::string fShaderPath, std::string gShaderPath = std::string());
	static std::shared_ptr<Shader> LoadShadersWithExclData(std::string shaderName, std::string vShaderData, std::string vShaderPath, std::string fShaderData, std::string fShaderPath, std::string gShaderData = std::string(), std::string gShaderPath = std::string());
};
glm::mat3 ModelToNormal(glm::mat4);	//what the fuck is it doing here