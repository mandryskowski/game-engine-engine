#include <UI/UICanvasActor.h>
#include <math/Transform.h>
#include <scene/UIButtonActor.h>
#include <input/InputDevicesStateRetriever.h>
#include <scene/ModelComponent.h>
#include <UI/UICanvasField.h>
#include <UI/UIListActor.h>

namespace GEE
{
	UICanvasActor::UICanvasActor(GameScene& scene, Actor* parentActor, UICanvasActor* canvasParent, const std::string& name, const Transform& t) :
		Actor(scene, parentActor, name, t),
		UICanvas((canvasParent) ? (canvasParent->GetCanvasDepth() + 1) : (1)),	//CanvasDepth starts at 1 (because it has to be higher than depth outside any canvas)
		FieldsList(nullptr),
		FieldSize(Vec3f(0.5f)),
		ScrollBarX(nullptr),
		ScrollBarY(nullptr),
		BothScrollBarsButton(nullptr),
		ResizeBarX(nullptr),
		ResizeBarY(nullptr),
		ScaleActor(nullptr),
		CanvasParent(canvasParent)
	{
		Scene.AddBlockingCanvas(*this);
	}

	UICanvasActor::UICanvasActor(GameScene& scene, Actor* parentActor, const std::string& name, const Transform& t) :
		UICanvasActor(scene, parentActor, nullptr, name, t)
	{}

	UICanvasActor::UICanvasActor(UICanvasActor&& canvasActor) :
		Actor(std::move(canvasActor)),
		UICanvas(std::move(canvasActor)),
		FieldsList(nullptr),
		FieldSize(canvasActor.FieldSize),
		ScrollBarX(nullptr),
		ScrollBarY(nullptr),
		ResizeBarX(nullptr),
		ResizeBarY(nullptr),
		ScaleActor(nullptr),
		CanvasParent(canvasActor.CanvasParent)
	{
		FieldsList = dynamic_cast<UIListActor*>(canvasActor.FindActor(Name + "_Fields_List"));
		Scene.AddBlockingCanvas(*this);
	}

	void UICanvasActor::OnStart()
	{
		Actor::OnStart();
		ScaleActor = &CreateChild<UIActorDefault>(Name + " scale actor");
		CreateScrollBars();
	}

	UIActorDefault* UICanvasActor::GetScaleActor()
	{
		return ScaleActor;
	}

	Mat4f UICanvasActor::GetViewMatrix() const
	{
		return UICanvas::GetViewMatrix() * glm::inverse(GetTransform()->GetWorldTransformMatrix());
	}

	NDCViewport UICanvasActor::GetViewport() const
	{
		return NDCViewport(GetTransform()->GetWorldTransform().GetPos() - GetTransform()->GetWorldTransform().GetScale(), GetTransform()->GetWorldTransform().GetScale());
	}

	const Transform* UICanvasActor::GetCanvasT() const
	{
		return GetTransform();
	}

	void UICanvasActor::SetCanvasView(const Transform& canvasView)
	{
		CanvasView = canvasView;
		ClampViewToElements();
		if (ScrollBarX) UpdateScrollBarT<VecAxis::X>();
		if (ScrollBarY) UpdateScrollBarT<VecAxis::Y>();
	}

	Transform UICanvasActor::FromCanvasSpace(const Transform& canvasSpaceTransform) const
	{
		return GetCanvasT()->GetWorldTransform() * canvasSpaceTransform;
	}

	Transform UICanvasActor::ToCanvasSpace(const Transform& worldTransform) const
	{
		return GetCanvasT()->GetWorldTransform().GetInverse() * worldTransform;
	}

	void UICanvasActor::RefreshFieldsList()
	{
		if (FieldsList)
		{
			FieldsList->Refresh();
			ClampViewToElements();
		}
	}

	void UICanvasActor::KillScrollBars()
	{
		if (ScrollBarX) ScrollBarX->MarkAsKilled();
		if (ScrollBarY) ScrollBarY->MarkAsKilled();
		if (BothScrollBarsButton) BothScrollBarsButton->MarkAsKilled();

		ScrollBarX = ScrollBarY = BothScrollBarsButton = nullptr;
	}

	void UICanvasActor::KillResizeBars()
	{
		if (ResizeBarX) ResizeBarX->MarkAsKilled();
		if (ResizeBarY) ResizeBarY->MarkAsKilled();

		ResizeBarX = ResizeBarY = nullptr;
	}

	void UICanvasActor::ClampViewToElements()
	{
		UICanvas::ClampViewToElements();

		if (ScrollBarX)	UpdateScrollBarT<VecAxis::X>();
		if (ScrollBarY) UpdateScrollBarT<VecAxis::Y>();
	}

