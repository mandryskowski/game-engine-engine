#pragma once
#include <UI/UIElement.h>
#include <scene/Component.h>

namespace GEE
{
	class UIComponent : public UICanvasElement
	{
	public:
		UIComponent(Actor& actorRef, Component* parentComp);
		UIComponent(const UIComponent&) = default;
		UIComponent(UIComponent&&) = default;
		virtual void AttachToCanvas(UICanvas& canvas) override;
		virtual ~UIComponent() = default;
	private:
		GameScene& SceneRef;
	};

	class UIComponentDefault : public Component, public UIComponent
	{
	public:
		UIComponentDefault(Actor& actorRef, Component* parentComp, const std::string& name) :
			Component(actorRef, parentComp, name),
			UIComponent(actorRef, parentComp) {}
		UIComponentDefault(const UIComponentDefault&) = default;
		UIComponentDefault(UIComponentDefault&&) = default;
		virtual ~UIComponentDefault() = default;
	};
}