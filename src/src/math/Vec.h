#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <utility/CerealNames.h>
#include <cereal/cereal.hpp>

namespace GEE
{
	using MathContainerUnit = glm::length_t;

	template <MathContainerUnit length, typename T, glm::qualifier Q = glm::packed_highp> using Vec = glm::vec<length, T, Q>;
	using Vec2i = Vec<2, int>;
	using Vec3i = Vec<3, int>;
	using Vec4i = Vec<4, int>;

	using Vec2u = Vec<2, unsigned int>;
	using Vec3u = Vec<3, unsigned int>;
	using Vec4u = Vec<4, unsigned int>;

	using Vec2f = Vec<2, float>;
	using Vec3f = Vec<3, float>;
	using Vec4f = Vec<4, float>;

	using Vec2d = Vec<2, double>;
	using Vec3d = Vec<3, double>;
	using Vec4d = Vec<4, double>;

	using Vec2b = Vec<2, bool>;
	using Vec3b = Vec<3, bool>;
	using Vec4b = Vec<4, bool>;

	template <typename T, glm::qualifier Q = glm::packed_highp> using Quat = glm::qua<T, Q>;
	using Quatf = Quat<float>;

	template <MathContainerUnit C, MathContainerUnit R, typename T, glm::qualifier Q = glm::packed_highp> using Mat = glm::mat<C, R, T, Q>;
	using Mat2f = Mat<2, 2, glm::f32>;
	using Mat3f = Mat<3, 3, glm::f32>;
	using Mat4f = Mat<4, 4, glm::f32>;

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

		template <typename T>
		int Round(T val)
		{
			return static_cast<int>(glm::round(val));
		}
		
		
		Vec2f GetRatioOfComponents(const Vec2f& vec);
		Mat3f ModelToNormal(const Mat4f& model);
	}
}

namespace cereal
{
	template <int length, typename T, glm::qualifier Q, typename Archive>
	void Serialize(Archive& archive, GEE::Vec<length, T, Q>& vec)
	{
		const std::string axisNames[] = { "x", "y", "z", "w" };
		for (int i = 0; i < length; i++)
			archive(cereal::make_nvp(axisNames[i], vec[i]));
	}
	template <typename T, glm::qualifier Q, typename Archive>
	void Serialize(Archive& archive, GEE::Quat<T, Q>& q)
	{
		archive(cereal::make_nvp("x", q.x), cereal::make_nvp("y", q.y), cereal::make_nvp("z", q.z), cereal::make_nvp("w", q.w));
	}

	template <GEE::MathContainerUnit C, GEE::MathContainerUnit R, typename T, glm::qualifier Q, typename Archive>
	void Serialize(Archive& archive, GEE::Mat<C, R, T, Q>& mat)
	{
		for (int i = 0; i < static_cast<int>(R); i++)
			archive(cereal::make_nvp("row" + std::to_string(i), mat[i]));
	}
}

template <GEE::MathContainerUnit length, glm::qualifier Q, typename T>
std::ostream& operator<<(std::ostream& os, const GEE::Vec<length, T, Q>& vec)
{
	for (int i = 0; i < length - 1; i++) //add commas after all components except the last one
		os << vec[i] << ", ";

	return (os << vec[length - 1]);	//add the last component
}

template <typename T, glm::qualifier Q>
std::ostream& operator<<(std::ostream& os, const GEE::Quat<T, Q>& q)
{
	for (int i = 0; i < 3; i++) //add commas after all components except the last one
		os << q[i] << ", ";


	return (os << q[3]);	//add the last component
}

template <GEE::MathContainerUnit C, GEE::MathContainerUnit R, typename T, glm::qualifier Q>
std::ostream& operator<<(std::ostream& os, const GEE::Mat<C, R, T, Q>& mat)
{
	auto prevPrecision = os.precision();
	os.precision(3);

	for (int y = 0; y < R; y++)
	{
		for (int x = 0; x < C; x++)
		{
			os << std::fixed << mat[x][y];
			if (!(x == C - 1 && y == R - 1))	// do not add a comma after the last value.
				os << ", ";
		}

		if (y != R - 1)	// do not add EOL after the last value.
			os << '\n';
		
	}

	os.precision(prevPrecision);
	return os;	//add the last component
}