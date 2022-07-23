#include <editor/EditorActions.h>
#include <game/GameScene.h>
#include <UI/UIListActor.h>
#include <scene/UIButtonActor.h>
#include <UI/UICanvasActor.h>
#include <scene/TextComponent.h>
#include <UI/UICanvasField.h>

#include <scene/LightComponent.h>
#include <scene/SoundSourceComponent.h>
#include <scene/CameraComponent.h>
#include <scene/LightProbeComponent.h>

#include <scene/Controller.h>
#include <scene/GunActor.h>
#include <scene/PawnActor.h>

#include <assetload/FileLoader.h>

#include <scene/hierarchy/HierarchyTree.h>
#include <scene/hierarchy/HierarchyNodeInstantiation.h> 
#include <scene/hierarchy/HierarchyNode.h>

#include <scene/ModelComponent.h>
#include <editor/EditorManager.h>

#include <rendering/RenderToolbox.h> // usun to natychmiast 1.07.2022

namespace GEE
{
	PopupDescription::PopupDescription(Editor::EditorPopup& popup, UIAutomaticListActor* actor, UICanvasActor* canvas):
		Popup(popup),
		DescriptionRoot(actor),
		PopupCanvas(canvas),
		PopupOptionSizePx(Vec2i(240, 30))
	{
		SharedPtr<Material> secondaryMat;
		if ((secondaryMat = actor->GetGameHandle()->GetRenderEngineHandle()->FindMaterial("GEE_E_Popup_Secondary")) == nullptr)
		{
			secondaryMat = MakeShared<Material>("GEE_E_Popup_Secondary");
			secondaryMat->SetColor(Vec3f(0.5f, 0.3f, 0.5f));
			actor->GetGameHandle()->GetRenderEngineHandle()->AddMaterial(secondaryMat);
		}
		PopupCanvas->CreateCanvasBackgroundModel(Vec3f(0.5f, 0.3f, 0.5f));
	}
	UIButtonActor& PopupDescription::AddOption(const std::string& buttonContent, std::function<void()> buttonFunc, const IconData& optionalIconData)
	{
		auto& button = DescriptionRoot->CreateChild<UIButtonActor>("Button_" + buttonContent, buttonContent, 
		[window = &Popup.Window.get(), buttonFunc]()
		{
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			if (buttonFunc)
				buttonFunc();
		}, Transform(Vec2f(0.0f), Vec2f(0.9f)));

		if (!buttonFunc)
			button.SetDisableInput(true);

		if (optionalIconData.IsValid())
		{
			auto& iconModel = button.CreateComponent<ModelComponent>("GEE_E_Button_Icon", Transform(Vec2f(-0.9f, 0.0f), Vec2f(0.1f, 0.8f)));
			iconModel.AddMeshInst(MeshInstance(DescriptionRoot->GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad), optionalIconData.GetMatInstance()));
		}

