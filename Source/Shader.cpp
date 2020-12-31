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

GLint Shader::FindLocation(std::string name) const
{
	auto loc = std::find_if(Locations.begin(), Locations.end(), [name](const UniformLocation& location) { return location.Name == name; });

	if (loc != Locations.end())
		return loc->Location;

	Locations.push_back(UniformLocation(name, glGetUniformLocation(Program, name.c_str())));
	return Locations.back().Location;
}

/*
==================================================================
==================================================================
==================================================================
*/

Shader::Shader(std::string name):
	Program(0),
	Name(name),
	ExpectedMatrices{}
{
	ShadersSource = { "", "", "" };
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
	Use();
	MaterialTextureUnits = materialTexUnits;
	for (unsigned int i = 0; i < MaterialTextureUnits.size(); i++)
		Uniform1i("material." + MaterialTextureUnits[i].second, MaterialTextureUnits[i].first);
}

void Shader::AddTextureUnit(unsigned int unitIndex, std::string nameInShader)
{
	Use();
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

void Shader::Uniform1i(std::string name, int val) const
{
	glUniform1i(FindLocation(name), val);
}

void Shader::Uniform1f(std::string name, float val) const
{
	glUniform1f(FindLocation(name), val);
}

void Shader::Uniform2fv(std::string name, glm::vec2 val) const
{
	glUniform2fv(FindLocation(name), 1, glm::value_ptr(val));
}

void Shader::Uniform3fv(std::string name, glm::vec3 val) const
{
	glUniform3fv(FindLocation(name), 1, glm::value_ptr(val));
}

void Shader::Uniform4fv(std::string name, glm::vec4 val) const
{
	glUniform4fv(FindLocation(name), 1, glm::value_ptr(val));
}

void Shader::UniformMatrix3fv(std::string name, glm::mat3 val) const
{
	glUniformMatrix3fv(FindLocation(name), 1, GL_FALSE, glm::value_ptr(val));
}

void Shader::UniformMatrix4fv(std::string name, const glm::mat4& val) const
{
	glUniformMatrix4fv(FindLocation(name), 1, GL_FALSE, glm::value_ptr(val));
}

void Shader::UniformBlockBinding(std::string name, unsigned int binding) const
{
	glUniformBlockBinding(Program, GetUniformBlockIndex(name), binding);
}

void Shader::UniformBlockBinding(unsigned int blockID, unsigned int binding) const
{
	glUniformBlockBinding(Program, blockID, binding);
}

unsigned int Shader::GetUniformBlockIndex(std::string name) const
{
	return glGetUniformBlockIndex(Program, name.c_str());
}

void Shader::Use() const
{
	glUseProgram(Program);
}

void Shader::BindMatrices(const glm::mat4& model, const glm::mat4* view, const glm::mat4* projection, const glm::mat4* VP) const
{
	if (ExpectedMatrices[MatrixType::MODEL])
		UniformMatrix4fv("model", model);
	if (ExpectedMatrices[MatrixType::VIEW])
		UniformMatrix4fv("view", *view);
	if (ExpectedMatrices[MatrixType::PROJECTION])
		UniformMatrix4fv("projection", *projection);
	if (ExpectedMatrices[MatrixType::MV])
		UniformMatrix4fv("MV", (*view) * model);
	if (ExpectedMatrices[MatrixType::VP])
		UniformMatrix4fv("VP", *VP);
	if (ExpectedMatrices[MatrixType::MVP])
		UniformMatrix4fv("MVP", (*VP) * model);
	if (ExpectedMatrices[MatrixType::NORMAL])
		UniformMatrix3fv("normalMat", ModelToNormal(model));
}

void Shader::Dispose()
{
	glDeleteProgram(Program);
	Program = 0;
}

unsigned int ShaderLoader::LoadShader(GLenum type, std::string shaderPath, std::string additionalData, std::string* shaderSourcePtr)
{
	additionalData = "#version 400 core\n" + additionalData;
	std::fstream shaderFile(shaderPath);

	if (!shaderFile.good())
	{
		std::cout << "ERROR! Cannot open: " << shaderPath << " :(\n";
		return -1;
	}

	std::stringstream shaderStream;
	shaderStream << shaderFile.rdbuf();

	std::string shaderSourceStr = additionalData + shaderStream.str();
	if (shaderSourcePtr)
		*shaderSourcePtr = shaderSourceStr;


	const char* shaderSource = shaderSourceStr.c_str();

	unsigned int shader = glCreateShader(type);
	glShaderSource(shader, 1, &shaderSource, 0);
	glCompileShader(shader);

	DebugShader(shader, shaderPath);

	return shader;
}

void ShaderLoader::DebugShader(unsigned int shaderID, std::string path)
{
	int result;
	char data[512];
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(shaderID, 512, NULL, data);
		std::cout << "Shader error (" + path + "): " << data;
	}
}

std::shared_ptr<Shader> ShaderLoader::LoadShaders(std::string shaderName, std::string vShaderPath, std::string fShaderPath, std::string gShaderPath)
{
	return LoadShadersWithInclData(shaderName, "", vShaderPath, fShaderPath, gShaderPath);
}

std::shared_ptr<Shader> ShaderLoader::LoadShadersWithInclData(std::string shaderName, std::string data, std::string vShaderPath, std::string fShaderPath, std::string gShaderPath)
{
	return LoadShadersWithExclData(shaderName, data, vShaderPath, data, fShaderPath, (gShaderPath.empty()) ? (std::string()) : (data), gShaderPath);
}

std::shared_ptr<Shader> ShaderLoader::LoadShadersWithExclData(std::string shaderName, std::string vShaderData, std::string vShaderPath, std::string fShaderData, std::string fShaderPath, std::string gShaderData, std::string gShaderPath)
{
	std::shared_ptr<Shader> shaderObj = std::make_shared<Shader>(shaderName);
	unsigned int nrShaders = (gShaderPath.empty()) ? (2) : (3);
	std::vector<unsigned int> shaders(nrShaders);

	shaders[0] = LoadShader(GL_VERTEX_SHADER, vShaderPath, vShaderData, &shaderObj->ShadersSource[0]);
	shaders[1] = LoadShader(GL_FRAGMENT_SHADER, fShaderPath, fShaderData, &shaderObj->ShadersSource[1]);
	if (nrShaders == 3) shaders[2] = LoadShader(GL_GEOMETRY_SHADER, gShaderPath, gShaderData, &shaderObj->ShadersSource[2]);

	shaderObj->Program = glCreateProgram();

	for (unsigned int i = 0; i < nrShaders; i++)
		glAttachShader(shaderObj->Program, shaders[i]);
	glLinkProgram(shaderObj->Program);

	for (unsigned int i = 0; i < nrShaders; i++)
	{
		glDetachShader(shaderObj->Program, shaders[i]);
		glDeleteShader(shaders[i]);
	}

	return shaderObj;
}


glm::mat3 ModelToNormal(glm::mat4 model)
{
	return glm::mat3(glm::transpose(glm::inverse(model)));
}