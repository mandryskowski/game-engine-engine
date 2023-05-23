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
#include <whereami.h>
#include <sstream>
#include <fstream>
#include <thread>
#include <algorithm>
#include <game/IDSystem.h>
#include <condition_variable>
#include <utility/Log.h>
#include <rendering/OutlineRenderer.h>
#include <editor/EditorRenderer.h>
#include <editor/DefaultEditorController.h>
#include <editor/GEditorToolbox.h>


#include <editor/MousePicking.h>
#include <editor/GraphRenderingComponent.h>

#include <rendering/Renderer.h>

#include "rendering/Framebuffer.h"


namespace GEE
{
	namespace Editor
	{
		void EditorEventProcessor::FileDropCallback(SystemWindow* window, int count, const char** paths)
		{
			if (count > 0)
			{
				std::cout << "Dropped " << paths[0] << '\n';
				auto addTree = [](const String& path) { EditorHandle->GetActions().PreviewHierarchyTree(*EngineDataLoader::LoadHierarchyTree(*EditorHandle->GetGameHandle()->GetMainScene(), path)); };
				String path = paths[0];
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
				EditorHandle->GetGameHandle()->GetGameSettings()->Video.Resolution = static_cast<Vec2f>(newSize) * Vec2f(0.7f);
				EditorHandle->UpdateGameSettings();
			}

			if (EditorHandle->GetEditorSettings()->Video.Resolution != static_cast<Vec2f>(newSize))
			{
				EditorHandle->GetEditorSettings()->Video.Resolution = newSize;
				EditorHandle->UpdateEditorSettings();
			}

			auto scene = EditorHandle->GetGameHandle()->GetScene("GEE_Editor");
			if (!scene)
				scene = EditorHandle->GetGameHandle()->GetActiveScene();

			scene->HandleEventAll(WindowResizedEvent());
			std::cout << "New res " << EditorHandle->GetGameHandle()->GetGameSettings()->Video.Resolution << '\n';
		}


