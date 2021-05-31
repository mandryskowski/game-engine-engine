#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cereal/cereal.hpp>

//namespace Math
//{
	class Vec2f : public glm::vec2
	{
		using glm::vec2::vec2;
	public:
		template <typename Archive> void Serialize(Archive& archive)
		{
			archive(CEREAL_NVP(x), CEREAL_NVP(y));
		}
		glm::vec2 GetGlmType() const
		{
			return static_cast<glm::vec2>(*this);
		}
	};

	class Vec3f : public glm::vec3
	{
		using glm::vec3::vec3;
	public:
		template <typename Archive> void Serialize(Archive& archive)
		{
			archive(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z));
		}
		glm::vec3 GetGlmType() const
		{
			return static_cast<glm::vec3>(*this);
		}
	};

	class Vec4f : public glm::vec4
	{
		using glm::vec4::vec4;
	public:
		template <typename Archive> void Serialize(Archive& archive)
		{
			archive(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z), CEREAL_NVP(w));
		}
		glm::vec4 GetGlmType() const
		{
			return static_cast<glm::vec4>(*this);
		}
	};

	class Quatf : public glm::quat
	{
		using glm::quat::quat;
	public:
		template <typename Archive> void Serialize(Archive& archive)
		{
			archive(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z), CEREAL_NVP(w));
		}
	};

	class Mat4f : public glm::mat4
	{
		using glm::mat4::mat4;
	public:
		template <typename Archive> void Save(Archive& archive) const
		{
			for (int i = 0; i < 4; i++)
				archive(cereal::make_nvp("row" + std::to_string(i), Vec4f((*this)[i])));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			for (int i = 0; i < 4; i++)
			{
				Vec4f row;
				archive(cereal::make_nvp("row" + std::to_string(i), row));
				(*this)[i] = row;
			}

		}
	};
//}