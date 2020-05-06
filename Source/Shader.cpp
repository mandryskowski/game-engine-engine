#include "Shader.h"

void Shader::DebugShader(unsigned int shader)
{
	int result;
	char data[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(shader, 512, NULL, data);
		std::cout << "Shader error: " << data;
	}
}

Shader::Shader():
	Program(0),
	Name("Internal"),
	ExpectedMatrices{ 0 }
{
}

Shader::Shader( std::string vShaderPath, std::string fShaderPath, std::string gShaderPath):
	Name("Internal")
{
	LoadShaders(vShaderPath, fShaderPath, gShaderPath);
}

std::string Shader::GetName()
{
	return Name;
}

std::vector<std::pair<unsigned int, std::string>>* Shader::GetMaterialTextureUnits()
{
	return &MaterialTextureUnits;
}

bool Shader::ExpectsMatrix(unsigned int index)
{
	return ExpectedMatrices[index];
}

void Shader::SetName(std::string name)
{
	Name = name;
}

void Shader::SetTextureUnitNames(std::vector <std::pair<unsigned int, std::string>> materialTexUnits)
{
	MaterialTextureUnits = materialTexUnits;
	for (unsigned int i = 0; i < MaterialTextureUnits.size(); i++)
		Uniform1i("material." + MaterialTextureUnits[i].second, MaterialTextureUnits[i].first);
}

void Shader::AddTextureUnit(unsigned int unitIndex, std::string nameInShader)
{
	MaterialTextureUnits.push_back(std::pair<unsigned int, std::string>(unitIndex, nameInShader));
	Uniform1i("material." + nameInShader, unitIndex);
}

void Shader::SetExpectedMatrices(std::vector <MatrixType> matrices)
{
	for (unsigned int i = 0; i < matrices.size(); i++)
		ExpectedMatrices[(unsigned int)matrices[i]] = true;
}

void Shader::AddExpectedMatrix(std::string matType)
{
	if (matType == "M")
		ExpectedMatrices[MatrixType::MODEL] = true;
	else if (matType == "V")
		ExpectedMatrices[MatrixType::VIEW] = true;
	else if (matType == "P")
		ExpectedMatrices[MatrixType::PROJECTION] = true;
	else if (matType == "MV")
		ExpectedMatrices[MatrixType::MV] = true;
	else if (matType == "VP")
		ExpectedMatrices[MatrixType::VP] = true;
	else if (matType == "MVP")
		ExpectedMatrices[MatrixType::MVP] = true;
	else if (matType == "NORMAL")
		ExpectedMatrices[MatrixType::NORMAL] = true;
	else
		std::cerr << "ERROR! Can't find matrix type " << matType << '\n';
}

unsigned int Shader::LoadShader(GLenum type, const char* shaderPath)
{
	std::fstream shaderFile(shaderPath);

	if (!shaderFile.good())
	{
		std::cout << "ERROR! Cannot open: " << shaderPath << " :(\n";
		return -1;
	}

	std::stringstream shaderStream;
	shaderStream << shaderFile.rdbuf();

	std::string shaderSourceStr = shaderStream.str();

	const char* shaderSource = shaderSourceStr.c_str();

	unsigned int shader = glCreateShader(type);
	glShaderSource(shader, 1, &shaderSource, 0);
	glCompileShader(shader);

	DebugShader(shader);

	return shader;
}

void Shader::LoadShaders(std::string vShaderPath, std::string fShaderPath, std::string gShaderPath)
{
	unsigned int nrShaders = (gShaderPath.empty()) ? (2) : (3);
	unsigned int* shaders = new unsigned int[nrShaders];
	shaders[0] = LoadShader(GL_VERTEX_SHADER, vShaderPath.c_str());
	shaders[1] = LoadShader(GL_FRAGMENT_SHADER, fShaderPath.c_str());
	if (nrShaders == 3) shaders[2] = LoadShader(GL_GEOMETRY_SHADER, gShaderPath.c_str());

	Program = glCreateProgram();
	for (unsigned int i = 0; i < nrShaders; i++)
		glAttachShader(Program, shaders[i]);
	glLinkProgram(Program);
	for (unsigned int i = 0; i < nrShaders; i++)
	{
		glDetachShader(Program, shaders[i]);
		glDeleteShader(shaders[i]);
	}
}

void Shader::Uniform1i(std::string name, int val)
{
	glUniform1i(glGetUniformLocation(Program, name.c_str()), val);
}

void Shader::Uniform1f(std::string name, float val)
{
	glUniform1f(glGetUniformLocation(Program, name.c_str()), val);
}

void Shader::Uniform2fv(std::string name, glm::vec2 val)
{
	glUniform2fv(glGetUniformLocation(Program, name.c_str()), 1, glm::value_ptr(val));
}

void Shader::Uniform3fv(std::string name, glm::vec3 val)
{
	glUniform3fv(glGetUniformLocation(Program, name.c_str()), 1, glm::value_ptr(val));
}

void Shader::Uniform4fv(std::string name, glm::vec4 val)
{
	glUniform4fv(glGetUniformLocation(Program, name.c_str()), 1, glm::value_ptr(val));
}

void Shader::UniformMatrix3fv(std::string name, glm::mat3 val)
{
	glUniformMatrix3fv(glGetUniformLocation(Program, name.c_str()), 1, GL_FALSE, glm::value_ptr(val));
}

void Shader::UniformMatrix4fv(std::string name, glm::mat4 val)
{
	glUniformMatrix4fv(glGetUniformLocation(Program, name.c_str()), 1, GL_FALSE, glm::value_ptr(val));
}

void Shader::UniformBlockBinding(std::string name, unsigned int binding)
{
	glUniformBlockBinding(Program, GetUniformBlockIndex(name), binding);
}

void Shader::UniformBlockBinding(unsigned int blockID, unsigned int binding)
{
	glUniformBlockBinding(Program, blockID, binding);
}

unsigned int Shader::GetUniformBlockIndex(std::string name)
{
	return glGetUniformBlockIndex(Program, name.c_str());
}

void Shader::Use()
{
	glUseProgram(Program);
}