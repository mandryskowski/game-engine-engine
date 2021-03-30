#pragma once
#include <glm/glm.hpp>
//These classes represent 2-dimensional rectangles and 3-dimensional cubes. Useful for defining bounding boxes or viewports, since they are always axis-aligned.

class Transform;

class Box2f
{
public:
	glm::vec2 Position;
	glm::vec2 Size;

	Box2f(const glm::vec2& pos, const glm::vec2& size);
	Box2f(const Transform&);

	float GetRight() const;
	float GetLeft() const;
	float GetTop() const;
	float GetBottom() const;

	bool Contains(const glm::vec2&) const;
	bool Contains(const Box2f&) const;
};