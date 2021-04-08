#include <scene/UIButtonActor.h>
#include <scene/ModelComponent.h>
#include <scene/HierarchyTemplate.h>
#include <game/GameSettings.h>
#include <input/Event.h>
#include <UI/UICanvas.h>
#include <scene/TextComponent.h>
#include <input/InputDevicesStateRetriever.h>
#include <math/Box.h>

UIButtonActor::UIButtonActor(GameScene& scene, const std::string& name, std::function<void()> onClickFunc, std::function<void()> whileBeingClickedFunc):
	UIActor(scene, name),
	OnClickFunc(onClickFunc),
	WhileBeingClickedFunc(whileBeingClickedFunc),
	MatIdle(nullptr),
	MatClick(nullptr),
	MatHover(nullptr),
	PrevDeducedMaterial(nullptr),
	State(EditorIconState::IDLE)
{
	ButtonModel = &RootComponent->CreateComponent(ModelComponent(Scene, Name + "'s_Button_Model"));

	Material *matIdle, *matHover, *matClick;
	if ((matIdle = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Idle")) == nullptr)
	{
		matIdle = new Material("GEE_Button_Idle");
		matIdle->SetColor(glm::vec4(0.520841f, 0.680359f, 0.773018f, 1.0f));
		matIdle->SetRenderShaderName("Forward_NoLight");
		GameHandle->GetRenderEngineHandle()->AddMaterial(matIdle);
	}
	if ((matHover = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Hover")) == nullptr)
	{
		matHover = new Material("GEE_Button_Hover");
		matHover->SetColor(glm::vec4(0.831684f, 0.2f, 0.2f, 1.0f));
		matHover->SetRenderShaderName("Forward_NoLight");
		GameHandle->GetRenderEngineHandle()->AddMaterial(matHover);
	}
	if ((matClick = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Click")) == nullptr)
	{
		matClick = new Material("GEE_Button_Click");
		matClick->SetColor(glm::vec4(0.773018f, 0.773018f, 0.773018f, 1.0f));
		matClick->SetRenderShaderName("Forward_NoLight");
		GameHandle->GetRenderEngineHandle()->AddMaterial(matClick);
	}

	MatIdle = std::make_shared<MaterialInstance>(MaterialInstance(*matIdle));
	MatHover = std::make_shared<MaterialInstance>(MaterialInstance(*matHover));
	MatClick = std::make_shared<MaterialInstance>(MaterialInstance(*matClick));


	ButtonModel->AddMeshInst(MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), MatIdle));
	PrevDeducedMaterial = MatIdle.get();
}

UIButtonActor::UIButtonActor(GameScene& scene, const std::string& name, const std::string& buttonTextContent, std::function<void()> onClickFunc, std::function<void()> whileBeingClickedFunc):
	UIButtonActor(scene, name, onClickFunc, whileBeingClickedFunc)
{
	CreateComponent(TextConstantSizeComponent(Scene, "ComponentsNameActorTextComp", Transform(glm::vec2(0.0f), glm::vec2(1.0f)), buttonTextContent, "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER)));
}

ModelComponent* UIButtonActor::GetButtonModel()
{
	return ButtonModel;
}

Box2f UIButtonActor::GetBoundingBox(bool world)
{
	if (!GetTransform())
		return Box2f(glm::vec2(0.0f), glm::vec2(0.0f));

	if (!world)
		return Box2f(GetTransform()->PositionRef, GetTransform()->ScaleRef);

	if (!CanvasPtr)
		return Box2f(GetTransform()->GetWorldTransform().PositionRef, GetTransform()->GetWorldTransform().ScaleRef);

	Transform canvasSpaceTransform = CanvasPtr->ToCanvasSpace(GetTransform()->GetWorldTransform());	//world == true, CanvasPtr isn't nullptr and this actor has got a Transform.
	return Box2f(canvasSpaceTransform.PositionRef, canvasSpaceTransform.ScaleRef);
}

void UIButtonActor::SetMatIdle(MaterialInstance&& mat)
{
	MatIdle = std::make_shared<MaterialInstance>(std::move(mat));
	DeduceMaterial();
}

void UIButtonActor::SetMatHover(MaterialInstance&& mat)
{
	MatHover = std::make_shared<MaterialInstance>(std::move(mat));
	DeduceMaterial();
}

void UIButtonActor::SetMatClick(MaterialInstance&& mat)
{
	MatClick = std::make_shared<MaterialInstance>(std::move(mat));
	DeduceMaterial();

}

void UIButtonActor::SetOnClickFunc(std::function<void()> onClickFunc)
{
	OnClickFunc = onClickFunc;
}

void UIButtonActor::SetWhileBeingClickedFunc(std::function<void()> whileBeingClickedFunc)
{
	WhileBeingClickedFunc = whileBeingClickedFunc;
}

void UIButtonActor::DeleteButtonModel()
{
	if (ButtonModel)
	{
		RootComponent->DetachChild(*ButtonModel);
		ButtonModel = nullptr;
	}
}

void UIButtonActor::HandleEvent(const Event& ev)
{
	if (ev.GetType() == EventType::MOUSE_MOVED)
	{
		glm::vec2 cursorPos = static_cast<glm::vec2>(dynamic_cast<const CursorMoveEvent*>(&ev)->GetNewPosition());
		glm::vec2 windowSize((glm::vec2)GameHandle->GetGameSettings()->WindowSize);
		glm::vec2 windowBottomLeft(0.0f);

		glm::vec2 cursorNDC = (glm::vec2(cursorPos.x, windowSize.y - cursorPos.y) - windowBottomLeft) / windowSize * 2.0f - 1.0f;

		bool bMouseInside = ContainsMouse(cursorNDC);


		if (bMouseInside)
		{
			if (State == EditorIconState::IDLE || State == EditorIconState::BEING_CLICKED_OUTSIDE)
				OnHover();
		}
		else if (State == EditorIconState::HOVER || State == EditorIconState::BEING_CLICKED_INSIDE)
			OnUnhover();
	}
	else if (ev.GetType() == EventType::MOUSE_PRESSED && dynamic_cast<const MouseButtonEvent&>(ev).GetButton() == MouseButton::LEFT && State == EditorIconState::HOVER)
		OnBeingClicked();
	else if (ev.GetType() == EventType::MOUSE_RELEASED && dynamic_cast<const MouseButtonEvent&>(ev).GetButton() == MouseButton::LEFT)
	{
		if (State == EditorIconState::BEING_CLICKED_INSIDE)
			OnClick();
		else if (State == EditorIconState::BEING_CLICKED_OUTSIDE)
			State = EditorIconState::IDLE;
	}
	else if (ev.GetType() == EventType::FOCUS_SWITCHED)
		State = EditorIconState::IDLE;

	if (State == EditorIconState::BEING_CLICKED_INSIDE || State ==  EditorIconState::BEING_CLICKED_OUTSIDE)
		WhileBeingClicked();

	DeduceMaterial();
}

void UIButtonActor::OnHover()
{
	State = (State == EditorIconState::BEING_CLICKED_OUTSIDE) ? (EditorIconState::BEING_CLICKED_INSIDE) : (EditorIconState::HOVER);
}

void UIButtonActor::OnUnhover()
{
	State = (State == EditorIconState::BEING_CLICKED_INSIDE) ? (EditorIconState::BEING_CLICKED_OUTSIDE) : (EditorIconState::IDLE);
}

void UIButtonActor::OnClick()
{
	std::cout << "Clicked " + Name + "!\n";
	State = EditorIconState::HOVER;
	if (OnClickFunc)
		OnClickFunc();
}

void UIButtonActor::OnBeingClicked()
{
	State = EditorIconState::BEING_CLICKED_INSIDE;
}

void UIButtonActor::WhileBeingClicked()
{
	if (WhileBeingClickedFunc)
		WhileBeingClickedFunc();
}

EditorIconState UIButtonActor::GetState()
{
	return State;
}

bool UIButtonActor::ContainsMouse(glm::vec2 cursorNDC)
{
	const Transform& worldT = GetTransform()->GetWorldTransform();

	if (CanvasPtr)
	{
		if (!CanvasPtr->GetViewport().Contains(cursorNDC))
			return false;

		glm::vec2 cursorCanvasSpace = glm::inverse(CanvasPtr->UICanvas::GetView()) * Transform(glm::vec3(0.0f), glm::vec3(0.0f), CanvasPtr->UICanvas::GetViewT().ScaleRef).GetMatrix() * glm::inverse(CanvasPtr->GetCanvasT()->GetWorldTransformMatrix()) * glm::vec4(cursorNDC, 0.0f, 1.0f);
		return CanvasPtr->GetViewport().Contains(cursorNDC) && CollisionTests::AlignedRectContainsPoint(CanvasPtr->ToCanvasSpace(worldT), cursorCanvasSpace);
	}

	return CollisionTests::AlignedRectContainsPoint(worldT, cursorNDC);
}

void UIButtonActor::DeduceMaterial()
{
	if (!ButtonModel)
		return;

	std::shared_ptr<MaterialInstance>* currentMatInst = nullptr;
	switch (State)
	{
	case EditorIconState::IDLE:
		currentMatInst = &MatIdle; break;
	case EditorIconState::HOVER:
	case EditorIconState::BEING_CLICKED_OUTSIDE:
		currentMatInst = &MatHover; break;
	case EditorIconState::BEING_CLICKED_INSIDE:
		currentMatInst = &MatClick; break;
	default:
		ButtonModel->OverrideInstancesMaterialInstances(nullptr);
		PrevDeducedMaterial = nullptr;
		return;
	}
	
	if (currentMatInst->get() != PrevDeducedMaterial)
	{
		ButtonModel->OverrideInstancesMaterialInstances(*currentMatInst);
		PrevDeducedMaterial = currentMatInst->get();
	}
}

bool CollisionTests::AlignedRectContainsPoint(const Transform& rect, const glm::vec2& point)
{
	glm::vec2 rectLeftBottom = rect.PositionRef - rect.ScaleRef;
	return point == glm::min(glm::max(rectLeftBottom, point), rectLeftBottom + glm::vec2(rect.ScaleRef) * 2.0f);
}

UIActivableButtonActor::UIActivableButtonActor(GameScene& scene, const std::string& name, std::function<void()> onClickFunc, std::function<void()> onDeactivationFunc):
	UIButtonActor(scene, name, onClickFunc),
	OnDeactivationFunc(onDeactivationFunc)
{

	Material* matActive;
	if ((matActive = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Active")) == nullptr)
	{
		matActive = new Material("GEE_Button_Active");
		matActive->SetColor(glm::vec4(1.0f));
		matActive->SetRenderShaderName("Forward_NoLight");
		GameHandle->GetRenderEngineHandle()->AddMaterial(matActive);
	}

	MatActive = std::make_shared<MaterialInstance>(MaterialInstance(*matActive));
}

void UIActivableButtonActor::SetMatActive(MaterialInstance&& mat)
{
	MatActive = std::make_shared<MaterialInstance>(std::move(mat));
	DeduceMaterial();
}

void UIActivableButtonActor::SetOnDeactivationFunc(std::function<void()> onDeactivationFunc)
{
	OnDeactivationFunc = onDeactivationFunc;
}

void UIActivableButtonActor::HandleEvent(const Event& ev)
{
	bool wasActive = State == EditorIconState::ACTIVATED;
	bool isActive = wasActive;
	if ((ev.GetType() == EventType::MOUSE_RELEASED && dynamic_cast<const MouseButtonEvent&>(ev).GetButton() == MouseButton::LEFT) || (ev.GetType() == EventType::FOCUS_SWITCHED))	//When the user releases LMB anywhere or our scene is de-focused, we disable writing to the InputBox.
		isActive = false;

	if ((!isActive && wasActive) || (isActive && (ev.GetType() == EventType::KEY_PRESSED || ev.GetType() == EventType::KEY_REPEATED) && dynamic_cast<const KeyEvent&>(ev).GetKeyCode() == Key::ENTER))
		OnDeactivation();

	UIButtonActor::HandleEvent(ev);	//If LMB is released while the mouse is hovering over the InputBox, OnClick() will be called and writing will be enabled.
}

void UIActivableButtonActor::OnClick()
{
	UIButtonActor::OnClick();
	State = EditorIconState::ACTIVATED;
}

void UIActivableButtonActor::OnDeactivation()
{
	std::cout << "Deactivating...\n";
	State = ((ContainsMouse(GameHandle->GetInputRetriever().GetMousePositionNDC())) ? (EditorIconState::HOVER) : (EditorIconState::IDLE));
	if (OnDeactivationFunc)
		OnDeactivationFunc();
}

void UIActivableButtonActor::DeduceMaterial()
{
	if (!ButtonModel)
		return;

	UIButtonActor::DeduceMaterial();

	if (State == EditorIconState::ACTIVATED)
	{
		ButtonModel->OverrideInstancesMaterialInstances(MatClick);
		PrevDeducedMaterial = MatClick.get();
	}
}

UIScrollBarActor::UIScrollBarActor(GameScene& scene, const std::string& name, std::function<void()> onClickFunc, std::function<void()> beingClickedFunc):
	UIButtonActor(scene, name, onClickFunc, beingClickedFunc),
	ClickPosNDC(glm::vec2(0.0f))
{
}

void UIScrollBarActor::OnBeingClicked()
{
	ClickPosNDC = GameHandle->GetInputRetriever().GetMousePositionNDC();
	UIButtonActor::OnBeingClicked();
}

void UIScrollBarActor::WhileBeingClicked()
{
	UIButtonActor::WhileBeingClicked();
	ClickPosNDC = GameHandle->GetInputRetriever().GetMousePositionNDC();
}

const glm::vec2& UIScrollBarActor::GetClickPosNDC()
{
	return ClickPosNDC;
}

void UIScrollBarActor::SetClickPosNDC(const glm::vec2& pos)
{
	ClickPosNDC = pos;
}
