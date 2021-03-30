#include <math/Box.h>
#include <math/Transform.h>
#include <iostream>

Box2f::Box2f(const glm::vec2& pos, const glm::vec2& size):
	Position(pos), Size(size)
{
}

Box2f::Box2f(const Transform& t):
	Position(t.PositionRef),
	Size(t.ScaleRef)
{
}

float Box2f::GetRight() const
{
	return Position.x + Size.x;
}

float Box2f::GetLeft() const
{
	return Position.x - Size.x;
}

float Box2f::GetTop() const
{
	return Position.y + Size.y;
}

float Box2f::GetBottom() const
{
	return Position.y - Size.y;
}

bool Box2f::Contains(const glm::vec2& point) const
{
	glm::vec2 thisRightUp = Position + Size;
	glm::vec2 thisLeftDown = Position - Size;

	//std::cout << glm::step(thisLeftDown, point).x << " " << glm::step(thisLeftDown, point).y << "||||" << glm::step(point, thisRightUp).x << " " << glm::step(point, thisRightUp).y << ";\n";
	if (glm::step(thisLeftDown, point) * glm::step(point, thisRightUp) == glm::vec2(1.0f))
		return true;

	return false;
}

bool Box2f::Contains(const Box2f& rhs) const
{
	glm::vec2 thisRightUp = Position + Size;
	glm::vec2 thisLeftDown = Position - Size;

	glm::vec2 rhsRightUp = rhs.Position + rhs.Size;
	glm::vec2 rhsLeftDown = rhs.Position - rhs.Size;

	if ((glm::step(thisLeftDown, rhsRightUp) * glm::step(rhsRightUp, thisRightUp) == glm::vec2(1.0f)) && (glm::step(thisLeftDown, rhsLeftDown) * glm::step(rhsLeftDown, thisRightUp) == glm::vec2(1.0f)))
		return true;

	return false;
}
