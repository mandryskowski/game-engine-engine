#pragma once
#include <scene/Actor.h>
#include <scene/Component.h>
#include <UI/UICanvas.h>
#include <UI/UIElement.h>

namespace GEE
{
	class UIActor : public UICanvasElement
	{
	public:
		UIActor(Actor* parentActor)
		{
			if (!parentActor)
				return;

			if (UICanvas* canvasCast = dynamic_cast<UICanvas*>(parentActor))
				canvasCast->AddTopLevelUIElement(*this);
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


	/*void ShowHideElements(Actor& actor, bool hide)
	{
		for (auto it : actor.GetChildren())
		{
			ShowHideElements(*it, hide);
		}

		std::vector<RenderableComponent*> renderables;
		actor.GetRoot()->GetAllComponents<RenderableComponent>(&renderables);
		bExpanded = !bExpanded;
		for (auto& it : renderables)
			it->SetHide(!bExpanded);

	}*/
}