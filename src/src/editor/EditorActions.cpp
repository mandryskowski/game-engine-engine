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
#include <editor/DefaultEditorController.h>

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


			if (Actor* foundActorCanvas = editorScene.GetRootActor()->FindActor("GEE_E_Actors_Components_Canvas"); !sameComp && foundActorCanvas)
			{
				if (prevSelectedComp)
					if (UIActivableButtonActor* foundButtonActor = foundActorCanvas->GetActor<UIActivableButtonActor>(prevSelectedComp->GetName()))
						foundButtonActor->SetActive(false);

				if (comp)
					if (UIActivableButtonActor* foundButtonActor = foundActorCanvas->GetActor<UIActivableButtonActor>(comp->GetName()))
						foundButtonActor->SetActive(true);
			}

			if (!comp)
				return;

			UICanvasActor& canvas = editorScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Components_Info_Canvas");
			canvas.KillResizeBars();
			canvas.SetTransform(Transform(Vec2f(0.85f, 0.0f), Vec2f(0.15f, 0.5f)));
			UIActorDefault* scaleActor = canvas.GetScaleActor();

			comp->GetEditorDescription(ComponentDescriptionBuilder(EditorHandle, *scaleActor));

			canvas.RefreshFieldsList();

			//Scenes.back()->RootActor->DebugHierarchy();

			if (!previousCanvasView.IsEmpty() && sameComp)
				canvas.SetCanvasView(previousCanvasView);
			else
				canvas.AutoClampView();

			EditorHandle.GetEditorController()->Reset((SelectedComp) ? (SelectedComp->GetTransform()) : (Transform()));
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

			if (Actor* foundSceneCanvas = editorScene.GetRootActor()->FindActor("GEE_E_Scene_Actors_Canvas"); !sameActor && foundSceneCanvas)
			{
				if (prevSelectedActor)
					if (UIActivableButtonActor* foundButtonActor = foundSceneCanvas->GetActor<UIActivableButtonActor>(prevSelectedActor->GetName()))
						foundButtonActor->SetActive(false);

				if (actor)
					if (UIActivableButtonActor* foundButtonActor = foundSceneCanvas->GetActor<UIActivableButtonActor>(actor->GetName()))
						foundButtonActor->SetActive(true);
			}

			if (!actor)
				return;

			UICanvasActor& canvas = editorScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Actors_Components_Canvas", Transform(Vec2f(-0.85f, -0.5f), Vec2f(0.15f, 0.375f)));
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
				}, IconData(renderHandle, "Assets/Editor/add_icon.png", Vec2i(3, 1), 0.0f));
				desc.AddOption("Delete", nullptr);
			});

			UIActorDefault* scaleActor = canvas.GetScaleActor();

			//auto& componentsListCat = canvas.AddCategory("List of components");
			UICanvasField& componentsListField = canvas.AddField("List of components");
			UIAutomaticListActor& listActor = componentsListField.CreateChild<UIAutomaticListActor>("ListActor");
			canvas.FieldsList->SetListElementOffset(canvas.FieldsList->GetListElementCount() - 1, [&listActor, &componentsListField]() { return static_cast<Mat3f>(componentsListField.GetTransform()->GetMatrix()) * (listActor.GetListOffset()); });
			canvas.FieldsList->SetListCenterOffset(canvas.FieldsList->GetListElementCount() - 1, [&componentsListField]() -> Vec3f { return Vec3f(0.0f, -componentsListField.GetTransform()->GetScale().y, 0.0f); });
			listActor.GetTransform()->Move(Vec2f(2.0f, 0.0f));

			AddActorToList<Component>(editorScene, *actor->GetRoot(), listActor, canvas);

			listActor.Refresh();
			canvas.RefreshFieldsList();

			actor->GetEditorDescription(EditorDescriptionBuilder(EditorHandle, *scaleActor));

			if (canvas.FieldsList)
				canvas.FieldsList->Refresh();

			canvas.GetTransform()->SetScale(Vec2f(0.15f, 0.3f));

			if (!previousCanvasView.IsEmpty() && sameActor)
				canvas.SetCanvasView(previousCanvasView);
			else
			{
				canvas.AutoClampView();
				//canvas.SetViewScale(Vec2f(canvas.GetViewT().GetScale2D().x, listActor.GetListElementCount() + 1.0f));
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
			else
			{
				previousCanvasView = Transform(Vec2f(0.0f), Vec2f(1.0f, 100.0f));
			}

			if (!selectedScene)
				return;

			UICanvasActor& canvas = editorScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Scene_Actors_Canvas", Transform(Vec2f(-0.85f, 0.0f), Vec2f(0.15f, 0.408f)));
			auto& titleButton = canvas.CreateChild<UIButtonActor>("GEE_E_Scene_Title", "Main Scene", nullptr, Transform(Vec2f(0.0f, 1.1f), Vec2f(1.0f, 0.1f)));
			titleButton.DetachFromCanvas();
			titleButton.SetMatIdle(std::make_shared<Material>("SceneTitleMat", Vec4f(hsvToRgb(Vec3f(288.0f, 0.82f, 0.3f)), 1.0f)));
			canvas.KillResizeBars();
			canvas.CreateCanvasBackgroundModel(hsvToRgb(Vec3f(288.0f, 0.7f, 0.2f)));
			if (!previousCanvasView.IsEmpty() && sameScene)
				canvas.SetCanvasView(previousCanvasView, false);
	
			// Popups
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
				desc.AddOption("Transform", [this, &editorScene, &gameHandle, &canvas]() { auto& window = editorScene.CreateActorAtRoot<UIWindowActor>("Button transform"); ComponentDescriptionBuilder descBuilder(*dynamic_cast<Editor::EditorManager*>(&gameHandle), window, window); canvas.GetRoot()->GetEditorDescription(descBuilder); window.AutoClampView(); window.RefreshFieldsList(); });
				desc.AddSubmenu("Create actor", [this, func](PopupDescription desc)
				{
					EditorHandle.GenerateActorList(desc, func);
				});
				desc.AddOption("Delete", nullptr);
			});

			// Change scene button
			//(canvas.CreateChild<UIButtonActor>("MainSceneButton", "Main", [this, &editorScene, &gameHandle]() { SelectScene(gameHandle.GetScene("GEE_Main"), editorScene); }, Transform(Vec2f(0.0f, -10.0f), Vec2f(1.0f))));
			//(canvas.CreateChild<UIButtonActor>("EditorSceneButton", "Editor", [this, &editorScene]() { SelectScene(&editorScene, editorScene); }, Transform(Vec2f(2.0f, -10.0f), Vec2f(1.0f))));

			UIAutomaticListActor& listActor = canvas.CreateChild<UIAutomaticListActor>("ListActor");
			AddActorToList<Actor>(editorScene, *selectedScene->GetRootActor(), listActor, canvas,
			[this, &editorScene](PopupDescription desc, Actor& actor){
				desc.AddOption("Copy", [](){  });
				desc.AddOption("Delete", [this, &actor, &editorScene]() { actor.MarkAsKilled(); SelectScene(GetSelectedScene(), editorScene); });
			});
			
			std::cout << "@@@List element count: " << listActor.GetListElementCount() << '\n';
			std::cout << "@@@List offset: " << listActor.GetListOffset().y << '\n';
			int totalActorCount = static_cast<int>(-listActor.GetListOffset().y / 2.0f);
			totalActorCount = glm::clamp(totalActorCount, 0, 18);
			{
				canvas.GetTransform()->SetScale(Vec2f(0.15f, 0.02f * totalActorCount));
				titleButton.GetTransform()->SetScale(Vec2f(1.0f, 3.0f / (totalActorCount)));
				titleButton.GetTransform()->SetVecAxis<TVec::Position, VecAxis::Y>(1.0f + titleButton.GetTransform()->GetScale().y);
			}

			listActor.Refresh();

			//editorScene.GetRootActor()->DebugActorHierarchy();

			canvas.AutoClampView();
			canvas.SetViewScale(Vec2f(canvas.GetViewT().GetScale2D().x, totalActorCount));

		}

		void EditorActions::PreviewHierarchyTree(Hierarchy::Tree& tree)
		{
			GameScene& editorScene = *EditorHandle.GetGameHandle()->GetScene("GEE_Editor");
			UIWindowActor& previewWindow = editorScene.CreateActorAtRoot<UIWindowActor>("HierarchyTreePreview");

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

				auto& nodeIcon = element.GetRoot()->CreateComponent<ModelComponent>("NodeIcon", Transform(Vec2f(-2.0f, 0.0f), Vec2f(1.0f / 3.0f, 1.0f)));
				nodeIcon.AddMeshInst(editorScene.GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad));
				nodeIcon.OverrideInstancesMaterialInstances(MakeShared<MaterialInstance>(node.GetCompBaseType().GetDebugMatInst(ButtonMaterialType::Idle)));

				for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
				{
					Hierarchy::NodeBase& child = *node.GetChild(i);
					UIAutomaticListActor& nestedList = listParent.CreateChild<UIAutomaticListActor>(child.GetCompBaseType().GetName() + "'s nestedlist");
					createNodeButtons(child, nestedList);
				}
			};

			createNodeButtons(tree.GetRoot(), list);

			auto instantiateFunc = [this, &tree, &editorScene, buttons](bool bActor) {
				// Get selected component to instantiate to
				std::function<Component* ()> getSelectedComp = [this]() -> Component* { return ((GetSelectedComponent()) ? (GetSelectedComponent()) : ((GetSelectedActor()) ? (GetSelectedActor()->GetRoot()) : (nullptr))); };

				// Get selected nodes
				std::vector<Hierarchy::NodeBase*> selectedNodes;
				for (auto& it : buttons)
					if (it.second->GetStateBits() & ButtonStateFlags::Active)
						selectedNodes.push_back(&it.first);

				// Instantiate
				if (!bActor)
				{
					if (Component* selectedComp = getSelectedComp(); selectedComp && !selectedNodes.empty())
						Hierarchy::Instantiation::TreeInstantiation(tree, true).ToComponents(tree.GetRoot(), *selectedComp, selectedNodes);
				}
				else
				{
					if (Actor* selectedActor = GetSelectedActor(); selectedActor && !selectedNodes.empty())
						Hierarchy::Instantiation::TreeInstantiation(tree, true).ToActors(tree.GetRoot(), *selectedActor, [](Actor& parent) -> Actor& { return parent.CreateChild<Actor>(""); }, selectedNodes);
				}

				// Refresh
				SelectActor(GetSelectedActor(), editorScene);
			};

			UIButtonActor& instantiateButton = list.CreateChild<UIButtonActor>("InstantiateButton", "Instantiate", [instantiateFunc]() { instantiateFunc(false); });

			instantiateButton.SetPopupCreationFunc([instantiateFunc](PopupDescription desc)
			{
				desc.AddSubmenu("Instantiate to Actors", [instantiateFunc](PopupDescription submenuDesc)
				{
					submenuDesc.AddOption("Nodes as roots", [instantiateFunc]() { instantiateFunc(true); });
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
			return ((SelectedComp) ? (SelectedComp) : (GetContextActor()->GetRoot()));
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