#include <UI/UIComponent.h>
#include <UI/UICanvas.h>
#include <scene/Actor.h>
#include <scene/Component.h>

#include <UI/UICanvasActor.h>

UIComponent::UIComponent(Actor& actorRef, Component* parentComp):
	SceneRef(actorRef.GetScene())
{
	UICanvasElement* elementCast = dynamic_cast<UICanvasElement*>(parentComp);
	if (!elementCast)
		elementCast = dynamic_cast<UICanvasElement*>(&actorRef);

	if (elementCast)
	{
		SetParentElement(*elementCast);
		SceneRef.GetRenderData()->MarkUIRenderableDepthsDirty();	//SetParentElement calls AttachToCanvas, but because overridden methods can't be called from the constructor, we have to call it here.
	}
}

void UIComponent::AttachToCanvas(UICanvas& canvas)
{
	UICanvasElement::AttachToCanvas(canvas);
	SceneRef.GetRenderData()->MarkUIRenderableDepthsDirty();
}
