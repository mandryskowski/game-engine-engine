#include <UI/UICanvas.h>
#include <UI/UIActor.h>
#include <scene/UIButtonActor.h>
#include <scene/Component.h>

namespace GEE
{
	UICanvas::UICanvas(const UICanvas& canvas) :
		TopLevelUIElements(canvas.TopLevelUIElements),
		CanvasView(canvas.CanvasView),
		bContainsMouse(false),
		bContainsMouseOutsideTrueCanvas(false),
		CanvasDepth(canvas.CanvasDepth)
	{
	}

	UICanvas::UICanvas(UICanvas&& canvas) :
		UICanvas(static_cast<const UICanvas&>(canvas))
	{
		std::for_each(TopLevelUIElements.begin(), TopLevelUIElements.end(), [this](UICanvasElement* element) { element->AttachToCanvas(*this); });
	}

	Mat4f UICanvas::GetViewMatrix() const
	{
		if (const Vec3f& scale = CanvasView.GetScale(); scale.x == 0.0f || scale.y == 0.0f || scale.z == 0.0f)
		{
			Transform canvasViewCopy = CanvasView;
			canvasViewCopy.SetScale(glm::max(scale, 0.001f));
			return glm::inverse(canvasViewCopy.GetMatrix());
		}


		return glm::inverse(CanvasView.GetMatrix());
	}

	const Transform& UICanvas::GetViewT() const
	{
		return CanvasView;
	}

	Mat4f UICanvas::GetProjection() const
	{
		return glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -2550.0f);
	}

	Boxf<Vec2f> UICanvas::GetBoundingBox() const
	{
		if (TopLevelUIElements.empty())
			return Boxf<Vec2f>(Vec2f(0.0f), Vec2f(0.0f));

		const Boxf<Vec2f> firstElementsBoundingBox = TopLevelUIElements.front()->GetBoundingBoxIncludingChildren();
		Vec2f canvasLeftDownMin(firstElementsBoundingBox.Position - firstElementsBoundingBox.Size);
		Vec2f canvasRightUpMax(firstElementsBoundingBox.Position + firstElementsBoundingBox.Size);

		for (auto it = TopLevelUIElements.begin() + 1; it != TopLevelUIElements.end(); it++)
		{
			Boxf<Vec2f> bBox = (*it)->GetBoundingBoxIncludingChildren();

			canvasLeftDownMin = glm::min(canvasLeftDownMin, bBox.Position - bBox.Size);
			canvasRightUpMax = glm::max(canvasRightUpMax, bBox.Position + bBox.Size);
		}

		return Boxf<Vec2f>::FromMinMaxCorners(canvasLeftDownMin, canvasRightUpMax);
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
		Vec2f canvasRightUp = bBox.Position + bBox.Size - static_cast<Vec2f>(CanvasView.GetScale());	//Canvas space
		Vec2f canvasLeftDown = bBox.Position - bBox.Size + static_cast<Vec2f>(CanvasView.GetScale());	//Canvas space

		Vec2f adjustedViewPos = glm::clamp(Vec2f(CanvasView.GetPos()), canvasLeftDown, canvasRightUp);

		if (canvasLeftDown.x > canvasRightUp.x)
			adjustedViewPos.x = bBox.Position.x;
		if (canvasLeftDown.y > canvasRightUp.y)
			adjustedViewPos.y = bBox.Position.y;

		CanvasView.SetPosition(adjustedViewPos);
	}

	void UICanvas::AutoClampView(bool trueHorizontalFalseVertical)
	{
		float size = (trueHorizontalFalseVertical) ? (GetBoundingBox().Size.x) : (GetBoundingBox().Size.y);
		SetViewScale(Vec2f(glm::max(0.001f, size)));
	}

	void UICanvas::AddTopLevelUIElement(UICanvasElement& element)
	{
		TopLevelUIElements.push_back(&element);
		element.AttachToCanvas(*this);
	}

	void UICanvas::EraseTopLevelUIElement(UICanvasElement& element)
	{
		auto found = std::find_if(TopLevelUIElements.begin(), TopLevelUIElements.end(), [&](UICanvasElement* elementVec) { return elementVec == &element; });
		if (found == TopLevelUIElements.end())
			return;

		TopLevelUIElements.erase(found);
	}


	void UICanvas::ScrollView(Vec2f offset)
	{
		CanvasView.Move(offset);
		ClampViewToElements();
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