#include <scene/UIButtonActor.h>
#include <scene/ModelComponent.h>
#include <game/GameSettings.h>
#include <input/Event.h>
#include <UI/UICanvas.h>
#include <scene/TextComponent.h>
#include <input/InputDevicesStateRetriever.h>
#include <math/Box.h>
#include <editor/EditorActions.h>

namespace GEE
{
	UIButtonActor::UIButtonActor(GameScene& scene, Actor* parentActor, const std::string& name, std::function<void()> onClickFunc, const Transform& t) :
		UIActorDefault(scene, parentActor, name, t),
		OnClickFunc(onClickFunc),
		OnDoubleClickFunc(nullptr),
		OnHoverFunc(nullptr),
		OnUnhoverFunc(nullptr),
		OnBeingClickedFunc(nullptr),
		WhileBeingClickedFunc(nullptr),
		PrevClickTime(0.0f),
		MaxDoubleClickTime(0.3f),
		MatIdle(nullptr),
		MatClick(nullptr),
		MatHover(nullptr),
		MatDisabled(nullptr),
		PrevDeducedMaterial(nullptr),
		State(EditorIconState::IDLE),
		bInputDisabled(false)
	{
		ButtonModel = &CreateComponent<ModelComponent>(Name + "'s_Button_Model");

		SharedPtr<Material> matIdle, matHover, matClick, matDisabled;
		if ((matIdle = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Idle")) == nullptr)
		{
			matIdle = MakeShared<Material>("GEE_Button_Idle");
			matIdle->SetColor(Vec4f(0.4118f, 0.0909f, 0.4973f, 1.0f));
			matIdle->SetRenderShaderName("Forward_NoLight");
			GameHandle->GetRenderEngineHandle()->AddMaterial(matIdle);
		}
		if ((matHover = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Hover")) == nullptr)
		{
			matHover = MakeShared<Material>("GEE_Button_Hover");
			matHover->SetColor(Vec4f(0.831684f, 0.2f, 0.2f, 1.0f));
			matHover->SetRenderShaderName("Forward_NoLight");
			GameHandle->GetRenderEngineHandle()->AddMaterial(matHover);
		}
		if ((matClick = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Click")) == nullptr)
		{
			matClick = MakeShared<Material>("GEE_Button_Click");
			matClick->SetColor(Vec4f(0.773018f, 0.773018f, 0.773018f, 1.0f));
			matClick->SetRenderShaderName("Forward_NoLight");
			GameHandle->GetRenderEngineHandle()->AddMaterial(matClick);
		}
		if ((matDisabled = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Disabled")) == nullptr)
		{
			matDisabled = MakeShared<Material>("GEE_Button_Disabled");
			matDisabled->SetColor(Vec4f(0.3f, 0.3f, 0.3f, 1.0f));
			matDisabled->SetRenderShaderName("Forward_NoLight");
			GameHandle->GetRenderEngineHandle()->AddMaterial(matDisabled);
		}

		MatIdle = MakeShared<MaterialInstance>(MaterialInstance(*matIdle));
		MatHover = MakeShared<MaterialInstance>(MaterialInstance(*matHover));
		MatClick = MakeShared<MaterialInstance>(MaterialInstance(*matClick));
		MatDisabled = MakeShared<MaterialInstance>(MaterialInstance(*matDisabled));


		ButtonModel->AddMeshInst(MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), MatIdle));
		PrevDeducedMaterial = MatIdle.get();
	}

	UIButtonActor::UIButtonActor(GameScene& scene, Actor* parentActor, const std::string& name, const std::string& buttonTextContent, std::function<void()> onClickFunc, const Transform& t) :
		UIButtonActor(scene, parentActor, name, onClickFunc, t)
	{
		CreateButtonText(buttonTextContent).Unstretch();
	}

	ModelComponent* UIButtonActor::GetButtonModel()
	{
		return ButtonModel;
	}

	Boxf<Vec2f> UIButtonActor::GetBoundingBox(bool world) const
	{
		if (!GetTransform())
			return Boxf<Vec2f>(Vec2f(0.0f), Vec2f(0.0f));

		if (!world)
			return Boxf<Vec2f>(GetTransform()->GetPos(), GetTransform()->GetScale());

		if (!CanvasPtr)
			return Boxf<Vec2f>(GetTransform()->GetWorldTransform().GetPos(), GetTransform()->GetWorldTransform().GetScale());

		Transform canvasSpaceTransform = CanvasPtr->ToCanvasSpace(GetTransform()->GetWorldTransform());	//world == true, CanvasPtr isn't nullptr and this actor has got a Transform.
		return Boxf<Vec2f>(canvasSpaceTransform.GetPos(), canvasSpaceTransform.GetScale());
	}

	void UIButtonActor::SetMatIdle(MaterialInstance&& mat)
	{
		MatIdle = MakeShared<MaterialInstance>(std::move(mat));
		DeduceMaterial();
	}

	void UIButtonActor::SetMatHover(MaterialInstance&& mat)
	{
		MatHover = MakeShared<MaterialInstance>(std::move(mat));
		DeduceMaterial();
	}

	void UIButtonActor::SetMatClick(MaterialInstance&& mat)
	{
		MatClick = MakeShared<MaterialInstance>(std::move(mat));
		DeduceMaterial();
	}

	void UIButtonActor::SetMatDisabled(MaterialInstance&& mat)
	{
		MatDisabled = MakeShared<MaterialInstance>(std::move(mat));
		DeduceMaterial();
	}

	void UIButtonActor::SetDisableInput(bool disable)
	{
		if (disable && !bInputDisabled)
			State = EditorIconState::IDLE;
		bInputDisabled = disable;
		DeduceMaterial();
	}

	void UIButtonActor::SetOnClickFunc(std::function<void()> onClickFunc)
	{
		OnClickFunc = onClickFunc;
	}

	void UIButtonActor::SetOnDoubleClickFunc(std::function<void()> onDoubleClickFunc)
	{
		OnDoubleClickFunc = onDoubleClickFunc;
	}

	void UIButtonActor::SetOnHoverFunc(std::function<void()> onHoverFunc)
	{
		OnHoverFunc = onHoverFunc;
	}

	void UIButtonActor::SetOnUnhoverFunc(std::function<void()> onUnhoverFunc)
	{
		OnUnhoverFunc = onUnhoverFunc;
	}

	void UIButtonActor::SetOnBeingClickedFunc(std::function<void()> onBeingClickedFunc)
	{
		OnBeingClickedFunc = onBeingClickedFunc;
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
		Actor::HandleEvent(ev);
		if (bInputDisabled)
			return;

		if (ev.GetType() == EventType::MouseMoved)
		{
			Vec2f cursorNDC = dynamic_cast<const CursorMoveEvent*>(&ev)->GetNewPositionNDC();

			bool bMouseInside = (Scene.GetUIData()->GetCurrentBlockingCanvas() && !Scene.GetUIData()->GetCurrentBlockingCanvas()->ShouldAcceptBlockedEvents(*this)) ? (false) : (ContainsMouse(cursorNDC));


			if (bMouseInside)
			{
				if (State == EditorIconState::IDLE || State == EditorIconState::BEING_CLICKED_OUTSIDE)
					OnHover();
			}
			else if (State == EditorIconState::HOVER || State == EditorIconState::BEING_CLICKED_INSIDE)
				OnUnhover();
		}
		else if (ev.GetType() == EventType::MousePressed && dynamic_cast<const MouseButtonEvent&>(ev).GetButton() == MouseButton::Left && State == EditorIconState::HOVER)
			OnBeingClicked();
		else if (ev.GetType() == EventType::MouseReleased && dynamic_cast<const MouseButtonEvent&>(ev).GetButton() == MouseButton::Left)
		{
			if (State == EditorIconState::BEING_CLICKED_INSIDE)
			{
				if (GameHandle->GetProgramRuntime() - PrevClickTime <= MaxDoubleClickTime)
					OnDoubleClick();
				else
					OnClick();
			}
			else if (State == EditorIconState::BEING_CLICKED_OUTSIDE)
				State = EditorIconState::IDLE;
		}
		else if (ev.GetType() == EventType::FocusSwitched)
			State = EditorIconState::IDLE;

		if (State == EditorIconState::BEING_CLICKED_INSIDE || State == EditorIconState::BEING_CLICKED_OUTSIDE)
			WhileBeingClicked();

		DeduceMaterial();
	}

	void UIButtonActor::OnHover()
	{
		State = (State == EditorIconState::BEING_CLICKED_OUTSIDE) ? (EditorIconState::BEING_CLICKED_INSIDE) : (EditorIconState::HOVER);
		if (OnHoverFunc)
			OnHoverFunc();
	}

	void UIButtonActor::OnUnhover()
	{
		State = (State == EditorIconState::BEING_CLICKED_INSIDE) ? (EditorIconState::BEING_CLICKED_OUTSIDE) : (EditorIconState::IDLE);
		if (OnUnhoverFunc)
			OnUnhoverFunc();
	}

	void UIButtonActor::OnClick()
	{
		std::cout << "Clicked " + Name + "!\n";
		State = EditorIconState::HOVER;
		PrevClickTime = GameHandle->GetProgramRuntime();
		if (OnClickFunc)
			OnClickFunc();
	}

	void UIButtonActor::OnDoubleClick()
	{
		State = EditorIconState::HOVER;
		PrevClickTime = GameHandle->GetProgramRuntime();
		if (OnDoubleClickFunc)
			OnDoubleClickFunc();
	}

	void UIButtonActor::OnBeingClicked()
	{
		State = EditorIconState::BEING_CLICKED_INSIDE;
		if (OnBeingClickedFunc)
			OnBeingClickedFunc();
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

	bool UIButtonActor::ContainsMouse(Vec2f cursorNDC)
	{
		const Transform& worldT = GetTransform()->GetWorldTransform();

		if (CanvasPtr)
		{
			if (!CanvasPtr->ContainsMouse())
				return false;
			Vec2f cursorCanvasSpace = glm::inverse(CanvasPtr->UICanvas::GetViewMatrix()) * glm::inverse(CanvasPtr->GetCanvasT()->GetWorldTransformMatrix()) * Vec4f(cursorNDC, 0.0f, 1.0f);
			return CollisionTests::AlignedRectContainsPoint(CanvasPtr->ToCanvasSpace(worldT), cursorCanvasSpace);
		}

		return CollisionTests::AlignedRectContainsPoint(worldT, cursorNDC);
	}

	void UIButtonActor::DeduceMaterial()
	{
		if (!ButtonModel)
			return;

		SharedPtr<MaterialInstance>* currentMatInst = nullptr;
		if (!bInputDisabled)
		{
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
		}
		else
			currentMatInst = &MatDisabled;

		if (currentMatInst->get() != PrevDeducedMaterial)
		{
			ButtonModel->OverrideInstancesMaterialInstances(*currentMatInst);
			PrevDeducedMaterial = currentMatInst->get();
		}
	}

	TextConstantSizeComponent& UIButtonActor::CreateButtonText(const std::string& content)
	{
		auto& text = CreateComponent<TextConstantSizeComponent>("ButtonText", Transform(Vec2f(0.0f), Vec2f(1.0f)), content, "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
		text.SetMaxSize(Vec2f(0.8f));
		text.Unstretch();
		text.Unstretch();
		text.Unstretch();
		text.Unstretch();
		text.Unstretch();
		return text;
	}

	bool CollisionTests::AlignedRectContainsPoint(const Transform& rect, const Vec2f& point)
	{
		Vec2f rectLeftBottom = rect.GetPos() - rect.GetScale();
		return point == glm::min(glm::max(rectLeftBottom, point), rectLeftBottom + Vec2f(rect.GetScale()) * 2.0f);
	}

	UIActivableButtonActor::UIActivableButtonActor(GameScene& scene, Actor* parentActor, const std::string& name,  std::function<void()> onClickFunc, std::function<void()> onDeactivationFunc, const Transform& transform) :
		UIButtonActor(scene, parentActor, name, onClickFunc, transform),
		OnDeactivationFunc(onDeactivationFunc)
	{

		SharedPtr<Material> matActive;
		if ((matActive = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Active")) == nullptr)
		{
			matActive = MakeShared<Material>("GEE_Button_Active");
			matActive->SetColor(Vec4f(1.0f));
			matActive->SetRenderShaderName("Forward_NoLight");
			GameHandle->GetRenderEngineHandle()->AddMaterial(matActive);
		}

		MatActive = MakeShared<MaterialInstance>(MaterialInstance(*matActive));
	}
	UIActivableButtonActor::UIActivableButtonActor(GameScene& scene, Actor* parentActor, const std::string& name, const std::string& buttonTextContent, std::function<void()> onClickFunc, std::function<void()> onDeactivationFunc, const Transform& transform) :
		UIActivableButtonActor(scene, parentActor, name, onClickFunc, onDeactivationFunc, transform)
	{
		CreateButtonText(buttonTextContent);
	}


	void UIActivableButtonActor::SetMatActive(MaterialInstance&& mat)
	{
		MatActive = MakeShared<MaterialInstance>(std::move(mat));
		DeduceMaterial();
	}

	void UIActivableButtonActor::SetOnDeactivationFunc(std::function<void()> onDeactivationFunc)
	{
		OnDeactivationFunc = onDeactivationFunc;
	}

	void UIActivableButtonActor::HandleEvent(const Event& ev)
	{
		UIButtonActor::HandleEvent(ev);
		bool wasActive = State == EditorIconState::ACTIVATED;
		bool isActive = wasActive;
		if ((ev.GetType() == EventType::MouseReleased && dynamic_cast<const MouseButtonEvent&>(ev).GetButton() == MouseButton::Left) || (ev.GetType() == EventType::FocusSwitched))	//When the user releases LMB anywhere or our scene is de-focused, we disable writing to the InputBox.
			isActive = false;

		if ((!isActive && wasActive) || (isActive && (ev.GetType() == EventType::KeyPressed || ev.GetType() == EventType::KeyRepeated) && dynamic_cast<const KeyEvent&>(ev).GetKeyCode() == Key::Enter))
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
		State = ((ContainsMouse(Scene.GetUIData()->GetWindowData().GetMousePositionNDC())) ? (EditorIconState::HOVER) : (EditorIconState::IDLE));
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

	UIScrollBarActor::UIScrollBarActor(GameScene& scene, Actor* parentActor, const std::string& name, std::function<void()> onClickFunc, std::function<void()> beingClickedFunc) :
		UIButtonActor(scene, parentActor, name, onClickFunc),
		ClickPosNDC(Vec2f(0.0f))
	{
		SetWhileBeingClickedFunc(beingClickedFunc);
	}

	void UIScrollBarActor::OnBeingClicked()
	{
		ClickPosNDC = Scene.GetUIData()->GetWindowData().GetMousePositionNDC();
		UIButtonActor::OnBeingClicked();
	}

	void UIScrollBarActor::WhileBeingClicked()
	{
		UIButtonActor::WhileBeingClicked();
		ClickPosNDC = Scene.GetUIData()->GetWindowData().GetMousePositionNDC();
	}

	const Vec2f& UIScrollBarActor::GetClickPosNDC()
	{
		return ClickPosNDC;
	}

	void UIScrollBarActor::SetClickPosNDC(const Vec2f& pos)
	{
		ClickPosNDC = pos;
	}

	void uiButtonActorUtil::ButtonMatsFromAtlas(UIButtonActor& button, AtlasMaterial& atlasMat, float idleID, float hoverID, float clickID, float disabledID)
	{
		button.SetMatIdle(MaterialInstance(atlasMat, atlasMat.GetTextureIDInterpolatorTemplate(idleID)));
		if (hoverID >= 0.0f) button.SetMatHover(MaterialInstance(atlasMat, atlasMat.GetTextureIDInterpolatorTemplate(hoverID)));
		if (clickID >= 0.0f) button.SetMatClick(MaterialInstance(atlasMat, atlasMat.GetTextureIDInterpolatorTemplate(clickID)));
		if (disabledID >= 0.0f) button.SetMatDisabled(MaterialInstance(atlasMat, atlasMat.GetTextureIDInterpolatorTemplate(disabledID)));
	}
}
