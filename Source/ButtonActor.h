#pragma once
#include "Actor.h"

class ButtonActor : public Actor
{
public:
	ButtonActor(GameScene*, const std::string& name);

	virtual void OnStart() override;

	virtual void HandleEvent(const Event& ev) override;

	virtual void OnHover();
	virtual void OnUnhover();

	virtual void OnClick();
	virtual void OnUnclick();

protected:
	virtual void DeduceMaterial();


	bool IsMouseHovering;
	bool IsClicked;
};

struct CollisionTests
{
public:
	static bool AlignedRectContainsPoint(const Transform& rect, const glm::vec2& point); //treats rect as a rectangle at position rect.PositionRef and size rect.ScaleRef
};