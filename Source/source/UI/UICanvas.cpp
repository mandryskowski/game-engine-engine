#include <UI/UICanvas.h>
#include <UI/UIActor.h>
#include <scene/UIButtonActor.h>
#include <scene/Component.h>
#include <cassert>
#include <scene/UIButtonActor.h>
#define assertm(exp, msg) assert(((void)msg, exp))

namespace GEE
{
	UICanvas::UICanvas(const UICanvas& canvas) :
		UIElements(canvas.UIElements),
		CanvasView(canvas.CanvasView),
		bContainsMouse(false),
		bContainsMouseOutsideTrueCanvas(false),
		CanvasDepth(canvas.CanvasDepth)
	{
	}

	UICanvas::UICanvas(UICanvas&& canvas) :
		UICanvas(static_cast<const UICanvas&>(canvas))
	{
		std::for_each(UIElements.begin(), UIElements.end(), [this](std::reference_wrapper<UICanvasElement>& element) { element.get().AttachToCanvas(*this); });
	}

	Mat4f UICanvas::GetViewMatrix() const
	{
		assertm(CanvasView.Scale().x != 0.0f && CanvasView.Scale().y != 0.0f && CanvasView.Scale().z != 0.0f, "Scale component equal to 0.");

		return glm::inverse(CanvasView.GetMatrix());
	}

	const Transform& UICanvas::GetViewT() const
	{
		return CanvasView;
	}

	Mat4f UICanvas::GetProjection() const
	{
		Vec2f size = static_cast<Vec2f>(CanvasView.Scale());
		return glm::ortho(-size.x, size.x, -size.y, size.y, 1.0f, -2550.0f);
	}

	Boxf<Vec2f> UICanvas::GetBoundingBox() const
	{
		Vec2f canvasRightUpMax(0.0f);
		Vec2f canvasLeftDownMin(0.0f);

		for (int i = 0; i < UIElements.size(); i++)
		{
			Boxf<Vec2f> bBox = UIElements[i].get().GetBoundingBox();
			if (bBox.Size == Vec2f(0.0f))
				continue;

			Vec2f elementRightUpMax(bBox.Position + bBox.Size);		//pos.x + size.x; pos.y + size.y;
			Vec2f elementLeftDownMin(bBox.Position - bBox.Size);	//pos.y - size.x; pos.y - size.y;

	//		std::cout << elementLeftDownMin.y << "??\n";

			canvasRightUpMax = glm::max(canvasRightUpMax, elementRightUpMax);
			canvasLeftDownMin = glm::min(canvasLeftDownMin, elementLeftDownMin);
		}

		Vec2f center = (canvasLeftDownMin + canvasRightUpMax) / 2.0f;

		//	std::cout << UIElements.size() << "\n";
		//	std::cout << canvasLeftDownMin.x << ", " << canvasLeftDownMin.y << " --> " << canvasRightUpMax.x << ", " << canvasRightUpMax.y << "\n";

		return Boxf<Vec2f>(center, canvasRightUpMax - center);
	}

	unsigned int UICanvas::GetCanvasDepth() const
	{
		return CanvasDepth;
	}

	void UICanvas::SetCanvasView(const Transform& canvasView)
	{
		CanvasView = canvasView;
	}

	void UICanvas::ClampViewToElements()
	{
		Boxf<Vec2f> bBox = GetBoundingBox();
		Vec2f canvasRightUp = bBox.Position + bBox.Size - glm::pow(static_cast<Vec2f>(CanvasView.Scale()), Vec2f(2.0f));	//Canvas space
		Vec2f canvasLeftDown = bBox.Position - bBox.Size + glm::pow(static_cast<Vec2f>(CanvasView.Scale()), Vec2f(2.0f));	//Canvas space

		Vec2f adjustedViewPos = glm::clamp(Vec2f(CanvasView.Pos()), canvasLeftDown, canvasRightUp);

		if (canvasLeftDown.x > canvasRightUp.x)
			adjustedViewPos.x = bBox.Position.x;
		if (canvasLeftDown.y > canvasRightUp.y)
			adjustedViewPos.y = bBox.Position.y;

		CanvasView.SetPosition(adjustedViewPos);
	}

	void UICanvas::AutoClampView(bool trueHorizontalFalseVertical)
	{
		float size = (trueHorizontalFalseVertical) ? (GetBoundingBox().Size.x) : (GetBoundingBox().Size.y);
		SetViewScale(Vec2f(glm::sqrt(size)));
	}

	void UICanvas::AddUIElement(UICanvasElement& element)
	{
		UIElements.push_back(element);
		element.AttachToCanvas(*this);
	}

	void UICanvas::EraseUIElement(UICanvasElement& element)
	{
		auto found = std::find_if(UIElements.begin(), UIElements.end(), [&](std::reference_wrapper<UICanvasElement>& elementVec) { return &elementVec.get() == &element; });
		if (found == UIElements.end())
			return;

		element.DetachFromCanvas();

		UIElements.erase(found);
	}


	void UICanvas::ScrollView(Vec2f offset)
	{
		CanvasView.Move(offset);
		ClampViewToElements();

		for (auto& it : UIElements)
		{
			//it.get().OnScroll();
		}
	}

	void UICanvas::SetViewScale(Vec2f scale)
	{
		CanvasView.SetScale(scale);
		ClampViewToElements();
	}

	bool UICanvas::ContainsMouse() const
	{
		return bContainsMouse;
	}

	bool UICanvas::ShouldAcceptBlockedEvents(UICanvasElement& element) const
	{
		if (element.GetCanvasPtr() == this && !bContainsMouseOutsideTrueCanvas)
			return true;
		else if (element.GetCanvasPtr())
			return false;

		std::vector<UIButtonActor*> externalButtons;
		GetExternalButtons(externalButtons);

		for (auto it : externalButtons)
			if (it == &element)
				return true;

		return false;
	}

	bool UICanvas::ContainsMouseCheck(const Vec2f& mouseNDC) const
	{
		bContainsMouse = GetViewport().Contains(mouseNDC);
		bContainsMouseOutsideTrueCanvas = false;
		if (!bContainsMouse)
		{
			std::vector<UIButtonActor*> externalButtons;
			GetExternalButtons(externalButtons);
			for (auto it : externalButtons)
				if (it && !it->IsBeingKilled() && it->ContainsMouse(mouseNDC))
					return bContainsMouse = bContainsMouseOutsideTrueCanvas = true;
		}

		return bContainsMouse;
	}

}