	Actor& UICanvasActor::AddChild(UniquePtr<Actor> actor)
	{
		Actor& actorRef = Actor::AddChild(std::move(actor));	//actor becomes invalid (nullptr)
		//if (UICanvasElement* elementCast = dynamic_cast<UICanvasElement*>(&actorRef))
		;// AddUIElement(*elementCast);

		return actorRef;
	}

	UICanvasFieldCategory& UICanvasActor::AddCategory(const std::string& name)
	{
		if (!FieldsList)
			FieldsList = &ScaleActor->CreateChild<UIListActor>(Name + "_Fields_List");

		UICanvasFieldCategory& category = FieldsList->CreateChild<UICanvasFieldCategory>(name);
		category.SetTransform(Transform(Vec2f(0.0f, -1.0f), Vec2f(FieldSize)));	//position doesn't matter if this is not the first field
		FieldsList->AddElement(UIListElement(category, [&category]() { category.Refresh(); return category.GetListOffset(); }, [&category]() { return Vec3f(0.0f, -1.5f, 0.0f); }));
		category.SetOnExpansionFunc([this]() { RefreshFieldsList(); });

		return category;
	}

	UICanvasField& UICanvasActor::AddField(const std::string& name, std::function<Vec3f()> getElementOffset)
	{
		if (!FieldsList)
			FieldsList = &ScaleActor->CreateChild<UIListActor>(Name + "_Fields_List");

		UICanvasField& field = FieldsList->CreateChild<UICanvasField>(name);
		(getElementOffset) ? (FieldsList->AddElement(UIListElement(field, getElementOffset))) : (FieldsList->AddElement(UIListElement(field, Vec3f(0.0f, -FieldSize.y * 2.0f, 0.0f))));

		field.SetTransform(Transform(Vec2f(0.0f), Vec2f(FieldSize)));	//position doesn't matter if this is not the first field


		return field;
	}

	void UICanvasActor::HandleEvent(const Event& ev)
	{
		if (ev.GetType() != EventType::MouseScrolled || Scene.GetCurrentBlockingCanvas() != this)
			return;

		const MouseScrollEvent& scrolledEv = dynamic_cast<const MouseScrollEvent&>(ev);

		if (GameHandle->GetInputRetriever().IsKeyPressed(Key::LeftControl))
		{
			Vec2f newScale = static_cast<Vec2f>(CanvasView.GetScale()) * glm::pow(Vec2f(0.5f), Vec2f(scrolledEv.GetOffset().y));
			printVector(newScale, "new scale");
			SetViewScale(newScale);
		}
		else
			ScrollView(scrolledEv.GetOffset());
	}

	void UICanvasActor::HandleEventAll(const Event& ev)
	{
		Actor::HandleEventAll(ev);

		/*bool bContainsMouse = ContainsMouse(GameHandle->GetInputRetriever().GetMousePositionNDC());
		bool focusedOnThis = bContainsMouse;
		if (bContainsMouse)
			Actor::HandleEventAll(ev);
		else
		{
			std::vector<UIButtonActor*> buttons;
			GetExternalButtons(buttons);
			for (auto button : buttons)
				if (button)
				{
					button->HandleEventAll(ev);
					if (button->GetState() != EditorIconState::IDLE)
						focusedOnThis = true;
				}
		}*/
	}

	RenderInfo UICanvasActor::BindForRender(const RenderInfo& info, const Vec2u& res)	//Note: RenderInfo is a relatively big object for real-time standards. HOPEFULLY the compiler will optimize here using NRVO and convertedInfo won't be copied D:
	{
		RenderInfo convertedInfo = info;
		convertedInfo.view = GetViewMatrix() * info.view;
		convertedInfo.CalculateVP();
		GetViewport().SetOpenGLState(res);

		return convertedInfo;
	}

	void UICanvasActor::UnbindForRender(const Vec2u& res)
	{
		Viewport(Vec2u(0), res).SetOpenGLState();
	}

	UICanvasActor::~UICanvasActor()
	{
		Scene.EraseBlockingCanvas(*this);
	}

	void UICanvasActor::GetExternalButtons(std::vector<UIButtonActor*>& buttons) const
	{
		buttons = { ScrollBarX, ScrollBarY, BothScrollBarsButton, ResizeBarX, ResizeBarY };
	}

	std::string UICanvasActor::GetFullCanvasName() const
	{
		if (CanvasParent)
			return CanvasParent->GetFullCanvasName() + ">" + Name;
		return Name;
	}

