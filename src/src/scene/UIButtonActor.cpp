#include <scene/UIButtonActor.h>
#include <game/GameScene.h>
#include <scene/ModelComponent.h>
#include <game/GameSettings.h>
#include <input/Event.h>
#include <UI/UICanvas.h>
#include <scene/TextComponent.h>
#include <input/InputDevicesStateRetriever.h>
#include <math/Box.h>
#include <editor/EditorActions.h>


#include <scene/UIWindowActor.h>

namespace GEE
{
	UIButtonActor::UIButtonActor(GameScene& scene, Actor* parentActor, const String& name, std::function<void()> onClickFunc, const Transform& t) :
		UIActorDefault(scene, parentActor, name, t),
		OnClickFunc(onClickFunc),
		OnDoubleClickFunc(nullptr),
		OnHoverFunc(nullptr),
		OnUnhoverFunc(nullptr),
		OnBeingClickedFunc(nullptr),
		WhileBeingClickedFunc(nullptr),
		PopupCreationFunc(nullptr),
		PrevClickTime(0.0f),
		MaxDoubleClickTime(0.3f),
		MatIdle(nullptr),
		MatHover(nullptr),
		MatClick(nullptr),  
		MatActive(nullptr),
		MatDisabled(nullptr),
		PrevDeducedMaterial(nullptr),
		StateBits(0),
		bInputDisabled(false)
	{
		ButtonModel = &CreateComponent<ModelComponent>(Name + "'s_Button_Model");

		SharedPtr<Material> matIdle, matHover, matClick, matActive, matDisabled;
		if ((matIdle = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Idle")) == nullptr)
		{
			matIdle = MakeShared<Material>("GEE_Button_Idle", Vec3f(0.4118f, 0.0909f, 0.4973f));
			matIdle->SetColor(Vec4f(0.4118f, 0.0909f, 0.4973f, 1.0f));
			GameHandle->GetRenderEngineHandle()->AddMaterial(matIdle);
		}
		if ((matHover = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Hover")) == nullptr)
		{
			matHover = MakeShared<Material>("GEE_Button_Hover", Vec3f(0.831684f, 0.2f, 0.2f));
			GameHandle->GetRenderEngineHandle()->AddMaterial(matHover);
		}
		if ((matClick = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Click")) == nullptr)
		{
			matClick = MakeShared<Material>("GEE_Button_Click", Vec3f(0.773018f, 0.773018f, 0.773018f));
			GameHandle->GetRenderEngineHandle()->AddMaterial(matClick);
		}
		if ((matActive = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Active")) == nullptr)
		{
			matActive = MakeShared<Material>("GEE_Button_Active", Vec3f(0.75f, 0.75f, 0.22f));
			GameHandle->GetRenderEngineHandle()->AddMaterial(matActive);
		}
		if ((matDisabled = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Disabled")) == nullptr)
		{
			matDisabled = MakeShared<Material>("GEE_Button_Disabled", Vec3f(0.3f, 0.3f, 0.3f));
			GameHandle->GetRenderEngineHandle()->AddMaterial(matDisabled);
		}

		MatIdle = MakeShared<MaterialInstance>(matIdle);
		MatHover = MakeShared<MaterialInstance>(matHover);
		MatClick = MakeShared<MaterialInstance>(matClick);
		MatActive = MakeShared<MaterialInstance>(matActive);
		MatDisabled = MakeShared<MaterialInstance>(matDisabled);


		ButtonModel->AddMeshInst(MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad), MatIdle));
		PrevDeducedMaterial = MatIdle.get();

		PopupCreationFunc = [this](PopupDescription desc) { desc.AddOption("Transform", [this]() { auto& window = Scene.CreateActorAtRoot<UIWindowActor>("Button transform"); ComponentDescriptionBuilder descBuilder(*dynamic_cast<Editor::EditorManager*>(GameHandle), window, window); GetRoot()->GetEditorDescription(descBuilder); window.AutoClampView(); window.RefreshFieldsList();  }); };
	}

	UIButtonActor::UIButtonActor(GameScene& scene, Actor* parentActor, const String& name, const String& buttonTextContent, std::function<void()> onClickFunc, const Transform& t) :
		UIButtonActor(scene, parentActor, name, onClickFunc, t)
	{
		CreateButtonText(buttonTextContent);
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
			StateBits = 0;
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

	void UIButtonActor::SetPopupCreationFunc(std::function<void(PopupDescription)> popupCreationFunc)
	{
		PopupCreationFunc = popupCreationFunc;
		if (popupCreationFunc)
		{
			PopupCreationFunc = [popupCreationFunc, this](PopupDescription desc) { popupCreationFunc(desc); if (CanvasPtr) desc.AddSubmenu("Canvas", dynamic_cast<UICanvasActor*>(CanvasPtr)->GetPopupCreationFunc()); };
		}
	}

	void UIButtonActor::CallOnClickFunc()
	{
		if (OnClickFunc)
			OnClickFunc();
	}

	void UIButtonActor::DeleteButtonModel()
	{
		if (ButtonModel)
		{
			ButtonModel->MarkAsKilled();
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


			if (bMouseInside && !(StateBits & ButtonStateFlags::Hover))
				OnHover();
			else if (!bMouseInside && StateBits & ButtonStateFlags::Hover)
				OnUnhover();
		}
		else if (ev.GetType() == EventType::MousePressed && dynamic_cast<const MouseButtonEvent&>(ev).GetButton() == MouseButton::Left && StateBits & ButtonStateFlags::Hover)
			OnBeingClicked();
		else if (ev.GetType() == EventType::MouseReleased && dynamic_cast<const MouseButtonEvent&>(ev).GetButton() == MouseButton::Left)
		{
			if (StateBits & ButtonStateFlags::BeingClicked && StateBits & ButtonStateFlags::Hover)
			{
				if (GameHandle->GetProgramRuntime() - PrevClickTime <= MaxDoubleClickTime)
					OnDoubleClick();
				else
					OnClick();
			}
			else if (StateBits & ButtonStateFlags::BeingClicked)
				StateBits &= ~ButtonStateFlags::BeingClicked;
		}
		else if (StateBits & ButtonStateFlags::Hover && ev.GetType() == EventType::MouseReleased && PopupCreationFunc && dynamic_cast<const MouseButtonEvent&>(ev).GetButton() == MouseButton::Right)
			dynamic_cast<Editor::EditorManager*>(GameHandle)->RequestPopupMenu(Scene.GetUIData()->GetWindowData().GetMousePositionNDC(), *Scene.GetUIData()->GetWindow(), PopupCreationFunc);
		else if (ev.GetType() == EventType::FocusSwitched)
			StateBits = 0;

		if (StateBits & ButtonStateFlags::BeingClicked)
			WhileBeingClicked();

		DeduceMaterial();
	}

	void UIButtonActor::OnHover()
	{
		StateBits |= ButtonStateFlags::Hover;
		if (OnHoverFunc)
			OnHoverFunc();
	}

	void UIButtonActor::OnUnhover()
	{
		//State = (State == EditorButtonState::BeingClickedInside) ? (EditorButtonState::BeingClickedOutside) : (EditorButtonState::Idle);
		StateBits &= ~ButtonStateFlags::Hover;
		if (OnUnhoverFunc)
			OnUnhoverFunc();
	}

	void UIButtonActor::OnClick()
	{
		std::cout << "Clicked " + Name + "!\n";
		StateBits &= ~ButtonStateFlags::BeingClicked;
		PrevClickTime = GameHandle->GetProgramRuntime();
		if (OnClickFunc)
			OnClickFunc();
	}

	void UIButtonActor::OnDoubleClick()
	{
		StateBits &= ~ButtonStateFlags::BeingClicked;
		PrevClickTime = GameHandle->GetProgramRuntime();
		if (OnDoubleClickFunc)
			OnDoubleClickFunc();
	}

	void UIButtonActor::OnBeingClicked()
	{
		StateBits |= ButtonStateFlags::BeingClicked;
		if (OnBeingClickedFunc)
			OnBeingClickedFunc();
	}

	void UIButtonActor::WhileBeingClicked()
	{
		if (WhileBeingClickedFunc)
			WhileBeingClickedFunc();
	}

	int UIButtonActor::GetStateBits()
	{
		return StateBits;
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
			if (StateBits & ButtonStateFlags::BeingClicked)
				currentMatInst = (StateBits & ButtonStateFlags::Hover) ? (&MatClick) : (&MatHover);
			else if (StateBits & ButtonStateFlags::Hover)
				currentMatInst = &MatHover;
			else
				currentMatInst = &MatIdle;
		}
		else
			currentMatInst = &MatDisabled;

		if (currentMatInst->get() != PrevDeducedMaterial)
		{
			ButtonModel->OverrideInstancesMaterialInstances(*currentMatInst);
			PrevDeducedMaterial = currentMatInst->get();
		}
	}

	void UIButtonActor::CreateButtonText(const String& content)
	{
		auto& text = CreateComponent<TextComponent>("ButtonText", Transform(Vec2f(0.0f), Vec2f(1.0f)), content, "", Alignment2D::Center());
		text.SetMaxSize(Vec2f(0.8f));
		text.Unstretch();
	}

	bool CollisionTests::AlignedRectContainsPoint(const Transform& rect, const Vec2f& point)
	{
		Vec2f rectLeftBottom = rect.GetPos() - rect.GetScale();
		return point == glm::min(glm::max(rectLeftBottom, point), rectLeftBottom + Vec2f(rect.GetScale()) * 2.0f);
	}

	UIActivableButtonActor::UIActivableButtonActor(GameScene& scene, Actor* parentActor, const String& name,  std::function<void()> onClickFunc, std::function<void()> onDeactivationFunc, const Transform& transform) :
		UIButtonActor(scene, parentActor, name, onClickFunc, transform),
		OnDeactivationFunc(onDeactivationFunc),
		bActivateOnClicking(true),
		bDeactivateOnClickingAnywhere(true),
		bDeactivateOnClickingAgain(true),
		bDeactivateOnPressingEnter(false)
	{
		SharedPtr<Material> matActive;
		if ((matActive = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Active")) == nullptr)
		{
			matActive = MakeShared<Material>("GEE_Button_Active");
			matActive->SetColor(Vec4f(1.0f));
			GameHandle->GetRenderEngineHandle()->AddMaterial(matActive);
		}

		MatActive = MakeShared<MaterialInstance>(matActive);
	}
	UIActivableButtonActor::UIActivableButtonActor(GameScene& scene, Actor* parentActor, const String& name, const String& buttonTextContent, std::function<void()> onClickFunc, std::function<void()> onDeactivationFunc, const Transform& transform) :
		UIActivableButtonActor(scene, parentActor, name, onClickFunc, onDeactivationFunc, transform)
	{
		CreateButtonText(buttonTextContent);
	}


	void UIActivableButtonActor::SetActivateOnClicking(bool activateOnClick)
	{
		bActivateOnClicking = activateOnClick;
	}

	void UIActivableButtonActor::SetDeactivateOnClickingAnywhere(bool deactivateOnClick)
	{
		bDeactivateOnClickingAnywhere = deactivateOnClick;
	}

	void UIActivableButtonActor::SetDeactivateOnClickingAgain(bool deactivateOnClick)
	{
		bDeactivateOnClickingAgain = deactivateOnClick;
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

	void UIActivableButtonActor::SetActive(bool active)
	{
		if (active)
			StateBits |= ButtonStateFlags::Active;
		else if (StateBits & ButtonStateFlags::Active)
			StateBits &= ~ButtonStateFlags::Active;

		DeduceMaterial();
	}

	void UIActivableButtonActor::HandleEvent(const Event& ev)
	{
		bool wasActive = StateBits & ButtonStateFlags::Active;

		bool isActive = wasActive;
		if (isActive)
		{
			const MouseButtonEvent* buttonEvCast = dynamic_cast<const MouseButtonEvent*>(&ev);	// todo: hide it inside an if so we dont use dynamic cast every time we handle an event

			bool mouseInside = (Scene.GetUIData()->GetCurrentBlockingCanvas() && !Scene.GetUIData()->GetCurrentBlockingCanvas()->ShouldAcceptBlockedEvents(*this)) ? (false) : (ContainsMouse(Scene.GetUIData()->GetWindowData().GetMousePositionNDC()));

			if (ev.GetType() == EventType::FocusSwitched)
				isActive = false;
			else if (bDeactivateOnPressingEnter && (ev.GetType() == EventType::KeyPressed || ev.GetType() == EventType::KeyRepeated) && dynamic_cast<const KeyEvent&>(ev).GetKeyCode() == Key::Enter)
				isActive = false;
			if (bDeactivateOnClickingAnywhere)
			{
				if (((ev.GetType() == EventType::MouseReleased && buttonEvCast->GetButton() == MouseButton::Left)) && (bDeactivateOnClickingAgain || !mouseInside) && !(buttonEvCast && buttonEvCast->GetModifierBits() & KeyModifierFlags::Alt))	//When the user releases LMB anywhere, we disable writing to the InputBox.
					isActive = false;
			}
			else if (bDeactivateOnClickingAgain)
			{
				if ((ev.GetType() == EventType::MouseReleased && buttonEvCast->GetButton() == MouseButton::Left) && mouseInside)
					isActive = false;
			}
		}

		UIButtonActor::HandleEvent(ev);	//If LMB is released while the mouse is hovering over the InputBox, OnClick() will be called and writing will be enabled.

		if (!isActive && wasActive)
		{
			OnDeactivation();
			DeduceMaterial();
		}
	}

	void UIActivableButtonActor::OnClick()
	{
		UIButtonActor::OnClick();
		if (bActivateOnClicking)
			StateBits |= ButtonStateFlags::Active;
	}

	void UIActivableButtonActor::OnDeactivation()
	{
		std::cout << "Deactivating...\n";
		StateBits &= ~(ButtonStateFlags::Active);
		if (OnDeactivationFunc)
			OnDeactivationFunc();
	}

	void UIActivableButtonActor::DeduceMaterial()
	{
		if (!ButtonModel)
			return;

		UIButtonActor::DeduceMaterial();

		if (StateBits & ButtonStateFlags::Active)
		{
			ButtonModel->OverrideInstancesMaterialInstances(MatActive);
			PrevDeducedMaterial = MatActive.get();
		}
	}

	UIScrollBarActor::UIScrollBarActor(GameScene& scene, Actor* parentActor, const String& name, std::function<void()> onClickFunc, std::function<void()> beingClickedFunc) :
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

	void uiButtonActorUtil::ButtonMatsFromAtlas(UIButtonActor& button, SharedPtr<AtlasMaterial> atlasMat, float idleID, float hoverID, float clickID, float disabledID)
	{
		button.SetMatIdle(MaterialInstance(atlasMat, atlasMat->GetTextureIDInterpolatorTemplate(idleID)));
		if (hoverID >= 0.0f) button.SetMatHover(MaterialInstance(atlasMat, atlasMat->GetTextureIDInterpolatorTemplate(hoverID)));
		if (clickID >= 0.0f) button.SetMatClick(MaterialInstance(atlasMat, atlasMat->GetTextureIDInterpolatorTemplate(clickID)));
		if (disabledID >= 0.0f) button.SetMatDisabled(MaterialInstance(atlasMat, atlasMat->GetTextureIDInterpolatorTemplate(disabledID)));
	}
}
