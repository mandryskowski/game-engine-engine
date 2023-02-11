#include <rendering/Shader.h>
#include <fstream>

#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIButtonActor.h>

namespace GEE
{
	template <> void Shader::Uniform<int>(const String& name, const int& val) const { glUniform1i(FindLocation(name), val); }
	template <> void Shader::Uniform<bool>(const String& name, const bool& val) const { glUniform1i(FindLocation(name), val); }
	template <> void Shader::Uniform<float>(const String& name, const float& val) const { glUniform1f(FindLocation(name), val); }

	template <> void Shader::Uniform<Vec2f>(const String& name, const Vec2f& val) const { glUniform2fv(FindLocation(name), 1, Math::GetDataPtr(val)); }
	template <> void Shader::Uniform<Vec3f>(const String& name, const Vec3f& val) const { glUniform3fv(FindLocation(name), 1, Math::GetDataPtr(val)); }
	template <> void Shader::Uniform<Vec4f>(const String& name, const Vec4f& val) const { glUniform4fv(FindLocation(name), 1, Math::GetDataPtr(val)); }

	template <> void Shader::Uniform<Vec2u>(const String& name, const Vec2u& val) const { glUniform2uiv(FindLocation(name), 1, Math::GetDataPtr(val)); }
	template <> void Shader::Uniform<Vec3u>(const String& name, const Vec3u& val) const { glUniform3uiv(FindLocation(name), 1, Math::GetDataPtr(val)); }
	template <> void Shader::Uniform<Vec4u>(const String& name, const Vec4u& val) const { glUniform4uiv(FindLocation(name), 1, Math::GetDataPtr(val)); }

	template <> void Shader::Uniform<Vec2i>(const String& name, const Vec2i& val) const { glUniform2iv(FindLocation(name), 1, Math::GetDataPtr(val)); }
	template <> void Shader::Uniform<Vec3i>(const String& name, const Vec3i& val) const { glUniform3iv(FindLocation(name), 1, Math::GetDataPtr(val)); }
	template <> void Shader::Uniform<Vec4i>(const String& name, const Vec4i& val) const { glUniform4iv(FindLocation(name), 1, Math::GetDataPtr(val)); }

	template <> void Shader::Uniform<Vec2b>(const String& name, const Vec2b& val) const { Uniform<Vec2i>(name, static_cast<const Vec2i>(val)); }
	template <> void Shader::Uniform<Vec3b>(const String& name, const Vec3b& val) const { Uniform<Vec3i>(name, static_cast<const Vec3i>(val)); }
	template <> void Shader::Uniform<Vec4b>(const String& name, const Vec4b& val) const { Uniform<Vec4i>(name, static_cast<const Vec4i>(val)); }

	template <> void Shader::Uniform<Mat3f>(const String& name, const Mat3f& val) const { glUniformMatrix3fv(FindLocation(name), 1, GL_FALSE, Math::GetDataPtr(val)); }

	template <> void Shader::Uniform<Mat4f>(const String& name, const Mat4f& val) const { glUniformMatrix4fv(FindLocation(name), 1, GL_FALSE, Math::GetDataPtr(val)); }



	template <> void Shader::UniformArray<int>(const String& name, const int* val, unsigned int size) const { glUniform1iv(FindLocation(name), size, val); }
	template <> void Shader::UniformArray<bool>(const String& name, const bool* val, unsigned int size) const { glUniform1iv(FindLocation(name), size, reinterpret_cast<const GLint*>(val)); }
	template <> void Shader::UniformArray<float>(const String& name, const float* val, unsigned int size) const { glUniform1fv(FindLocation(name), size, val); }

	template <> void Shader::UniformArray<Vec2f>(const String& name, const Vec2f* val, unsigned int size) const { glUniform2fv(FindLocation(name), size, &(*val)[0]); }
	template <> void Shader::UniformArray<Vec3f>(const String& name, const Vec3f* val, unsigned int size) const { glUniform3fv(FindLocation(name), size, &(*val)[0]); }
	template <> void Shader::UniformArray<Vec4f>(const String& name, const Vec4f* val, unsigned int size) const { glUniform4fv(FindLocation(name), size, &(*val)[0]); }

