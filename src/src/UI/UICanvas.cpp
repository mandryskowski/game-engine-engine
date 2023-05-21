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
		CanvasDepth(canvas.CanvasDepth),
		CanvasSpaceMinExtent(canvas.CanvasSpaceMinExtent),
		CanvasSpaceMaxExtent(canvas.CanvasSpaceMaxExtent)
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

	Boxf<Vec2f> UICanvas::GetCanvasSpaceBoundingBox() const
	{
		if (TopLevelUIElements.empty())
			return Boxf<Vec2f>::FromMinMaxCorners(CanvasSpaceMinExtent, CanvasSpaceMaxExtent);

		Vec2f runningMin = CanvasSpaceMinExtent, runningMax = CanvasSpaceMaxExtent;

		for (auto it = TopLevelUIElements.begin(); it != TopLevelUIElements.end(); it++)
		{
			Boxf<Vec2f> bBox = (*it)->GetBoundingBoxIncludingChildren();

			runningMin = glm::min(runningMin, bBox.Position - bBox.Size);
			runningMax = glm::max(runningMax, bBox.Position + bBox.Size);
		}

		return Boxf<Vec2f>::FromMinMaxCorners(runningMin, runningMax);
	}

	unsigned int UICanvas::GetCanvasDepth() const
	{
		return CanvasDepth;
	}

	void UICanvas::SetCanvasView(const Transform& canvasView, bool clampView)
	{
		CanvasView = canvasView;
		PushCanvasViewChangeEvent();
	}

	void UICanvas::ClampViewToElements()
	{
		Boxf<Vec2f> bBox = GetCanvasSpaceBoundingBox();
		Vec2f canvasRightUp = bBox.Position + bBox.Size - static_cast<Vec2f>(CanvasView.GetScale());	//Canvas space
		Vec2f canvasLeftDown = bBox.Position - bBox.Size + static_cast<Vec2f>(CanvasView.GetScale());	//Canvas space

		Vec2f adjustedViewPos = glm::clamp(Vec2f(CanvasView.GetPos()), canvasLeftDown, canvasRightUp);

		if (canvasLeftDown.x > canvasRightUp.x)
			adjustedViewPos.x = bBox.Position.x;
		if (canvasLeftDown.y > canvasRightUp.y)
			adjustedViewPos.y = bBox.Position.y;

		CanvasView.SetPosition(adjustedViewPos);
	}

	void UICanvas::AutoClampView(bool horizontal, bool vertical)
	{
		if (horizontal && vertical)
		{
			SetViewScale(glm::max(Vec2f(0.001f), GetCanvasSpaceBoundingBox().Size));
			return;
		}

		float size;
		if (horizontal) size = GetCanvasSpaceBoundingBox().Size.x;
		else if (vertical) size = GetCanvasSpaceBoundingBox().Size.y;
		else return;

		SetViewScale(Vec2f(glm::max(0.001f, size)));
	}

	void UICanvas::AddTopLevelUIElement(UICanvasElement& element)
	{
		TopLevelUIElements.push_back(&element);
		element.AttachToCanvas(*this);
	}

	void UICanvas::EraseTopLevelUIElement(UICanvasElement& element)
	{
		TopLevelUIElements.erase(std::remove_if(TopLevelUIElements.begin(), TopLevelUIElements.end(), [this, &element](UICanvasElement* elementVec) {return elementVec == &element; }), TopLevelUIElements.end());
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
		PushCanvasViewChangeEvent();
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

	Vec4f uiCanvasUtil::ScreenToCanvasSpace(UICanvas& canvas, const Vec4f& vec)
	{
		return glm::inverse(canvas.UICanvas::GetViewMatrix()) * glm::inverse(canvas.GetCanvasT()->GetWorldTransformMatrix()) * vec;
	}

	Vec4f uiCanvasUtil::ScreenToLocalInCanvas(UICanvas& canvas, const Mat4f& localMatrix, const Vec4f& vec)
	{
		return glm::inverse(canvas.ToCanvasSpace(localMatrix)) * ScreenToCanvasSpace(canvas, vec);
	}

}