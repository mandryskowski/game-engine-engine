#include <editor/GameEngineEngineEditor.h>
#include <editor/EditorActions.h>
#include <scene/hierarchy/HierarchyTree.h>
#include <scene/hierarchy/HierarchyNode.h>
#include <scene/TextComponent.h>
#include <scene/CameraComponent.h>
#include <scene/UIWindowActor.h>
#include <scene/ModelComponent.h>
#include <scene/LightComponent.h>
#include <scene/LightProbeComponent.h>
#include <scene/UIInputBoxActor.h>
#include <UI/UICanvasField.h>
#include <UI/UIListActor.h>
#include <scene/GunActor.h>
#include <scene/PawnActor.h>
#include <scene/Controller.h>
#include <input/InputDevicesStateRetriever.h>
#include <math/Box.h>
#include <whereami/whereami.h>
#include <sstream>
#include <fstream>
#include <thread>
#include <algorithm>
#include <game/IDSystem.h>
#include <condition_variable>
#include <utility/Log.h>
#include <rendering/OutlineRenderer.h>
#include <editor/EditorRenderer.h>


#include <editor/MousePicking.h>
#include <editor/GraphRenderingComponent.h>

#include <rendering/Renderer.h>

namespace GEE
{
	namespace Editor
	{
		void EditorEventProcessor::FileDropCallback(SystemWindow* window, int count, const char** paths)
		{
			if (count > 0)
			{
				std::cout << "Dropped " << paths[0] << '\n';
				auto addTree = [](const std::string& path) { EditorHandle->GetActions().PreviewHierarchyTree(*EngineDataLoader::LoadHierarchyTree(*EditorHandle->GetGameHandle()->GetMainScene(), path)); };
				std::string path = paths[0];
				GenericUITemplates(*EditorHandle->GetGameHandle()->GetScene("GEE_Editor")).ConfirmationBox([=]() { addTree(EditorHandle->ToRelativePath(path)); }, [=]() {addTree(path); }, "Convert path to relative?");
			}
			
		}

		void EditorEventProcessor::Resize(SystemWindow* window, int width, int height)
		{
			if (width <= 0 || height <= 0)
				return;

			Vec2u newSize(width, height);
			if (EditorHandle->GetGameHandle()->GetGameSettings()->Video.Resolution != static_cast<Vec2f>(newSize))
			{
				EditorHandle->GetGameHandle()->GetGameSettings()->Video.Resolution = static_cast<Vec2f>(newSize) * Vec2f(0.4f, 0.6f);
				EditorHandle->UpdateGameSettings();
			}

			if (EditorHandle->GetEditorSettings()->Video.Resolution != static_cast<Vec2f>(newSize))
			{
				EditorHandle->GetEditorSettings()->Video.Resolution = newSize;
				EditorHandle->UpdateEditorSettings();
			}

			std::cout << "New res " << EditorHandle->GetGameHandle()->GetGameSettings()->Video.Resolution << '\n';
		}


	GameEngineEngineEditor::GameEngineEngineEditor(SystemWindow* window, const GameSettings& settings) :
		Game(settings.Video.Shading, settings),
		EditorScene(nullptr),
		bDebugRenderComponents(true),
		bDebugRenderPhysicsMeshes(true),
		GameController(nullptr),
		EEForceForwardShading(false),
		LastRenderPopupRequest(nullptr),
		bViewportMaximized(false),
		Actions(MakeUnique<EditorActions>(*this)),
		TestTranslateLastPos(Vec2f(0.0f))
	{
		{
			int length = wai_getExecutablePath(nullptr, 0, nullptr), dirnameLength = 0;
			std::string path;
			path.resize(length);
			wai_getExecutablePath(&path[0], length, &dirnameLength);
			ExecutableFolder = path.substr(0, dirnameLength);
			std::cout << "Executable folder: " << ExecutableFolder << '\n';
		}

			Settings = MakeUnique<GameSettings>(GameSettings(EngineDataLoader::LoadSettingsFromFile<GameSettings>("Settings.ini")));
			Vec2f windowSize = Settings->Video.Resolution;
			Settings->Video.Resolution = Vec2f(windowSize.x * 0.4f, windowSize.y * 0.6f);
			Settings->Video.Shading = ShadingAlgorithm::SHADING_PBR_COOK_TORRANCE;

			EditorSettings.Video.Resolution = windowSize;
			Init(window, windowSize);
		}

		bool GameEngineEngineEditor::GameLoopIteration(float timeStep, float deltaTime)
		{
			bool returnVal = Game::GameLoopIteration(timeStep, deltaTime);

			//////////////////////POPUPS
			OpenPopups.erase(std::remove_if(OpenPopups.begin(), OpenPopups.end(), [](EditorPopup& popup) { if (glfwWindowShouldClose(&popup.Window.get())) { glfwDestroyWindow(&popup.Window.get()); return true; } return false; }), OpenPopups.end());

			if (Profiler.HasBeenStarted())
				Profiler.AddTime(deltaTime);

			return returnVal;
		}

		GameManager* GameEngineEngineEditor::GetGameHandle()
		{
			return this;
		}

		std::vector<GameScene*> GameEngineEngineEditor::GetScenes()
		{
			std::vector<GameScene*> scenes(Scenes.size());
			std::transform(Scenes.begin(), Scenes.end(), scenes.begin(), [](UniquePtr<GameScene>& sceneVec) {return sceneVec.get(); });
			return scenes;
		}

		GameSettings* GameEngineEngineEditor::GetEditorSettings()
		{
			return &EditorSettings;
		}

		AtlasMaterial* GameEngineEngineEditor::GetDefaultEditorMaterial(EditorDefaultMaterial icon)
		{
			switch (icon)
			{
			case EditorDefaultMaterial::Close: return nullptr;
			}
			return nullptr;
		}

		EditorActions& GameEngineEngineEditor::GetActions()
		{
			return *Actions;
		}

		void GameEngineEngineEditor::RequestPopupMenu(const Vec2f& posWindowSpace, SystemWindow& relativeWindow, std::function<void(PopupDescription)> creationFunc)
		{
			LastRenderPopupRequest = [=, &relativeWindow]()
			{
				PopupDescription desc = CreatePopupMenu(posWindowSpace, relativeWindow);
				creationFunc(desc);
				desc.RefreshPopup();
			};
		}

		std::string GameEngineEngineEditor::ToRelativePath(const std::string& filepath)
		{
			std::cout << "Executable folder: " << ExecutableFolder;
			if (auto foundPos = filepath.find(ExecutableFolder); foundPos != std::string::npos)
				return filepath.substr(foundPos + ExecutableFolder.length() + 1);

			if (EditorScene)
				GetEditorLogger(*EditorScene).Log("Cannot convert path to relative", EditorMessageLogger::MessageType::Error);

			return filepath;
		}

		void GameEngineEngineEditor::Init(SystemWindow* window, const Vec2u& windowSize)
		{
			Game::Init(window, windowSize);

			GLFWimage image;
			image.pixels = stbi_load("gee_icon.png", &image.width, &image.height, nullptr, 4);
			if (!image.pixels)
				std::cout << "INFO: Could not load icon file gee_icon.png.\n";
			glfwSetWindowIcon(window, 1, &image);
			stbi_image_free(image.pixels);

			EditorEventProcessor::EditorHandle = this;
			glfwSetDropCallback(Window, EditorEventProcessor::FileDropCallback);
			glfwSetFramebufferSizeCallback(Window, EditorEventProcessor::Resize);

			glfwSetWindowCloseCallback(Window, [](SystemWindow* window) { GameEngineEngineEditor* thisPtr = static_cast<GameEngineEngineEditor*>(glfwGetWindowUserPointer(window)); GenericUITemplates(*thisPtr->EditorScene).ConfirmationBox([thisPtr]() { thisPtr->SaveProject(); thisPtr->TerminateGame(); }, [=]() { thisPtr->TerminateGame(); }, "Save the project?"); });


			SharedPtr<AtlasMaterial> addIconMat = MakeShared<AtlasMaterial>("GEE_E_Add_Icon_Mat", glm::ivec2(3, 1));
			GetRenderEngineHandle()->AddMaterial(addIconMat);
			addIconMat->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("Assets/Editor/add_icon.png", Texture::Format::RGBA(), false, Texture::MinFilter::NearestInterpolateMipmap()), "albedo1"));

