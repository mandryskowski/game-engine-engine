#pragma once
#include <UI/UIElement.h>
#include <scene/Component.h>

namespace GEE
{
	class UIComponent : public UICanvasElement
	{
	public:
		UIComponent(Actor& actorRef, Component* parentComp);
		virtual void AttachToCanvas(UICanvas& canvas) override;
	private:
		GameScene& SceneRef;
	};

	class UIComponentDefault : public Component, public UIComponent
	{
	public:
		UIComponentDefault(Actor& actorRef, Component* parentComp, const std::string& name) :
			Component(actorRef, parentComp, name),
			UIComponent(actorRef, parentComp) {}
	};
}