	template <> void Shader::UniformArray<Vec2u>(const String& name, const Vec2u* val, unsigned int size) const { glUniform2uiv(FindLocation(name), size, &(*val)[0]); }
	template <> void Shader::UniformArray<Vec3u>(const String& name, const Vec3u* val, unsigned int size) const { glUniform3uiv(FindLocation(name), size, &(*val)[0]); }
	template <> void Shader::UniformArray<Vec4u>(const String& name, const Vec4u* val, unsigned int size) const { glUniform4uiv(FindLocation(name), size, &(*val)[0]); }

	template <> void Shader::UniformArray<Vec2i>(const String& name, const Vec2i* val, unsigned int size) const { glUniform2iv(FindLocation(name), size, &(*val)[0]); }
	template <> void Shader::UniformArray<Vec3i>(const String& name, const Vec3i* val, unsigned int size) const { glUniform3iv(FindLocation(name), size, &(*val)[0]); }
	template <> void Shader::UniformArray<Vec4i>(const String& name, const Vec4i* val, unsigned int size) const { glUniform4iv(FindLocation(name), size, &(*val)[0]); }

	template <> void Shader::UniformArray<Mat3f>(const String& name, const Mat3f* val, unsigned int size) const { glUniformMatrix3fv(FindLocation(name), size, GL_FALSE, &(*val)[0][0]); }

	template <> void Shader::UniformArray<Mat4f>(const String& name, const Mat4f* val, unsigned int size) const { glUniformMatrix4fv(FindLocation(name), size, GL_FALSE, &(*val)[0][0]); }

	Shader::Shader(const String& name) :
		Program(0),
		Name(name),
		ExpectedMatrices{},
		OnMaterialWholeDataUpdateFunc(nullptr)
	{
		ShadersSource = { "", "", "" };
	}

	String Shader::GetName()
	{
		return Name;
	}

	Vector<Pair<unsigned int, String>>* Shader::GetMaterialTextureUnits()
	{
		return &MaterialTextureUnits;
	}

	bool Shader::ExpectsMatrix(unsigned int index) const
	{
		return ExpectedMatrices[index];
	}

	void Shader::SetName(const String& name)
	{
		Name = name;
	}

	void Shader::SetTextureUnitNames(Vector <Pair<unsigned int, String>> materialTexUnits)
	{
		Use();
		MaterialTextureUnits = materialTexUnits;
		for (auto& MaterialTextureUnit : MaterialTextureUnits)
			Uniform<int>("material." + MaterialTextureUnit.second, MaterialTextureUnit.first);
	}

	void Shader::AddTextureUnit(unsigned int unitIndex, const String& nameInShader)
	{
		Use();
		MaterialTextureUnits.push_back(Pair<unsigned int, String>(unitIndex, nameInShader));
		Uniform<int>("material." + nameInShader, unitIndex);
	}

	void Shader::SetExpectedMatrices(Vector <MatrixType> matrices)
	{
		for (unsigned int i = 0; i < matrices.size(); i++)
			ExpectedMatrices[(unsigned int)matrices[i]] = true;
	}

	void Shader::AddExpectedMatrix(const String& matType)
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

	void Shader::CallOnMaterialWholeDataUpdateFunc(const Material& mat)
	{
		if (OnMaterialWholeDataUpdateFunc)
			OnMaterialWholeDataUpdateFunc(*this, mat);
	}


	void Shader::UniformBlockBinding(const String& name, unsigned int binding) const
	{
		glUniformBlockBinding(Program, GetUniformBlockIndex(name), binding);
	}

	void Shader::UniformBlockBinding(unsigned int blockID, unsigned int binding) const
	{
		glUniformBlockBinding(Program, blockID, binding);
	}

	unsigned int Shader::GetUniformBlockIndex(const String& name) const
	{
		return glGetUniformBlockIndex(Program, name.c_str());
	}

