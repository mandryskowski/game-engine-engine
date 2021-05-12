#include <UI/UICanvasActor.h>
#include <math/Transform.h>
#include <scene/UIButtonActor.h>
#include <input/InputDevicesStateRetriever.h>
#include <scene/ModelComponent.h>
#include <UI/UICanvasField.h>
#include <UI/UIListActor.h>

UICanvasActor::UICanvasActor(GameScene& scene, const std::string& name):
	Actor(scene, name),
	FieldsList(nullptr),
	FieldSize(glm::vec3(0.5f)),
	ScrollBarX(nullptr),
	ScrollBarY(nullptr),
	ResizeBarX(nullptr),
	ResizeBarY(nullptr),
	ScaleActor(nullptr)
{
}

UICanvasActor::UICanvasActor(UICanvasActor&& canvasActor):
	Actor(std::move(canvasActor)),
	UICanvas(std::move(canvasActor)),
	FieldsList(canvasActor.FieldsList),
	FieldSize(canvasActor.FieldSize),
	ScrollBarX(canvasActor.ScrollBarX),
	ScrollBarY(canvasActor.ScrollBarY),
	ResizeBarX(canvasActor.ResizeBarX),
	ResizeBarY(canvasActor.ResizeBarY),
	ScaleActor(canvasActor.ScaleActor)
{
}

void UICanvasActor::OnStart()
{
	Actor::OnStart();
	CreateScrollBars();
	ScaleActor = &CreateChild<UIActor>(Name + " scale actor");
	AddUIElement(*ScaleActor);
}

UIActor* UICanvasActor::GetScaleActor()
{
	return ScaleActor;
}

glm::mat4 UICanvasActor::GetView() const
{
	return UICanvas::GetView() * glm::inverse(GetTransform()->GetWorldTransformMatrix());
}

NDCViewport UICanvasActor::GetViewport() const
{
	return NDCViewport(GetTransform()->GetWorldTransform().PositionRef - GetTransform()->GetWorldTransform().ScaleRef, GetTransform()->GetWorldTransform().ScaleRef);
}

