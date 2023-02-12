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
		explicit Shader(const String& name = "undefinedShader");
		[[nodiscard]] String GetName();
		[[nodiscard]] Vector<Pair<unsigned int, String>>* GetMaterialTextureUnits();
		[[nodiscard]] bool ExpectsMatrix(unsigned int) const;

		void SetName(const String&);
		void SetTextureUnitNames(std::vector<std::pair<unsigned int, String>>);
		void AddTextureUnit(unsigned int, const String&);
		void SetExpectedMatrices(std::vector<MatrixType>);
		void AddExpectedMatrix(const String&);

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

		template <typename T>
		void Uniform(const String& nameInShader, const T&) const;
		template <typename T>
		void UniformArray(const String& nameInShader, const T*, unsigned int size) const;

		void UniformBlockBinding(const String&, unsigned int) const;
		void UniformBlockBinding(unsigned int, unsigned int) const;

		unsigned int GetUniformBlockIndex(const String&) const;
		void Use() const;
		void BindMatrices(const Mat4f& model, const Mat4f* view, const Mat4f* projection, const Mat4f* VP);

		void Dispose();

		void GetEditorDescription(EditorDescriptionBuilder);

	private:
		static void DebugShader(unsigned int);
		GLint FindLocation(const String&) const;

		unsigned int Program;
		String Name;

		std::vector <std::pair<unsigned int, String>> MaterialTextureUnits;
		bool ExpectedMatrices[MATRICES_NB];

		std::function<void()> PreRenderFunc;

		mutable std::unordered_map<String, GLenum> Locations;
		std::array<String, 3> ShadersSource;

		std::function<void(Shader&, const Material&)> OnMaterialWholeDataUpdateFunc;

		friend class RenderEngine;
	};

	struct ShaderLoader
	{
	private:
		static unsigned int LoadShader(GLenum type, const String& path, String additionalData = String(), String* shaderSourcePtr = nullptr);
		/**
		 * @brief Debug a shader by logging any compilation errors, if such exist.
		 * @param shaderID: The OpenGL id of the shader.
		 * @param path: The filepath to the file that contains the shader source.
		 * @return: A boolean indicating whether compilation was successful.
		*/
		static bool DebugShader(unsigned int shaderID, const String& path = String());

	public:
		static SharedPtr<Shader> LoadShaders(const String& shaderName, const String& vShaderPath, const String& fShaderPath, const String& gShaderPath = String());
		static SharedPtr<Shader> LoadShadersWithInclData(const String& shaderName, const String& data, const String& vShaderPath, const String& fShaderPath, const String& gShaderPath = String());
		static SharedPtr<Shader> LoadShadersWithExclData(const String& shaderName, const String& vShaderData, const String& vShaderPath, const String& fShaderData, const String& fShaderPath, const String& gShaderData = String(), const String& gShaderPath = String());
	};


}