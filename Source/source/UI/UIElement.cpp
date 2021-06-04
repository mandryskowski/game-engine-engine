#include <UI/UIElement.h>
#include <UI/UICanvas.h>
#include <scene/Component.h>
#include <scene/Actor.h>

namespace GEE
{
	UICanvasElement::UICanvasElement() :
		CanvasPtr(nullptr),
		ParentElement(nullptr),
		ElementDepth(0)
	{
	}

	UICanvasElement::UICanvasElement(UICanvasElement&& element) :
		CanvasPtr(nullptr),
		ParentElement(nullptr),
		ElementDepth(element.ElementDepth)
	{
		if (element.ParentElement)
			SetParentElement(*element.ParentElement);
		else if (element.CanvasPtr)
			element.CanvasPtr->AddUIElement(*this);
	}

	Boxf<Vec2f> UICanvasElement::GetBoundingBox(bool world)
	{
		return Boxf<Vec2f>(Vec2f(0.0f), Vec2f(0.0f));
	}

	UICanvas* UICanvasElement::GetCanvasPtr()
	{
		return CanvasPtr;
	}

	unsigned int UICanvasElement::GetElementDepth() const
	{
		return ElementDepth;
	}

	void UICanvasElement::SetParentElement(UICanvasElement& element)
	{
		element.AddChildElement(*this);
	}

	void UICanvasElement::AddChildElement(UICanvasElement& element)
	{
		if (GetCanvasPtr())
		{
			GetCanvasPtr()->AddUIElement(element);
			element.ParentElement = this;
			ChildElements.push_back(&element);
		}
	}

	void UICanvasElement::EraseChildElement(UICanvasElement& element)
	{
		element.DetachFromCanvas();
		ChildElements.erase(std::remove_if(ChildElements.begin(), ChildElements.end(), [&element](UICanvasElement* elementVec) {return elementVec == &element; }), ChildElements.end());
	}

	void UICanvasElement::AttachToCanvas(UICanvas& canvas)
	{
		CanvasPtr = &canvas;
		ElementDepth = CanvasPtr->GetCanvasDepth();

	}

	void UICanvasElement::DetachFromCanvas()
	{
		for (auto& it : ChildElements)
			it->DetachFromCanvas();

		ChildElements.clear();
		CanvasPtr = nullptr;
		ParentElement = nullptr;
	}

	UICanvasElement::~UICanvasElement()
	{
		if (CanvasPtr)
			CanvasPtr->EraseUIElement(*this);
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
}