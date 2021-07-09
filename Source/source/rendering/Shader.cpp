#include <rendering/Shader.h>
#include <fstream>

#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIButtonActor.h>

namespace GEE
{
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

	Shader::Shader(std::string name) :
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

	void Shader::CallPreRenderFunc()
	{
		if (PreRenderFunc)
			PreRenderFunc();
	}

	void Shader::Uniform1i(std::string name, int val) const
	{
		glUniform1i(FindLocation(name), val);
	}

	void Shader::Uniform1f(std::string name, float val) const
	{
		glUniform1f(FindLocation(name), val);
	}

	void Shader::Uniform2fv(std::string name, Vec2f val) const
	{
		glUniform2fv(FindLocation(name), 1, Math::GetDataPtr(val));
	}

	void Shader::Uniform3fv(std::string name, Vec3f val) const
	{
		glUniform3fv(FindLocation(name), 1, Math::GetDataPtr(val));
	}

	void Shader::Uniform4fv(std::string name, Vec4f val) const
	{
		glUniform4fv(FindLocation(name), 1, Math::GetDataPtr(val));
	}

	void Shader::UniformMatrix3fv(std::string name, Mat3f val) const
	{
		glUniformMatrix3fv(FindLocation(name), 1, GL_FALSE, Math::GetDataPtr(val));
	}

	void Shader::UniformMatrix4fv(std::string name, const Mat4f& val) const
	{
		glUniformMatrix4fv(FindLocation(name), 1, GL_FALSE, Math::GetDataPtr(val));
	}


	template <> void Shader::Uniform<int>(const std::string& name, const int& val)		{ glUniform1i(FindLocation(name), val); }

	template <> void Shader::Uniform<float>(const std::string& name, const float& val)	{ glUniform1f(FindLocation(name), val);	}

	template <> void Shader::Uniform<Vec2f>(const std::string& name, const Vec2f& val) { glUniform2fv(FindLocation(name), 1, Math::GetDataPtr(val)); }
	template <> void Shader::Uniform<Vec3f>(const std::string& name, const Vec3f& val) { glUniform3fv(FindLocation(name), 1, Math::GetDataPtr(val)); }
	template <> void Shader::Uniform<Vec4f>(const std::string& name, const Vec4f& val) { glUniform4fv(FindLocation(name), 1, Math::GetDataPtr(val)); }

	template <> void Shader::Uniform<Vec2u>(const std::string& name, const Vec2u& val) { glUniform2uiv(FindLocation(name), 1, Math::GetDataPtr(val)); }
	template <> void Shader::Uniform<Vec3u>(const std::string& name, const Vec3u& val) { glUniform3uiv(FindLocation(name), 1, Math::GetDataPtr(val)); }
	template <> void Shader::Uniform<Vec4u>(const std::string& name, const Vec4u& val) { std::cout << "AAA VEC4U " << name << " loc:" << FindLocation(name) << '\n'; glUniform4uiv(FindLocation(name), 1, Math::GetDataPtr(val)); }

	template <> void Shader::Uniform<Vec2i>(const std::string& name, const Vec2i& val) { glUniform2iv(FindLocation(name), 1, Math::GetDataPtr(val)); }
	template <> void Shader::Uniform<Vec3i>(const std::string& name, const Vec3i& val) { glUniform3iv(FindLocation(name), 1, Math::GetDataPtr(val)); }
	template <> void Shader::Uniform<Vec4i>(const std::string& name, const Vec4i& val) { glUniform4iv(FindLocation(name), 1, Math::GetDataPtr(val)); }

	template <> void Shader::Uniform<Mat3f>(const std::string& name, const Mat3f& val)	{ glUniformMatrix3fv(FindLocation(name), 1, GL_FALSE, Math::GetDataPtr(val));	}

	template <> void Shader::Uniform<Mat4f>(const std::string& name, const Mat4f& val)	{ glUniformMatrix4fv(FindLocation(name), 1, GL_FALSE, Math::GetDataPtr(val));	}


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

	void Shader::BindMatrices(const Mat4f& model, const Mat4f* view, const Mat4f* projection, const Mat4f* VP) const
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

	void Shader::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		descBuilder.AddField("Shader name").CreateChild<UIButtonActor>("ShaderName", Name).SetDisableInput(true);
		descBuilder.AddField("Program ID").CreateChild<UIButtonActor>("ProgramID", std::to_string(Program)).SetDisableInput(true);

		descBuilder.AddField("Texture units").GetTemplates().ListSelection<std::pair<unsigned int, std::string>>(MaterialTextureUnits.begin(), MaterialTextureUnits.end(), [](UIAutomaticListActor& listActor, std::pair<unsigned int, std::string>& object) { listActor.CreateChild<UIButtonActor>(object.second, object.second + " - " + std::to_string(object.first)); });
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
		if (shaderPath == "Shaders/depth.fs")
			std::cout << shaderSourceStr << '\n';	
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

	SharedPtr<Shader> ShaderLoader::LoadShaders(std::string shaderName, std::string vShaderPath, std::string fShaderPath, std::string gShaderPath)
	{
		return LoadShadersWithInclData(shaderName, "", vShaderPath, fShaderPath, gShaderPath);
	}

	SharedPtr<Shader> ShaderLoader::LoadShadersWithInclData(std::string shaderName, std::string data, std::string vShaderPath, std::string fShaderPath, std::string gShaderPath)
	{
		return LoadShadersWithExclData(shaderName, data, vShaderPath, data, fShaderPath, (gShaderPath.empty()) ? (std::string()) : (data), gShaderPath);
	}

	SharedPtr<Shader> ShaderLoader::LoadShadersWithExclData(std::string shaderName, std::string vShaderData, std::string vShaderPath, std::string fShaderData, std::string fShaderPath, std::string gShaderData, std::string gShaderPath)
	{
		SharedPtr<Shader> shaderObj = MakeShared<Shader>(shaderName);
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


	Mat3f ModelToNormal(Mat4f model)
	{
		return Mat3f(glm::transpose(glm::inverse(model)));
	}
}