		GameEngineEngineEditor::GameEngineEngineEditor(SystemWindow* window, const GameSettings& settings) :
			Game(settings.Video.Shading, settings),
			EditorScene(nullptr),
			GameController(nullptr),
			EditorController(nullptr),
			LastRenderPopupRequest(nullptr),
			bViewportMaximized(false),
			TestTranslateLastPos(Vec2f(0.0f)),
			Actions(MakeUnique<EditorActions>(*this))
		{
			{
				int length = wai_getExecutablePath(nullptr, 0, nullptr), dirnameLength = 0;
				String path;
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

		bool GameEngineEngineEditor::GameLoopIteration(Time timeStep, Time deltaTime)
		{
			bool returnVal = Game::GameLoopIteration(timeStep, deltaTime);

			//////////////////////POPUPS
			OpenPopups.erase(std::remove_if(OpenPopups.begin(), OpenPopups.end(), [this](EditorPopup& popup) { if (glfwWindowShouldClose(&popup.Window.get())) { glfwDestroyWindow(&popup.Window.get()); RenderEng.EraseRenderTbCollection(popup.TbCol); return true; } return false; }), OpenPopups.end());

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

		template <typename T>
		void AddActorOption(PopupDescription desc)
		{
			desc.AddOption(GEERegisteredActor<T>::GetName(), []() { GEERegisteredActor<T>::InstantiateDefault(); });
		}

		void GameEngineEngineEditor::GenerateActorList(PopupDescription desc, std::function<void(Actor&)> func)
		{
			auto& actor = *Actions->GetContextActor();
			desc.AddOption("Actor", [&, func]() { func(actor.CreateChild<Actor>("An Actor")); }, IconData(RenderEng, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
			desc.AddOption("Pawn", [&, func]() { func(actor.CreateChild<PawnActor>("A PawnActor")); }, IconData(RenderEng, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
			desc.AddOption("Roaming Controller", [&, func]() { func(actor.CreateChild<FreeRoamingController>("A FreeRoamingController")); }, IconData(RenderEng, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
			desc.AddOption("FPS Controller", [&, func]() { func(actor.CreateChild<FPSController>("A FPSController")); }, IconData(RenderEng, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
			desc.AddOption("Shooting Controller", [&, func]() { func(actor.CreateChild<ShootingController>("A ShootingController")); }, IconData(RenderEng, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
			desc.AddOption("Gun Actor", [&, func]() { func(actor.CreateChild<GunActor>("A GunActor")); }, IconData(RenderEng, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
			//desc.AddOption("Cue Controller", [&, func]() { func(actor.CreateChild<CueController>("A CueController")); }, IconData(RenderEng, "Assets/Editor/component_icon.png", Vec2i(3, 1), 0.0f));
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

		String GameEngineEngineEditor::ToRelativePath(const String& filepath)
		{
			std::cout << "Executable folder: " << ExecutableFolder << '\n';
			if (auto foundPos = filepath.find(ExecutableFolder); foundPos != String::npos)
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
			glfwSetDropCallback(GameWindow, EditorEventProcessor::FileDropCallback);
			glfwSetFramebufferSizeCallback(GameWindow, EditorEventProcessor::Resize);

			glfwSetWindowCloseCallback(GameWindow, [](SystemWindow* window) { GameScene* scene = static_cast<GameScene*>(glfwGetWindowUserPointer(window)); GameEngineEngineEditor* thisPtr = dynamic_cast<GameEngineEngineEditor*>(scene->GetGameHandle()); GenericUITemplates(*scene).ConfirmationBox([scene, thisPtr]() { thisPtr->SaveProject(); thisPtr->TerminateGame(); }, [thisPtr]() { thisPtr->TerminateGame(); }, "Save the project?"); });


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
			uiSettings.bDrawToWindowFBO = true;
			uiSettings.POMLevel = SettingLevel::SETTING_NONE;
			uiSettings.Resolution = WindowData(*GameWindow).GetWindowSize();
			uiSettings.Shading = ShadingAlgorithm::SHADING_FULL_LIT;
			uiSettings.ShadowLevel = SettingLevel::SETTING_NONE;
			uiSettings.TMType = ToneMappingType::TM_NONE;
			uiSettings.MonitorGamma = 1.0f;

			ViewportRenderCollection = &RenderEng.AddRenderTbCollection(MakeUnique<RenderToolboxCollection>("GameSceneRenderCollection", Settings->Video, RenderEng));
			
			{
				auto hudCol = MakeUnique<GEditorRenderToolboxCollection>("HUDRenderCollection", uiSettings, RenderEng);
				HUDRenderCollection = hudCol.get();
				RenderEng.AddRenderTbCollection(static_unique_pointer_cast<RenderToolboxCollection, GEditorRenderToolboxCollection>(std::move(hudCol)));
			}

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
			SystemWindow* window = glfwCreateWindow(240, 30, "popup", nullptr, GameWindow);
			glfwSetWindowPos(window, Math::Round(posScreenSpace.x), Math::Round(posScreenSpace.y));

			glfwWindowHint(GLFW_RESIZABLE, true);
			glfwWindowHint(GLFW_DECORATED, true);

			glfwSetCursorPosCallback(window, WindowEventProcessor::CursorPosCallback);
			glfwSetMouseButtonCallback(window, WindowEventProcessor::MouseButtonCallback);
			glfwSetCursorEnterCallback(window, WindowEventProcessor::CursorLeaveEnterCallback);
			glfwSetCharCallback(window, WindowEventProcessor::CharEnteredCallback);
			glfwSetKeyCallback(window, WindowEventProcessor::KeyPressedCallback);
			glfwSetScrollCallback(window, WindowEventProcessor::ScrollCallback);
			glfwSetDropCallback(window, WindowEventProcessor::FileDropCallback);
			glfwSetWindowFocusCallback(window, [](SystemWindow* window, int focused) { if (!focused) glfwSetWindowShouldClose(window, GLFW_TRUE); });

			GameScene& popupScene = CreateUIScene("GEE_E_Popup_Window", *window, false);

			glfwSetWindowUserPointer(window, &popupScene);

			{
				auto settings = MakeShared<GameSettings::VideoSettings>();
				auto& tbCol = RenderEng.AddRenderTbCollection(MakeUnique<RenderToolboxCollection>("PopupTbCol", *settings, RenderEng), false);
				OpenPopups.push_back(EditorPopup(static_cast<unsigned>(IDSystem<EditorPopup>::GenerateID()), *window, popupScene, tbCol, settings));

			}
			auto& editorPopup = OpenPopups.back();

			auto& popupCanvas = popupScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Popup_Canvas", Transform(Vec2f(0.0f), Vec2f(1.0f)));

			auto& buttonList = popupCanvas.CreateChild<UIAutomaticListActor>("PopupButtonList");
			buttonList.Refresh();

			return PopupDescription(editorPopup, &buttonList, &popupCanvas);
		}

		void GameEngineEngineEditor::UpdateGameSettings()
		{
			enum class CurrPreviewTexType
			{
				Position,
				Normal,
				Albedo,
				Final
			} previewTexType = CurrPreviewTexType::Final;

			//Update scene preview texture, since we created a new one
			Material* previewMaterial = RenderEng.FindMaterial("GEE_3D_SCENE_PREVIEW_MATERIAL").get();
			if (!previewMaterial || previewMaterial->GetTextureCount() == 0)
			{
				std::cout << "ERROR: No scene preview material detected.\n";
				return;
			}
			else
			{
				auto tex = previewMaterial->GetTexture(0);
				if (tex.GetID() == ViewportRenderCollection->GetTb<DeferredShadingToolbox>()->GetGeometryFramebuffer()->GetColorTexture(0).GetID())
					previewTexType = CurrPreviewTexType::Albedo;
			}			

			ViewportRenderCollection->AddTbsRequiredBySettings();
			for (auto& it : Scenes)
				it->GetRenderData()->InvalidateAllShadowmaps();

			if (previewMaterial)
			{
				previewMaterial->Textures.clear();

				SharedPtr<NamedTexture> tex = nullptr;
				switch (previewTexType)
				{
				case CurrPreviewTexType::Albedo: tex = MakeShared<NamedTexture>(ViewportRenderCollection->GetTb<DeferredShadingToolbox>()->GetGeometryFramebuffer()->GetColorTexture(0), "albedo1"); break;
				case CurrPreviewTexType::Final:
				default: tex = MakeShared<NamedTexture>(ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorTexture(0), "albedo1"); break;
				}

				previewMaterial->AddTexture(tex);
			}
		}

		void GameEngineEngineEditor::UpdateEditorSettings()
		{
			HUDRenderCollection->AddTbsRequiredBySettings();
		}

		void GameEngineEngineEditor::SetupEditorScene()
		{
			GameScene& editorScene = CreateUIScene("GEE_Editor", *GameWindow);

			EditorController = &editorScene.CreateActorAtRoot<Editor::DefaultEditorController>("GEE_Default_Editor_Controller");

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
				auto& scenePreviewActor = editorScene.CreateActorAtRoot<UIButtonActor>("SceneViewportActor", nullptr, Transform(Vec2f(0.0f, 0.25f), Vec2f(0.7f)));
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
							if (EditorSettings.bDebugRenderComponents)
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

					if (pickedComponent && GetDefInputRetriever().IsKeyPressed(Key::LeftAlt)) // Pick the root component if left alt is pressed
						pickedComponent = pickedComponent->GetActor().GetRoot();

					if (pickedComponent && pickedComponent != Actions->GetSelectedComponent())
					{
						Actions->SelectActor(&pickedComponent->GetActor(), editorScene);
						Actions->SelectComponent(pickedComponent, editorScene);
					}
					else
						Actions->SelectActor(nullptr, editorScene);

				});

				UIActorDefault& controllerInfo = scenePreviewActor.CreateChild<UIActorDefault>("ControllerCameraInfoTextActor", Transform(Vec2f(-0.45f, -1.07f), Vec2f(0.5f, 0.025f)));
				controllerInfo.CreateComponent<TextComponent>("ControllerInfoText", Transform(Vec2f(0.0f, 1.5f)), "", "", Alignment2D::Center()).SetMaxSize(Vec2f(1.0f));
				controllerInfo.CreateComponent<TextComponent>("CameraInfoText", Transform(Vec2f(0.0f, -1.5f)), "", "", Alignment2D::Center()).SetMaxSize(Vec2f(1.0f));
				UpdateControllerCameraInfo();



				UIButtonActor& selectPreviewButton = scenePreviewActor.CreateChild<UIButtonActor>("SelectPreview_Empty", "Final", nullptr, Transform(Vec2f(0.6f, -1.07f), Vec2f(0.4f, 0.05f)));
				auto selectPreviewButtonText = selectPreviewButton.GetRoot()->GetComponent<TextComponent>("ButtonText");
				selectPreviewButtonText->SetMaxSize(Vec2f(1.0f));
				selectPreviewButtonText->Unstretch();
				//selectPreviewButtonText->SetMaxSize(Vec2f(1.0f, 1.0f / 8.0f));
				selectPreviewButton.SetOnClickFunc([&, selectPreviewButtonText]() {
						PopupDescription desc = CreatePopupMenu(editorScene.GetUIData()->GetWindowData().GetMousePositionNDC(), *editorScene.GetUIData()->GetWindow());

						auto addPreview = [&, selectPreviewButtonText](PopupDescription previewDesc, const String& name, std::function<Texture()> getTextureToPreview, float iconAtlasID) {
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

				UIActorDefault& forwardShadingSwitch = scenePreviewActor.CreateChild<UIActorDefault>("ForwardShadingSwitch", Transform(Vec2f(0.1f, -1.07f), Vec2f(0.05f)));
				forwardShadingSwitch.CreateComponent<TextComponent>("SwitchForwardText", Transform(Vec2f(0.0f, -1.4f), Vec2f(1.0f, 0.4f)), "Forward", "", Alignment2D::Center()).SetMaxSize(Vec2f(1.0f));

				UIElementTemplates(forwardShadingSwitch).TickBox([this, &selectPreviewButton, selectPreviewButtonText, &scenePreviewQuad](bool val)
				{
					selectPreviewButton.SetDisableInput(val);
					selectPreviewButton.GetButtonModel()->SetHide(val);
					selectPreviewButtonText->SetHide(val);
					GetGameSettings()->Video.bForceForwardRendering = val; }, [this]() { return GetGameSettings()->Video.bForceForwardRendering;
				});
			}

			UIActorDefault& scaleActor = editorScene.CreateActorAtRoot<UIActorDefault>("TextTestButton", Transform(Vec2f(0.0f, 0.0f), Vec2f(0.1f, 0.1f)));

			// Canvas lists
			{
				auto& leftList = editorScene.CreateActorAtRoot<UIListActor>("GEE_E_Left_Canvas_List");
				leftList.SetTransform(Transform(Vec2f(-0.85f, 1.0f), Vec2f(0.15f, 0.02f)));
				auto& rightList = editorScene.CreateActorAtRoot<UIListActor>("GEE_E_Right_Canvas_List");
				rightList.SetTransform(Transform(Vec2f(0.85f, 1.0f), Vec2f(0.15f, 0.02f)));
				auto& bottomList = editorScene.CreateActorAtRoot<UIListActor>("GEE_E_Bottom_Canvas_List");
				bottomList.SetTransform(Transform(Vec2f(0.0f, -0.5f), Vec2f(0.15f, 0.02f)));

				auto& addCanvasButton = leftList.CreateChild<UIButtonActor>("AddCanvas", "+", []()
				{

				}, Transform(Vec2f(), Vec2f(1.0f, 1.0f)));
				addCanvasButton.SetPopupCreationFunc([this, &editorScene, &leftList](PopupDescription desc)
				{
					desc.AddOption("Materials", [this, &editorScene, &leftList]()
					{
						auto canvasPair = Actions->AddToCanvasList("GEE_E_Materials_Preview");
						auto& canvas = canvasPair.first;
						auto& canvasList = canvasPair.second;


						UIAutomaticListActor& list = canvas.CreateChild<UIAutomaticListActor>("MaterialsList");

						UIButtonActor& addMaterialButton = list.CreateChild<UIButtonActor>("CreateMaterialButton", [&]() {
							const auto& view = canvas.GetViewT();
						GetRenderEngineHandle()->AddMaterial(MakeShared<AtlasMaterial>("CreatedMaterial" + std::to_string(IDSystem<Material>::GenerateID())));
						canvas.MarkAsKilled();
						//materialsButton.OnClick();
						if (auto newWindow = dynamic_cast<UICanvasActor*>(editorScene.FindActor("MaterialsPreviewWindow")))
							newWindow->SetCanvasView(view);
							});
						uiButtonActorUtil::ButtonMatsFromAtlas(addMaterialButton, GetRenderEngineHandle()->FindMaterial<AtlasMaterial>("GEE_E_Add_Icon_Mat"), 0.0f, 1.0f, 2.0f);

						for (auto& it : RenderEng.Materials)
						{
							String name = (it) ? (it->GetLocalization().GetFullStr()) : ("NULLPTR???");
							UIButtonActor& button = list.CreateChild<UIButtonActor>(name + "Button", name, [this, &canvas, &it, name]() { UIWindowActor& matWindow = canvas.CreateChildCanvas<UIWindowActor>(name + "MaterialWindow"); EditorDescriptionBuilder descBuilder(*this, matWindow, matWindow); it->GetEditorDescription(descBuilder); matWindow.AutoClampView(); matWindow.RefreshFieldsList(); });
							std::cout << name << '\n';
						}

						const auto maxSize = 6.0f;
						canvas.SetViewScale(Vec2f(1.0f, 6.0f));
						canvas.GetTransform()->SetScale(Vec2f(1.0f, 6.0f));


						list.Refresh();


						Actions->PolishCanvasListElement(canvas, canvasList, "Materials", 6, false);
						canvas.ClampViewToElements();
				});



				desc.AddOption("Trees", [this, &editorScene, &leftList]()
					{
						auto makeCanvasFunc = [this, &editorScene, &leftList](auto refreshFunc) mutable -> void
						{
							auto canvasPair = Actions->AddToCanvasList("GEE_E_Hierarchy_Trees_Preview");
							auto& canvas = canvasPair.first;
							auto& canvasList = canvasPair.second;

							canvas.SetViewScale(Vec2f(1.0f, 6.0f));


							UIAutomaticListActor& meshTreesList = canvas.CreateChild<UIAutomaticListActor>("HierarchyTreeList");
							for (auto& scene : Scenes)
								for (auto& tree : scene->HierarchyTrees)
								{
									auto& button = meshTreesList.CreateChild<UIButtonActor>("HierarchyTreeButton", tree->GetName().GetPath(), [this, &tree]() { Actions->PreviewHierarchyTree(*tree); });
									button.SetPopupCreationFunc([scenePtr = scene.get(), treePtr = tree.get(), refreshFunc](PopupDescription desc)
										{
											desc.AddOption("Copy", [scenePtr, treePtr, refreshFunc]() mutable { scenePtr->CopyHierarchyTree(*treePtr); refreshFunc(refreshFunc); });
									desc.AddOption("Delete", [scenePtr, treePtr, refreshFunc]() mutable { scenePtr->HierarchyTrees.erase(std::remove_if(scenePtr->HierarchyTrees.begin(), scenePtr->HierarchyTrees.end(), [treePtr](auto& treeVec) { return treePtr == treeVec.get(); }), scenePtr->HierarchyTrees.end()); refreshFunc(refreshFunc); });
										});
								}

							auto& addTreeButton = meshTreesList.CreateChild<UIButtonActor>("AddHierarchyTreeButton", [this, refreshFunc]() mutable { /* set name to make the tree a local resource (this is a hack) */ GetScene("GEE_Main")->CreateHierarchyTree("").SetName("CreatedTree"); refreshFunc(refreshFunc); });
							uiButtonActorUtil::ButtonMatsFromAtlas(addTreeButton, GetRenderEngineHandle()->FindMaterial<AtlasMaterial>("GEE_E_Add_Icon_Mat"), 0.0f, 1.0f, 2.0f);

							meshTreesList.Refresh();
							canvas.ClampViewToElements();
							Actions->PolishCanvasListElement(canvas, canvasList, "Hierarchy Trees", 6, false);
						};

						makeCanvasFunc(makeCanvasFunc);
					});
						desc.AddOption("Settings", nullptr);
					});

					leftList.AddElement(UIListElement(&addCanvasButton,
						[&addCanvasButton]() { return Vec3f(0.0f, -addCanvasButton.GetTransform()->GetScale2D().y * 2.0f, 0.0f); },
						[&addCanvasButton]() { return Vec3f(0.0f, -addCanvasButton.GetTransform()->GetScale2D().y, 0.0f); }));
					leftList.Refresh();
			}


			// Main buttons
			{
				auto iconsMat = RenderEng.FindMaterial<AtlasMaterial>("GEE_E_Icons");

				auto mainButtonTheme = [&](UIButtonActor& button, float iconID) {
					button.GetRoot()->GetComponent("ButtonText")->GetTransform().SetPosition(Vec2f(0.0f, -0.6f));
					button.CreateComponent<ModelComponent>("SettingsIcon", Transform(Vec2f(0.0f, 0.3f), Vec2f(0.75f))).AddMeshInst(MeshInstance(RenderEng.GetBasicShapeMesh(EngineBasicShape::Quad), MakeShared<MaterialInstance>(iconsMat, iconsMat->GetTextureIDInterpolatorTemplate(iconID))));
					button.GetRoot()->GetComponent<ModelComponent>("SettingsIcon")->OverrideInstancesMaterialInstances(MakeShared<MaterialInstance>(iconsMat, iconsMat->GetTextureIDInterpolatorTemplate(iconID)));
				};
			
			
				UIButtonActor& hierarchyTreesButton = editorScene.CreateActorAtRoot<UIButtonActor>("HierarchyTreesButton", "Hierarchy trees", nullptr, Transform(Vec2f(-0.8f, -0.8f), Vec2f(0.1f)));
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
						String name = (it) ? (it->GetLocalization().GetFullStr()) : ("NULLPTR???");
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

					// Editor
					{
						auto& editorCat = window.AddCategory("GEditor");
						editorCat.AddField("Render debug icons").GetTemplates().TickBox(EditorSettings.bDebugRenderComponents);
						editorCat.AddField("Render grid").GetTemplates().TickBox(EditorSettings.bRenderGrid);
						editorCat.AddField("Debug physics meshes").GetTemplates().TickBox(EditorSettings.bDebugRenderPhysicsMeshes);
					}

					// Video
					{
						auto& videoCat = window.AddCategory("Video");
						videoCat.AddField("Bloom").GetTemplates().TickBox([this](bool bloom) { GetGameSettings()->Video.bBloom = bloom; UpdateGameSettings(); }, [this]() -> bool { return GetGameSettings()->Video.bBloom; });
						videoCat.AddField("Wireframe").GetTemplates().TickBox([this](bool wireframe) { GetGameSettings()->Video.bForceWireframeRendering = wireframe; UpdateGameSettings(); }, [this]() -> bool { return GetGameSettings()->Video.bForceWireframeRendering; });

						UIInputBoxActor& gammaInputBox = videoCat.AddField("Gamma").CreateChild<UIInputBoxActor>("GammaInputBox");
						gammaInputBox.SetOnInputFunc([this](float val) { GetGameSettings()->Video.MonitorGamma = val; UpdateGameSettings(); }, [this]()->float { return GetGameSettings()->Video.MonitorGamma; });

						videoCat.AddField("Default font").GetTemplates().PathInput([this](const String& path) { Fonts.push_back(MakeShared<Font>(*DefaultFont)); *DefaultFont = *EngineDataLoader::LoadFont(*this, path); }, [this]() {return GetDefaultFont()->GetVariation(FontStyle::Regular)->GetPath(); }, { "*.ttf", "*.otf" });
						videoCat.AddField("Rebuild light probes").CreateChild<UIButtonActor>("RebuildProbesButton", "Rebuild", [this]() { SceneRenderer(RenderEng, 0).PreRenderLoopPassStatic(0, GetSceneRenderDatas()); });

						{
							auto& aaSelectionList = videoCat.AddField("Anti-aliasing").CreateChild<UIAutomaticListActor>("AASelectionList", Vec3f(2.0f, 0.0f, 0.0f));
							aaSelectionList.GetTransform()->Move(Vec2f(-1.0f, 0.0f));
							std::function<void(AntiAliasingType)> setAAFunc = [this](AntiAliasingType type) { const_cast<GameSettings::VideoSettings&>(ViewportRenderCollection->GetVideoSettings()).AAType = type; UpdateGameSettings(); };
							aaSelectionList.CreateChild<UIButtonActor>("NoAAButton", "None", [=]() { setAAFunc(AntiAliasingType::AA_NONE); });
							aaSelectionList.CreateChild<UIButtonActor>("SMAA1XButton", "SMAA1X", [=]() { setAAFunc(AntiAliasingType::AA_SMAA1X); });
							aaSelectionList.CreateChild<UIButtonActor>("SMAAT2XButton", "SMAAT2X", [=]() { setAAFunc(AntiAliasingType::AA_SMAAT2X); });
							aaSelectionList.Refresh();
						}

						{
							auto& shadowSelectionList = videoCat.AddField("Shadow quality").CreateChild<UIAutomaticListActor>("ShadowSelectionList", Vec3f(2.0f, 0.0f, 0.0f));
							shadowSelectionList.GetTransform()->Move(Vec2f(-1.0f, 0.0f));
							std::function<void(SettingLevel, const String&)> addShadowButton = [this, &shadowSelectionList](SettingLevel setting, const String& caption)
							{
								shadowSelectionList.CreateChild<UIActivableButtonActor>("Button" + caption, caption,
									[=]()
									{
										const_cast<GameSettings::VideoSettings&>(ViewportRenderCollection->GetVideoSettings()).ShadowLevel = setting;
										UpdateGameSettings();
									});
							};
							addShadowButton(SettingLevel::SETTING_LOW, "Low");
							addShadowButton(SettingLevel::SETTING_MEDIUM, "Medium");
							addShadowButton(SettingLevel::SETTING_HIGH, "High");
							addShadowButton(SettingLevel::SETTING_ULTRA, "Ultra");

							shadowSelectionList.Refresh();

							videoCat.AddField("Max 2D shadow lights").CreateChild<UIInputBoxActor>("Shadow2DCount", [this](float count) { GetGameSettings()->Video.Max2DShadows = static_cast<unsigned int>(count); UpdateGameSettings(); }, [this]() { return static_cast<float>(GetGameSettings()->Video.Max2DShadows); });
							videoCat.AddField("Max 3D shadow lights").CreateChild<UIInputBoxActor>("Shadow3DCount", [this](float count) { GetGameSettings()->Video.Max3DShadows = static_cast<unsigned int>(count); UpdateGameSettings(); }, [this]() { return static_cast<float>(GetGameSettings()->Video.Max3DShadows); });
						}

						{
							auto& viewTexField = videoCat.AddField("View texture");
							SharedPtr<unsigned int> texID = MakeShared<unsigned int>(0);

							viewTexField.CreateChild<UIInputBoxActor>("PreviewTexIDInputBox").SetOnInputFunc([texID](float val) { *texID = static_cast<unsigned int>(val); }, [texID]() { return static_cast<float>(*texID); });
							viewTexField.CreateChild<UIButtonActor>("OKButton", "OK", [this, texID, &window]() {
								if (!glIsTexture(*texID))
									return;
								UIWindowActor& texPreviewWindow = window.CreateChildCanvas<UIWindowActor>("PreviewTexWindow");
								ModelComponent& texPreviewQuad = texPreviewWindow.CreateChild<UIActorDefault>("TexPreviewActor").CreateComponent<ModelComponent>("TexPreviewQuad");
								auto mat = MakeShared<Material>("TexturePreviewMat");
								mat->AddTexture(MakeShared<NamedTexture>(Texture::FromGeneratedGlId(Vec2u(0), GL_TEXTURE_2D, *texID, Texture::Format::RGBA()), "albedo1"));
								texPreviewQuad.AddMeshInst(MeshInstance(GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad), mat));

								texPreviewWindow.AutoClampView();

								}).GetTransform()->Move(Vec2f(2.0f, 0.0f));
						}

						auto& ssaoSamplesIB = videoCat.AddField("SSAO Samples").CreateChild<UIInputBoxActor>("SSAOSamplesInputBox");
						ssaoSamplesIB.SetOnInputFunc([this](float sampleCount) { GetGameSettings()->Video.AmbientOcclusionSamples = static_cast<unsigned int>(sampleCount); UpdateGameSettings(); }, [this]() { return static_cast<float>(GetGameSettings()->Video.AmbientOcclusionSamples); });

						videoCat.AddField("Parallax Occlusion Mapping").GetTemplates().TickBox([this](bool val) { GetGameSettings()->Video.POMLevel = (val) ? (SettingLevel::SETTING_LOW) : (SettingLevel::SETTING_NONE); UpdateGameSettings(); }, [this]() { return GetGameSettings()->Video.POMLevel != SettingLevel::SETTING_NONE; });
					}

					auto& physicsCat = window.AddCategory("Physics");
					physicsCat.AddField("Connect to PVD").CreateChild<UIButtonActor>("PVDConnectButton", "Connect", [this]() { PhysicsEng.ConnectToPVD(); });
					physicsCat.AddField("Bounce threshold").GetTemplates().SliderRawInterval(0.0f, 2.0f, [this](float threshold) { GetMainScene()->GetPhysicsData()->GetPxScene()->setBounceThresholdVelocity(threshold); }, GetMainScene()->GetPhysicsData()->GetPxScene()->getBounceThresholdVelocity());
					physicsCat.AddField("Gravity").GetTemplates().SliderRawInterval(0.0f, 20.0f, [this](float threshold) { GetMainScene()->GetPhysicsData()->GetPxScene()->setGravity(physx::PxVec3(0, -threshold, 0)); }, -GetMainScene()->GetPhysicsData()->GetPxScene()->getGravity().y);

					window.RefreshFieldsList();
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

				editorScene.CreateActorAtRoot<UIButtonActor>("Save as button", "Save as",
				[this, &editorScene]() {
					UIWindowActor& window = editorScene.CreateActorAtRoot<UIWindowActor>("Save as window");
					SharedPtr<String> folderpath = MakeUnique<String>(ExecutableFolder + "/");
					if (auto foundSlash = GetProjectFilepath().find_last_of(static_cast<char>(92)); foundSlash != String::npos)
						*folderpath = GetProjectFilepath().substr(0, foundSlash);
					SharedPtr<String> projectName = MakeUnique<String>(ProjectName);

					window.AddField("Project folder").GetTemplates().FolderInput([folderpath](const String& str) { *folderpath = str; }, [folderpath]() { return *folderpath; }, ExecutableFolder + String(1, char(92)) + *folderpath);

					UIInputBoxActor& projectNameInputBox = window.AddField("Project name").CreateChild<UIInputBoxActor>("ProjectNameInputBox", Transform(Vec2f(2.0f, 0.0f), Vec2f(3.0f, 1.0f)));
					//UIInputBoxActor& projectNameInputBox

					projectNameInputBox.SetOnInputFunc([projectName](const String& input) { *projectName = input; }, [projectName]() { return *projectName; });

					UIButtonActor& okButton = window.AddField("").CreateChild<UIButtonActor>("OKButton", "OK", [this, folderpath, projectName, &window]() { window.MarkAsKilled(); String saveAsPath = *folderpath + "/" + *projectName + ".json"; SetProjectFilepath(saveAsPath); SaveProject(saveAsPath); });

					window.RefreshFieldsList();
					window.AutoClampView();
				}, Transform(Vec2f(0.8f, -0.8f), Vec2f(0.1f)));
			}
		 

			Actor& cameraActor = editorScene.CreateActorAtRoot<Actor>("OrthoCameraActor");
			CameraComponent& orthoCameraComp = cameraActor.CreateComponent<CameraComponent>("OrthoCameraComp", glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -2550.0f));

			editorScene.BindActiveCamera(&orthoCameraComp);


			EditorScene = &editorScene;

			EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->AddMeshInst(GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad));
			//EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->SetTransform(Transform(Vec2f(0.0f, 0.4f), Vec2f(1.0f)));

			SharedPtr<Material> scenePreviewMaterial = MakeShared<Material>("GEE_3D_SCENE_PREVIEW_MATERIAL");
			RenderEng.AddMaterial(scenePreviewMaterial);
			scenePreviewMaterial->AddTexture(MakeShared<NamedTexture>(ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorTexture(0), "albedo1"));
			EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->OverrideInstancesMaterial(scenePreviewMaterial);

			editorScene.MarkAsStarted();
		}

		void GameEngineEngineEditor::SetupMainMenu()
		{
			GameScene& mainMenuScene = CreateUIScene("GEE_Main_Menu", *GameWindow);

			//SetCanvasContext(&mainMenuScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Canvas_Context"));
			Actor* aauifix = nullptr;
			//UI Fix
			{
				auto& uifix = mainMenuScene.CreateActorAtRoot<UIActorDefault>("uifix", Transform(Vec2f(0.5f, 0.0f), Vec2f(0.1f)));
				//UIElementTemplates(uifix).ObjectInput<Actor, Actor>(*mainMenuScene.GetRootActor(), aauifix);
				uifix.CreateChild<UIButtonActor>("fixinput", [this, &mainMenuScene]() {
					auto& window = mainMenuScene.CreateActorAtRoot<UIWindowActor>("uifixwindow", Transform(Vec2f(0.0f), Vec2f(0.5f, 0.3f)));
					auto& texts = window.CreateChild<UIActorDefault>("textsActor");
					texts.CreateComponent<TextComponent>("LAlign", Transform(), "CAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlign", "", Alignment2D::LeftCenter()).SetMaxSize(Vec2f(3.0f, 1.0f));
					texts.CreateComponent<TextComponent>("CAlign", Transform(Vec2f(0.0f, -1.0f)), "CAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlign", "", Alignment2D::Center()).SetMaxSize(Vec2f(3.0f, 1.0f));
					texts.CreateComponent<TextComponent>("RAlign", Transform(Vec2f(0.0f, -2.0f)), "CAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlignCAlign", "", Alignment2D::RightCenter()).SetMaxSize(Vec2f(3.0f, 1.0f));
					window.SetPopupCreationFunc([this, &window, &mainMenuScene](PopupDescription desc) {
						desc.AddOption("Canvas view", [this, &window, &mainMenuScene]()
							{
								auto& editWindow = mainMenuScene.CreateActorAtRoot<UIWindowActor>("Canvas view transform");
								ComponentDescriptionBuilder descBuilder(*dynamic_cast<Editor::EditorManager*>(this),
									editWindow, editWindow);
								const_cast<Transform&>(window.GetViewT()).GetEditorDescription(descBuilder);
								editWindow.AutoClampView();
								editWindow.RefreshFieldsList();
							});
						});
				}, Transform(Vec2f(0.0f, 3.0f), Vec2f(2.0f, 0.5f)));
			}

			{
				Actor& engineTitleActor = mainMenuScene.CreateActorAtRoot<Actor>("GEE_Engine_Title", Transform(Vec2f(0.0f, 0.6f)));
				TextComponent& engineTitleTextComp = engineTitleActor.CreateComponent<TextComponent>("GEE_Engine_Title_Text", Transform(), "Game Engine Engine", "", Alignment2D::Center());
				engineTitleTextComp.SetFontStyle(FontStyle::Bold);
				engineTitleTextComp.SetTransform(Transform(Vec2f(0.0f), Vec2f(0.09f)));
				std::cout << "title comp transform ptr: " << &engineTitleTextComp.GetTransform() << '\n';

				SharedPtr<Material> titleMaterial = MakeShared<Material>("GEE_Engine_Title");
				RenderEng.AddMaterial(titleMaterial);
				titleMaterial->SetColor(hsvToRgb(Vec3f(300.0f, 1.0f, 1.0f)));

				Interpolation titleColorInterpolation(0.0f, 10.0f, InterpolationType::Linear, false, AnimBehaviour::STOP, AnimBehaviour::REPEAT);
				titleColorInterpolation.SetOnUpdateFunc([this, titleMaterial](Time time) -> bool
				{
					titleMaterial->SetColor(hsvToRgb(Vec3f(static_cast<float>(time * 360.0), 0.6f, 0.6f)));

					if (!GetScene("GEE_Main_Menu"))
						return true;
					return false;
				});
				AddInterpolation(titleColorInterpolation);

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
				
				window.AddField("Project folder").GetTemplates().FolderInput([this](const String& str) { ProjectFilepath = str + "/"; }, [this]() { return ProjectFilepath; }, ExecutableFolder);

				UIInputBoxActor& projectNameInputBox = window.AddField("Project name").CreateChild<UIInputBoxActor>("ProjectNameInputBox", Transform(Vec2f(2.0f, 0.0f), Vec2f(3.0f, 1.0f)));
				//UIInputBoxActor& projectNameInputBox

				ProjectName = "Project1";
				projectNameInputBox.SetOnInputFunc([this](const String& input) { ProjectName = input; }, [this]() { return ProjectName; });

				UIButtonActor& okButton = window.AddField("").CreateChild<UIButtonActor>("OKButton", "OK", [this, &window]() { window.MarkAsKilled(); NewProject(ProjectFilepath + ProjectName + ".json"); });

				window.RefreshFieldsList();
				window.AutoClampView();

			}).SetTransform(Transform(Vec2f(-0.3f, 0.0f), Vec2f(0.2f)));

			mainMenuScene.CreateActorAtRoot<UIButtonActor>("LoadProjectButton", "Load project", [this, &mainMenuScene]() {
				UIWindowActor& window = mainMenuScene.CreateActorAtRoot<UIWindowActor>("ProjectLoaderWindow");
				window.SetTransform(Transform(Vec2f(0.0f), Vec2f(0.5f)));


				window.AddField("Project filepath").GetTemplates().PathInput([this](const String& filepath) { ProjectFilepath = ToRelativePath(filepath);  extractDirectoryAndFilename(filepath, &ProjectName); }, [this]() { return ProjectFilepath; }, { "*.json", "*.geeproject", "*.geeprojectold" });

				UIButtonActor& okButton = window.AddField("").CreateChild<UIButtonActor>("OKButton", "OK", [this, &window]() { window.MarkAsKilled(); LoadProject(ProjectFilepath); });

				window.RefreshFieldsList();
				window.AutoClampView();

			}).SetTransform(Transform(Vec2f(0.3f, 0.0f), Vec2f(0.2f)));

			{
				std::ifstream recentProjectsFile("recent.txt");
				std::vector <String> filepaths;
				String line;
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
					UIElementTemplates(recentProjectsActor).ListSelection<String>(filepaths.begin(), filepaths.end(), [this](UIAutomaticListActor& listActor, String& filepath) { listActor.CreateChild<UIButtonActor>("RecentFilepathButton", getFileName(filepath, false), [this, filepath]() { LoadProject(filepath); }, Transform(Vec2f(0.0f), Vec2f(9.0f, 1.0f))).GetRoot(); });
				}
			}

			Actor& cameraActor = mainMenuScene.CreateActorAtRoot<Actor>("OrthoCameraActor");
			CameraComponent& orthoCameraComp = cameraActor.CreateComponent<CameraComponent>("OrthoCameraComp", glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -2550.0f));

			mainMenuScene.BindActiveCamera(&orthoCameraComp);
			SetActiveScene(&mainMenuScene);
			mainMenuScene.MarkAsStarted();
		}

		void GameEngineEngineEditor::Update(Time deltaTime)
		{
			if (GameController && GameController->IsBeingKilled())
				GameController = nullptr;

			UpdateControllerCameraInfo();

			UpdateControllerCameraInfo();

			Game::Update(deltaTime);
		}

		void GameEngineEngineEditor::HandleEvents()
		{
			if (!ActiveScene)
			{
				std::cerr << "ERROR! No scene was set as active. Events will not be handled by any scene.\n";
				return;
			}

			for (auto& scene : Scenes)
			{
				while (SharedPtr<Event> polledEvent = EventHolderObj.PollEvent(*scene))
				{
					LastRenderPopupRequest = nullptr;	// last popup request generated from this event; if it stays at nullptr, we know that no such request has been made.

					if (polledEvent->GetType() == EventType::KeyPressed)
					{
						KeyEvent& keyEventCast = dynamic_cast<KeyEvent&>(*polledEvent);
						if (scene.get() == EditorScene)
						{
							switch (keyEventCast.GetKeyCode())
							{
								case Key::Tab:
								{
									// If the editor is currently active, set the active scene to the main one and pass control to a game controller.
									if (GetMainScene())
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
											SetActiveScene(GetMainScene());
											PassMouseControl(GameController);

											for (auto& it : Scenes)	// Scene has changed; push this event to all scenes
												it->RootActor->HandleEventAll(FocusSwitchedEvent());
										}
									}

									UpdateControllerCameraInfo();
									break;
								}

								case Key::S:
									if (keyEventCast.GetModifierBits() & KeyModifierFlags::Control)
										SaveProject();
									break;

								case Key::F10: MaximizeViewport(); break;

								case Key::F11:	// Also recompiles all shaders
									UpdateGameSettings();
									UpdateEditorSettings();
									break;

								case Key::F12:
								{
									if (!Profiler.HasBeenStarted())
										StartProfiler();
									else
										StopProfiler();
									break;
								}
							}
						}
						else if (scene.get() == GetScene("GEE_Main"))
						{
							if (keyEventCast.GetKeyCode() == Key::Tab)
							{
								// Set the active scene to the editor scene and release control.
								if (EditorScene)
								{
									PassMouseControl(nullptr);
									SetActiveScene(EditorScene);

									for (auto& it : Scenes)
										it->RootActor->HandleEventAll(FocusSwitchedEvent());
								}

								UpdateControllerCameraInfo();
							}
							else if (keyEventCast.GetKeyCode() == Key::F12)
							{
								if (!Profiler.HasBeenStarted())
									StartProfiler();
								else
									StopProfiler();
							}
						}
					}

					scene->HandleEventAll(*polledEvent);

					if (LastRenderPopupRequest)
						LastRenderPopupRequest();
				}
			}


			for (auto& popup : OpenPopups)
			{
				while (SharedPtr<Event> polledEvent = EventHolderObj.PollEvent(popup.Scene))
				{
					popup.Scene.get().HandleEventAll(*polledEvent);
				}
			}
		}

		void GameEngineEngineEditor::PassMouseControl(Controller* controller)
		{
			if (ActiveScene == EditorScene)
				GameController = controller;
			else
			{
				if (!controller)
					controller = EditorController;
				Game::PassMouseControl(controller);
			}

			UpdateControllerCameraInfo();
		}

		void GameEngineEngineEditor::Render()
		{
			EditorRenderer(*this, EditorSettings, *ViewportRenderCollection, *HUDRenderCollection, *GameWindow).RenderPass(GetMainScene(), EditorScene, GetScene("GEE_Main_Menu"), GetScene("GEE_Mesh_Preview_Scene"));
		}

		void GameEngineEngineEditor::NewProject(const String& filepath)
		{
			LoadProject(filepath);

			/*#ifdef GEE_OS_WINDOWS



			// Open new .sln/.vcxproj
			STARTUPINFO startupInfo;
			PROCESS_INFORMATION processInformation;

			ZeroMemory(&startupInfo, sizeof(startupInfo));
			startupInfo.cb = sizeof(startupInfo);
			ZeroMemory(&processInformation, sizeof(processInformation));

			String path;// = "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/devenv.exe";

			if (!path.empty())
				CreateProcess(path.c_str(), "", nullptr, nullptr, false, 0, nullptr, nullptr, &startupInfo, &processInformation);
			else
				system("start devenv");
			//CreateProcess("devenv.exe", "", nullptr, nullptr, false, 0, nullptr, nullptr, &startupInfo, &processInformation);

			CloseHandle(processInformation.hProcess);
			CloseHandle(processInformation.hThread);

			#endif	//GEE_OS_WINDOWS*/
		}

		void GameEngineEngineEditor::LoadProject(const String& filepath)
		{
			SetProjectFilepath(filepath);

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
				auto defaultRes = static_cast<Vec2f>(WindowData(*GameWindow).GetWindowSize());
				GetGameSettings()->Video.Resolution = static_cast<Vec2i>(defaultRes * 0.7f);
				std::cout << "Default res: " << defaultRes << '\n';
				auto& camera = cameraActor.CreateComponent<CameraComponent>("A CameraComponent", glm::perspective(glm::radians(90.0f), defaultRes.x / defaultRes.y, 0.01f, 100.0f));
				scene->BindActiveCamera(&camera);

				cameraActor.CreateChild<FreeRoamingController>("A FreeRoamingController").SetPossessedActor(&cameraActor);

				////
				auto& sphereActor = scene->CreateActorAtRoot<Actor>("Default sphere", Transform(Vec3f(0.0f, 0.0f, -3.0f)));
				sphereActor.CreateComponent<ModelComponent>("A ModelComponent").AddMeshInst(RenderEng.GetBasicShapeMesh(EngineBasicShape::Sphere));
				sphereActor.CreateComponent<LightComponent>("A LightComponent").GetTransform().SetPosition(Vec3f(0.0f, 0.0f, 2.0f));
			}
			SetMainScene(GetScene("GEE_Main"));
			SetActiveScene(EditorScene);

			GetScene("GEE_Main")->MarkAsStarted();

			Actions->SelectScene(GetMainScene(), *EditorScene);
		
			SceneRenderer(RenderEng, 0).PreRenderLoopPassStatic(0, GetSceneRenderDatas());

			UpdateRecentProjects();

			UpdateGameSettings();
			glfwRequestWindowAttention(GameWindow);
		}

		void GameEngineEngineEditor::SaveProject(String filepath)
		{
			if (filepath.empty())
				filepath = ProjectFilepath;

			if (getFilepathExtension(filepath) == ".geeprojectold")
				filepath = filepath.substr(0, filepath.find(".geeprojectold")) + ".json";
			std::cout << "Saving project to path " << filepath << "\n";

			GetEditorLogger(*EditorScene).Log("Saving project", EditorMessageLogger::MessageType::Information);

			std::stringstream serializationStream(filepath);
			try
			{
				cereal::JSONOutputArchive archive(serializationStream);
				archive(cereal::make_nvp("GEditorSettings", EditorSettings));
				GetMainScene()->Save(archive);
				archive.serializeDeferments();
			}
			catch (cereal::Exception& exception)
			{
				std::ofstream brokenFileOutput(filepath + "broken");	// Do not save to file if the process wasn't successful; we don't want to corrupt existing data.
				brokenFileOutput << serializationStream.rdbuf();
				GetEditorLogger(*EditorScene).Log(exception.what(), EditorMessageLogger::MessageType::Error);
				return;
			}

			std::ofstream workingFileOutput(filepath);
			workingFileOutput << serializationStream.str();
			workingFileOutput.close();	// serialization stream must be closed after destroying the archive

			UpdateRecentProjects();

			std::cout << "Project " + ProjectName + " (" + filepath + ") saved successfully.\n";
			GetEditorLogger(*EditorScene).Log("Project saved", EditorMessageLogger::MessageType::Success);
		}

		void GameEngineEngineEditor::StartProfiler()
		{
			GEE_LOG("Starting the profiler...");
			Profiler.StartProfiling(GetProgramRuntime());
		}

		void GameEngineEngineEditor::StopProfiler()
		{
			GEE_LOG("Stopping the profiler...");
			Profiler.StopAndSaveToFile(GetProgramRuntime());
		}

		void GameEngineEngineEditor::SetProjectFilepath(const String& filepath)
		{
			ProjectFilepath = filepath;
			glfwSetWindowTitle(GameWindow, filepath.c_str());
		}

		const std::deque<EditorPopup>& GameEngineEngineEditor::GetPopupsForRendering()
		{
			return OpenPopups;
		}

		void GameEngineEngineEditor::UpdateRecentProjects()
		{
			std::vector<String> filepaths;
			{
				std::ifstream recentProjectsInput("recent.txt");

				String line;
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
		auto windowSize = WindowData(*GameWindow).GetWindowSize();
		if (!bViewportMaximized)
		{
			GetGameSettings()->Video.Resolution = windowSize;
			GetScene("GEE_Editor")->FindActor("SceneViewportActor")->SetTransform(Transform(Vec2f(0.0f), Vec2f(1.0f)));
		}
		else
		{
			GetGameHandle()->GetGameSettings()->Video.Resolution = static_cast<Vec2f>(windowSize) * Vec2f(0.7f);
			GetScene("GEE_Editor")->FindActor("SceneViewportActor")->SetTransform(Transform(Vec2f(0.0f, 0.2f), Vec2f(0.7f)));
		}

		Vec2f viewportSize = static_cast<Vec2f>(GetGameSettings()->Video.Resolution);

		if (auto activeCamera = GetMainScene()->GetActiveCamera())
			activeCamera->SetProjectionMat(glm::perspective(glm::radians(90.0f), viewportSize.x / viewportSize.y, 0.1f, 100.0f));

		UpdateGameSettings();
		bViewportMaximized = !bViewportMaximized;
	}

	void GameEngineEngineEditor::UpdateControllerCameraInfo()
	{
		if (EditorScene)
			if (auto foundActor = EditorScene->FindActor("ControllerCameraInfoTextActor"))
			{
				
				if (auto foundText = foundActor->GetRoot()->GetComponent<TextComponent>("ControllerInfoText"))
					foundText->SetContent((GameController) ? ((ActiveScene == &GameController->GetScene()) ? (GameController->GetName()) : ("Editor control")) : ("No controller"));
				if (auto foundText = foundActor->GetRoot()->GetComponent<TextComponent>("CameraInfoText"))
					foundText->SetContent((GetMainScene() && GetMainScene()->GetActiveCamera()) ? (GetMainScene()->GetActiveCamera()->GetName()) : ("No camera"));
			}
	}

	EditorMessageLogger& GameEngineEngineEditor::GetEditorLogger(GameScene& scene)
	{
		if (Logs.find(&scene) == Logs.end())
			Logs.emplace(std::make_pair(&scene, EditorMessageLogger(*this, scene)));

		return Logs.find(&scene)->second;
	}

	}
}
