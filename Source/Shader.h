#pragma once
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
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
	NORMAL
};

class Shader
{
	unsigned int Program;
	std::string Name;

	std::vector <std::pair<unsigned int, std::string>> MaterialTextureUnits;
	bool ExpectedMatrices[7];

	void DebugShader(unsigned int);
public:
	Shader();
	Shader(std::string, std::string, std::string=std::string());
	std::string GetName();
	std::vector<std::pair<unsigned int, std::string>>* GetMaterialTextureUnits();
	bool ExpectsMatrix(unsigned int);
	void SetName(std::string);
	void SetTextureUnitNames(std::vector<std::pair<unsigned int, std::string>>);
	void AddTextureUnit(unsigned int, std::string);
	void SetExpectedMatrices(std::vector<MatrixType>);
	void AddExpectedMatrix(std::string);
	unsigned int LoadShader(GLenum, const char*);
	void LoadShaders(std::string, std::string, std::string = std::string());
	void Uniform1i(std::string, int);
	void Uniform1f(std::string, float);
	void Uniform2fv(std::string, glm::vec2);
	void Uniform3fv(std::string, glm::vec3);
	void Uniform4fv(std::string, glm::vec4);
	void UniformMatrix3fv(std::string, glm::mat3);
	void UniformMatrix4fv(std::string, glm::mat4);
	void UniformBlockBinding(std::string, unsigned int);
	void UniformBlockBinding(unsigned int, unsigned int);
	unsigned int GetUniformBlockIndex(std::string);
	void Use();
};

unsigned int textureFromFile(std::string, bool);