			SharedPtr<AtlasMaterial> refreshIconMat = MakeShared<AtlasMaterial>("GEE_E_Refresh_Icon_Mat", glm::ivec2(3, 1));
			GetRenderEngineHandle()->AddMaterial(refreshIconMat);
			refreshIconMat->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("Assets/Editor/refresh_icon.png", Texture::Format::RGBA(), false, Texture::MinFilter::NearestInterpolateMipmap()), "albedo1"));
		
			SharedPtr<AtlasMaterial> moreMat = MakeShared<AtlasMaterial>("GEE_E_More_Mat", glm::ivec2(3, 1));
			GetRenderEngineHandle()->AddMaterial(moreMat);
			moreMat->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("Assets/Editor/more_icon.png", Texture::Format::RGBA(), true), "albedo1"));

			
			GetRenderEngineHandle()->AddMaterial(MakeShared<Material>("GEE_Default_Text_Material", Vec3f(1.0f), *RenderEng.FindShader("TextShader")));
			GetRenderEngineHandle()->AddMaterial(MakeShared<Material>("GEE_Default_X_Axis", Vec3f(0.39f, 0.18f, 0.18f)));
			GetRenderEngineHandle()->AddMaterial(MakeShared<Material>("GEE_Default_Y_Axis", Vec3f(0.18f, 0.39f, 0.25f)));
			GetRenderEngineHandle()->AddMaterial(MakeShared<Material>("GEE_Default_Z_Axis", Vec3f(0.18f, 0.25f, 0.39f)));
			GetRenderEngineHandle()->AddMaterial(MakeShared<Material>("GEE_Default_W_Axis", Vec3f(0.45f, 0.46f, 0.5f)));
			GetRenderEngineHandle()->AddMaterial(MakeShared<Material>("GEE_E_Canvas_Background_Material", Vec3f(0.26f, 0.34f, 0.385f)));

			{
				auto createdMaterial = MakeShared<AtlasMaterial>("GEE_E_Icons", Vec2i(5, 2));
				GetGameHandle()->GetRenderEngineHandle()->AddMaterial(createdMaterial);
				createdMaterial->AddTexture(NamedTexture(Texture::Loader<>::FromFile2D("Assets/Editor/Icons.png", Texture::Format::RGBA(), true), "albedo1"));
			}

			{
				SharedPtr<Material> material = MakeShared<Material>("GEE_No_Camera_Icon");
				GetRenderEngineHandle()->AddMaterial(material);
				material->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("Assets/Editor/no_camera_icon.png", Texture::Format::RGBA(), true, Texture::MinFilter::Trilinear(), Texture::MagFilter::Bilinear()), "albedo1"));
			}

			//LoadSceneFromFile("level.txt", "GEE_Main");
			//SetMainScene(GetScene("GEE_Main"));

			GameSettings::VideoSettings& uiSettings = EditorSettings.Video;
			uiSettings.AAType = AA_SMAA1X;
			uiSettings.AmbientOcclusionSamples = 0;
			uiSettings.bBloom = false;
			uiSettings.DrawToWindowFBO = true;
			uiSettings.POMLevel = SettingLevel::SETTING_NONE;
			uiSettings.Resolution = WindowData(*Window).GetWindowSize();
			uiSettings.Shading = ShadingAlgorithm::SHADING_FULL_LIT;
			uiSettings.ShadowLevel = SettingLevel::SETTING_NONE;
			uiSettings.TMType = ToneMappingType::TM_NONE;
			uiSettings.MonitorGamma = 1.0f;

			ViewportRenderCollection = &RenderEng.AddRenderTbCollection(RenderToolboxCollection("GameSceneRenderCollection", Settings->Video, RenderEng));

			HUDRenderCollection = &RenderEng.AddRenderTbCollection(RenderToolboxCollection("HUDRenderCollection", uiSettings, RenderEng));

			GameManager::DefaultScene = GetMainScene();
			SetActiveScene(GetMainScene());

			//SelectScene(ActiveScene, *EditorScene);
		}

		PopupDescription GameEngineEngineEditor::CreatePopupMenu(const Vec2f& posWindowSpace, SystemWindow& relativeWindow)
		{
			// 1. Convert pos to screen space
			Vec2f posScreenSpace(0.0f);
			{
				Vec2i mainWindowPos, mainWindowSize;
				glfwGetWindowPos(&relativeWindow, &mainWindowPos.x, &mainWindowPos.y);
				glfwGetWindowSize(&relativeWindow, &mainWindowSize.x, &mainWindowSize.y);
			
				posScreenSpace = static_cast<Vec2f>(mainWindowSize) * (posWindowSpace * Vec2f(0.5f, -0.5f) + 0.5f) + static_cast<Vec2f>(mainWindowPos);
			}
		
			glfwWindowHint(GLFW_RESIZABLE, false);
			glfwWindowHint(GLFW_DECORATED, false);
			SystemWindow* window = glfwCreateWindow(240, 30, "popup", nullptr, Window);
			glfwSetWindowPos(window, posScreenSpace.x, posScreenSpace.y);

			glfwWindowHint(GLFW_RESIZABLE, true);
			glfwWindowHint(GLFW_DECORATED, true);

			glfwSetCursorPosCallback(window, WindowEventProcessor::CursorPosCallback);
			glfwSetMouseButtonCallback(window, WindowEventProcessor::MouseButtonCallback);
			glfwSetCursorEnterCallback(window, WindowEventProcessor::CursorLeaveEnterCallback);
			glfwSetWindowFocusCallback(window, [](SystemWindow* window, int focused) { if (!focused) glfwSetWindowShouldClose(window, GLFW_TRUE); });

		
			GameScene& popupScene = CreateUIScene("GEE_E_Popup_Window", *window, false);

			//popupScene.CreateActorAtRoot<UIButtonActor>("CreateComponentButton", "Create component", [=]() { glfwSetWindowShouldClose(window, GLFW_TRUE); }, Transform(Vec2f(0.0f), Vec2f(1.0f)));
			OpenPopups.push_back(EditorPopup(IDSystem<EditorPopup>::GenerateID(), *window, popupScene));

			auto& popupCanvas = popupScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Popup_Canvas", Transform(Vec2f(0.0f), Vec2f(1.0f)));

			auto& buttonList = popupCanvas.CreateChild<UIAutomaticListActor>("PopupButtonList");
			buttonList.Refresh();
			popupCanvas.AutoClampView(); 
			//PopupDescription(&buttonList).AddOption("dziala", [=]() { glfwSetWindowShouldClose(window, GLFW_TRUE); });
			return PopupDescription(*window, &buttonList, &popupCanvas);
		}

		void GameEngineEngineEditor::UpdateGameSettings()
		{
			ViewportRenderCollection->AddTbsRequiredBySettings();

			//Update scene preview texture, since we created a new one
			Material* previewMaterial = RenderEng.FindMaterial("GEE_3D_SCENE_PREVIEW_MATERIAL").get();
			if (!previewMaterial)
			{
				std::cout << "ERROR: No scene preview material detected.\n";
				return;
			}
			previewMaterial->Textures.clear();
			previewMaterial->AddTexture(MakeShared<NamedTexture>(ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorTexture(0), "albedo1"));
		}

		void GameEngineEngineEditor::UpdateEditorSettings()
		{
			HUDRenderCollection->AddTbsRequiredBySettings();
		}

		void GameEngineEngineEditor::SetupEditorScene()
		{
			GameScene& editorScene = CreateUIScene("GEE_Editor", *Window);

			//settings.ViewportData = glm::uvec4(res.x * 0.3f, res.y * 0.4f, res.x * 0.4, res.y * 0.6f);

			// Canvases
			/*{
				UICanvasActor& rightCanvas = editorScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Right_Main_Canvas", Transform(Vec2f(0.7f, 0.4f), Vec2f(0.3f, 0.6f)));
				rightCanvas.SetCanvasView(Transform(Vec2f(0.0f, -4.5f), Vec2f(8.0f, 5.0f)), false);
				auto& mainSceneCategory = rightCanvas.AddCategory("Main scene");

				mainSceneCategory.AddField("Testlol123");
				mainSceneCategory.AddCategory("Scene graph");
				mainSceneCategory.AddCategory("Selected actor");
				mainSceneCategory.AddCategory("Selected component");

				rightCanvas.RefreshFieldsList();

			}*/
			// Viewport
			{
				auto& scenePreviewActor = editorScene.CreateActorAtRoot<UIButtonActor>("SceneViewportActor", nullptr, Transform(Vec2f(0.0f, 0.4f), Vec2f(0.4f, 0.6f)));
				scenePreviewActor.DeleteButtonModel();

				ModelComponent& scenePreviewQuad = scenePreviewActor.CreateComponent<ModelComponent>("SceneViewportQuad");
				scenePreviewActor.SetOnClickFunc([&]() {

					// Get mouse pos in NDC (preview quad space)
					Vec2f localMousePos = static_cast<Vec2f>(glm::inverse(scenePreviewActor.GetTransform()->GetWorldTransformMatrix()) * Vec4f(static_cast<Vec2f>(editorScene.GetUIData()->GetWindowData().GetMousePositionNDC()), 0.0f, 1.0f));
					std::cout << "#$# Mouse picking " << localMousePos << '\n';

					GameScene& scene = *GetScene("GEE_Main");
					if (!scene.GetActiveCamera())
						return;

					// Get all components
					std::vector<Component*> allComponents;
					{
						std::vector<Actor*> allActors;
						allActors.push_back(GetScene("GEE_Main")->GetRootActor());
						GetScene("GEE_Main")->GetRootActor()->GetAllActors(&allActors);

						for (auto it : allActors)
						{
							if (bDebugRenderComponents)
							{
								allComponents.push_back(it->GetRoot());
								it->GetRoot()->GetAllComponents<Component>(&allComponents);
							}
							else
							{
								if (dynamic_cast<RenderableComponent*>(it->GetRoot()))
									allComponents.push_back(it->GetRoot());
								it->GetRoot()->GetAllComponents<Component, RenderableComponent>(&allComponents);
							}
						}
					}

					MousePickingRenderer renderer(RenderEng);
					SceneMatrixInfo info = scene.GetActiveCamera()->GetRenderInfo(0, *ViewportRenderCollection);
					Component* pickedComponent = renderer.PickComponent(info, scene, GetGameSettings()->Video.Resolution, static_cast<Vec2u>((localMousePos * 0.5f + 0.5f) * static_cast<Vec2f>(GetGameSettings()->Video.Resolution)), allComponents);

					if (pickedComponent && GetInputRetriever().IsKeyPressed(Key::LeftAlt)) // Pick the root component if left alt is pressed
						pickedComponent = pickedComponent->GetActor().GetRoot();

					if (pickedComponent && pickedComponent != Actions->GetSelectedComponent())
					{
						Actions->SelectActor(&pickedComponent->GetActor(), editorScene);
						Actions->SelectComponent(pickedComponent, editorScene);
					}
					else
						Actions->SelectActor(nullptr, editorScene);

					});



				UIButtonActor& selectPreviewButton = scenePreviewActor.CreateChild<UIButtonActor>("SelectPreview_Empty", "Final", nullptr, Transform(Vec2f(0.6f, -1.07f), Vec2f(0.4f, 0.05f)));
				auto selectPreviewButtonText = selectPreviewButton.GetRoot()->GetComponent<TextConstantSizeComponent>("ButtonText");
				selectPreviewButtonText->Unstretch();
				//selectPreviewButtonText->SetMaxSize(Vec2f(1.0f, 1.0f / 8.0f));
				selectPreviewButton.SetOnClickFunc([&, selectPreviewButtonText]() {
						PopupDescription desc = CreatePopupMenu(editorScene.GetUIData()->GetWindowData().GetMousePositionNDC(), *editorScene.GetUIData()->GetWindow());

						auto addPreview = [&, selectPreviewButtonText](PopupDescription previewDesc, const std::string& name, std::function<Texture()> getTextureToPreview, float iconAtlasID) {
							previewDesc.AddOption(name, [&, getTextureToPreview, selectPreviewButtonText, name]() {
								Material* previewMaterial = RenderEng.FindMaterial("GEE_3D_SCENE_PREVIEW_MATERIAL").get();
								previewMaterial->Textures.clear();
								previewMaterial->AddTexture(MakeShared<NamedTexture>(getTextureToPreview(), "albedo1"));

								selectPreviewButtonText->SetContent(name);
							}, IconData(RenderEng, "Assets/Editor/scene_preview_types.png", Vec2i(2, 3), iconAtlasID));
						};

						//desc.AddSubmenu("Light shadow map", [this, addPreview](PopupDescription submenuDesc) { addPreview(submenuDesc, "Light 1", [this]() { return ViewportRenderCollection->GetTb<ShadowMappingToolbox>()->ShadowMapArray; }, 4.0f); });

						addPreview(desc, "Albedo", [this]() { return ViewportRenderCollection->GetTb<DeferredShadingToolbox>()->GetGeometryFramebuffer()->GetColorTexture(0); }, 0.0f);
						addPreview(desc, "Position", [this]() {return ViewportRenderCollection->GetTb<DeferredShadingToolbox>()->GetGeometryFramebuffer()->GetColorTexture(1); }, 1.0f);
						addPreview(desc, "Normal", [this]() { return ViewportRenderCollection->GetTb<DeferredShadingToolbox>()->GetGeometryFramebuffer()->GetColorTexture(2); }, 2.0f);
						addPreview(desc, "Final", [this]() { return ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorTexture(0); }, 3.0f);

						desc.RefreshPopup();
				});

				UIActorDefault& forwardShadingSwitch = scenePreviewActor.CreateChild<UIActorDefault>("Extended_Essay_ForwardShadingSwitch", Transform(Vec2f(0.1f, -1.07f), Vec2f(0.05f)));
				forwardShadingSwitch.CreateComponent<TextConstantSizeComponent>("SwitchForwardText", Transform(Vec2f(0.0f, -1.4f), Vec2f(1.0f, 0.4f)), "Forward", "", Alignment2D::Center()).Unstretch();
				UIElementTemplates(forwardShadingSwitch).TickBox([this, &selectPreviewButton, selectPreviewButtonText, &scenePreviewQuad](bool val) {
					selectPreviewButton.SetDisableInput(val); 
					selectPreviewButton.GetButtonModel()->SetHide(val);
					selectPreviewButtonText->SetHide(val);

					 EEForceForwardShading = val; }, [this]() { return EEForceForwardShading; });
			}

			UIActorDefault& scaleActor = editorScene.CreateActorAtRoot<UIActorDefault>("TextTestButton", Transform(Vec2f(0.0f, 0.0f), Vec2f(0.1f, 0.1f)));

			UIWindowActor& window1 = editorScene.CreateActorAtRoot<UIWindowActor>("MyTestWindow");
			window1.SetTransform(Transform(Vec2f(0.0, -0.5f), Vec2f(0.2f)));

			UIButtonActor& bigButtonActor = window1.CreateChild<UIButtonActor>("MojTestowyButton", "big button");
			bigButtonActor.SetTransform(Transform(Vec2f(0.0f, 1.5f), Vec2f(0.4f)));

			// Main buttons
			{
				auto iconsMat = RenderEng.FindMaterial<AtlasMaterial>("GEE_E_Icons");

				auto mainButtonTheme = [&](UIButtonActor& button, float iconID) {
					button.GetRoot()->GetComponent("ButtonText")->GetTransform().SetPosition(Vec2f(0.0f, -0.6f));
					button.CreateComponent<ModelComponent>("SettingsIcon", Transform(Vec2f(0.0f, 0.3f), Vec2f(0.75f))).AddMeshInst(MeshInstance(RenderEng.GetBasicShapeMesh(EngineBasicShape::QUAD), MakeShared<MaterialInstance>(iconsMat, iconsMat->GetTextureIDInterpolatorTemplate(iconID))));
					button.GetRoot()->GetComponent<ModelComponent>("SettingsIcon")->OverrideInstancesMaterialInstances(MakeShared<MaterialInstance>(iconsMat, iconsMat->GetTextureIDInterpolatorTemplate(iconID)));
				};
			
			
				UIButtonActor& hierarchyTreesButton = editorScene.CreateActorAtRoot<UIButtonActor>("HierarchyTreesButton", "HierarchyTrees", nullptr, Transform(Vec2f(-0.8f, -0.8f), Vec2f(0.1f)));
				hierarchyTreesButton.SetOnClickFunc([this, &editorScene, &hierarchyTreesButton]() {
					UIWindowActor& window = editorScene.CreateActorAtRoot<UIWindowActor>("MeshTreesWindow");
					window.SetViewScale(Vec2f(1.0f, 6.0f));

					auto windowRefreshFunc = [&window, &hierarchyTreesButton]() { window.MarkAsKilled(); hierarchyTreesButton.OnClick(); };

					UIAutomaticListActor& meshTreesList = window.CreateChild<UIAutomaticListActor>("HierarchyTreeList");
					for (auto& scene : Scenes)
						for (auto& tree : scene->HierarchyTrees)
						{
							auto& button = meshTreesList.CreateChild<UIButtonActor>("HierarchyTreeButton", tree->GetName().GetPath(), [this, &tree]() { Actions->PreviewHierarchyTree(*tree); });
							button.SetPopupCreationFunc([scenePtr = scene.get(), treePtr = tree.get(), windowRefreshFunc](PopupDescription desc)
							{
								desc.AddOption("Copy", [scenePtr, treePtr, windowRefreshFunc]() { scenePtr->CopyHierarchyTree(*treePtr); windowRefreshFunc(); });
								desc.AddOption("Delete", [scenePtr, treePtr, windowRefreshFunc]() { scenePtr->HierarchyTrees.erase(std::remove_if(scenePtr->HierarchyTrees.begin(), scenePtr->HierarchyTrees.end(), [treePtr](auto& treeVec) { return treePtr == treeVec.get(); }), scenePtr->HierarchyTrees.end()); windowRefreshFunc(); });
							});
						}

					auto& addTreeButton = meshTreesList.CreateChild<UIButtonActor>("AddHierarchyTreeButton", [this, windowRefreshFunc]() { /* set name to make the tree a local resource (this is a hack) */ GetScene("GEE_Main")->CreateHierarchyTree("").SetName("CreatedTree"); windowRefreshFunc(); });
					uiButtonActorUtil::ButtonMatsFromAtlas(addTreeButton, GetRenderEngineHandle()->FindMaterial<AtlasMaterial>("GEE_E_Add_Icon_Mat"), 0.0f, 1.0f, 2.0f);

					meshTreesList.Refresh();
					window.ClampViewToElements();
				});
				mainButtonTheme(hierarchyTreesButton, 1.0f);

				auto& materialsButton = editorScene.CreateActorAtRoot<UIButtonActor>("MaterialsButton", "Materials", nullptr, Transform(Vec2f(-0.5f, -0.8f), Vec2f(0.1f)));
				materialsButton.SetOnClickFunc([this, &editorScene, &materialsButton]() {
					UIWindowActor& window = editorScene.CreateActorAtRoot<UIWindowActor>("MaterialsPreviewWindow");
					window.SetViewScale(Vec2f(1.0f, 6.0f));
					UIAutomaticListActor& list = window.CreateChild<UIAutomaticListActor>("MaterialsList");

					UIButtonActor& addMaterialButton = list.CreateChild<UIButtonActor>("CreateMaterialButton", [&]() {
						const auto& view = window.GetViewT();
						GetRenderEngineHandle()->AddMaterial(MakeShared<AtlasMaterial>("CreatedMaterial" + std::to_string(IDSystem<Material>::GenerateID())));
						window.MarkAsKilled();
						materialsButton.OnClick();
						if (auto newWindow = dynamic_cast<UIWindowActor*>(editorScene.FindActor("MaterialsPreviewWindow")))
							newWindow->SetCanvasView(view);
					});
					uiButtonActorUtil::ButtonMatsFromAtlas(addMaterialButton, GetRenderEngineHandle()->FindMaterial<AtlasMaterial>("GEE_E_Add_Icon_Mat"), 0.0f, 1.0f, 2.0f);

					for (auto& it : RenderEng.Materials)
					{
						std::string name = (it) ? (it->GetLocalization().GetFullStr()) : ("NULLPTR???");
						UIButtonActor& button = list.CreateChild<UIButtonActor>(name + "Button", name, [this, &window, &it, name]() { UIWindowActor& matWindow = window.CreateChildCanvas<UIWindowActor>(name + "MaterialWindow"); EditorDescriptionBuilder descBuilder(*this, matWindow, matWindow); it->GetEditorDescription(descBuilder); matWindow.AutoClampView(); matWindow.RefreshFieldsList(); });
						std::cout << name << '\n';
					}

					list.Refresh();
					window.ClampViewToElements();
				});
				mainButtonTheme(materialsButton, 0.0f);

				auto& settingsButton = editorScene.CreateActorAtRoot<UIButtonActor>("SettingsButton", "Settings", [this, &editorScene]() {
					UIWindowActor& window = editorScene.CreateActorAtRoot<UIWindowActor>("MainSceneSettingsWindow");
					window.GetTransform()->SetScale(Vec2f(0.5f));
					window.AddField("Bloom").GetTemplates().TickBox([this](bool bloom) { GetGameSettings()->Video.bBloom = bloom; UpdateGameSettings(); }, [this]() -> bool { return GetGameSettings()->Video.bBloom; });

					UIInputBoxActor& gammaInputBox = window.AddField("Gamma").CreateChild<UIInputBoxActor>("GammaInputBox");
					gammaInputBox.SetOnInputFunc([this](float val) { GetGameSettings()->Video.MonitorGamma = val; UpdateGameSettings(); }, [this]()->float { return GetGameSettings()->Video.MonitorGamma; });

					window.AddField("Default font").GetTemplates().PathInput([this](const std::string& path) { Fonts.push_back(MakeShared<Font>(*DefaultFont)); *DefaultFont = *EngineDataLoader::LoadFont(*this, path); }, [this]() {return GetDefaultFont()->GetVariation(FontStyle::Regular)->GetPath(); }, { "*.ttf", "*.otf" });
					window.AddField("Rebuild light probes").CreateChild<UIButtonActor>("RebuildProbesButton", "Rebuild", [this]() { SceneRenderer(RenderEng, 0).PreRenderLoopPassStatic(0, GetSceneRenderDatas()); });

					{
						auto& aaSelectionList = window.AddField("Anti-aliasing").CreateChild<UIAutomaticListActor>("AASelectionList", Vec3f(2.0f, 0.0f, 0.0f));
						std::function<void(AntiAliasingType)> setAAFunc = [this](AntiAliasingType type) { const_cast<GameSettings::VideoSettings&>(ViewportRenderCollection->GetSettings()).AAType = type; UpdateGameSettings(); };
						aaSelectionList.CreateChild<UIButtonActor>("NoAAButton", "None", [=]() { setAAFunc(AntiAliasingType::AA_NONE); });
						aaSelectionList.CreateChild<UIButtonActor>("SMAA1XButton", "SMAA1X", [=]() { setAAFunc(AntiAliasingType::AA_SMAA1X); });
						aaSelectionList.CreateChild<UIButtonActor>("SMAAT2XButton", "SMAAT2X", [=]() { setAAFunc(AntiAliasingType::AA_SMAAT2X); });
						aaSelectionList.Refresh();
					}

					{
						auto& shadowSelectionList = window.AddField("Shadow quality").CreateChild<UIAutomaticListActor>("ShadowSelectionList", Vec3f(2.0f, 0.0f, 0.0f));
						std::function<void(SettingLevel, const std::string&)> addShadowButton = [this, &shadowSelectionList](SettingLevel setting, const std::string& caption)
						{ 
							shadowSelectionList.CreateChild<UIActivableButtonActor>("Button" + caption, caption,
							[=]()
							{
								const_cast<GameSettings::VideoSettings&>(ViewportRenderCollection->GetSettings()).ShadowLevel = setting;
								UpdateGameSettings();
							});
						};
						addShadowButton(SettingLevel::SETTING_LOW, "Low");
						addShadowButton(SettingLevel::SETTING_MEDIUM, "Medium");
						addShadowButton(SettingLevel::SETTING_HIGH, "High");
						addShadowButton(SettingLevel::SETTING_ULTRA, "Ultra");

						shadowSelectionList.Refresh(); 
					}

					{
						auto& viewTexField = window.AddField("View texture");
						SharedPtr<unsigned int> texID = MakeShared<unsigned int>(0);

						viewTexField.CreateChild<UIInputBoxActor>("PreviewTexIDInputBox").SetOnInputFunc([texID](float val) { *texID = val; }, [texID]() { return *texID; });
						viewTexField.CreateChild<UIButtonActor>("OKButton", "OK", [this, texID, &window]() {
							if (!glIsTexture(*texID))
								return;
							UIWindowActor& texPreviewWindow = window.CreateChildCanvas<UIWindowActor>("PreviewTexWindow");
							ModelComponent& texPreviewQuad = texPreviewWindow.CreateChild<UIActorDefault>("TexPreviewActor").CreateComponent<ModelComponent>("TexPreviewQuad");
							auto mat = MakeShared<Material>("TexturePreviewMat");
							mat->AddTexture(MakeShared<NamedTexture>(Texture::FromGeneratedGlId(Vec2u(0), GL_TEXTURE_2D, *texID, Texture::Format::RGBA()), "albedo1"));
							texPreviewQuad.AddMeshInst(MeshInstance(GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), mat));

							texPreviewWindow.AutoClampView();

							}).GetTransform()->Move(Vec2f(2.0f, 0.0f));
					}

					window.AddField("Render debug icons").GetTemplates().TickBox(bDebugRenderComponents);
					window.AddField("Debug physics meshes").GetTemplates().TickBox(bDebugRenderPhysicsMeshes);

					auto& ssaoSamplesIB = window.AddField("SSAO Samples").CreateChild<UIInputBoxActor>("SSAOSamplesInputBox");
					ssaoSamplesIB.SetOnInputFunc([this](float sampleCount) { GetGameSettings()->Video.AmbientOcclusionSamples = sampleCount; UpdateGameSettings(); }, [this]() { return GetGameSettings()->Video.AmbientOcclusionSamples; });

					window.AddField("Parallax Occlusion Mapping").GetTemplates().TickBox([this](bool val) { GetGameSettings()->Video.POMLevel = (val) ? (SettingLevel::SETTING_LOW) : (SettingLevel::SETTING_NONE); UpdateGameSettings(); }, [this]() { return GetGameSettings()->Video.POMLevel != SettingLevel::SETTING_NONE; });

					window.FieldsList->Refresh();
					window.AutoClampView();

				}, Transform(Vec2f(-0.2f, -0.8f), Vec2f(0.1f)));
				mainButtonTheme(settingsButton, 3.0f);

			
				auto& shadersButton = editorScene.CreateActorAtRoot<UIButtonActor>("ShadersButton", "Shaders", [this, &editorScene]() {
					UIWindowActor& window = editorScene.CreateActorAtRoot<UIWindowActor>("ShadersPreviewWindow");
					UIAutomaticListActor& list = window.CreateChild<UIAutomaticListActor>("ShadersList");
					for (auto& it : RenderEng.Shaders)
					{
						list.CreateChild<UIButtonActor>(it->GetName() + "Button", it->GetName(), [this, &window, &it]() { UIWindowActor& shaderWindow = window.CreateChildCanvas<UIWindowActor>("ShaderPreviewWindow"); EditorDescriptionBuilder descBuilder(*this, shaderWindow, shaderWindow); it->GetEditorDescription(descBuilder); shaderWindow.AutoClampView(); shaderWindow.RefreshFieldsList(); });
					}
					list.Refresh();
				}, Transform(Vec2f(0.1f, -0.8f), Vec2f(0.1f)));
				mainButtonTheme(shadersButton, 2.0f);

				// Profiling
				Shader& graphShader = *RenderEng.AddShader(ShaderLoader::LoadShaders("GraphShader", "Shaders/graph/graph.vs", "Shaders/graph/graph.fs"));
				graphShader.SetExpectedMatrices({ MatrixType::MVP });

				auto& profilerButton = editorScene.CreateActorAtRoot<UIButtonActor>("SIEMANDZIOBUTTON", "Profile", nullptr, Transform(Vec2f(0.4f, -0.8f), Vec2f(0.1f)));
				profilerButton.SetOnClickFunc([&]() {
					auto& profilerWindow = editorScene.CreateActorAtRoot<UIWindowActor>("Profiler window");

					// Additional info
					auto& additionalInfoActor = profilerWindow.CreateChild<UIActorDefault>("AdditionalFPSData", Transform(Vec2f(1.5f, 0.0f), Vec2f(1.0f)));

					// Actual graph
					auto graphMaterial = MakeShared<Material>("GraphMaterial", graphShader);
					RenderEng.AddMaterial(graphMaterial);
					auto& graphButton = profilerWindow.CreateChild<UIScrollBarActor>("GraphActor");
					graphButton.DeleteButtonModel();
					auto& fpsGraph = graphButton.CreateComponent<FPSGraphRenderingComponent>("FPSGraph");
					//fpsGraph.SetGraphView(Transform(Vec2f(0.0f), Vec2f(30.0f, 2000.0F)));
					fpsGraph.SetGraphView(Transform(Vec2f(0.0f), Vec2f(30.0f, 240.0f)));

					graphButton.SetOnBeingClickedFunc([&] {
						float rangeEndpoint = (glm::inverse(profilerWindow.UICanvas::GetViewMatrix()) * Vec4f(profilerWindow.ToCanvasSpace(graphButton.GetClickPosNDC()), 0.0f, 1.0f)).x * 0.5f + 0.5f;
						fpsGraph.GetSelectedRange().SetEndpoint1(rangeEndpoint);
						fpsGraph.GetSelectedRange().SetEndpoint2(rangeEndpoint);
					});
					graphButton.SetWhileBeingClickedFunc([&] {
						float rangeEndpoint = (glm::inverse(profilerWindow.UICanvas::GetViewMatrix()) * Vec4f(profilerWindow.ToCanvasSpace(editorScene.GetUIData()->GetWindowData().GetMousePositionNDC()), 0.0f, 1.0f)).x * 0.5f + 0.5f;
						fpsGraph.GetSelectedRange().SetEndpoint2(rangeEndpoint);
						fpsGraph.UpdateFPSTextInfo();
					});
					graphButton.SetOnDoubleClickFunc([&]() { fpsGraph.GetSelectedRange().SetEndpoints(Vec2f(0.0f, 1.0f)); fpsGraph.UpdateFPSTextInfo(); });

					/*fpsGraph.PushRawMarker(Vec2f(0.0f, 0.3f));
					fpsGraph.PushRawMarker(Vec2f(0.2f, 0.7f));
					fpsGraph.PushRawMarker(Vec2f(0.4f, 0.1f));
					fpsGraph.PushRawMarker(Vec2f(0.6f, 0.5f));
					fpsGraph.PushRawMarker(Vec2f(0.6f, 0.0f));
					fpsGraph.PushRawMarker(Vec2f(0.8f, 0.9f));
					fpsGraph.PushRawMarker(Vec2f(1.0f, 0.9f));*/

					// Axis unit texts
					graphButton.CreateComponent<TextComponent>("XUnitMain", Transform(Vec2f(1.0f, -1.0f), Vec2f(0.1f)), "[seconds]", "", Alignment2D::LeftTop());
					graphButton.CreateComponent<TextComponent>("YUnitMain", Transform(Vec2f(-1.0f, 1.0f), Vec2f(0.1f)), "[FPS]", "", Alignment2D::RightBottom());

					graphButton.CreateComponent<TextComponent>("XUnitText1", Transform(Vec2f(-1.0f, -1.0f), Vec2f(0.1f)), "-30", "", Alignment2D::LeftTop());
					graphButton.CreateComponent<TextComponent>("XUnitText2", Transform(Vec2f(0.0f, -1.0f), Vec2f(0.1f)), "-15", "", Alignment2D::Top());
					graphButton.CreateComponent<TextComponent>("XUnitText3", Transform(Vec2f(1.0f, -1.0f), Vec2f(0.1f)), "0", "", Alignment2D::RightBottom());

				graphButton.CreateComponent<TextComponent>("YUnitText1", Transform(Vec2f(-1.0f, -1.0f), Vec2f(0.1f)), "0", "", Alignment2D::RightBottom());
				TextComponent* secondImportant = &graphButton.CreateComponent<TextComponent>("YUnitText2", Transform(Vec2f(-1.0f, 0.0f), Vec2f(0.1f)), "150", "", Alignment2D::RightCenter());
				TextComponent* important = &graphButton.CreateComponent<TextComponent>("YUnitText3", Transform(Vec2f(-1.0f, 1.0f), Vec2f(0.1f)), "240", "", Alignment2D::RightTop());


				float* maxFPS = new float(240.0f);
				auto& unitInputBox = graphButton.CreateChild<UIInputBoxActor>("FPSUnitButton", [=, &fpsGraph](float val) mutable { *maxFPS = val; fpsGraph.SetGraphView(Transform(Vec2f(0.0f), Vec2f(30.0f, *maxFPS))); important->SetContent(ToStringPrecision (*maxFPS, 0)); secondImportant->SetContent(ToStringPrecision(*maxFPS / 2.0f, 0)); }, [=, &fpsGraph]() { return *maxFPS; });
				unitInputBox.SetTransform(Transform(Vec2f(-0.5f, -1.2f), Vec2f(0.2f)));
				auto& infoText = graphButton.CreateComponent<TextComponent>("InfoText", Transform(Vec2f(-0.0f, 1.0f), Vec2f(0.05f)), "FPS - the lower the better\n<Hold SHIFT while dragging mouse to inspect a range>", "", Alignment2D::Bottom());
				auto greyMaterial = MakeShared<Material>("Grey");
				greyMaterial->SetColor(Vec4f(Vec3f(0.5f), 1.0f));
				infoText.SetMaterialInst(RenderEng.AddMaterial(greyMaterial));

					profilerWindow.AutoClampView();
				});
				mainButtonTheme(profilerButton, 4.0f);
			}
		 

			Actor& cameraActor = editorScene.CreateActorAtRoot<Actor>("OrthoCameraActor");
			CameraComponent& orthoCameraComp = cameraActor.CreateComponent<CameraComponent>("OrthoCameraComp", glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -2550.0f));

			editorScene.BindActiveCamera(&orthoCameraComp);


			EditorScene = &editorScene;

			EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->AddMeshInst(GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
			//EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->SetTransform(Transform(Vec2f(0.0f, 0.4f), Vec2f(1.0f)));

			SharedPtr<Material> scenePreviewMaterial = MakeShared<Material>("GEE_3D_SCENE_PREVIEW_MATERIAL");
			RenderEng.AddMaterial(scenePreviewMaterial);
			scenePreviewMaterial->AddTexture(MakeShared<NamedTexture>(ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorTexture(0), "albedo1"));
			EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->OverrideInstancesMaterial(scenePreviewMaterial);
		}

		void GameEngineEngineEditor::SetupMainMenu()
		{
			GameScene& mainMenuScene = CreateUIScene("GEE_Main_Menu", *Window);

			//SetCanvasContext(&mainMenuScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Canvas_Context"));

			{
				Actor& engineTitleActor = mainMenuScene.CreateActorAtRoot<Actor>("EngineTitleActor", Transform(Vec2f(0.0f, 0.6f)));
				TextComponent& engineTitleTextComp = engineTitleActor.CreateComponent<TextComponent>("EngineTitleTextComp", Transform(), "Game Engine Engine", "", Alignment2D::Center());
				engineTitleTextComp.SetFontStyle(FontStyle::Bold);
				engineTitleTextComp.SetTransform(Transform(Vec2f(0.0f), Vec2f(0.09f)));

				SharedPtr<Material> titleMaterial = MakeShared<Material>("GEE_Engine_Title");
				RenderEng.AddMaterial(titleMaterial);
				titleMaterial->SetColor(hsvToRgb(Vec3f(300.0f, 1.0f, 1.0f)));

				engineTitleTextComp.SetMaterialInst(MaterialInstance(titleMaterial));

				TextComponent& engineSubtitleTextComp = engineTitleActor.CreateComponent<TextComponent>("EngineSubtitleTextComp", Transform(), "made by swetroniusz", "", Alignment2D::Center());
				engineSubtitleTextComp.SetTransform(Transform(Vec2f(0.0f, -0.15f), Vec2f(0.045f)));
				engineSubtitleTextComp.SetFontStyle(FontStyle::BoldItalic);
				auto  subtitleMaterial = RenderEng.AddMaterial(MakeShared<Material>("GEE_Engine_Subtitle"));
				subtitleMaterial->SetColor(hsvToRgb(Vec3f(300.0f, 0.4f, 0.3f)));
				engineSubtitleTextComp.SetMaterialInst(subtitleMaterial);
			}

			mainMenuScene.CreateActorAtRoot<UIButtonActor>("NewProjectButton", "New project", [this, &mainMenuScene]() {
				UIWindowActor& window = mainMenuScene.CreateActorAtRoot<UIWindowActor>("ProjectCreatorWindow");
				window.SetTransform(Transform(Vec2f(0.0f), Vec2f(0.5f)));

				ProjectFilepath = "Projects/";
				window.AddField("Project folder").GetTemplates().FolderInput([this](const std::string& str) { ProjectFilepath = str + "/"; }, [this]() { return ProjectFilepath; });

				UIInputBoxActor& projectNameInputBox = window.AddField("Project name").CreateChild<UIInputBoxActor>("ProjectNameInputBox");
				//UIInputBoxActor& projectNameInputBox

				ProjectName = "Project1";
				projectNameInputBox.SetOnInputFunc([this](const std::string& input) { ProjectName = input; }, [this]() { return ProjectName; });

				UIButtonActor& okButton = window.AddField("").CreateChild<UIButtonActor>("OKButton", "OK", [this, &window]() { window.MarkAsKilled(); LoadProject(ProjectFilepath + ProjectName + ".json"); });

				window.RefreshFieldsList();
				window.AutoClampView();

			}).SetTransform(Transform(Vec2f(-0.3f, 0.0f), Vec2f(0.2f)));

			mainMenuScene.CreateActorAtRoot<UIButtonActor>("LoadProjectButton", "Load project", [this, &mainMenuScene]() {
				UIWindowActor& window = mainMenuScene.CreateActorAtRoot<UIWindowActor>("ProjectLoaderWindow");
				window.SetTransform(Transform(Vec2f(0.0f), Vec2f(0.5f)));


				window.AddField("Project filepath").GetTemplates().PathInput([this](const std::string& filepath) { ProjectFilepath = ToRelativePath(filepath);  extractDirectoryAndFilename(filepath, ProjectName, std::string()); }, [this]() { return ProjectFilepath; }, { "*.json", "*.geeproject", "*.geeprojectold" });

				UIButtonActor& okButton = window.AddField("").CreateChild<UIButtonActor>("OKButton", "OK", [this, &window]() { window.MarkAsKilled(); LoadProject(ProjectFilepath); });

				window.RefreshFieldsList();
				window.AutoClampView();

			}).SetTransform(Transform(Vec2f(0.3f, 0.0f), Vec2f(0.2f)));

			{
				std::ifstream recentProjectsFile("recent.txt");
				std::vector <std::string> filepaths;
				std::string line;
				while (std::getline(recentProjectsFile, line))
				{
					if (!line.empty())
						filepaths.push_back(line);
				}
				recentProjectsFile.close();

				if (!filepaths.empty())
				{
					UIActorDefault& recentProjectsActor = mainMenuScene.CreateActorAtRoot<UIActorDefault>("Recent", Transform(Vec2f(0.0f, -0.5f), Vec2f(0.1f)));
					recentProjectsActor.CreateComponent<TextComponent>("RecentsText", Transform(Vec2f(0.0f, 1.6f), Vec2f(0.5f)), "Recent projects", "", Alignment2D::Center()).SetFontStyle(FontStyle::Bold);
					UIElementTemplates(recentProjectsActor).ListSelection<std::string>(filepaths.begin(), filepaths.end(), [this](UIAutomaticListActor& listActor, std::string& filepath) { listActor.CreateChild<UIButtonActor>("RecentFilepathButton", getFileName(filepath), [this, filepath]() { LoadProject(filepath); }, Transform(Vec2f(0.0f), Vec2f(9.0f, 1.0f))).GetRoot()->GetComponent<TextConstantSizeComponent>("ButtonText")->Unstretch(); });
				}
			}

			Actor& cameraActor = mainMenuScene.CreateActorAtRoot<Actor>("OrthoCameraActor");
			CameraComponent& orthoCameraComp = cameraActor.CreateComponent<CameraComponent>("OrthoCameraComp", glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -2550.0f));

			mainMenuScene.BindActiveCamera(&orthoCameraComp);
			SetActiveScene(&mainMenuScene);
		}

		void GameEngineEngineEditor::Update(float deltaTime)
		{
			if (GameController && GameController->IsBeingKilled())
				GameController = nullptr;

			if (EditorScene && GetMainScene() && GetMainScene()->GetActiveCamera() && GetInputRetriever().IsKeyPressed(Key::G))
			{
				if (TestTranslateLastPos != Vec2f(-1.0f))
				{
					Vec2f translationVec(((Vec2f)EditorScene->GetUIData()->GetWindowData().GetMousePositionPx() - TestTranslateLastPos) * 0.01f);
					Vec3f rightVector = glm::normalize(glm::cross(GetMainScene()->GetActiveCamera()->GetTransform().GetWorldTransform().GetFrontVec(), Vec3f(0.0f, 1.0f, 0.0f)));
					Vec3f upVector = glm::normalize(glm::cross(rightVector, GetMainScene()->GetActiveCamera()->GetTransform().GetWorldTransform().GetFrontVec()));
					std::cout << "Right vector: " << rightVector << '\n';
					Actions->GetContextComp()->GetTransform().Move(rightVector * translationVec.x + upVector * translationVec.y);
				}
				TestTranslateLastPos = EditorScene->GetUIData()->GetWindowData().GetMousePositionPx();
			}
			else
				TestTranslateLastPos = Vec2f(-1.0f);

			Game::Update(deltaTime);
		}

		void GameEngineEngineEditor::HandleEvents()
		{
			if (!ActiveScene)
			{
				std::cerr << "ERROR! No scene was set as active. Events will not be handled by any scene.\n";
				return;
			}

			while (SharedPtr<Event> polledEvent = EventHolderObj.PollEvent(*Window))
			{
				LastRenderPopupRequest = nullptr;
				if (polledEvent->GetType() == EventType::KeyPressed)
				{
					KeyEvent& keyEventCast = dynamic_cast<KeyEvent&>(*polledEvent);
					if (ActiveScene == EditorScene && keyEventCast.GetKeyCode() == Key::Escape)
					{
						GenericUITemplates(*EditorScene).ConfirmationBox([=]() { SaveProject(); TerminateGame(); }, [=]() { TerminateGame(); }, "Save the project?");
					}
					else if (keyEventCast.GetKeyCode() == Key::Tab)
					{
						std::cout << "Pressed TAB!\n";
						bool sceneChanged = false;
						// If the editor is currently active, set the active scene to the main one and pass control to a game controller.
						if (ActiveScene == EditorScene && GetMainScene())
						{
							if (!GameController)
							{
								std::vector<Controller*> controllers;
								GetMainScene()->GetRootActor()->GetAllActors(&controllers);
								if (!controllers.empty())
									GameController = controllers[0];
							}
							if (GameController)
							{
								ActiveScene = GetMainScene();
								PassMouseControl(GameController);
								sceneChanged = true;
							}
						}
						// Otherwise, set the active scene to the editor scene and release control.
						else if (EditorScene)
						{
							PassMouseControl(nullptr);
							sceneChanged = true;
							ActiveScene = EditorScene;
						}

						if (sceneChanged)
							for (auto& it : Scenes)
								it->RootActor->HandleEventAll(Event(EventType::FocusSwitched));
					}
					else if (keyEventCast.GetKeyCode() == Key::F10)
					{
						MaximizeViewport();
					}
					else if (keyEventCast.GetKeyCode() == Key::F12)
					{
							if (!Profiler.HasBeenStarted())
							{
								Profiler.StartProfiling(GetProgramRuntime());
								GEE_LOG("Started profiling.");
							}
							else
							{
								Profiler.StopAndSaveToFile(GetProgramRuntime());
								GEE_LOG("Stopped profiling.");
							}
					}
					else if (ActiveScene == EditorScene && keyEventCast.GetKeyCode() == Key::S && keyEventCast.GetModifierBits() & KeyModifierFlags::Control)
						SaveProject();
			}

				if (ActiveScene)
					ActiveScene->HandleEventAll(*polledEvent);

				if (LastRenderPopupRequest)
					LastRenderPopupRequest();
			}


			for (auto& popup : OpenPopups)
			{
				while (SharedPtr<Event> polledEvent = EventHolderObj.PollEvent(popup.Window))
				{
					popup.Scene.get().HandleEventAll(*polledEvent);
				}
			}
		}

		/*void GameEngineEngineEditor::PreviewHierarchyTree(HierarchyTemplate::HierarchyTreeT& tree)
		{
			GameScene& editorScene = *EditorScene;
			UIWindowActor& previewWindow = editorScene.CreateActorAtRoot<UIWindowActor>("Mesh node preview window");

			previewWindow.AddField("Tree origin").CreateChild<UIButtonActor>("OriginButton", (tree.GetName().IsALocalResource()) ? ("Local") : ("File")).SetDisableInput(true);
			previewWindow.AddField("Delete tree").CreateChild<UIButtonActor>("DeleteButton", "Delete").SetDisableInput(true);
			auto& list = previewWindow.AddField("Mesh nodes list").CreateChild<UIAutomaticListActor>("MeshNodesList");

			
			std::vector<std::pair<HierarchyTemplate::HierarchyNodeBase&, UIActivableButtonActor*>> buttons;

			std::function<void(HierarchyTemplate::HierarchyNodeBase&, UIAutomaticListActor&)> createNodeButtons = [&](HierarchyTemplate::HierarchyNodeBase& node, UIAutomaticListActor& listParent) {
				auto& element = listParent.CreateChild<UIActivableButtonActor>("Button", node.GetCompBaseType().GetName(), nullptr);
				buttons.push_back(std::pair<HierarchyTemplate::HierarchyNodeBase&, UIActivableButtonActor*>(node, & element));
				element.SetDeactivateOnClickAnywhere(false);
				element.OnClick();
				element.DeduceMaterial();
				element.SetTransform(Transform(Vec2f(1.5f, 0.0f), Vec2f(3.0f, 1.0f)));
				element.SetPopupCreationFunc([](PopupDescription desc) { desc.AddOption("It works", nullptr); desc.AddSubmenu("Add node", [](PopupDescription desc) { desc.AddOption("Component", nullptr); }); });
				element.SetOnClickFunc(
					[&]() {
						auto& nodeWindow = editorScene.CreateActorAtRoot<UIWindowActor>("Node preview window");
						node.GetCompBaseType().GetEditorDescription(EditorDescriptionBuilder(*this, nodeWindow, nodeWindow));

						nodeWindow.RefreshFieldsList();
						nodeWindow.AutoClampView();
					});

				for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
				{
					HierarchyTemplate::HierarchyNodeBase& child = *node.GetChild(i);
					UIAutomaticListActor& nestedList = listParent.CreateChild<UIAutomaticListActor>(child.GetCompBaseType().GetName() + "'s nestedlist");
					createNodeButtons(child, nestedList);
				}
			};

			createNodeButtons(tree.GetRoot(), list);

			UIButtonActor& instantiateButton = list.CreateChild<UIButtonActor>("InstantiateButton", "Instantiate", [this, &tree, &editorScene, buttons]() {
				std::function<Component* ()> getSelectedComp = [this]() -> Component* { return ((Actions->GetSelectedComponent()) ? (Actions->GetSelectedComponent()) : ((Actions->GetSelectedActor()) ? (Actions->GetSelectedActor()->GetRoot()) : (nullptr))); };

				if (Component* selectedComp = getSelectedComp())
					EngineDataLoader::InstantiateTree(*selectedComp, tree);

				// Refresh
				Actions->SelectActor(Actions->GetSelectedActor(), editorScene);
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
		}*/

		void GameEngineEngineEditor::PassMouseControl(Controller* controller)
		{
			if (ActiveScene == EditorScene)
				GameController = controller;
			else
				Game::PassMouseControl(controller);
		}

		void GameEngineEngineEditor::Render()
		{
			EditorRenderer(*this, *ViewportRenderCollection, *HUDRenderCollection, *Window).RenderPass(GetMainScene(), EditorScene, GetScene("GEE_Main_Menu"), GetScene("GEE_Mesh_Preview_Scene"));
		}

		void GameEngineEngineEditor::LoadProject(const std::string& filepath)
		{
			ProjectFilepath = filepath;

			std::cout << "Opening project with filepath: " << filepath << '\n';
			SetupEditorScene();
			EditorScene = GetScene("GEE_Editor");
			if (GameScene* mainMenuScene = GetScene("GEE_Main_Menu"))
				mainMenuScene->MarkAsKilled();
			if (!LoadSceneFromFile(filepath, "GEE_Main"))	// Load file, and if file not found, load a simple scene
			{
				auto scene = GetScene("GEE_Main");

				////
				auto& cameraActor = scene->CreateActorAtRoot<Actor>("A Camera");
				auto& camera = cameraActor.CreateComponent<CameraComponent>("A CameraComponent", glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 100.0f));
				scene->BindActiveCamera(&camera);

				cameraActor.CreateChild<FreeRoamingController>("A FreeRoamingController").SetPossessedActor(&cameraActor);

				////
				auto& sphereActor = scene->CreateActorAtRoot<Actor>("Default sphere", Transform(Vec3f(0.0f, 0.0f, -3.0f)));
				sphereActor.CreateComponent<ModelComponent>("A ModelComponent").AddMeshInst(RenderEng.GetBasicShapeMesh(EngineBasicShape::SPHERE));
				sphereActor.CreateComponent<LightComponent>("A LightComponent").GetTransform().SetPosition(Vec3f(0.0f, 0.0f, 2.0f));
			}
			SetMainScene(GetScene("GEE_Main"));
			SetActiveScene(EditorScene);
			Actions->SelectScene(GetMainScene(), *EditorScene);
		
			SceneRenderer(RenderEng, 0).PreRenderLoopPassStatic(0, GetSceneRenderDatas());

			UpdateRecentProjects();

			glfwRequestWindowAttention(Window);
			glfwSetWindowTitle(Window, filepath.c_str());
		}

		void GameEngineEngineEditor::SaveProject()
		{
			if (getFilepathExtension(ProjectFilepath) == ".geeprojectold")
				ProjectFilepath = ProjectFilepath.substr(0, ProjectFilepath.find(".geeprojectold")) + ".json";
			std::cout << "Saving project to path " << ProjectFilepath << "\n";

			GetEditorLogger(*EditorScene).Log("Saving project", EditorMessageLogger::MessageType::Information);

			std::stringstream serializationStream(ProjectFilepath);
			try
			{
				cereal::JSONOutputArchive archive(serializationStream);
				GetMainScene()->Save(archive);
				archive(CEREAL_NVP(bDebugRenderComponents));
				archive.serializeDeferments();
			}
			catch (cereal::Exception& exception)
			{
				std::ofstream brokenFileOutput(ProjectFilepath + "broken");	// Do not save to file if the process wasn't successful; we don't want to corrupt existing data.
				brokenFileOutput << serializationStream.rdbuf();
				GetEditorLogger(*EditorScene).Log(exception.what(), EditorMessageLogger::MessageType::Error);
				return;
			}

			std::ofstream workingFileOutput(ProjectFilepath);
			workingFileOutput << serializationStream.str();
			workingFileOutput.close();	// serialization stream must be closed after destroying the archive

			UpdateRecentProjects();

			std::cout << "Project " + ProjectName + " (" + ProjectFilepath + ") saved successfully.\n";
			GetEditorLogger(*EditorScene).Log("Project saved", EditorMessageLogger::MessageType::Success);
		}

		void GameEngineEngineEditor::SetDebugRenderComponents(bool debugRenderComponents)
		{
			bDebugRenderComponents = debugRenderComponents;
		}

		const std::vector<EditorPopup>& GameEngineEngineEditor::GetPopupsForRendering()
		{
			return OpenPopups;
		}

		void GameEngineEngineEditor::UpdateRecentProjects()
		{
			std::vector<std::string> filepaths;
			{
				std::ifstream recentProjectsInput("recent.txt");

				std::string line;
				while (std::getline(recentProjectsInput, line) && filepaths.size() < 4)	//do not go past the limit of 4 filepaths
					if (line != ProjectFilepath)
						filepaths.push_back(line);

				recentProjectsInput.close();
			}

			{
				std::ofstream recentProjectsOutput("recent.txt");

				recentProjectsOutput << ProjectFilepath << '\n';
				for (auto& it : filepaths)
					recentProjectsOutput << it << '\n';

			recentProjectsOutput.close();
		}
	}

	void GameEngineEngineEditor::MaximizeViewport()
	{
		glm::ivec2 windowSize;
		glfwGetWindowSize(Window, &windowSize.x, &windowSize.y);
		if (!bViewportMaximized)
		{
			GetGameSettings()->Video.Resolution = windowSize;
			GetScene("GEE_Editor")->FindActor("SceneViewportActor")->SetTransform(Transform(Vec2f(0.0f), Vec2f(1.0f)));
		}
		else
		{
			GetGameHandle()->GetGameSettings()->Video.Resolution = static_cast<Vec2f>(windowSize) * Vec2f(0.4f, 0.6f);
			GetScene("GEE_Editor")->FindActor("SceneViewportActor")->SetTransform(Transform(Vec2f(0.0f, 0.4f), Vec2f(0.4f, 0.6f)));
		}

		UpdateGameSettings();
		bViewportMaximized = !bViewportMaximized;
	}

	EditorMessageLogger& GameEngineEngineEditor::GetEditorLogger(GameScene& scene)
	{
		if (Logs.find(&scene) == Logs.end())
			Logs.emplace(std::make_pair(&scene, EditorMessageLogger(*this, scene)));

		return Logs.find(&scene)->second;
	}

	}
}