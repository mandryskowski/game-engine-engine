#include "UICanvasActor.h"
#include "Transform.h"

glm::mat4 UICanvas::GetView() const
{
	return CanvasView.GetMatrix();
}

glm::mat4 UICanvas::GetProjection() const
{
	glm::vec2 size = static_cast<glm::vec2>(CanvasView.ScaleRef) / 2.0f;
	return glm::ortho(-size.x, size.x, -size.y, size.y);
}

void UICanvas::ScrollView(glm::vec2 offset)
{
	CanvasView.Move(offset);
}



UICanvasActor::UICanvasActor(GameScene* scene, const std::string& name):
	Actor(scene, name)
{
}

glm::mat4 UICanvasActor::GetView() const
{
	return UICanvas::GetView() * glm::inverse(GetTransform()->GetWorldTransformMatrix());
}

NDCViewport UICanvasActor::GetViewport() const
{
	return NDCViewport(GetTransform()->GetWorldTransform().PositionRef - GetTransform()->GetWorldTransform().ScaleRef, GetTransform()->GetWorldTransform().ScaleRef);
}

void UICanvasActor::HandleEvent(const Event& ev)
{
	if (ev.GetType() != EventType::MOUSE_SCROLLED)
		return;

	const MouseScrollEvent& scrolledEv = dynamic_cast<const MouseScrollEvent&>(ev);

	ScrollView(scrolledEv.GetOffset());
}