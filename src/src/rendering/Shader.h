#pragma once
#include <utility/Utility.h>
#include <string>
#include <array>

namespace GEE
{
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

	/*struct ShaderVariant
	{
		unsigned int Program;
		const GameSettings* Settings;
	};*/

	class EditorDescriptionBuilder;
	class Material;

	class Shader
	{
		friend struct ShaderLoader;
	public:
		Shader(std::string name = "undefinedShader");
		std::string GetName();
		std::vector<std::pair<unsigned int, std::string>>* GetMaterialTextureUnits();
		bool ExpectsMatrix(unsigned int);

		void SetName(std::string);
		void SetTextureUnitNames(std::vector<std::pair<unsigned int, std::string>>);
		void AddTextureUnit(unsigned int, std::string);
		void SetExpectedMatrices(std::vector<MatrixType>);
		void AddExpectedMatrix(std::string);

		/**
		 * @param func: A procedure (void function) with no parameters.
		*/
		template <typename Functor>
		void SetPreRenderFunc(Functor&& func)
		{
			PreRenderFunc = func;
		}

		/**
		 * @tparam func: A procedure (void function) taking a Shader& and a const Material&.
		*/
		template <typename Functor>
		void SetOnMaterialWholeDataUpdateFunc(Functor&& onMaterialWholeDataUpdateFunc)
		{
			OnMaterialWholeDataUpdateFunc = onMaterialWholeDataUpdateFunc;
		}

		void CallPreRenderFunc();
		void CallOnMaterialWholeDataUpdateFunc(const Material&);

		void Uniform1i(std::string, int) const;
		void Uniform1f(std::string, float) const;
		void Uniform2fv(std::string, Vec2f) const;
		void Uniform3fv(std::string, Vec3f) const;
		void Uniform4fv(std::string, Vec4f) const;

		template <typename T>
		void Uniform(const std::string& nameInShader, const T&);
		template <typename T>
		void UniformArray(const std::string& nameInShader, const T*, unsigned int size);

		void UniformMatrix3fv(std::string, Mat3f) const;
		void UniformMatrix4fv(std::string, const Mat4f&) const;
		void UniformBlockBinding(std::string, unsigned int) const;
		void UniformBlockBinding(unsigned int, unsigned int) const;

		unsigned int GetUniformBlockIndex(std::string) const;
		void Use() const;
		void BindMatrices(const Mat4f& model, const Mat4f* view, const Mat4f* projection, const Mat4f* VP);

		void Dispose();

		void GetEditorDescription(EditorDescriptionBuilder);

	private:
		static void DebugShader(unsigned int);
		GLint FindLocation(std::string) const;

		unsigned int Program;
		std::string Name;

		std::vector <std::pair<unsigned int, std::string>> MaterialTextureUnits;
		bool ExpectedMatrices[MATRICES_NB];

		std::function<void()> PreRenderFunc;

		mutable std::unordered_map<std::string, GLenum> Locations;
		std::array<std::string, 3> ShadersSource;

		std::function<void(Shader&, const Material&)> OnMaterialWholeDataUpdateFunc;

		friend class RenderEngine;
	};

	struct ShaderLoader
	{
	private:
		static unsigned int LoadShader(GLenum type, std::string path, std::string additionalData = std::string(), std::string* shaderSourcePtr = nullptr);
		/**
		 * @brief Debug a shader by logging any compilation errors, if such exist.
		 * @param shaderID: The OpenGL id of the shader.
		 * @param path: The filepath to the file that contains the shader source.
		 * @return: A boolean indicating whether compilation was successful.
		*/
		static bool DebugShader(unsigned int shaderID, std::string path = std::string());

	public:
		static SharedPtr<Shader> LoadShaders(std::string shaderName, std::string vShaderPath, std::string fShaderPath, std::string gShaderPath = std::string());
		static SharedPtr<Shader> LoadShadersWithInclData(std::string shaderName, std::string data, std::string vShaderPath, std::string fShaderPath, std::string gShaderPath = std::string());
		static SharedPtr<Shader> LoadShadersWithExclData(std::string shaderName, std::string vShaderData, std::string vShaderPath, std::string fShaderData, std::string fShaderPath, std::string gShaderData = std::string(), std::string gShaderPath = std::string());
	};


	Mat3f ModelToNormal(Mat4f);
	//what the fuck is it doing here 
}