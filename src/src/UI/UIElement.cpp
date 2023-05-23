#include <UI/UIElement.h>
#include <UI/UICanvas.h>
#include <scene/Component.h>
#include <scene/Actor.h>
#include <UI/UICanvasActor.h>


#include <scene/TextComponent.h>
namespace GEE
{
	UICanvasElement::UICanvasElement() :
		CanvasPtr(nullptr),
		ElementDepth(0),
		ParentElement(nullptr)
	{
	}

	UICanvasElement::UICanvasElement(UICanvasElement&& element) :
		CanvasPtr(nullptr),
		ElementDepth(element.ElementDepth),
		ParentElement(nullptr)
	{
		if (element.ParentElement)
			SetParentElement(*element.ParentElement);
		else if (element.CanvasPtr)
			element.CanvasPtr->AddTopLevelUIElement(*this);
	}

	Boxf<Vec2f> UICanvasElement::GetBoundingBox(bool world) const
	{
		return Boxf<Vec2f>(Vec2f(0.0f), Vec2f(0.0f));
	}

	Boxf<Vec2f> UICanvasElement::GetBoundingBoxIncludingChildren(bool world) const
	{
		const Boxf<Vec2f> thisBoundingBox = GetBoundingBox(world);
		Vec2f canvasLeftDownMin(thisBoundingBox.Position - thisBoundingBox.Size);
		Vec2f canvasRightUpMax(thisBoundingBox.Position + thisBoundingBox.Size);

		for (auto it : ChildUIElements)
		{
			Boxf<Vec2f> bBox = it->GetBoundingBoxIncludingChildren(world);

			canvasLeftDownMin = glm::min(canvasLeftDownMin, bBox.Position - bBox.Size);
			canvasRightUpMax = glm::max(canvasRightUpMax, bBox.Position + bBox.Size);
		}

		return Boxf<Vec2f>::FromMinMaxCorners(canvasLeftDownMin, canvasRightUpMax);
	}

	UICanvas* UICanvasElement::GetCanvasPtr()
	{
		return CanvasPtr;
	}

	const UICanvas* UICanvasElement::GetCanvasPtr() const
	{
		return CanvasPtr;
	}

	unsigned int UICanvasElement::GetElementDepth() const
	{
		return ElementDepth;
	}

	void UICanvasElement::SetParentElement(UICanvasElement& element)
	{
		GEE_CORE_ASSERT(&element);
		element.AddChildElement(*this);
	}

	void UICanvasElement::AddChildElement(UICanvasElement& element)
	{
		GEE_CORE_ASSERT(&element);
		element.ParentElement = this;
		ChildUIElements.push_back(&element);
		if (GetCanvasPtr())
			element.AttachToCanvas(*GetCanvasPtr());

	}

	void UICanvasElement::EraseChildElement(UICanvasElement& element)
	{
		element.DetachFromCanvas();
	}

	void UICanvasElement::AttachToCanvas(UICanvas& canvas)
	{
		GEE_CORE_ASSERT(&canvas);
		CanvasPtr = &canvas;
		ElementDepth = CanvasPtr->GetCanvasDepth();
	}

	void UICanvasElement::DetachFromCanvas()
	{
		if (ParentElement)
		{
			auto& vec = ParentElement->ChildUIElements;
			vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
			ParentElement = nullptr;
		}
		else if (CanvasPtr)
			CanvasPtr->EraseTopLevelUIElement(*this);


		while (!ChildUIElements.empty())
			EraseChildElement(*ChildUIElements.front());

		CanvasPtr = nullptr;
	}

	UICanvasElement::~UICanvasElement()
	{
		DetachFromCanvas();
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