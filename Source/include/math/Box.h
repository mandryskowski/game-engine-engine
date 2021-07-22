#pragma once
//These classes represent 2-dimensional rectangles and 3-dimensional cubes. Useful for defining bounding boxes or viewports, since they are always axis-aligned.

#include <math/Transform.h>
/*
class Transform;
class Vec2f;
class Vec3f;*/

namespace GEE
{
	template <typename VecType = Vec2f> class Boxf
	{
	public:
		VecType Position;
		VecType Size;

		Boxf(const VecType& pos, const VecType& size);
		Boxf(const Transform&);

		float GetRight() const;
		float GetLeft() const;
		float GetTop() const;
		float GetBottom() const;

		bool Contains(const VecType&) const;
		bool Contains(const Boxf<VecType>&) const;

		

		static Boxf<VecType> FromMinMaxCorners(const VecType& minCorner, const VecType& maxCorner);
	};

	namespace GeomTests
	{
		template <typename VecType> bool Contains(const VecType& point, const VecType& leftDown, const VecType& rightUp)
		{
			return glm::step(leftDown, point) * glm::step(point, rightUp) == VecType(1.0f);
		}
		template <typename VecType> bool Contains(const VecType& leftDown1, const VecType& rightUp1, const VecType& leftDown2, const VecType& rightUp2)
		{
			return (glm::step(leftDown1, rightUp2) * glm::step(rightUp2, rightUp1) == VecType(1.0f)) && (glm::step(leftDown1, leftDown2) * glm::step(leftDown2, rightUp1) == VecType(1.0f));
		}
	}
}