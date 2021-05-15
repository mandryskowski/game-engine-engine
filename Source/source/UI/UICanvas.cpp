#include <UI/UICanvas.h>
#include <UI/UIActor.h>
#include <scene/UIButtonActor.h>
#include <scene/Component.h>
#include <cassert>
#define assertm(exp, msg) assert(((void)msg, exp))

UICanvas::UICanvas(const UICanvas& canvas):
	UIElements(canvas.UIElements),
	CanvasView(canvas.CanvasView)
{
}

UICanvas::UICanvas(UICanvas&& canvas):
	UICanvas(static_cast<const UICanvas&>(canvas))
{
	std::for_each(UIElements.begin(), UIElements.end(), [this](std::reference_wrapper<UICanvasElement>& element) { element.get().AttachToCanvas(*this); });
}

glm::mat4 UICanvas::GetView() const
{
	assertm(CanvasView.ScaleRef.x != 0.0f && CanvasView.ScaleRef.y != 0.0f && CanvasView.ScaleRef.z != 0.0f, "Scale component equal to 0.");
	
	return glm::inverse(CanvasView.GetMatrix());
}

const Transform& UICanvas::GetViewT() const
{
	return CanvasView;
}

glm::mat4 UICanvas::GetProjection() const
{
	glm::vec2 size = static_cast<glm::vec2>(CanvasView.ScaleRef);
	return glm::ortho(-size.x, size.x, -size.y, size.y, 1.0f, -255.0f);
}

Boxf<Vec2f> UICanvas::GetBoundingBox() const
{
	glm::vec2 canvasRightUpMax(0.0f);
	glm::vec2 canvasLeftDownMin(0.0f);

	for (int i = 0; i < UIElements.size(); i++)
	{
		Boxf<Vec2f> bBox = UIElements[i].get().GetBoundingBox();
		if (bBox.Size == glm::vec2(0.0f))
			continue;

		glm::vec2 elementRightUpMax(bBox.Position + bBox.Size);		//pos.x + size.x; pos.y + size.y;
		glm::vec2 elementLeftDownMin(bBox.Position - bBox.Size);	//pos.y - size.x; pos.y - size.y;

//		std::cout << elementLeftDownMin.y << "??\n";

		canvasRightUpMax = glm::max(canvasRightUpMax, elementRightUpMax);
		canvasLeftDownMin = glm::min(canvasLeftDownMin, elementLeftDownMin);
	}

	glm::vec2 center = (canvasLeftDownMin + canvasRightUpMax) / 2.0f;
	
//	std::cout << UIElements.size() << "\n";
//	std::cout << canvasLeftDownMin.x << ", " << canvasLeftDownMin.y << " --> " << canvasRightUpMax.x << ", " << canvasRightUpMax.y << "\n";

	return Boxf<Vec2f>(center, canvasRightUpMax - center);
}

void UICanvas::ClampViewToElements()
{
	Boxf<Vec2f> bBox = GetBoundingBox();
	glm::vec2 canvasRightUp = bBox.Position + bBox.Size - glm::pow(static_cast<glm::vec2>(CanvasView.ScaleRef), glm::vec2(2.0f));	//Canvas space
	glm::vec2 canvasLeftDown = bBox.Position - bBox.Size + glm::pow(static_cast<glm::vec2>(CanvasView.ScaleRef), glm::vec2(2.0f));	//Canvas space

	glm::vec2 adjustedViewPos = glm::clamp(glm::vec2(CanvasView.PositionRef), canvasLeftDown, canvasRightUp);

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
	if (element.ParentElement)
		element.ParentElement->EraseChildElement(element);
	else
		element.DetachFromCanvas();
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

void UICanvas::SetViewScale(glm::vec2 scale)
{
	CanvasView.SetScale(scale);
	ClampViewToElements();
}

bool UICanvas::ContainsMouse() const
{
	return bContainsMouse;
}

bool UICanvas::ContainsMouseCheck(const Vec2f& mouseNDC) const
{
	return bContainsMouse = GetViewport().Contains(mouseNDC);
}
