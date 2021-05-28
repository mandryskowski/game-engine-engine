#pragma once
#include <scene/Actor.h>
#include <scene/Component.h>
#include <UI/UICanvas.h>
#include <UI/UIElement.h>


class UIActor : public UICanvasElement
{
public:
	UIActor(Actor* parentActor)
	{
		if (!parentActor)
			return;

		if (UICanvas* canvasCast = dynamic_cast<UICanvas*>(parentActor))
			canvasCast->AddUIElement(*this);
		else if (UICanvasElement* elementCast = dynamic_cast<UICanvasElement*>(parentActor))
			SetParentElement(*elementCast);
	}
};

class UIActorDefault : public Actor, public UIActor
{
public:
	UIActorDefault(GameScene& scene, Actor* parentActor, const std::string& name, const Transform& t = Transform()) :
		Actor(scene, parentActor, name, t),
		UIActor(parentActor) {}
};