	void Shader::Use() const
	{
		glUseProgram(Program);
	}

	void Shader::BindMatrices(const Mat4f& model, const Mat4f* view, const Mat4f* projection, const Mat4f* VP)
	{
		if (ExpectedMatrices[MatrixType::MODEL])
			Uniform("model", model);
		if (ExpectedMatrices[MatrixType::VIEW])
			Uniform("view", *view);
		if (ExpectedMatrices[MatrixType::PROJECTION])
			Uniform("projection", *projection);
		if (ExpectedMatrices[MatrixType::MV])
			Uniform("MV", (*view) * model);
		if (ExpectedMatrices[MatrixType::VP])
			Uniform("VP", *VP);
		if (ExpectedMatrices[MatrixType::MVP])
			Uniform("MVP", (*VP) * model);
		if (ExpectedMatrices[MatrixType::NORMAL])
			Uniform<Mat3f>("normalMat", Math::ModelToNormal(model));
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

		descBuilder.AddField("Texture units").GetTemplates().ListSelection<std::pair<unsigned int, String>>(MaterialTextureUnits.begin(), MaterialTextureUnits.end(), [](UIAutomaticListActor& listActor, std::pair<unsigned int, String>& object) { listActor.CreateChild<UIButtonActor>(object.second, object.second + " - " + std::to_string(object.first)); });
	}

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

	GLint Shader::FindLocation(const String& name) const
	{
		if (Locations[name] == 0)
			Locations[name] = glGetUniformLocation(Program, name.c_str());

		return Locations[name];
	}


	unsigned int ShaderLoader::LoadShader(GLenum type, const String& shaderPath, String additionalData, String* shaderSourcePtr)
	{
		additionalData = "#version 400 core\n" + additionalData;
		std::fstream shaderFile(shaderPath);

		if (!shaderFile.good())
		{
			std::cout << "ERROR: Cannot open: " << shaderPath << "(\n";
			return -1;
		}

		std::stringstream shaderStream;
		shaderStream << shaderFile.rdbuf();

		String shaderSourceStr = additionalData + shaderStream.str();
		if (shaderSourcePtr)
			*shaderSourcePtr = shaderSourceStr;


		const char* shaderSource = shaderSourceStr.c_str();

		unsigned int shader = glCreateShader(type);
		glShaderSource(shader, 1, &shaderSource, 0);
		glCompileShader(shader);

		if (!DebugShader(shader, shaderPath))
			std::cout << "Faulty shader source:\n" << shaderSourceStr << "\n";

		return shader;
	}

	bool ShaderLoader::DebugShader(unsigned int shaderID, const String& path)
	{
		int result;
		char data[512];
		glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
		if (!result)
		{
			glGetShaderInfoLog(shaderID, 512, NULL, data);
			std::cout << "Shader error (" + path + ")\n" << data << '\n';
			return false;
		}
		return true;
	}

	SharedPtr<Shader> ShaderLoader::LoadShaders(const String& shaderName, const String& vShaderPath, const String& fShaderPath, const String& gShaderPath)
	{
		return LoadShadersWithInclData(shaderName, "", vShaderPath, fShaderPath, gShaderPath);
	}

	SharedPtr<Shader> ShaderLoader::LoadShadersWithInclData(const String& shaderName, const String& data, const String& vShaderPath, const String& fShaderPath, const String& gShaderPath)
	{
		return LoadShadersWithExclData(shaderName, data, vShaderPath, data, fShaderPath, (gShaderPath.empty()) ? (String()) : (data), gShaderPath);
	}

	SharedPtr<Shader> ShaderLoader::LoadShadersWithExclData(const String& shaderName, const String& vShaderData, const String& vShaderPath, const String& fShaderData, const String& fShaderPath, const String& gShaderData, const String& gShaderPath)
	{
		SharedPtr<Shader> shaderObj = MakeShared<Shader>(shaderName);
		unsigned int nrShaders = (gShaderPath.empty()) ? (2) : (3);
		Vector<unsigned int> shaders(nrShaders);


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
}