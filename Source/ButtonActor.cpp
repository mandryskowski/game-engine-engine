#include "ButtonActor.h"
#include "HUDModelComponent.h"
#include "MeshSystem.h"
#include "GameSettings.h"
#include "Event.h"

ButtonActor::ButtonActor(GameScene* scene, const std::string& name):
	Actor(scene, name),
	IsMouseHovering(false),
	IsClicked(false)
{

}

void ButtonActor::OnStart()
{
	Material* buttonMaterial = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Idle");
	if (!buttonMaterial)
	{
		buttonMaterial = new Material("GEE_Button_Idle");
		buttonMaterial->SetColor(glm::vec4(0.25f, 0.5f, 0.75f, 1.0f));
		buttonMaterial->SetRenderShaderName("Forward_NoLight");
		GameHandle->GetRenderEngineHandle()->AddMaterial(buttonMaterial);

		buttonMaterial = new Material("GEE_Button_Hover");
		buttonMaterial->SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
		buttonMaterial->SetRenderShaderName("Forward_NoLight");
		GameHandle->GetRenderEngineHandle()->AddMaterial(buttonMaterial);

		buttonMaterial = new Material("GEE_Button_Click");
		buttonMaterial->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		buttonMaterial->SetRenderShaderName("Forward_NoLight");
		GameHandle->GetRenderEngineHandle()->AddMaterial(buttonMaterial);
	}

	buttonMaterial = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Idle");

	auto buttonModelComp = ModelComponent::Of(ModelComponent(Scene, "ButtonModel"));
	RootComponent->AddComponent(buttonModelComp.get());
	buttonModelComp->AddMeshInst(MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), buttonMaterial));

}

void ButtonActor::HandleEvent(const Event& ev)
{
	if (ev.GetType() == EventType::MOUSE_MOVED)
	{
		glm::vec2 cursorPos = static_cast<glm::vec2>(dynamic_cast<const CursorMoveEvent*>(&ev)->GetNewPosition());
		glm::vec2 windowSize((glm::vec2)GameHandle->GetGameSettings()->WindowSize);
		glm::vec2 windowBottomLeft(0.0f);

		glm::vec2 cursorNDC = (glm::vec2(cursorPos.x, windowSize.y - cursorPos.y) - windowBottomLeft) / windowSize * 2.0f - 1.0f;

		if (CollisionTests::AlignedRectContainsPoint(GetTransform()->GetWorldTransform(), cursorNDC))
			OnHover();
		else
			OnUnhover();
	}
	else if (ev.GetType() == EventType::MOUSE_PRESSED && dynamic_cast<const MouseButtonEvent&>(ev).GetButton() == MouseButton::LEFT  && IsMouseHovering)
		OnClick();
	else if (ev.GetType() == EventType::MOUSE_RELEASED && dynamic_cast<const MouseButtonEvent&>(ev).GetButton() == MouseButton::LEFT)
		OnUnclick();

	DeduceMaterial();
}


void ButtonActor::OnHover()
{
	IsMouseHovering = true;
}

void ButtonActor::OnUnhover()
{
	IsMouseHovering = false;
	IsClicked = false;
}

void ButtonActor::OnClick()
{
	IsClicked = true;
}

void ButtonActor::OnUnclick()
{
	IsClicked = false;
}

void ButtonActor::DeduceMaterial()
{
	if (IsClicked)
		RootComponent->GetComponent<ModelComponent>("ButtonModel")->OverrideInstancesMaterial(GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Click"));
	else if (IsMouseHovering)
		RootComponent->GetComponent<ModelComponent>("ButtonModel")->OverrideInstancesMaterial(GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Hover"));
	else
		RootComponent->GetComponent<ModelComponent>("ButtonModel")->OverrideInstancesMaterial(GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Idle"));

}

bool CollisionTests::AlignedRectContainsPoint(const Transform& rect, const glm::vec2& point)
{
	glm::vec2 rectLeftBottom = rect.PositionRef - rect.ScaleRef / 2.0f;
	return point == glm::min(glm::max(rectLeftBottom, point), rectLeftBottom + glm::vec2(rect.ScaleRef));
}

