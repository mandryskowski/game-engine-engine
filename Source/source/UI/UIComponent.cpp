#include <UI/UIComponent.h>
#include <UI/UICanvas.h>
#include <scene/Actor.h>
#include <scene/Component.h>

#include <UI/UICanvasActor.h>

UIComponent::UIComponent(Actor& actorRef, Component* parentComp)
{
	UICanvasElement* elementCast = dynamic_cast<UICanvasElement*>(parentComp);
	if (!elementCast)
		elementCast = dynamic_cast<UICanvasElement*>(&actorRef);

	if (elementCast)
		SetParentElement(*elementCast);
}