#include <UI/UIElement.h>
#include <UI/UICanvas.h>
#include <scene/Component.h>
#include <scene/Actor.h>

UICanvasElement::UICanvasElement():
	CanvasPtr(nullptr)
{
}

UICanvasElement::UICanvasElement(UICanvasElement&& element):
	CanvasPtr(nullptr)
{
	if (element.CanvasPtr)
	{
		element.CanvasPtr->AddUIElement(*this);
		element.CanvasPtr->EraseUIElement(element);
	}
}

Boxf<Vec2f> UICanvasElement::GetBoundingBox(bool world)
{
	return Boxf<Vec2f>(glm::vec2(0.0f), glm::vec2(0.0f));
}

UICanvas* UICanvasElement::GetCanvasPtr()
{
	return CanvasPtr;
}

UICanvasElement::~UICanvasElement()
{
	if (CanvasPtr)
		CanvasPtr->RemoveUIElement(this);
}

void UICanvasElement::AttachToCanvas(UICanvas& canvas)
{
	CanvasPtr = &canvas;
}

/*Transform UICanvasComponent::GetCanvasSpaceTransform()
{
	assert(dynamic_cast<Component*>(this));
	return CanvasPtr->ToCanvasSpace(dynamic_cast<Component*>(this)->GetTransform().GetWorldTransform());
}

Transform UICanvasActor::GetCanvasSpaceTransform()
{
	assert(dynamic_cast<Actor*>(this));
	return CanvasPtr->ToCanvasSpace(dynamic_cast<Actor*>(this)->GetTransform()->GetWorldTransform());
}*/