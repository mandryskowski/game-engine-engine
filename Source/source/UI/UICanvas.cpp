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
	return glm::ortho(-size.x, size.x, -size.y, size.y);
}

Box2f UICanvas::GetBoundingBox() const
{
	glm::vec2 canvasRightUpMax(0.0f);
	glm::vec2 canvasLeftDownMin(0.0f);

	for (int i = 0; i < UIElements.size(); i++)
	{
		Box2f bBox = UIElements[i].get().GetBoundingBox();
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

	return Box2f(center, canvasRightUpMax - center);
}

void UICanvas::ClampViewToElements()
{
	Box2f bBox = GetBoundingBox();
	glm::vec2 canvasRightUp = bBox.Position + bBox.Size - glm::pow(static_cast<glm::vec2>(CanvasView.ScaleRef), glm::vec2(2.0f));	//Canvas space
	glm::vec2 canvasLeftDown = bBox.Position - bBox.Size + glm::pow(static_cast<glm::vec2>(CanvasView.ScaleRef), glm::vec2(2.0f));	//Canvas space

	glm::vec2 adjustedViewPos = glm::clamp(glm::vec2(CanvasView.PositionRef), canvasLeftDown, canvasRightUp);

	if (canvasLeftDown.x > canvasRightUp.x)
		adjustedViewPos.x = bBox.Position.x;
	if (canvasLeftDown.y > canvasRightUp.y)
		adjustedViewPos.y = bBox.Position.y;

	CanvasView.SetPosition(adjustedViewPos);
}

void UICanvas::AddUIElement(UICanvasElement& element)
{
	UIElements.push_back(element);
	element.AttachToCanvas(*this);		
}

void UICanvas::EraseUIElement(UICanvasElement& element)
{
	UIElements.erase(std::remove_if(UIElements.begin(), UIElements.end(), [&element](std::reference_wrapper<UICanvasElement>& elementVec) {return &elementVec.get() == &element; }), UIElements.end());
}

void UICanvas::AddUIElement(Actor& actor)
{
	std::vector<UICanvasElement*> elements;
	actor.GetRoot()->GetAllComponents<UICanvasElement>(&elements);

	for (auto& it : elements)
		(this->*static_cast<void(UICanvas::*)(UICanvasElement&)>(&UICanvas::AddUIElement))(*it);	//Watch out: AddUIElement both attaches the element to canvas and makes it expand the canvas. TODO: Change it so an element can only be renderable in canvas, only expand the canvas or both.

	std::vector<Actor*> children = actor.GetChildren();
	for (auto& it : children)
	{
		if (UIActor* cast = dynamic_cast<UIActor*>(it))
			(this->*static_cast<void(UICanvas::*)(UIActor&)>(&UICanvas::AddUIElement))(*cast);
		else
			(this->*static_cast<void(UICanvas::*)(Actor&)>(&UICanvas::AddUIElement))(*it);
		
	}
}

void UICanvas::AddUIElement(UIActor& actor)
{
	(this->*static_cast<void(UICanvas::*)(UICanvasElement&)>(&UICanvas::AddUIElement))(actor);
	(this->*static_cast<void(UICanvas::*)(Actor&)>(&UICanvas::AddUIElement))(actor);
}

void UICanvas::RemoveUIElement(UICanvasElement* element)
{
	UIElements.erase(std::remove_if(UIElements.begin(), UIElements.end(), [element](std::reference_wrapper<UICanvasElement>& vecElement) { return element == &vecElement.get(); }), UIElements.end());
}

void UICanvas::ScrollView(glm::vec2 offset)
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
