#pragma once
#include "Actor.h"

class ButtonActor : public Actor
{
public:
	ButtonActor(GameScene*, const std::string& name);

	virtual void HandleInputs(GLFWwindow* window) override;

	virtual void OnStart() override;
	virtual void OnHover();
	virtual void OnUnhover();

protected:
	bool IsMouseHovering;
};

struct CollisionTests
{
public:
	static bool AlignedRectContainsPoint(const Transform& rect, const glm::vec2& point); //treats rect as a rectangle at position rect.PositionRef and size rect.ScaleRef
};