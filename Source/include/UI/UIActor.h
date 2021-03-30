#pragma once
#include <scene/Actor.h>
#include <UI/UIElement.h>


class UIActor : public Actor, public UICanvasElement
{
public:
	using Actor::Actor;
};