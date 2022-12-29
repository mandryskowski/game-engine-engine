#include <UI/UICanvasActor.h>
#include <game/GameScene.h>
#include <math/Transform.h>
#include <scene/UIButtonActor.h>
#include <input/InputDevicesStateRetriever.h>
#include <scene/ModelComponent.h>
#include <UI/UICanvasField.h>
#include <UI/UIListActor.h>
#include <editor/EditorActions.h>

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
		CanvasBackground(nullptr),
		CanvasParent(canvasParent),
		PopupCreationFunc(nullptr)
	{
		Scene.GetUIData()->AddBlockingCanvas(*this);
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
		CanvasParent(canvasActor.CanvasParent),
		PopupCreationFunc(canvasActor.PopupCreationFunc)
	{
		FieldsList = dynamic_cast<UIListActor*>(canvasActor.FindActor(Name + "_Fields_List"));
		Scene.GetUIData()->AddBlockingCanvas(*this);
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

	void UICanvasActor::SetPopupCreationFunc(std::function<void(PopupDescription)> popupCreation)
	{
		PopupCreationFunc = popupCreation;
	}

	void UICanvasActor::SetCanvasView(const Transform& canvasView, bool clampView)
	{
		CanvasView = canvasView;
		if (clampView) ClampViewToElements();
		if (ScrollBarX) UpdateScrollBarT<VecAxis::X>();
		if (ScrollBarY) UpdateScrollBarT<VecAxis::Y>();

		PushCanvasViewChangeEvent();
	}

	ModelComponent* UICanvasActor::GetCanvasBackground()
	{
		return CanvasBackground;
	}

	Vec2f UICanvasActor::FromCanvasSpace(const Vec2f& canvasSpacePos) const
	{
		return GetCanvasT()->GetWorldTransformMatrix() * Vec4f(canvasSpacePos, 0.0f, 1.0f);
	}

	Vec2f UICanvasActor::ScaleFromCanvasSpace(const Vec2f& canvasSpaceScale) const
	{
		return (Mat2f)GetCanvasT()->GetWorldTransformMatrix() * canvasSpaceScale;
	}

	Mat4f UICanvasActor::FromCanvasSpace(const Mat4f& canvasSpaceMat) const
	{
		return GetCanvasT()->GetWorldTransformMatrix() * canvasSpaceMat;
	}

	Transform UICanvasActor::FromCanvasSpace(const Transform& canvasSpaceTransform) const
	{
		return GetCanvasT()->GetWorldTransform() * canvasSpaceTransform;
	}

	Vec2f UICanvasActor::ToCanvasSpace(const Vec2f& worldPos) const
	{
		return Math::SafeInverseMatrix(GetCanvasT()->GetWorldTransformMatrix()) * Vec4f(worldPos, 0.0f, 1.0f);
	}

	Mat4f UICanvasActor::ToCanvasSpace(const Mat4f& worldMat) const
	{
		return Math::SafeInverseMatrix(GetCanvasT()->GetWorldTransformMatrix()) * worldMat;
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

	void UICanvasActor::CreateCanvasBackgroundModel(const Vec3f& color)
	{
		if (CanvasBackground)
			CanvasBackground->MarkAsKilled();
		CanvasBackground = &CreateComponent<ModelComponent>("WindowBackground");
		CanvasBackground->AddMeshInst(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad));

		if (color == Vec3f(-1.0f))
			CanvasBackground->OverrideInstancesMaterial(GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_E_Canvas_Background_Material"));
		else
		{
			CanvasBackground->OverrideInstancesMaterial(GameHandle->GetRenderEngineHandle()->AddMaterial(MakeShared<Material>("CanvasBackgroundMaterial", color, MaterialShaderHint::Simple)));
		}

		AddTopLevelUIElement(*CanvasBackground);	//Add the background model as a UIElement to give it UIDepth
		CanvasBackground->DetachFromCanvas();	//Erase it immediately because we do not want it to be an element of the canvas, but its background
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
		FieldsList->AddElement(UIListElement(UIListElement::ReferenceToUIActor(&category), [&category]() { category.Refresh(); return category.GetListOffset(); }, [&category]() { return Vec3f(0.0f, -1.5f, 0.0f); }));
		category.SetOnExpansionFunc([this]() { RefreshFieldsList(); });

		return category;
	}

	UICanvasField& UICanvasActor::AddField(const std::string& name, std::function<Vec3f()> getElementOffset)
	{
		if (!FieldsList)
			FieldsList = &ScaleActor->CreateChild<UIListActor>(Name + "_Fields_List");

		UICanvasField& field = FieldsList->CreateChild<UICanvasField>(name);
		(getElementOffset) ? (FieldsList->AddElement(UIListElement(UIListElement::ReferenceToUIActor(&field), getElementOffset))) : (FieldsList->AddElement(UIListElement(UIListElement::ReferenceToUIActor(&field), Vec3f(0.0f, -FieldSize.y * 2.0f, 0.0f))));

		field.SetTransform(Transform(Vec2f(0.0f), Vec2f(FieldSize)));	//position doesn't matter if this is not the first field


		return field;
	}

	void UICanvasActor::HandleEvent(const Event& ev)
	{
		if (Scene.GetUIData()->GetCurrentBlockingCanvas() != this && (ev.GetType() != EventType::WindowResized))
			return;

		Actor::HandleEvent(ev);

		switch (ev.GetType())
		{
			case EventType::MouseScrolled:
			{
				const MouseScrollEvent& scrolledEv = dynamic_cast<const MouseScrollEvent&>(ev);

				if (GameHandle->GetDefInputRetriever().IsKeyPressed(Key::LeftControl))
				{
					Vec2f newScale = static_cast<Vec2f>(CanvasView.GetScale()) * glm::pow(Vec2f(0.5f), Vec2f(scrolledEv.GetOffset().y));
					printVector(newScale, "new scale");
					SetViewScale(newScale);
				}
				else
					ScrollView(scrolledEv.GetOffset());
				break;
			}
			case EventType::MouseReleased:
			{
				const MouseButtonEvent& buttonEv = dynamic_cast<const MouseButtonEvent&>(ev);
				
				if (buttonEv.GetButton() == MouseButton::Right && PopupCreationFunc)
					dynamic_cast<Editor::EditorManager*>(GameHandle)->RequestPopupMenu(Scene.GetUIData()->GetWindowData().GetMousePositionNDC(), *Scene.GetUIData()->GetWindow(), PopupCreationFunc);
				break;
			}
			case EventType::KeyReleased:
			{
				const KeyEvent& keyEv = dynamic_cast<const KeyEvent&>(ev);
				if (keyEv.GetKeyCode() == Key::F && keyEv.GetModifierBits() & KeyModifierFlags::Control)
				{
					auto& searchBar = CreateChild<UIInputBoxActor>("SearchBar", [](const std::string&) {}, []() { return "search..."; });
					searchBar.SetMatIdle(MakeShared<Material>("SearchBarMaterial", hsvToRgb(Vec3f(220.0f, 0.53f, 0.4f))));
					searchBar.SetTransform(Transform(Vec2f(0.0f), Vec2f(0.6f, 0.6f / 4.0f)));
					searchBar.DetachFromCanvas();
					searchBar.CallOnClickFunc();

					{
						auto searchBarText = dynamic_cast<TextComponent*>(searchBar.GetContentTextComp());
						searchBarText->GetTransform().Move(Vec2f(0.25f, 0.0f));
						searchBarText->SetMaxSize(Vec2f(0.75f, 1.0f) * 0.6f);
						searchBarText->Unstretch();
						searchBarText->SetMaterialInst(MakeShared<Material>("SearchBarTextMaterial", Vec4f(1.0f, 1.0f, 1.0f, 0.3f)));
					}

					

					auto& searchIcon = searchBar.GetRoot()->CreateComponent<ModelComponent>("SearchIcon");
					auto& renderHandle = *GameHandle->GetRenderEngineHandle();
					auto iconsMaterial = renderHandle.FindMaterial<AtlasMaterial>("GEE_E_Icons");
					searchIcon.AddMeshInst(MeshInstance(renderHandle.GetBasicShapeMesh(EngineBasicShape::Quad), MakeShared<MaterialInstance>(iconsMaterial, iconsMaterial->GetTextureIDInterpolatorTemplate(9.0f))));
					AddTopLevelUIElement(searchIcon);
					searchIcon.DetachFromCanvas();
					searchIcon.SetTransform(Transform(Vec2f(-0.75f, 0.0f), Vec2f(1.0f / 4.0f, 1.0f)));
				}
			}
		}
	}

	void UICanvasActor::HandleEventAll(const Event& ev)
	{
		Actor::HandleEventAll(ev);

		/*bool bContainsMouse = ContainsMouse(GameHandle->GetDefInputRetriever().GetMousePositionNDC());
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
					if (button->GetState() != EditorIconState::Idle)
						focusedOnThis = true;
				}
		}*/
	}

	SceneMatrixInfo UICanvasActor::BindForRender(const SceneMatrixInfo& info)	//Note: RenderInfo is a relatively big object for real-time standards. HOPEFULLY the compiler will optimize here using NRVO and convertedInfo won't be copied D:
	{
		SceneMatrixInfo convertedInfo = info;
		convertedInfo.SetView(GetViewMatrix() * info.GetView());
		convertedInfo.CalculateVP();
		GEE_CORE_ASSERT(Scene.GetUIData()->GetWindow());
		GetViewport().SetOpenGLState(Scene.GetUIData()->GetWindowData().GetWindowSize());

		return convertedInfo;
	}

	void UICanvasActor::UnbindForRender()
	{
		GEE_CORE_ASSERT(Scene.GetUIData()->GetWindow());
		Viewport(Vec2u(0), Scene.GetUIData()->GetWindowData().GetWindowSize()).SetOpenGLState();
	}

	UICanvasActor::~UICanvasActor()
	{
		Scene.GetUIData()->EraseBlockingCanvas(*this);
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

	void UICanvasActor::PushCanvasViewChangeEvent()
	{
		Scene.GetEventPusher().PushEvent(Event(EventType::CanvasViewChanged, this));
	}

	void UICanvasActor::CreateScrollBars()
	{
		ScrollBarX = &CreateChild<UIScrollBarActor>("ScrollBarX");
		ScrollBarX->SetTransform(Transform(Vec2f(0.0f, -1.10f), Vec2f(0.05f, 0.1f)));
		ScrollBarX->SetWhileBeingClickedFunc([this]() {
			ScrollView(Vec2f((glm::inverse(GetCanvasT()->GetWorldTransformMatrix()) * Vec4f(static_cast<float>(Scene.GetUIData()->GetWindowData().GetMousePositionNDC().x) - ScrollBarX->GetClickPosNDC().x, 0.0f, 0.0f, 0.0f)).x * GetBoundingBox().Size.x, 0.0f));
			UpdateScrollBarT<VecAxis::X>();
			ScrollBarX->SetClickPosNDC(Scene.GetUIData()->GetWindowData().GetMousePositionNDC());
			});

		ScrollBarY = &CreateChild<UIScrollBarActor>("ScrollBarY");
		ScrollBarY->SetTransform(Transform(Vec2f(1.10f, 0.0f), Vec2f(0.1f, 0.05f)));
		ScrollBarY->SetWhileBeingClickedFunc([this]() {
			ScrollView(Vec2f(0.0f, (glm::inverse(GetCanvasT()->GetWorldTransformMatrix()) * Vec4f(0.0f, static_cast<float>(Scene.GetUIData()->GetWindowData().GetMousePositionNDC().y) - ScrollBarY->GetClickPosNDC().y, 0.0f, 0.0f)).y * GetBoundingBox().Size.y));
			UpdateScrollBarT<VecAxis::Y>();
			ScrollBarY->SetClickPosNDC(Scene.GetUIData()->GetWindowData().GetMousePositionNDC());
			});

		BothScrollBarsButton = &CreateChild<UIScrollBarActor>("BothScrollBarsButton", nullptr, [this]() {
			if (ScrollBarX->GetStateBits() == 0)	ScrollBarX->OnBeingClicked();
			if (ScrollBarY->GetStateBits() == 0)	ScrollBarY->OnBeingClicked();

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
			float scale = ResizeBarX->GetClickPosNDC().y - Scene.GetUIData()->GetWindowData().GetMousePositionNDC().y;
			this->GetTransform()->SetScale((Vec2f)this->GetTransform()->GetScale() * (1.0f + scale));
			ResizeBarX->SetClickPosNDC(Scene.GetUIData()->GetWindowData().GetMousePositionNDC());
			});

		ScrollBarX->DetachFromCanvas();
		ScrollBarY->DetachFromCanvas();
		BothScrollBarsButton->DetachFromCanvas();
		ResizeBarX->DetachFromCanvas();
		//ResizeBarY->DetachFromCanvas();
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
		scrollBar->GetTransform()->SetVecAxis<TVec::Position, barAxis>(barPos);
		scrollBar->GetTransform()->SetVecAxis<TVec::Scale, barAxis>(barSize);

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

	EditorDescriptionBuilder::EditorDescriptionBuilder(Editor::EditorManager& editorHandle, UICanvasFieldCategory& cat):
		EditorDescriptionBuilder(editorHandle, cat, *cat.GetCanvasPtr())
	{
		OptionalCategory = &cat;
	}

	EditorDescriptionBuilder::EditorDescriptionBuilder(Editor::EditorManager& editorHandle, UIActorDefault& descriptionParent) :
		EditorDescriptionBuilder(editorHandle, descriptionParent, *descriptionParent.GetCanvasPtr())
	{
	}

	EditorDescriptionBuilder::EditorDescriptionBuilder(Editor::EditorManager& editorHandle, Actor& descriptionParent, UICanvas& canvas) :
		EditorHandle(editorHandle), EditorScene(descriptionParent.GetScene()), DescriptionParent(descriptionParent), CanvasRef(canvas), OptionalCategory(nullptr), DeleteFunction(nullptr)
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

	Editor::EditorManager& EditorDescriptionBuilder::GetEditorHandle()
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
		EditorHandle.GetActions().SelectComponent(comp, EditorScene);
	}

	void EditorDescriptionBuilder::SelectActor(Actor* actor)
	{
		EditorHandle.GetActions().SelectActor(actor, EditorScene);
	}

	void EditorDescriptionBuilder::RefreshComponent()
	{
		SelectComponent(EditorHandle.GetActions().GetSelectedComponent());
	}

	void EditorDescriptionBuilder::RefreshActor()
	{
		SelectActor(EditorHandle.GetActions().GetSelectedActor());
	}

	void EditorDescriptionBuilder::RefreshScene()
	{
		EditorHandle.GetActions().SelectScene(EditorHandle.GetActions().GetSelectedScene(), EditorScene);
	}

	void EditorDescriptionBuilder::DeleteDescription()
	{
		for (auto& it : DescriptionParent.GetChildren())
			it->MarkAsKilled();
	}

	ComponentDescriptionBuilder::ComponentDescriptionBuilder(Editor::EditorManager& editorHandle, UICanvasFieldCategory& category):
		EditorDescriptionBuilder(editorHandle, category),
		OptionalBuiltNode(nullptr)
	{
	}

	ComponentDescriptionBuilder::ComponentDescriptionBuilder(Editor::EditorManager& editorHandle, UIActorDefault& actor):
		EditorDescriptionBuilder(editorHandle, actor),
		OptionalBuiltNode(nullptr)
	{
	}

	ComponentDescriptionBuilder::ComponentDescriptionBuilder(Editor::EditorManager& editorHandle, Actor& actor, UICanvas& canvas):
		EditorDescriptionBuilder(editorHandle, actor, canvas),
		OptionalBuiltNode(nullptr)
	{
	}

	void ComponentDescriptionBuilder::Refresh()
	{
		auto actions = GetEditorHandle().GetActions();
		if (IsNodeBeingBuilt())
			actions.PreviewHierarchyNode(*GetNodeBeingBuilt(), GetEditorScene());
		else
			actions.SelectComponent(actions.GetSelectedComponent(), GetEditorScene());
	}

	bool ComponentDescriptionBuilder::IsNodeBeingBuilt() const
	{
		return OptionalBuiltNode != nullptr;
	}

	void ComponentDescriptionBuilder::SetNodeBeingBuilt(Hierarchy::NodeBase* node)
	{
		OptionalBuiltNode = node;
	}

	Hierarchy::NodeBase* ComponentDescriptionBuilder::GetNodeBeingBuilt()
	{
		return OptionalBuiltNode;
	}

}