const Transform* UICanvasActor::GetCanvasT() const
{
	return GetTransform();
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

void UICanvasActor::HideScrollBars()
{
	ScrollBarX->MarkAsKilled();
	ScrollBarY->MarkAsKilled();
	BothScrollBarsButton->MarkAsKilled();

	ScrollBarX = ScrollBarY = BothScrollBarsButton = nullptr;
}

void UICanvasActor::ClampViewToElements()
{
	UICanvas::ClampViewToElements();

	if (ScrollBarX)	UpdateScrollBarT<VecAxis::X>();
	if (ScrollBarY) UpdateScrollBarT<VecAxis::Y>();
}

UICanvasFieldCategory& UICanvasActor::AddCategory(const std::string& name)
{
	if (!FieldsList)
		FieldsList = &ScaleActor->CreateChild<UIListActor>(Name + "_Fields_List");

	UICanvasFieldCategory& category = FieldsList->CreateChild<UICanvasFieldCategory>(name);
	category.SetTransform(Transform(glm::vec2(0.0f), Vec2f(FieldSize)));	//position doesn't matter if this is not the first field
	FieldsList->AddElement(UIListElement(category, [&category]() { category.Refresh(); return category.GetListOffset(); }, [&category]() { return Vec3f(0.0f, -1.5f, 0.0f); }));
	(this->*static_cast<void(UICanvas::*)(UIActor&)>(&UICanvas::AddUIElement))(category);
	category.SetOnExpansionFunc([this]() { RefreshFieldsList(); });

	return category;
}

UICanvasField& UICanvasActor::AddField(const std::string& name, std::function<glm::vec3()> getElementOffset)
{
	if (!FieldsList)
		FieldsList = &ScaleActor->CreateChild<UIListActor>(Name + "_Fields_List");

	UICanvasField& field = FieldsList->CreateChild<UICanvasField>(name);
	(getElementOffset) ? (FieldsList->AddElement(UIListElement(field, getElementOffset))) : (FieldsList->AddElement(UIListElement(field, glm::vec3(0.0f, -FieldSize.y * 2.0f, 0.0f))));

	field.SetTransform(Transform(glm::vec2(0.0f), glm::vec2(FieldSize)));	//position doesn't matter if this is not the first field
	(this->*static_cast<void(UICanvas::*)(UIActor&)>(&UICanvas::AddUIElement))(field);


	return field;
}

void UICanvasActor::HandleEvent(const Event& ev)
{
	if (ev.GetType() != EventType::MOUSE_SCROLLED || !Boxf<Vec2f>(GetTransform()->GetWorldTransform()).Contains(GameHandle->GetInputRetriever().GetMousePositionNDC()))
		return;

	const MouseScrollEvent& scrolledEv = dynamic_cast<const MouseScrollEvent&>(ev);

	if (GameHandle->GetInputRetriever().IsKeyPressed(Key::LEFT_CONTROL))
	{
		glm::vec2 newScale = static_cast<glm::vec2>(CanvasView.ScaleRef) * glm::pow(glm::vec2(0.5f), glm::vec2(scrolledEv.GetOffset().y));
		printVector(newScale, "new scale");
		SetViewScale(newScale);
		if (ScrollBarX)	UpdateScrollBarT<VecAxis::X>();
		if (ScrollBarY) UpdateScrollBarT<VecAxis::Y>();
	}
	else
		ScrollView(scrolledEv.GetOffset());
}

RenderInfo UICanvasActor::BindForRender(const RenderInfo& info, const glm::uvec2& res)	//Note: RenderInfo is a relatively big object for real-time standards. HOPEFULLY the compiler will optimize here using NRVO and convertedInfo won't be copied D:
{
	RenderInfo convertedInfo = info;
	convertedInfo.view = GetView();
	convertedInfo.projection = GetProjection();
	convertedInfo.CalculateVP();
	GetViewport().SetOpenGLState(res);

	return convertedInfo;
}

void UICanvasActor::UnbindForRender(const glm::uvec2& res)
{
	Viewport(glm::uvec2(0), res).SetOpenGLState();
}

void UICanvasActor::CreateScrollBars()
{
	Material* markerMaterial = new Material("MarkerMaterial", 0.0f, GameHandle->GetRenderEngineHandle()->FindShader("Forward_NoLight"));
	markerMaterial->SetColor(glm::vec4(1.0f, 1.0f, 0.1f, 1.0f));
	Actor& marker = CreateChild<Actor>("Gowno");
	ModelComponent& markerModel = marker.CreateComponent<ModelComponent>("Gownomodel");
	//markerModel.AddMeshInst(MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), markerMaterial));

	ScrollBarX = &CreateChild<UIScrollBarActor>("ScrollBarX");
	ScrollBarX->SetTransform(Transform(glm::vec2(0.0f, -1.10f), glm::vec2(0.05f, 0.1f)));
	ScrollBarX->SetWhileBeingClickedFunc([this]() {
		ScrollView(glm::vec2((glm::inverse(GetCanvasT()->GetWorldTransformMatrix()) * glm::vec4(static_cast<float>(GameHandle->GetInputRetriever().GetMousePositionNDC().x) - ScrollBarX->GetClickPosNDC().x, 0.0f, 0.0f, 0.0f)).x * GetBoundingBox().Size.x, 0.0f));
		UpdateScrollBarT<VecAxis::X>();
		ScrollBarX->SetClickPosNDC(GameHandle->GetInputRetriever().GetMousePositionNDC());
		});

	ScrollBarY = &CreateChild<UIScrollBarActor>("ScrollBarY");
	ScrollBarY->SetTransform(Transform(glm::vec2(1.10f, 0.0f), glm::vec2(0.1f, 0.05f)));
	ScrollBarY->SetWhileBeingClickedFunc([this]() {
		ScrollView(glm::vec2(0.0f, (glm::inverse(GetCanvasT()->GetWorldTransformMatrix()) * glm::vec4(0.0f, static_cast<float>(GameHandle->GetInputRetriever().GetMousePositionNDC().y) - ScrollBarY->GetClickPosNDC().y, 0.0f, 0.0f)).y * GetBoundingBox().Size.y));
		UpdateScrollBarT<VecAxis::Y>();
		ScrollBarY->SetClickPosNDC(GameHandle->GetInputRetriever().GetMousePositionNDC());
		});

	BothScrollBarsButton = &CreateChild<UIScrollBarActor>("BothScrollBarsButton", nullptr, [this]() {
		if (ScrollBarX->GetState() == EditorIconState::IDLE)	ScrollBarX->OnBeingClicked();
		if (ScrollBarY->GetState() == EditorIconState::IDLE)	ScrollBarY->OnBeingClicked();

		ScrollBarX->WhileBeingClicked();
		ScrollBarY->WhileBeingClicked();
		});
	BothScrollBarsButton->SetTransform(Transform(glm::vec2(1.10f, -1.10f), glm::vec2(0.1f)));


	UpdateScrollBarT<VecAxis::X>();
	UpdateScrollBarT<VecAxis::Y>();


	ResizeBarX = &CreateChild<UIScrollBarActor>("ResizeBarX");
	ResizeBarX->SetTransform(Transform(glm::vec2(0.0f, -1.2625f), glm::vec2(1.0f, 0.0375f)));
	ResizeBarX->SetWhileBeingClickedFunc([this]() {
		float scale = ResizeBarX->GetClickPosNDC().y - GameHandle->GetInputRetriever().GetMousePositionNDC().y;
		this->GetTransform()->SetScale((glm::vec2)this->GetTransform()->ScaleRef * (1.0f + scale));
		ResizeBarX->SetClickPosNDC(GameHandle->GetInputRetriever().GetMousePositionNDC());
		});
}

template<VecAxis barAxis>
inline void UICanvasActor::UpdateScrollBarT()
{
	unsigned int axisIndex = static_cast<unsigned int>(barAxis);
	Boxf<Vec2f> canvasBBox = GetBoundingBox();

	float barSize = glm::min(1.0f, glm::pow(CanvasView.ScaleRef[axisIndex], 2.0f) / canvasBBox.Size[axisIndex]);

	float viewMovePos = CanvasView.PositionRef[axisIndex] - glm::pow(CanvasView.ScaleRef[axisIndex], 2.0f) - ((barAxis == VecAxis::X) ? (canvasBBox.GetLeft()) : (canvasBBox.GetBottom()));
	float viewMoveSize = (canvasBBox.Size[axisIndex] - glm::pow(CanvasView.ScaleRef[axisIndex], 2.0f)) * 2.0f;

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

	bool hide = barSize == 1.0f;	//hide if we can't scroll on this axis
	((barAxis == VecAxis::X) ? (ScrollBarX) : (ScrollBarY))->GetButtonModel()->SetHide(hide);
}

template void UICanvasActor::UpdateScrollBarT<VecAxis::X>();
template void UICanvasActor::UpdateScrollBarT<VecAxis::Y>();

#include <UI/UICanvasField.h>

UICanvasField& AddFieldToCanvas(const std::string& name, UICanvasElement& element, std::function<glm::vec3()> getFieldOffsetFunc)
{
	return element.GetCanvasPtr()->AddField(name, getFieldOffsetFunc);
}

EditorDescriptionBuilder::EditorDescriptionBuilder(EditorManager& editorHandle, UIActor& descriptionParent):
	EditorDescriptionBuilder(editorHandle, descriptionParent, *descriptionParent.GetCanvasPtr())
{
}

EditorDescriptionBuilder::EditorDescriptionBuilder(EditorManager& editorHandle, Actor& descriptionParent, UICanvas& canvas):
	EditorHandle(editorHandle), EditorScene(descriptionParent.GetScene()), DescriptionParent(descriptionParent), CanvasRef(canvas)
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

UICanvasField& EditorDescriptionBuilder::AddField(const std::string& name, std::function<glm::vec3()> getFieldOffsetFunc)
{
	return GetCanvas().AddField(name, getFieldOffsetFunc);
}

void EditorDescriptionBuilder::SelectComponent(Component* comp)
{
	EditorHandle.SelectComponent(comp, EditorScene);
}

void EditorDescriptionBuilder::SelectActor(Actor* actor)
{
	EditorHandle.SelectActor(actor, EditorScene);
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
