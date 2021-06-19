#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cereal/cereal.hpp>

namespace GEE
{
	template <unsigned int length, typename VecType> using Vec = glm::vec<length, VecType>;
	using Vec2u = Vec<2, unsigned int>;
	using Vec3u = Vec<3, unsigned int>;
	using Vec4u = Vec<4, unsigned int>;

	using Vec2f = Vec<2, float>;
	using Vec3f = Vec<3, float>;
	using Vec4f = Vec<4, float>;

	template <typename QuatType> using Quat = glm::qua<QuatType>;
	using Quatf = Quat<float>;

	template <unsigned int C, unsigned int R, typename MatType> using Mat = glm::mat<C, R, MatType>;
	using Mat3f = Mat<3, 3, float>;
	using Mat4f = Mat<4, 4, float>;

	namespace Math
	{
		template <typename MathType>
		auto GetDataPtr(MathType& obj)
		{
			return glm::value_ptr(obj);
		}
		template <typename MathType>
		auto GetDataPtr(const MathType& obj)
		{
			return glm::value_ptr(obj);
		}
	}
}

namespace cereal
{
	template <int length, typename VecType, typename Archive>
	void Serialize(Archive& archive, GEE::Vec<length, VecType>& vec)
	{
		const std::string axisNames[] = { "x", "y", "z", "w" };
		for (int i = 0; i < length; i++)
			archive(cereal::make_nvp(axisNames[i], vec[i]));
	}
	template <typename QuatType, typename Archive>
	void Serialize(Archive& archive, GEE::Quat<QuatType>& q)
	{
		archive(cereal::make_nvp("x", q.x), cereal::make_nvp("y", q.y), cereal::make_nvp("z", q.z), cereal::make_nvp("w", q.w));
	}

	template <unsigned int C, unsigned int R, typename MatType, typename Archive>
	void Serialize(Archive& archive, GEE::Mat<C, R, MatType>& mat)
	{
		for (int i = 0; i < static_cast<int>(R); i++)
			archive(cereal::make_nvp("row" + std::to_string(i), mat[i]));
	}
}

template <unsigned int length, typename VecType>
std::ostream& operator<<(std::ostream& os, const GEE::Vec<length, VecType>& vec)
{
	for (int i = 0; i < length - 1; i++) //add commas after all components except the last one
		os << vec[i] << ", ";

	return (os << vec[length - 1]);	//add the last component
}