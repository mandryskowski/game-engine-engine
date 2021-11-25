#include <editor/EditorActions.h>
#include <UI/UIListActor.h>
#include <scene/UIButtonActor.h>
#include <UI/UICanvasActor.h>
#include <scene/TextComponent.h>
#include <UI/UICanvasField.h>

#include <scene/LightComponent.h>
#include <scene/SoundSourceComponent.h>
#include <scene/CameraComponent.h>
#include <scene/LightProbeComponent.h>




#include <scene/ModelComponent.h>
#include <editor/EditorManager.h>
namespace GEE
{
	PopupDescription::PopupDescription(SystemWindow& window, UIAutomaticListActor* actor, UICanvasActor* canvas):
		Window(window),
		DescriptionRoot(actor),
		PopupCanvas(canvas),
		bViewComputed(false),
		LastComputedView(Transform()),
		PopupOptionSizePx(Vec2i(240, 30))
	{
		SharedPtr<Material> secondaryMat;
		if ((secondaryMat = actor->GetGameHandle()->GetRenderEngineHandle()->FindMaterial("GEE_E_Popup_Secondary")) == nullptr)
		{
			secondaryMat = MakeShared<Material>("GEE_E_Popup_Secondary");
			secondaryMat->SetColor(Vec3f(0.5f, 0.3f, 0.5f));
			secondaryMat->SetRenderShaderName("Forward_NoLight");
			actor->GetGameHandle()->GetRenderEngineHandle()->AddMaterial(secondaryMat);
		}
		PopupCanvas->CreateCanvasBackgroundModel(Vec3f(0.5f, 0.3f, 0.5f));
	}
	void PopupDescription::AddOption(const std::string& buttonContent, std::function<void()> buttonFunc, const IconData& optionalIconData)
	{
		auto& button = DescriptionRoot->CreateChild<UIButtonActor>("Button_" + buttonContent, buttonContent, 
		[window = &Window, buttonFunc]()
		{
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			if (buttonFunc)
				buttonFunc();
		}, Transform(Vec2f(0.0f), Vec2f(0.9f)));

		if (optionalIconData.IsValid())
		{
			auto& iconModel = button.CreateComponent<ModelComponent>("GEE_E_Button_Icon", Transform(Vec2f(-0.9f, 0.0f), Vec2f(0.1f, 0.8f)));
			iconModel.AddMeshInst(MeshInstance(DescriptionRoot->GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), optionalIconData.GetMatInstance()));
		}