	void UICanvasActor::CreateScrollBars()
	{
		ScrollBarX = &CreateChild<UIScrollBarActor>("ScrollBarX");
		ScrollBarX->SetTransform(Transform(Vec2f(0.0f, -1.10f), Vec2f(0.05f, 0.1f)));
		ScrollBarX->SetWhileBeingClickedFunc([this]() {
			ScrollView(Vec2f((glm::inverse(GetCanvasT()->GetWorldTransformMatrix()) * Vec4f(static_cast<float>(GameHandle->GetInputRetriever().GetMousePositionNDC().x) - ScrollBarX->GetClickPosNDC().x, 0.0f, 0.0f, 0.0f)).x * GetBoundingBox().Size.x, 0.0f));
			UpdateScrollBarT<VecAxis::X>();
			ScrollBarX->SetClickPosNDC(GameHandle->GetInputRetriever().GetMousePositionNDC());
			});

		ScrollBarY = &CreateChild<UIScrollBarActor>("ScrollBarY");
		ScrollBarY->SetTransform(Transform(Vec2f(1.10f, 0.0f), Vec2f(0.1f, 0.05f)));
		ScrollBarY->SetWhileBeingClickedFunc([this]() {
			ScrollView(Vec2f(0.0f, (glm::inverse(GetCanvasT()->GetWorldTransformMatrix()) * Vec4f(0.0f, static_cast<float>(GameHandle->GetInputRetriever().GetMousePositionNDC().y) - ScrollBarY->GetClickPosNDC().y, 0.0f, 0.0f)).y * GetBoundingBox().Size.y));
			UpdateScrollBarT<VecAxis::Y>();
			ScrollBarY->SetClickPosNDC(GameHandle->GetInputRetriever().GetMousePositionNDC());
			});

		BothScrollBarsButton = &CreateChild<UIScrollBarActor>("BothScrollBarsButton", nullptr, [this]() {
			if (ScrollBarX->GetState() == EditorIconState::IDLE)	ScrollBarX->OnBeingClicked();
			if (ScrollBarY->GetState() == EditorIconState::IDLE)	ScrollBarY->OnBeingClicked();

			ScrollBarX->WhileBeingClicked();
			ScrollBarY->WhileBeingClicked();
			});
		BothScrollBarsButton->SetTransform(Transform(Vec2f(1.10f, -1.10f), Vec2f(0.1f)));


		UpdateScrollBarT<VecAxis::X>();
		UpdateScrollBarT<VecAxis::Y>();


		ResizeBarX = &CreateChild<UIScrollBarActor>("ResizeBarX");
		ResizeBarX->SetTransform(Transform(Vec2f(0.0f, -1.2625f), Vec2f(1.0f, 0.0375f)));
		ResizeBarX->SetOnHoverFunc([this]() { GameHandle->SetCursorIcon(DefaultCursorIcon::VerticalResize); });
		ResizeBarX->SetOnUnhoverFunc([this]() { GameHandle->SetCursorIcon(DefaultCursorIcon::Regular); });
		ResizeBarX->SetWhileBeingClickedFunc([this]() {
			float scale = ResizeBarX->GetClickPosNDC().y - GameHandle->GetInputRetriever().GetMousePositionNDC().y;
			this->GetTransform()->SetScale((Vec2f)this->GetTransform()->GetScale() * (1.0f + scale));
			ResizeBarX->SetClickPosNDC(GameHandle->GetInputRetriever().GetMousePositionNDC());
			});

		EraseUIElement(*ScrollBarX);
		EraseUIElement(*ScrollBarY);
		EraseUIElement(*BothScrollBarsButton);
		EraseUIElement(*ResizeBarX);
		//EraseUIElement(*ResizeBarY);
	}