		DescriptionRoot->Refresh();
		PopupCanvas->AutoClampView(true, true);
		return button;
	}
	UIInputBoxActor& PopupDescription::AddInputBox(std::function<void(std::string)> onInputFunc, const std::string& defaultContent)
	{
		auto& inputbox = DescriptionRoot->CreateChild<UIInputBoxActor>("InputBox_" + defaultContent,
			[window = &Popup.Window.get(), onInputFunc](std::string input)
			{
				glfwSetWindowShouldClose(window, GLFW_TRUE);
				if (onInputFunc)
					onInputFunc(input);
			}, [defaultContent]() { return defaultContent; }, Transform(Vec2f(0.0f), Vec2f(0.9f)));

		if (!onInputFunc)
			inputbox.SetDisableInput(true);

		DescriptionRoot->Refresh();
		PopupCanvas->AutoClampView(true, true);
		return inputbox;
	}
	PopupDescription PopupDescription::AddSubmenu(const std::string& buttonContent, std::function<void(PopupDescription)> prepareSubmenuFunc, const IconData& optionalIconData)
	{
		int optionIndex = DescriptionRoot->GetListElementCount();

		auto createSubPopup = [gameHandle = DescriptionRoot->GetGameHandle(), optionIndex, *this, prepareSubmenuFunc, window = &Popup.Window.get()]() mutable
		{

			Vec2f submenuPos(1.0f, 1.0f);
			auto t = PopupCanvas->FromCanvasSpace(PopupCanvas->GetViewT().GetInverse() * PopupCanvas->ToCanvasSpace(DescriptionRoot->GetListElement(optionIndex).GetActorRef().GetTransform()->GetWorldTransform()));
			submenuPos = t.GetPos2D() + t.GetScale2D();
		
			PopupDescription desc = dynamic_cast<Editor::EditorManager*>(gameHandle)->CreatePopupMenu(submenuPos, *window);
			if (prepareSubmenuFunc)
				prepareSubmenuFunc(desc);
			desc.RefreshPopup();
		};
		auto& button = DescriptionRoot->CreateChild<UIButtonActor>("Button_" + buttonContent, buttonContent, nullptr, Transform(Vec2f(0.0f), Vec2f(0.9f)));

		if (!prepareSubmenuFunc)
			button.SetDisableInput(true);

		button.SetOnHoverFunc([window = &Popup.Window.get(), createSubPopup]() mutable
		{
			createSubPopup();
			glfwSetWindowShouldClose(window, false);	// do not close the parent popup
		});
		button.SetOnUnhoverFunc([buttonPtr = &button, window = &Popup.Window.get()]() { if (GeomTests::NDCContains(buttonPtr->GetScene().GetUIData()->GetWindowData().GetMousePositionNDC())) glfwFocusWindow(window); });

		auto& visualCue = button.GetRoot()->CreateComponent<ModelComponent>("GEE_E_Submenu_Triangle", Transform(Vec2f(0.9f, 0.0f), Vec2f(0.1f, 0.8f)));
		visualCue.AddMeshInst(visualCue.GetScene().GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad));
		//visualCue.OverrideInstancesMaterial(visualCue.GetScene().GetGameHandle()->GetRenderEngineHandle()->FindMaterial("GEE_E_Popup_Secondary").get());
		visualCue.OverrideInstancesMaterial(IconData(*DescriptionRoot->GetGameHandle()->GetRenderEngineHandle(), "Assets/Editor/more.png").GetMatInstance()->GetMaterialPtr());
			
		if (optionalIconData.IsValid())
		{
			auto& iconModel = button.CreateComponent<ModelComponent>("GEE_E_Button_Icon", Transform(Vec2f(-0.9f, 0.0f), Vec2f(0.1f, 0.8f)));
			iconModel.AddMeshInst(MeshInstance(DescriptionRoot->GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad), optionalIconData.GetMatInstance()));
		}

		DescriptionRoot->Refresh();
		PopupCanvas->AutoClampView(true, true);
		return *this;
	}
	void PopupDescription::RefreshPopup()
	{
		Vec2i windowSize(PopupOptionSizePx.x, static_cast<int>(static_cast<float>(PopupOptionSizePx.y) * DescriptionRoot->GetBoundingBoxIncludingChildren().Size.y));
		glfwSetWindowSize(&Popup.Window.get(), windowSize.x, windowSize.y);
		Popup.Settings->Resolution = windowSize;

		DescriptionRoot->HandleEventAll(Event(EventType::WindowResized));
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
			if (auto materialCast = std::dynamic_pointer_cast<AtlasMaterial, Material>(IconMaterial))
				return MakeShared<MaterialInstance>(materialCast, materialCast->GetTextureIDInterpolatorTemplate(IconAtlasID));

		return MakeShared<MaterialInstance>(IconMaterial);
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

		Component* EditorActions::GetSelectedComponent()
		{
			return SelectedComp;
		}

		Actor* EditorActions::GetSelectedActor()
		{
			return SelectedActor;
		}

		GameScene* EditorActions::GetSelectedScene()
		{
			return SelectedScene;
		}

		void EditorActions::SelectComponent(Component* comp, GameScene& editorScene)
		{
			if (comp && comp->IsBeingKilled())	//Components and Actors that are being killed should never be chosen - doing it is a perfect opportunity for runtime crashes to occur.
				comp = nullptr;


			bool sameComp = SelectedComp == comp;
			Component* prevSelectedComp = SelectedComp;
			SelectedComp = comp;
			Transform previousCanvasView;
			if (const Actor* found = editorScene.GetRootActor()->FindActor("GEE_E_Components_Info_Canvas"))
			{
				previousCanvasView = dynamic_cast<UICanvasActor*>(const_cast<Actor*>(found))->CanvasView;
				const_cast<Actor*>(found)->MarkAsKilled();
			}

			if (!comp)
				return;

			if (Actor* foundActorCanvas = editorScene.GetRootActor()->FindActor("GEE_E_Actors_Components_Canvas"); !sameComp && foundActorCanvas)
			{
				if (prevSelectedComp)
					if (UIActivableButtonActor* foundButtonActor = foundActorCanvas->GetActor<UIActivableButtonActor>(prevSelectedComp->GetName()))
						foundButtonActor->SetActive(false);

				if (UIActivableButtonActor* foundButtonActor = foundActorCanvas->GetActor<UIActivableButtonActor>(comp->GetName()))
					foundButtonActor->SetActive(true);
			}

			UICanvasActor& canvas = editorScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Components_Info_Canvas");
			canvas.KillResizeBars();
			canvas.SetTransform(Transform(Vec2f(0.7f, -0.125f), Vec2f(0.3f, 0.375f)));
			UIActorDefault* scaleActor = canvas.GetScaleActor();

			comp->GetEditorDescription(ComponentDescriptionBuilder(EditorHandle, *scaleActor));

			canvas.RefreshFieldsList();

			//Scenes.back()->RootActor->DebugHierarchy();

			if (!previousCanvasView.IsEmpty() && sameComp)
				canvas.SetCanvasView(previousCanvasView);
			else
				canvas.AutoClampView();
		}

		void EditorActions::SelectActor(Actor* actor, GameScene& editorScene)
		{
			GameManager& gameHandle = *editorScene.GetGameHandle();
			RenderEngineManager& renderHandle = *gameHandle.GetRenderEngineHandle();

			if (actor && actor->IsBeingKilled())	//Components and Actors that are being killed should never be chosen - doing it is a perfect opportunity for runtime crashes to occur.
				actor = nullptr;


			bool sameActor = SelectedActor == actor;
			Actor* prevSelectedActor = SelectedActor;
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

			if (Actor* foundSceneCanvas = editorScene.GetRootActor()->FindActor("GEE_E_Scene_Actors_Canvas"); !sameActor && foundSceneCanvas)
			{
				if (prevSelectedActor)
					if (UIActivableButtonActor* foundButtonActor = foundSceneCanvas->GetActor<UIActivableButtonActor>(prevSelectedActor->GetName()))
						foundButtonActor->SetActive(false);

				if (UIActivableButtonActor* foundButtonActor = foundSceneCanvas->GetActor<UIActivableButtonActor>(actor->GetName()))
					foundButtonActor->SetActive(true);
			}
			UICanvasActor& canvas = editorScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Actors_Components_Canvas", Transform(Vec2f(0.7f, 0.625f), Vec2f(0.3f, 0.375f)));
			canvas.KillResizeBars();
			//canvas.SetViewScale(Vec2f(1.0f, glm::sqrt(0.5f / 0.32f)));
			canvas.SetPopupCreationFunc([&](PopupDescription desc)
			{
				auto& refreshButton = desc.AddOption("Refresh",[&]() { SelectActor(GetSelectedActor(), editorScene); }, IconData(renderHandle, "Assets/Editor/refresh_icon.png", Vec2i(3, 1), 0.0f));
				desc.AddSubmenu("Create component", [&](PopupDescription desc)
				{
					ContextMenusFactory(renderHandle).AddComponent(desc, *GetContextComp(), [&](Component& comp)
					{
						refreshButton.CallOnClickFunc();
						SelectComponent(&comp, editorScene);
						if (Actor* foundActorCanvas = editorScene.GetRootActor()->FindActor("GEE_E_Components_Info_Canvas"))
							if (UIInputBoxActor* foundNameBoxActor = foundActorCanvas->GetActor<UIInputBoxActor>("ComponentsNameActor"))
								foundNameBoxActor->SetActive(true);
					});
				});
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
			uiButtonActorUtil::ButtonMatsFromAtlas(refreshButton, gameHandle.GetRenderEngineHandle()->FindMaterial<AtlasMaterial>("GEE_E_Refresh_Icon_Mat"), 0.0f, 1.0f, 2.0f);

			UIButtonActor& addActorButton = scaleActor->CreateChild<UIButtonActor>("GEE_E_Scene_Add_Actor_Button", [&, this, actor]() {
				UIWindowActor& compSelectWindow = editorScene.CreateActorAtRoot<UIWindowActor>(&canvas, "GEE_E_Component_Selection_Window", Transform(Vec2f(0.0f, -0.5f), Vec2f(0.3f)));

				auto createCompButton = [&, this, actor](int i, std::function<Component& ()> createCompFunc) {
					UIButtonActor& compButton = compSelectWindow.CreateChild<UIButtonActor>("GEE_E_Scene_Add_Comp_Button", [this, &editorScene, &refreshButton, actor, createCompFunc]() {
						createCompFunc();
						SelectActor(actor, editorScene);
						}, Transform(Vec2f(-0.7f + static_cast<float>(i) * 0.2f, 0.7f), Vec2f(0.1f)));

					Component& exampleComponent = createCompFunc();


					compButton.SetMatIdle(exampleComponent.GetDebugMatInst(ButtonMaterialType::Idle));
					compButton.SetMatHover(exampleComponent.GetDebugMatInst(ButtonMaterialType::Hover));
					compButton.SetMatClick(exampleComponent.GetDebugMatInst(ButtonMaterialType::BeingClicked));

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

			auto addIconMat = gameHandle.GetRenderEngineHandle()->FindMaterial<AtlasMaterial>("GEE_E_Add_Icon_Mat");
			addIconMat->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("Assets/Editor/add_icon.png", Texture::Format::RGBA(), false, Texture::MinFilter::NearestInterpolateMipmap()), "albedo1"));
			uiButtonActorUtil::ButtonMatsFromAtlas(addActorButton, addIconMat, 0.0f, 1.0f, 2.0f);

			if (!previousCanvasView.IsEmpty() && sameActor)
				canvas.SetCanvasView(previousCanvasView);
			else
			{
				canvas.AutoClampView();
				//canvas.SetViewScale(static_cast<Vec2f>(canvas.CanvasView.GetScale()) * Vec2f(1.0f, glm::sqrt(0.5f / 0.32f)));
			}
		}

		void EditorActions::SelectScene(GameScene* selectedScene, GameScene& editorScene)
		{
			GameManager& gameHandle = *editorScene.GetGameHandle();
			RenderEngineManager& renderHandle = *gameHandle.GetRenderEngineHandle();
			bool sameScene = SelectedScene == selectedScene;
			SelectedScene = selectedScene;
			SelectActor(nullptr, editorScene);

			Transform previousCanvasView;
			if (const Actor* found = editorScene.GetRootActor()->FindActor("GEE_E_Scene_Actors_Canvas"))
			{
				const_cast<Actor*>(found)->MarkAsKilled();
				previousCanvasView = dynamic_cast<UICanvasActor*>(const_cast<Actor*>(found))->CanvasView;
			}

			if (!selectedScene)
				return;

			auto& aaa = editorScene.CreateActorAtRoot<UIActorDefault>("AAAAA");
			aaa.SetTransform(Transform(Vec2f(-0.68f, 0.1f), Vec2f(0.0272f, 0.0272f)));

			auto& plswork = aaa.CreateChild<UIActorDefault>("eeafksnflak");

			UICanvasActor& canvas = plswork.CreateChild<UICanvasActor>("GEE_E_Scene_Actors_Canvas", Transform(Vec2f(0.0f), Vec2f(10.0f)));
			if (!previousCanvasView.IsEmpty() && sameScene)
				canvas.CanvasView = previousCanvasView;
			canvas.SetPopupCreationFunc([&](PopupDescription desc)
			{
				auto& refreshButton = desc.AddOption("Refresh", [&]() { SelectScene(GetSelectedScene(), editorScene); }, IconData(renderHandle, "Assets/Editor/refresh_icon.png", Vec2i(3, 1), 0.0f));
				auto func = [&](Actor& actor) 
				{					
					refreshButton.CallOnClickFunc();
					SelectActor(&actor, editorScene);
					if (Actor* foundActorCanvas = editorScene.GetRootActor()->FindActor("GEE_E_Actors_Components_Canvas"))
						if (UIInputBoxActor* foundNameBoxActor = foundActorCanvas->GetActor<UIInputBoxActor>("ActorsNameActor"))
							foundNameBoxActor->SetActive(true);
				};
				desc.AddSubmenu("Create actor", [&, func](PopupDescription desc)
				{
					desc.AddOption("Actor", [&, func]() { func(GetContextActor()->CreateChild<Actor>("An Actor")); }, IconData(renderHandle, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
					desc.AddOption("Pawn", [&, func]() { func(GetContextActor()->CreateChild<PawnActor>("A PawnActor")); }, IconData(renderHandle, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
					desc.AddOption("Roaming Controller", [&, func]() { func(GetContextActor()->CreateChild<FreeRoamingController>("A FreeRoamingController")); }, IconData(renderHandle, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
					desc.AddOption("FPS Controller", [&, func]() { func(GetContextActor()->CreateChild<FPSController>("A FPSController")); }, IconData(renderHandle, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
					desc.AddOption("Shooting Controller", [&, func]() { func(GetContextActor()->CreateChild<ShootingController>("A ShootingController")); }, IconData(renderHandle, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
					desc.AddOption("Gun Actor", [&, func]() { func(GetContextActor()->CreateChild<GunActor>("A GunActor")); }, IconData(renderHandle, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
					desc.AddOption("Cue Controller", [&, func]() { func(GetContextActor()->CreateChild<CueController>("A CueController")); }, IconData(renderHandle, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
				});
				desc.AddOption("Delete", nullptr);
			});

			UIButtonActor& refreshButton = canvas.CreateChild<UIButtonActor>("GEE_E_Scene_Refresh_Button", [this, selectedScene, &editorScene]() { this->SelectScene(selectedScene, editorScene); }, Transform(Vec2f(6.0f, 0.0f), Vec2f(0.5f)));
			uiButtonActorUtil::ButtonMatsFromAtlas(refreshButton, renderHandle.FindMaterial<AtlasMaterial>("GEE_E_Refresh_Icon_Mat"), 0.0f, 1.0f, 2.0f);



			UIButtonActor& addActorButton = canvas.CreateChild<UIButtonActor>("GEE_E_Scene_Add_Actor_Button", [this, &editorScene, &refreshButton, selectedScene, &canvas]() {
				UIWindowActor& actorSelectWindow = canvas.CreateChildCanvas<UIWindowActor>("GEE_E_Actor_Selection_Window");
				actorSelectWindow.SetTransform(Transform(Vec2f(0.0f, -0.5f), Vec2f(0.3f)));

				auto createActorButton = [this, &actorSelectWindow, &editorScene, &refreshButton, selectedScene](int i, std::function<Actor& ()> createActorFunc, const std::string& className) {
					UIButtonActor& actorButton = actorSelectWindow.CreateChild<UIButtonActor>("GEE_E_Scene_Add_Comp_Button", className, [this, &editorScene, &refreshButton, &actorSelectWindow, selectedScene, createActorFunc]() {
						Actor& createdActor = createActorFunc();
						actorSelectWindow.MarkAsKilled();
						SelectScene(selectedScene, editorScene);
						SelectActor(&createdActor, editorScene);
						}, Transform(Vec2f(-0.7f, 0.7f - static_cast<float>(i) * 0.2f), Vec2f(0.1f)));
				};

				std::function<Actor& ()> getSelectedActor = [this, selectedScene]() -> Actor& { return ((SelectedActor) ? (*SelectedActor) : (const_cast<Actor&>(*selectedScene->GetRootActor()))); };

				createActorButton(0, [this, &editorScene, &selectedScene, getSelectedActor]() -> Actor& { return getSelectedActor().CreateChild<Actor>(selectedScene->GetUniqueActorName("An Actor")); }, "Actor");
				createActorButton(1, [this, &editorScene, &selectedScene, getSelectedActor]() -> GunActor& { return getSelectedActor().CreateChild<GunActor>(selectedScene->GetUniqueActorName("A GunActor")); }, "GunActor");
				createActorButton(2, [this, &editorScene, &selectedScene, getSelectedActor]() -> PawnActor& { return getSelectedActor().CreateChild<PawnActor>(selectedScene->GetUniqueActorName("A PawnActor")); }, "PawnActor");
				createActorButton(3, [this, &editorScene, &selectedScene, getSelectedActor]() -> FreeRoamingController& { return getSelectedActor().CreateChild<FreeRoamingController>(selectedScene->GetUniqueActorName("A FreeRoamingController")); }, "FreeRoamingController");
				createActorButton(4, [this, &editorScene, &selectedScene, getSelectedActor]() -> FPSController& { return getSelectedActor().CreateChild<FPSController>(selectedScene->GetUniqueActorName("An FPSController")); }, "FPSController");
				createActorButton(5, [this, &editorScene, &selectedScene, getSelectedActor]() -> ShootingController& { return getSelectedActor().CreateChild<ShootingController>(selectedScene->GetUniqueActorName("A ShootingController")); }, "ShootingController");
				createActorButton(6, [this, &editorScene, &selectedScene, getSelectedActor]() -> CueController& { return getSelectedActor().CreateChild<CueController>(selectedScene->GetUniqueActorName("A CueController")); }, "CueController");
				}, Transform(Vec2f(8.0f, 0.0f), Vec2f(0.5f)));

			auto addActorMat = MakeShared<AtlasMaterial>(Material("GEE_E_Add_Actor_Material"), glm::ivec2(3, 1));
			addActorMat->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("Assets/Editor/add_icon.png", Texture::Format::RGBA(), false, Texture::MinFilter::NearestInterpolateMipmap()), "albedo1"));
			uiButtonActorUtil::ButtonMatsFromAtlas(addActorButton, addActorMat, 0.0f, 1.0f, 2.0f);

			UIAutomaticListActor& listActor = canvas.CreateChild<UIAutomaticListActor>("ListActor");
			AddActorToList<Actor>(editorScene, *selectedScene->GetRootActor(), listActor, canvas,
			[this, &editorScene](PopupDescription desc, Actor& actor){
				desc.AddOption("Copy", [](){  });
				desc.AddOption("Delete", [this, &actor, &editorScene]() { actor.MarkAsKilled(); SelectScene(GetSelectedScene(), editorScene); });
			});

			listActor.Refresh();

			//editorScene.GetRootActor()->DebugActorHierarchy();

			canvas.AutoClampView();
			canvas.SetViewScale(static_cast<Vec2f>(canvas.CanvasView.GetScale()) * Vec2f(1.0f, glm::sqrt(0.32f / 0.32f)));
		}

		void EditorActions::PreviewHierarchyTree(Hierarchy::Tree& tree)
		{
			GameScene& editorScene = *EditorHandle.GetGameHandle()->GetScene("GEE_Editor");
			UIWindowActor& previewWindow = editorScene.CreateActorAtRoot<UIWindowActor>("Mesh node preview window");

			previewWindow.AddField("Name").CreateChild<UIInputBoxActor>("NameInputBox", [&tree](const std::string& name) { tree.SetName(name); }, [&tree]() { return tree.GetName().GetPath(); }).SetDisableInput(!tree.GetName().IsALocalResource());
			previewWindow.AddField("Tree origin").CreateChild<UIButtonActor>("OriginButton", (tree.GetName().IsALocalResource()) ? ("Local") : ("File")).SetDisableInput(true);
			previewWindow.AddField("Delete tree").CreateChild<UIButtonActor>("DeleteButton", "Delete").SetDisableInput(true);
			auto& list = previewWindow.AddField("Mesh nodes list").CreateChild<UIAutomaticListActor>("MeshNodesList");


			std::vector<std::pair<Hierarchy::NodeBase&, UIActivableButtonActor*>> buttons;

			std::function<void(Hierarchy::NodeBase&, UIAutomaticListActor&)> createNodeButtons = [&](Hierarchy::NodeBase& node, UIAutomaticListActor& listParent) {
				auto& element = listParent.CreateChild<UIActivableButtonActor>("Button", node.GetCompBaseType().GetName(), nullptr);
				buttons.push_back(std::pair<Hierarchy::NodeBase&, UIActivableButtonActor*>(node, &element));
				element.SetDeactivateOnClickingAnywhere(false);
				element.OnClick();
				element.DeduceMaterial();
				element.SetTransform(Transform(Vec2f(1.5f, 0.0f), Vec2f(3.0f, 1.0f)));
				element.SetPopupCreationFunc([&](PopupDescription desc)
				{
					desc.AddOption("Instantiate to selected", [this, &node, &editorScene, &tree]()
					{
						if (!GetContextComp())
							return;

						Actor& actor = GetContextComp()->GetActor();
						Component* createdComp = nullptr;
						if (GetContextComp() == actor.GetRoot())
						{
							actor.ReplaceRoot(node.GenerateComp(MakeShared<Hierarchy::Instantiation::Data>(tree), actor));
							createdComp = actor.GetRoot();
						}
						else
						{
							createdComp = &GetContextComp()->ParentComponent->AddComponent(node.GenerateComp(MakeShared<Hierarchy::Instantiation::Data>(tree), actor));
							GetContextComp()->MarkAsKilled();
						}

						SelectActor(GetContextActor(), editorScene);
						SelectComponent(createdComp, editorScene);
					});
					desc.AddOption("Edit", (tree.GetName().IsALocalResource()) ? (
					[*this, editorScenePtr = &editorScene, nodePtr = &node]() mutable {
						PreviewHierarchyNode(*nodePtr, *editorScenePtr);
					}) : (std::function<void()>(nullptr)));
					desc.AddSubmenu("Add node", (tree.GetName().IsALocalResource()) ? ([&](PopupDescription submenuDesc)
					{
						//Component& tempComp = tree.GetTempActor().CreateComponent<Component>("GEE_E_ADD_NODE_TEMP_COMPONENT");
						ContextMenusFactory(*EditorHandle.GetGameHandle()->GetRenderEngineHandle()).AddComponent(submenuDesc, FunctorHierarchyNodeCreator(node), [&]() { previewWindow.MarkAsKilled(); PreviewHierarchyTree(tree); });
						/*ContextMenusFactory(*EditorHandle.GetGameHandle()->GetRenderEngineHandle()).AddComponent(submenuDesc, tempComp, [&]()
							{

							std::vector<Component*> createdCompVector;
							tempComp.GetAllComponents<Component>(&createdCompVector);
							node.CreateChild<decltype(createdCompVector[0])>("test");
							tempComp.MarkAsKilled();
						});*/
					}) : (std::function<void(PopupDescription)>(nullptr)));
					
					desc.AddOption("Delete node", (&node == &tree.GetRoot() || !tree.GetName().IsALocalResource()) ? (std::function<void()>(nullptr)) : ([&]() { node.Delete(); }));
				});

				for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
				{
					Hierarchy::NodeBase& child = *node.GetChild(i);
					UIAutomaticListActor& nestedList = listParent.CreateChild<UIAutomaticListActor>(child.GetCompBaseType().GetName() + "'s nestedlist");
					createNodeButtons(child, nestedList);
				}
			};

			createNodeButtons(tree.GetRoot(), list);

			UIButtonActor& instantiateButton = list.CreateChild<UIButtonActor>("InstantiateButton", "Instantiate", [this, &tree, &editorScene, buttons]() {
				// Get selected component to instantiate to
				std::function<Component* ()> getSelectedComp = [this]() -> Component* { return ((GetSelectedComponent()) ? (GetSelectedComponent()) : ((GetSelectedActor()) ? (GetSelectedActor()->GetRoot()) : (nullptr))); };

				// Get selected nodes
				std::vector<Hierarchy::NodeBase*> selectedNodes;
				for (auto& it : buttons)
					if (it.second->GetStateBits() & ButtonStateFlags::Active)
						selectedNodes.push_back(&it.first);

				// Instantiate
				if (Component* selectedComp = getSelectedComp(); selectedComp && !selectedNodes.empty())
					EngineDataLoader::InstantiateTree(*selectedComp, tree, selectedNodes);

				// Refresh
				SelectActor(GetSelectedActor(), editorScene);
			});

			instantiateButton.SetPopupCreationFunc([](PopupDescription desc)
			{
				desc.AddSubmenu("Instantiate to Actors", [](PopupDescription submenuDesc)
				{
					submenuDesc.AddOption("Nodes as roots", []() {});
					submenuDesc.AddOption("Nodes as roots' children", []() {});
				});
			});

			list.CreateChild<UIButtonActor>("SelectAllButton", "Select all", [this, buttons]() {
				for (auto it : buttons)
				{
					it.second->OnClick();
					it.second->DeduceMaterial();
				}

				});

			list.CreateChild<UIButtonActor>("DeselectAllButton", "Deselect all", [this, buttons]() {
				for (auto it : buttons)
				{
					it.second->OnDeactivation();
					it.second->DeduceMaterial();
				}
				});

			previewWindow.RefreshFieldsList();
			list.Refresh();
			previewWindow.AutoClampView();
		}

		void EditorActions::PreviewHierarchyNode(Hierarchy::NodeBase& node, GameScene& editorScene)
		{
			Transform previousCanvasView;
			if (Actor* foundWindow = editorScene.FindActor("GEE_E_Node_Preview_Window"))
			{
				previousCanvasView = dynamic_cast<UICanvasActor*>(const_cast<Actor*>(foundWindow))->CanvasView;
				foundWindow->MarkAsKilled();
			}
			auto& nodeWindow = editorScene.CreateActorAtRoot<UIWindowActor>("GEE_E_Node_Preview_Window");
			nodeWindow.CreateCanvasBackgroundModel(Vec3f(0.6f, 0.3f, 0.6f));

			ComponentDescriptionBuilder nodeCompDesc(EditorHandle, nodeWindow, nodeWindow);
			nodeCompDesc.SetNodeBeingBuilt(&node);
			node.GetCompBaseType().GetEditorDescription(nodeCompDesc);

			nodeWindow.RefreshFieldsList();

			if (!previousCanvasView.IsEmpty())
				nodeWindow.SetCanvasView(previousCanvasView);
			else
				nodeWindow.AutoClampView();
		}

		Component* EditorActions::GetContextComp()
		{
			return ((SelectedComp) ? (SelectedComp) : (GetSelectedActor()->GetRoot()));
		}

		Actor* EditorActions::GetContextActor()
		{
			return ((SelectedActor) ? (SelectedActor) : (const_cast<Actor*>(GetSelectedScene()->GetRootActor())));
		}

		void EditorActions::CreateComponentWindow(Actor& actor)
		{
			std::function<Component& ()> getSelectedComp = [this, &actor]() -> Component& { return ((SelectedComp) ? (*SelectedComp) : (*actor.GetRoot())); };

			//SelectComponent(GetSelectedComponent(), )
		}

		template <> void EditorActions::Select<Component>(Component* obj, GameScene& editorScene) { SelectComponent(obj, editorScene); }
		template <> void EditorActions::Select<Actor>(Actor* obj, GameScene& editorScene) { SelectActor(obj, editorScene); }
		template <> void EditorActions::Select<GameScene>(GameScene* obj, GameScene& editorScene) { SelectScene(obj, editorScene); }

		ContextMenusFactory::ContextMenusFactory(RenderEngineManager& renderHandle):
			RenderHandle(renderHandle)
		{
		}

		void ContextMenusFactory::AddComponent(PopupDescription desc, Component& contextComp, std::function<void(Component&)> onCreation)
		{
			desc.AddOption("Component", [&, onCreation]() { auto& comp = contextComp.CreateComponent<Component>("A Component"); if (onCreation) onCreation(comp); }, IconData(RenderHandle, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
			desc.AddOption("Light", [&, onCreation]() { auto& comp = contextComp.CreateComponent<LightComponent>("A LightComponent"); if (onCreation) onCreation(comp); }, IconData(RenderHandle, "Assets/Editor/lightcomponent_icon.png", Vec2i(3, 1), 0.0f));
			desc.AddOption("Sound source", [&, onCreation]() { auto& comp = contextComp.CreateComponent<Audio::SoundSourceComponent>("A SoundSourceComponent"); if (onCreation) onCreation(comp); }, IconData(RenderHandle, "Assets/Editor/soundsourcecomponent_debug.png", Vec2i(3, 1), 0.0f));
			desc.AddOption("Camera", [&, onCreation]() { auto& comp = contextComp.CreateComponent<CameraComponent>("A CameraComponent"); if (onCreation) onCreation(comp); }, IconData(RenderHandle, "Assets/Editor/cameracomponent_icon.png", Vec2i(3, 1), 0.0f));
			desc.AddOption("Text", [&, onCreation]() { auto& comp = contextComp.CreateComponent<TextComponent>("A TextComponent", Transform(), "Sample text", ""); if (onCreation) onCreation(comp); }, IconData(RenderHandle, "Assets/Editor/textcomponent_icon.png", Vec2i(3, 1), 0.0f));
			desc.AddOption("Light probe", [&, onCreation]() { auto& comp = contextComp.CreateComponent<LightProbeComponent>("A LightProbeComponent"); if (onCreation) onCreation(comp); }, IconData(RenderHandle, "Assets/Editor/lightprobecomponent_icon.png", Vec2i(3, 1), 0.0f));
		}

		template<typename TFunctor>
		void ContextMenusFactory::AddComponent(PopupDescription desc, TFunctor&& functor, std::function<void()> onCreation)
		{
			desc.AddOption("Component", [onCreation, functor]() mutable { functor.Create<Component>("A Component"); if (onCreation) onCreation(); }, IconData(RenderHandle, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
			desc.AddOption("Model", [onCreation, functor]() mutable { functor.Create<ModelComponent>("A ModelComponent"); if (onCreation) onCreation(); }, IconData(RenderHandle, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
			desc.AddOption("Light", [onCreation, functor]() mutable { functor.Create<LightComponent>("A LightComponent"); if (onCreation) onCreation(); }, IconData(RenderHandle, "Assets/Editor/lightcomponent_icon.png", Vec2i(3, 1), 0.0f));
		//	desc.AddOption("Sound source", [&]() { functor.Create<Audio::SoundSourceComponent>("A SoundSourceComponent"); if (onCreation) onCreation(); }, IconData(RenderHandle, "Assets/Editor/soundsourcecomponent_debug.png", Vec2i(3, 1), 0.0f));
			//desc.AddOption("Camera", [&]() { functor.Create<CameraComponent>("A CameraComponent"); if (onCreation) onCreation(); }, IconData(RenderHandle, "Assets/Editor/cameracomponent_icon.png", Vec2i(3, 1), 0.0f));
			//desc.AddOption("Text", [&]() { functor.Create<TextComponent>("A TextComponent", Transform(), "Sample text", ""); if (onCreation) onCreation(); }, IconData(RenderHandle, "Assets/Editor/textcomponent_icon.png", Vec2i(3, 1), 0.0f));
			//desc.AddOption("Light probe", [&]() { functor.Create<LightProbeComponent>("A LightProbeComponent"); if (onCreation) onCreation(); }, IconData(RenderHandle, "Assets/Editor/lightprobecomponent_icon.png", Vec2i(3, 1), 0.0f));
		}
		template void ContextMenusFactory::AddComponent<FunctorHierarchyNodeCreator>(PopupDescription desc, FunctorHierarchyNodeCreator&& functor, std::function<void()> onCreation);
}

}	