		DescriptionRoot->Refresh();
		PopupCanvas->AutoClampView(true, true);
		//button.GetRoot()->GetComponent<TextConstantSizeComponent>("ButtonText")->Unstretch(UISpace::Window);
	}
	PopupDescription PopupDescription::AddSubmenu(const std::string& buttonContent, std::function<void(PopupDescription)> prepareSubmenu, const IconData& optionalIconData)
	{
		int optionIndex = DescriptionRoot->GetListElementCount();

		auto createSubPopup = [gameHandle = DescriptionRoot->GetGameHandle(), optionIndex, *this, prepareSubmenu, window = &Window]() mutable
		{

			Vec2f submenuPos(1.0f, 1.0f);
			auto t = PopupCanvas->FromCanvasSpace(PopupCanvas->GetViewT().GetInverse() * PopupCanvas->ToCanvasSpace(DescriptionRoot->GetListElement(optionIndex).GetActorRef().GetTransform()->GetWorldTransform()));
			submenuPos = t.GetPos2D() + t.GetScale2D();
		
			PopupDescription desc = dynamic_cast<Editor::EditorManager*>(gameHandle)->CreatePopupMenu(submenuPos, *window);
			prepareSubmenu(desc);
			desc.RefreshPopup();
		};
		auto& button = DescriptionRoot->CreateChild<UIButtonActor>("Button_" + buttonContent, buttonContent, nullptr, Transform(Vec2f(0.0f), Vec2f(0.9f)));


		button.SetOnHoverFunc([window = &Window, createSubPopup]() mutable
		{
			createSubPopup();
			glfwSetWindowShouldClose(window, false);	// do not close the parent popup
		});
		button.SetOnUnhoverFunc([buttonPtr = &button, window = &Window]() { if (GeomTests::NDCContains(buttonPtr->GetScene().GetUIData()->GetWindowData().GetMousePositionNDC())) glfwFocusWindow(window); });

		auto& visualCue = button.GetRoot()->CreateComponent<ModelComponent>("GEE_E_Submenu_Triangle", Transform(Vec2f(0.9f, 0.0f), Vec2f(0.1f, 0.8f)));
		visualCue.AddMeshInst(visualCue.GetScene().GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
		//visualCue.OverrideInstancesMaterial(visualCue.GetScene().GetGameHandle()->GetRenderEngineHandle()->FindMaterial("GEE_E_Popup_Secondary").get());
		visualCue.OverrideInstancesMaterial(&IconData(*DescriptionRoot->GetGameHandle()->GetRenderEngineHandle(), "Assets/Editor/more.png").GetMatInstance()->GetMaterialRef());
			
		if (optionalIconData.IsValid())
		{
			auto& iconModel = button.CreateComponent<ModelComponent>("GEE_E_Button_Icon", Transform(Vec2f(-0.9f, 0.0f), Vec2f(0.1f, 0.8f)));
			iconModel.AddMeshInst(MeshInstance(DescriptionRoot->GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), optionalIconData.GetMatInstance()));
		}

		DescriptionRoot->Refresh();
		PopupCanvas->AutoClampView(true, true);
		return *this;
	}
	void PopupDescription::RefreshPopup()
	{
		ComputeView();
		glfwSetWindowSize(&Window, PopupOptionSizePx.x, static_cast<int>(static_cast<float>(PopupOptionSizePx.y) * DescriptionRoot->GetBoundingBoxIncludingChildren().Size.y));
		DescriptionRoot->HandleEventAll(Event(EventType::WindowResized));
	}
	void PopupDescription::ComputeView()
	{
		
	}
	IconData::IconData(RenderEngineManager& renderHandle, const std::string& texFilepath, Vec2i iconAtlasSize, float iconAtlasID):
		IconMaterial(nullptr),
		IconAtlasID(iconAtlasID)
	{
		if ((IconMaterial = renderHandle.FindMaterial("TEX:" + texFilepath)) == nullptr)
		{
			if (iconAtlasSize != Vec2i(0))
				IconMaterial = MakeShared<AtlasMaterial>("TEX:" + texFilepath, iconAtlasSize);
			else
				IconMaterial = MakeShared<Material>("TEX:" + texFilepath);

			IconMaterial->AddTexture(Texture::Loader<>::FromFile2D(texFilepath, Texture::Format::RGBA(), true, Texture::MinFilter::Bilinear()));
			IconMaterial->SetRenderShaderName("Forward_NoLight");
			renderHandle.AddMaterial(IconMaterial);
		}
	}
	bool IconData::IsValid() const
	{
		return IconMaterial != nullptr;
	}
	SharedPtr<MaterialInstance> IconData::GetMatInstance() const
	{
		GEE_CORE_ASSERT(IconMaterial);
		if (IconAtlasID >= 0.0f)
			if (auto materialCast = dynamic_cast<AtlasMaterial*>(IconMaterial.get()))
				return MakeShared<MaterialInstance>(*materialCast, materialCast->GetTextureIDInterpolatorTemplate(IconAtlasID));

		return MakeShared<MaterialInstance>(*IconMaterial);
	}

	namespace Editor
	{
		EditorActions::EditorActions(EditorManager& editorManager) :
			EditorHandle(editorManager),
			SelectedScene(nullptr),
			SelectedActor(nullptr),
			SelectedComp(nullptr)
		{
		}
		void EditorActions::SelectActor(Actor* actor, GameScene& editorScene)
		{
			if (actor && actor->IsBeingKilled())	//Components and Actors that are being killed should never be chosen - doing it is a perfect opportunity for runtime crashes to occur.
				actor = nullptr;

			GameManager& gameHandle = *editorScene.GetGameHandle();

			bool sameActor = SelectedActor == actor;
			SelectedActor = actor;
			SelectComponent(nullptr, editorScene);

			Transform previousCanvasView;
			if (const Actor* found = editorScene.GetRootActor()->FindActor("GEE_E_Actors_Components_Canvas"))
			{
				previousCanvasView = dynamic_cast<UICanvasActor*>(const_cast<Actor*>(found))->CanvasView;
				const_cast<Actor*>(found)->MarkAsKilled();
			}

			if (!actor)
				return;

			UICanvasActor& canvas = editorScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Actors_Components_Canvas", Transform(Vec2f(0.7f, 0.625f), Vec2f(0.3f, 0.375f)));
			canvas.KillResizeBars();
			//canvas.SetViewScale(Vec2f(1.0f, glm::sqrt(0.5f / 0.32f)));
			canvas.SetPopupCreationFunc([&](PopupDescription desc)
				{
					desc.AddSubmenu("Create component", [&](PopupDescription desc) { desc.AddOption("Component", nullptr, IconData(*gameHandle.GetRenderEngineHandle(), "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f)); desc.AddOption("Light", nullptr, IconData(*gameHandle.GetRenderEngineHandle(), "Assets/Editor/lightcomponent_icon.png", Vec2i(3, 1), 0.0f)); });
					desc.AddOption("Refresh", nullptr, IconData(*gameHandle.GetRenderEngineHandle(), "Assets/Editor/refresh_icon.png", Vec2i(3, 1), 0.0f));
					desc.AddOption("Delete", nullptr);
				});
			UIActorDefault* scaleActor = canvas.GetScaleActor();

			UICanvasField& componentsListField = canvas.AddField("List of components");
			UIAutomaticListActor& listActor = componentsListField.CreateChild<UIAutomaticListActor>("ListActor");
			canvas.FieldsList->SetListElementOffset(canvas.FieldsList->GetListElementCount() - 1, [&listActor, &componentsListField]() { return static_cast<Mat3f>(componentsListField.GetTransform()->GetMatrix()) * (listActor.GetListOffset()); });
			canvas.FieldsList->SetListCenterOffset(canvas.FieldsList->GetListElementCount() - 1, [&componentsListField]() -> Vec3f { return Vec3f(0.0f, -componentsListField.GetTransform()->GetScale().y, 0.0f); });
			listActor.GetTransform()->Move(Vec2f(0.5f, 0.0f));

			AddActorToList<Component>(editorScene, *actor->GetRoot(), listActor, canvas);

			listActor.Refresh();
			canvas.RefreshFieldsList();

			actor->GetEditorDescription(EditorDescriptionBuilder(EditorHandle, *scaleActor));

			if (canvas.FieldsList)
				canvas.FieldsList->Refresh();

			UIButtonActor& refreshButton = scaleActor->CreateChild<UIButtonActor>("GEE_E_Scene_Refresh_Button", [this, actor, &editorScene]() { this->SelectActor(actor, editorScene); }, Transform(Vec2f(6.0f, 0.0f), Vec2f(0.5f)));
			uiButtonActorUtil::ButtonMatsFromAtlas(refreshButton, *dynamic_cast<AtlasMaterial*>(gameHandle.GetRenderEngineHandle()->FindMaterial("GEE_E_Refresh_Icon_Mat").get()), 0.0f, 1.0f, 2.0f);

			UIButtonActor& addActorButton = scaleActor->CreateChild<UIButtonActor>("GEE_E_Scene_Add_Actor_Button", [&, this, actor]() {
				UIWindowActor& compSelectWindow = editorScene.CreateActorAtRoot<UIWindowActor>(&canvas, "GEE_E_Component_Selection_Window", Transform(Vec2f(0.0f, -0.5f), Vec2f(0.3f)));

				auto createCompButton = [&, this, actor](int i, std::function<Component& ()> createCompFunc) {
					UIButtonActor& compButton = compSelectWindow.CreateChild<UIButtonActor>("GEE_E_Scene_Add_Comp_Button", [this, &editorScene, &refreshButton, actor, createCompFunc]() {
						createCompFunc();
						SelectActor(actor, editorScene);
						}, Transform(Vec2f(-0.7f + static_cast<float>(i) * 0.2f, 0.7f), Vec2f(0.1f)));

					Component& exampleComponent = createCompFunc();

					compButton.SetMatIdle(exampleComponent.LoadDebugMatInst(EditorIconState::IDLE));
					compButton.SetMatHover(exampleComponent.LoadDebugMatInst(EditorIconState::HOVER));
					compButton.SetMatClick(exampleComponent.LoadDebugMatInst(EditorIconState::BEING_CLICKED_INSIDE));

					actor->GetRoot()->DetachChild(exampleComponent);
				};

				std::function<Component& ()> getSelectedComp = [this, actor]() -> Component& { return ((SelectedComp) ? (*SelectedComp) : (*actor->GetRoot())); };

				createCompButton(0, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent<Component>("A Component", Transform()); });
				createCompButton(1, [this, &editorScene, getSelectedComp, &gameHandle]() -> Component& { LightComponent& comp = getSelectedComp().CreateComponent<LightComponent>("A LightComponent", LightType::POINT, gameHandle.GetMainScene()->GetRenderData()->GetAvailableLightIndex(), gameHandle.GetMainScene()->GetRenderData()->GetAvailableLightIndex()); comp.CalculateLightRadius(); return comp;  });
				createCompButton(2, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent<Audio::SoundSourceComponent>("A SoundSourceComponent"); });
				createCompButton(3, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent<CameraComponent>("A CameraComponent"); });
				createCompButton(4, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent<TextComponent>("A TextComponent", Transform(), "", ""); });
				createCompButton(5, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent<LightProbeComponent>("A LightProbeComponent"); });

				}, Transform(Vec2f(8.0f, 0.0f), Vec2f(0.5f)));

			AtlasMaterial& addIconMat = *dynamic_cast<AtlasMaterial*>(gameHandle.GetRenderEngineHandle()->FindMaterial("GEE_E_Add_Icon_Mat").get());
			addIconMat.AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("Assets/Editor/add_icon.png", Texture::Format::RGBA(), false, Texture::MinFilter::NearestInterpolateMipmap()), "albedo1"));
			uiButtonActorUtil::ButtonMatsFromAtlas(addActorButton, addIconMat, 0.0f, 1.0f, 2.0f);

			if (!previousCanvasView.IsEmpty() && sameActor)
				canvas.SetCanvasView(previousCanvasView);
			else
			{
				canvas.AutoClampView();
				//canvas.SetViewScale(static_cast<Vec2f>(canvas.CanvasView.GetScale()) * Vec2f(1.0f, glm::sqrt(0.5f / 0.32f)));
			}
		}
	}

}