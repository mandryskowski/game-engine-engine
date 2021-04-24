#pragma once
//These classes represent 2-dimensional rectangles and 3-dimensional cubes. Useful for defining bounding boxes or viewports, since they are always axis-aligned.

#include <math/Transform.h>
/*
class Transform;
class Vec2f;
class Vec3f;*/

class BoxBase
{
public:
	template <typename VecType> bool Contains(const VecType& point, const VecType& leftDown, const VecType& rightUp) const
	{
		return glm::step(leftDown.GetGlmType(), point.GetGlmType()) * glm::step(point.GetGlmType(), rightUp.GetGlmType()) == VecType(1.0f);
	}
	template <typename VecType> bool Contains(const VecType& leftDown1, const VecType& rightUp1, const VecType& leftDown2, const VecType& rightUp2) const
	{
		return (glm::step(leftDown1.GetGlmType(), rightUp2.GetGlmType()) * glm::step(rightUp2.GetGlmType(), rightUp1.GetGlmType()) == VecType(1.0f)) && (glm::step(leftDown1.GetGlmType(), leftDown2.GetGlmType()) * glm::step(leftDown2.GetGlmType(), rightUp1.GetGlmType()) == VecType(1.0f));
	}
};

template <typename VecType = Vec2f> class Boxf : public BoxBase
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
};