	template<VecAxis barAxis>
	inline void UICanvasActor::UpdateScrollBarT()
	{
		unsigned int axisIndex = static_cast<unsigned int>(barAxis);
		Boxf<Vec2f> canvasBBox = GetBoundingBox();

		float barSize = glm::min(1.0f, CanvasView.GetScale()[axisIndex] / canvasBBox.Size[axisIndex]);

		float viewMovePos = CanvasView.GetPos()[axisIndex] - CanvasView.GetScale()[axisIndex] - ((barAxis == VecAxis::X) ? (canvasBBox.GetLeft()) : (canvasBBox.GetBottom()));
		float viewMoveSize = (canvasBBox.Size[axisIndex] - CanvasView.GetScale()[axisIndex]) * 2.0f;

		float barPos = 0.0F;
		if (viewMoveSize > 0.0f)
		{
			barPos = (viewMoveSize > 0.0f) ? (viewMovePos / viewMoveSize) : (0.5f);	//0.0 -> 1.0
			barPos = barPos * 2.0f - 1.0f;	//-1.0 -> 1.0
			barPos /= 2.0f / (2.0f - barSize * 2.0f);	//-1.0 + barHalfExtent -> 1.0 - barHalfExtent
		}


		UIScrollBarActor* scrollBar = ((barAxis == VecAxis::X) ? (ScrollBarX) : (ScrollBarY));
		if (!scrollBar)
		{
			std::cerr << "ERROR! No scroll bar" << ((barAxis == VecAxis::X) ? (" X ") : (" Y ")) << "present.\n";
			return;
		}
		scrollBar->GetTransform()->SetVecAxis<TVec::POSITION, barAxis>(barPos);
		scrollBar->GetTransform()->SetVecAxis<TVec::SCALE, barAxis>(barSize);

		bool hide = floatComparison(barSize, 1.0f, std::numeric_limits<float>().epsilon());	//hide if we can't scroll on this axis
		scrollBar->GetButtonModel()->SetHide(hide);
		scrollBar->SetDisableInput(hide);

		// If the both scroll bars button exists and there are no visible scroll bars, hide the button.
		if (BothScrollBarsButton)
		{
			bool hideButton = (!ScrollBarX || ScrollBarX->GetButtonModel()->GetHide()) || (!ScrollBarY || ScrollBarY->GetButtonModel()->GetHide());
			BothScrollBarsButton->GetButtonModel()->SetHide(hideButton);
			BothScrollBarsButton->SetDisableInput(hideButton);
		}

	}

	template void UICanvasActor::UpdateScrollBarT<VecAxis::X>();
	template void UICanvasActor::UpdateScrollBarT<VecAxis::Y>();

	UICanvasField& AddFieldToCanvas(const std::string& name, UICanvasElement& element, std::function<Vec3f()> getFieldOffsetFunc)
	{
		return element.GetCanvasPtr()->AddField(name, getFieldOffsetFunc);
	}

	EditorDescriptionBuilder::EditorDescriptionBuilder(EditorManager& editorHandle, UICanvasFieldCategory& cat):
		EditorDescriptionBuilder(editorHandle, cat, *cat.GetCanvasPtr())
	{
		OptionalCategory = &cat;
	}

	EditorDescriptionBuilder::EditorDescriptionBuilder(EditorManager& editorHandle, UIActorDefault& descriptionParent) :
		EditorDescriptionBuilder(editorHandle, descriptionParent, *descriptionParent.GetCanvasPtr())
	{
	}

	EditorDescriptionBuilder::EditorDescriptionBuilder(EditorManager& editorHandle, Actor& descriptionParent, UICanvas& canvas) :
		EditorHandle(editorHandle), EditorScene(descriptionParent.GetScene()), DescriptionParent(descriptionParent), CanvasRef(canvas), OptionalCategory(nullptr)
	{
	}

	GameScene& EditorDescriptionBuilder::GetEditorScene()
	{
		return EditorScene;
	}

	UICanvas& EditorDescriptionBuilder::GetCanvas()
	{
		return CanvasRef;
	}

	Actor& EditorDescriptionBuilder::GetDescriptionParent()
	{
		return DescriptionParent;
	}

	EditorManager& EditorDescriptionBuilder::GetEditorHandle()
	{
		return EditorHandle;
	}

	UICanvasField& EditorDescriptionBuilder::AddField(const std::string& name, std::function<Vec3f()> getFieldOffsetFunc)
	{
		return (OptionalCategory) ? (OptionalCategory->AddField(name, getFieldOffsetFunc)) : (GetCanvas().AddField(name, getFieldOffsetFunc));
	}

	UICanvasFieldCategory& EditorDescriptionBuilder::AddCategory(const std::string& name)
	{
		return GetCanvas().AddCategory(name);
	}

	void EditorDescriptionBuilder::SelectComponent(Component* comp)
	{
		EditorHandle.SelectComponent(comp, EditorScene);
	}

	void EditorDescriptionBuilder::SelectActor(Actor* actor)
	{
		EditorHandle.SelectActor(actor, EditorScene);
	}

	void EditorDescriptionBuilder::RefreshComponent()
	{
		SelectComponent(EditorHandle.GetSelectedComponent());
	}

	void EditorDescriptionBuilder::RefreshScene()
	{
		EditorHandle.SelectScene(EditorHandle.GetSelectedScene(), EditorScene);
	}

	void EditorDescriptionBuilder::DeleteDescription()
	{
		for (auto& it : DescriptionParent.GetChildren())
			it->MarkAsKilled();
	}

}