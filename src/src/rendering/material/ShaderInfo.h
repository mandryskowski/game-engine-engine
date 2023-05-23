#pragma once
namespace cereal
{
	template <class T> class construct;
}
namespace GEE
{
	class RenderToolboxCollection;
	class Shader;

	enum class MaterialShaderHint
	{
		None,
		Simple,
		Shaded,
	};

	struct VaryingMaterialShaderHints
	{
	public:
		static MaterialShaderHint Shaded() { return MaterialShaderHint::Shaded; }
	};


	struct ShaderInfo
	{
		ShaderInfo(MaterialShaderHint hint) : ShaderHint(hint), CustomShader(nullptr)
		{
		}

		ShaderInfo(Shader& customShader) : ShaderHint(MaterialShaderHint::None), CustomShader(&customShader)
		{
		}

		bool IsValid() const { return UsesShaderHint() || CustomShader; }
		bool UsesShaderHint() const { return ShaderHint != MaterialShaderHint::None; }

		bool MatchesRequiredInfo(const ShaderInfo& requiredInfo)
		{
			if (requiredInfo.UsesShaderHint())
				return requiredInfo.GetShaderHint() == GetShaderHint();

			return (!requiredInfo.GetCustomShader() || requiredInfo.GetCustomShader() == GetCustomShader());
		}

		MaterialShaderHint GetShaderHint() const { return ShaderHint; }
		Shader* GetCustomShader() { return CustomShader; }
		const Shader* GetCustomShader() const { return CustomShader; }

		/**
		 * @brief If this ShaderInfo refers to a custom shader, the shader will be returned. If this ShaderInfo contains a hint, the correct shader will be retrieved from the render toolbox collection.
		 * @return a pointer to a Shader useful in rendering. Can be nullptr if this Shader info is not valid.
		*/
		Shader* RetrieveShaderForRendering(RenderToolboxCollection&);

		template <typename Archive>
		void Save(Archive& archive) const;
		template <typename Archive>
		void Load(Archive& archive);

		template <typename Archive>
		static void load_and_construct(Archive& archive, cereal::construct<ShaderInfo>& construct);

	private:
		/*
		*	Shader hints are useful if our material dose not use a specific type of shader
		*	If they do, use the optional pointer
		*/
		MaterialShaderHint ShaderHint;
		Shader* CustomShader;
	};
}
