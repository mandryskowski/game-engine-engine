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
				EditorHandle->PreviewHierarchyTree(*EngineDataLoader::LoadHierarchyTree(*EditorHandle->GetGameHandle()->GetMainScene(), paths[0]));
			}
		}

		void EditorEventProcessor::Resize(SystemWindow* window, int width, int height)
		{
			if (width <= 0 || height <= 0)
				return;

			Vec2u newSize(width, height);
			if (EditorHandle->GetGameHandle()->GetGameSettings()->WindowSize != newSize || EditorHandle->GetGameHandle()->GetGameSettings()->Video.Resolution != static_cast<Vec2f>(newSize))
			{
				EditorHandle->GetGameHandle()->GetGameSettings()->WindowSize = newSize;
				EditorHandle->GetGameHandle()->GetGameSettings()->Video.Resolution = static_cast<Vec2f>(newSize) * Vec2f(0.4f, 0.6f);
				EditorHandle->UpdateGameSettings();
			}

			if (EditorHandle->GetEditorSettings()->WindowSize != newSize || EditorHandle->GetEditorSettings()->Video.Resolution != static_cast<Vec2f>(newSize))
			{
				EditorHandle->GetEditorSettings()->WindowSize = newSize;
				EditorHandle->GetEditorSettings()->Video.Resolution = newSize;
				EditorHandle->UpdateEditorSettings();
			}

			std::cout << "New res " << EditorHandle->GetGameHandle()->GetGameSettings()->Video.Resolution << '\n';
		}


		GameEngineEngineEditor::GameEngineEngineEditor(SystemWindow* window, const GameSettings& settings) :
			Game(settings.Video.Shading, settings),
			EditorScene(nullptr),
			SelectedComp(nullptr),
			SelectedActor(nullptr),
			SelectedScene(nullptr),
			bDebugRenderComponents(true),
			GameController(nullptr),
			EEForceForwardShading(false),
			ThreadPool(1),
			bRenderingPopups(false)
			//CanvasContext(nullptr)
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
			Vec2f res = Settings->WindowSize;
			//Settings->ViewportData = glm::uvec4(res.x * 0.3f, res.y * 0.4f, res.x * 0.4, res.y * 0.6f);
			Settings->Video.Resolution = Vec2f(res.x * 0.4f, res.y * 0.6f);
			//Settings->Video.Resolution = Vec2f(res.x, res.y);
			Settings->Video.Shading = ShadingAlgorithm::SHADING_PBR_COOK_TORRANCE;
			Init(window);
		}

		GameManager* GameEngineEngineEditor::GetGameHandle()
		{
			return this;
		}

		Component* GameEngineEngineEditor::GetSelectedComponent()
		{
			return SelectedComp;
		}

		GameScene* GameEngineEngineEditor::GetSelectedScene()
		{
			return SelectedScene;
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

		void GameEngineEngineEditor::Init(SystemWindow* window)
		{
			Game::Init(window);

			GLFWimage image;
			image.pixels = stbi_load("gee_icon.png", &image.width, &image.height, nullptr, 4);
			if (!image.pixels)
				std::cout << "INFO: Could not load icon file gee_icon.png.\n";
			glfwSetWindowIcon(window, 1, &image);
			stbi_image_free(image.pixels);

			EditorEventProcessor::EditorHandle = this;
			glfwSetDropCallback(Window, EditorEventProcessor::FileDropCallback);
			glfwSetFramebufferSizeCallback(Window, EditorEventProcessor::Resize);

			glfwSetWindowCloseCallback(Window, [](GLFWwindow* window) { GameEngineEngineEditor* thisPtr = static_cast<GameEngineEngineEditor*>(glfwGetWindowUserPointer(window)); GenericUITemplates(*thisPtr->EditorScene).ConfirmationBox([thisPtr]() { thisPtr->SaveProject(); thisPtr->TerminateGame(); }, [=]() { thisPtr->TerminateGame(); }, "Save the project?"); });


			SharedPtr<AtlasMaterial> addIconMat = MakeShared<AtlasMaterial>(Material("GEE_E_Add_Icon_Mat", 0.0f, RenderEng.FindShader("Forward_NoLight")), glm::ivec2(3, 1));
			GetRenderEngineHandle()->AddMaterial(addIconMat);
			addIconMat->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("EditorAssets/add_icon.png", Texture::Format::RGBA(), false, Texture::MinFilter::NearestInterpolateMipmap()), "albedo1"));

			SharedPtr<AtlasMaterial> refreshIconMat = MakeShared<AtlasMaterial>(Material("GEE_E_Refresh_Icon_Mat", 0.0f, RenderEng.FindShader("Forward_NoLight")), glm::ivec2(3, 1));
			GetRenderEngineHandle()->AddMaterial(refreshIconMat);
			refreshIconMat->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("EditorAssets/refresh_icon.png", Texture::Format::RGBA(), false, Texture::MinFilter::NearestInterpolateMipmap()), "albedo1"));
		
			SharedPtr<AtlasMaterial> moreMat = MakeShared<AtlasMaterial>(Material("GEE_E_More_Mat", 0.0f, RenderEng.FindShader("Forward_NoLight")), glm::ivec2(3, 1));
			GetRenderEngineHandle()->AddMaterial(moreMat);
			moreMat->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("EditorAssets/more_icon.png", Texture::Format::RGBA(), true), "albedo1"));

			{
				SharedPtr<Material> material = MakeShared<Material>("GEE_Default_Text_Material", 0.0f, RenderEng.FindShader("TextShader"));
				material->SetColor(Vec4f(1.0f));
				GetRenderEngineHandle()->AddMaterial(material);
			}
			{
				SharedPtr<Material> material = MakeShared<Material>("GEE_Default_X_Axis", 0.0f, RenderEng.FindShader("Forward_NoLight"));
				material->SetColor(Vec4f(0.39f, 0.18f, 0.18f, 1.0f));
				GetRenderEngineHandle()->AddMaterial(material);
			}
			{
				SharedPtr<Material> material = MakeShared<Material>(Material("GEE_Default_Y_Axis", 0.0f, RenderEng.FindShader("Forward_NoLight")));
				material->SetColor(Vec4f(0.18f, 0.39f, 0.25f, 1.0f));
				GetRenderEngineHandle()->AddMaterial(material);
			}
			{
				SharedPtr<Material> material = MakeShared<Material>(Material("GEE_Default_Z_Axis", 0.0f, RenderEng.FindShader("Forward_NoLight")));
				material->SetColor(Vec4f(0.18f, 0.25f, 0.39f, 1.0f));
				GetRenderEngineHandle()->AddMaterial(material);
			}
			{
				SharedPtr<Material> material = MakeShared<Material>(Material("GEE_Default_W_Axis", 0.0f, RenderEng.FindShader("Forward_NoLight")));
				material->SetColor(Vec4f(0.45f, 0.46f, 0.5f, 1.0f));
				GetRenderEngineHandle()->AddMaterial(material);
			}
			{
				SharedPtr<Material> material = MakeShared<Material>(Material("GEE_E_Canvas_Background_Material", 0.0f, RenderEng.FindShader("Forward_NoLight")));
				material->SetColor(Vec4f(0.26f, 0.34f, 0.385f, 1.0f));
				GetRenderEngineHandle()->AddMaterial(material);
			}

			{
				SharedPtr<Material> material = MakeShared<Material>(Material("GEE_No_Camera_Icon", 0.0f, RenderEng.FindShader("Forward_NoLight")));
				GetRenderEngineHandle()->AddMaterial(material);
				material->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("EditorAssets/no_camera_icon.png", Texture::Format::RGBA(), true, Texture::MinFilter::Trilinear(), Texture::MagFilter::Bilinear()), "albedo1"));
			}

			SharedPtr<AtlasMaterial> kopecMat = MakeShared<AtlasMaterial>(Material("Kopec", 0.0f, RenderEng.FindShader("Forward_NoLight")), glm::ivec2(2, 1));
			GetRenderEngineHandle()->AddMaterial(kopecMat);
			kopecMat->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("kopec_tekstura.png", Texture::Format::RGBA(), true, Texture::MinFilter::NearestInterpolateMipmap(), Texture::MagFilter::Bilinear()), "albedo1"));

			//LoadSceneFromFile("level.txt", "GEE_Main");
			//SetMainScene(GetScene("GEE_Main"));

			GameSettings::VideoSettings& renderSettings = EditorSettings.Video;
			renderSettings.AAType = AA_SMAA1X;
			renderSettings.AmbientOcclusionSamples = 0;
			renderSettings.bBloom = false;
			renderSettings.DrawToWindowFBO = true;
			renderSettings.POMLevel = SettingLevel::SETTING_NONE;
			renderSettings.Resolution = Settings->WindowSize;
			renderSettings.Shading = ShadingAlgorithm::SHADING_FULL_LIT;
			renderSettings.ShadowLevel = SettingLevel::SETTING_NONE;
			renderSettings.TMType = ToneMappingType::TM_NONE;
			renderSettings.MonitorGamma = 1.0f;

			ViewportRenderCollection = &RenderEng.AddRenderTbCollection(RenderToolboxCollection("GameSceneRenderCollection", Settings->Video));

			HUDRenderCollection = &RenderEng.AddRenderTbCollection(RenderToolboxCollection("HUDRenderCollection", renderSettings));

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
			//glfwSetWindowFocusCallback(window, [](SystemWindow* window, int focused) { glfwSetWindowShouldClose(window, GLFW_TRUE); });

		
			GameScene& popupScene = CreateUIScene("GEE_E_Popup_Window", *window, false);

			//popupScene.CreateActorAtRoot<UIButtonActor>("CreateComponentButton", "Create component", [=]() { glfwSetWindowShouldClose(window, GLFW_TRUE); }, Transform(Vec2f(0.0f), Vec2f(1.0f)));
			OpenPopups.push_back(Popup(IDSystem<Popup>::GenerateID(), *window, popupScene));

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

					if (pickedComponent && pickedComponent != GetSelectedComponent())
					{
						SelectActor(&pickedComponent->GetActor(), editorScene);
						SelectComponent(pickedComponent, editorScene);
					}
					else
						SelectActor(nullptr, editorScene);

					});



				UIButtonActor& selectPreviewButton = scenePreviewActor.CreateChild<UIButtonActor>("SelectPreview_Empty", "Final", nullptr, Transform(Vec2f(0.6f, -1.07f), Vec2f(0.4f, 0.05f)));
				auto selectPreviewButtonText = selectPreviewButton.GetRoot()->GetComponent<TextConstantSizeComponent>("ButtonText");
				selectPreviewButtonText->Unstretch();
				//selectPreviewButtonText->SetMaxSize(Vec2f(1.0f, 1.0f / 8.0f));
				selectPreviewButton.SetOnClickFunc([&, selectPreviewButtonText]() {
						auto& window = editorScene.CreateActorAtRoot<UIWindowActor>("SelectPreviewWindow");
						auto& previewsList = window.CreateChild<UIAutomaticListActor>("PreviewsList");

						auto addPreview = [&, selectPreviewButtonText](const std::string& name, std::function<Texture()> getTextureToPreview ) {
							previewsList.CreateChild<UIButtonActor>(name + "Button", name, [&, getTextureToPreview, selectPreviewButtonText, name]() {
								Material* previewMaterial = RenderEng.FindMaterial("GEE_3D_SCENE_PREVIEW_MATERIAL").get();
								previewMaterial->Textures.clear();
								previewMaterial->AddTexture(MakeShared<NamedTexture>(getTextureToPreview(), "albedo1"));
								window.MarkAsKilled();

								selectPreviewButtonText->SetContent(name);
							});
						};

						addPreview("Albedo", [this]() { return ViewportRenderCollection->GetTb<DeferredShadingToolbox>()->GetGeometryFramebuffer()->GetColorTexture(0); });
						addPreview("Position", [this]() {return ViewportRenderCollection->GetTb<DeferredShadingToolbox>()->GetGeometryFramebuffer()->GetColorTexture(1); });
						addPreview("Normal", [this]() { return ViewportRenderCollection->GetTb<DeferredShadingToolbox>()->GetGeometryFramebuffer()->GetColorTexture(2); });
						addPreview("Final", [this]() { return ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorTexture(0); });

						previewsList.Refresh();

						window.AutoClampView();

				});

				UIActorDefault& forwardShadingSwitch = scenePreviewActor.CreateChild<UIActorDefault>("Extended_Essay_ForwardShadingSwitch", Transform(Vec2f(0.1f, -1.07f), Vec2f(0.05f)));
				forwardShadingSwitch.CreateComponent<TextConstantSizeComponent>("SwitchForwardText", Transform(Vec2f(0.0f, -1.4f), Vec2f(1.0f, 0.4f)), "Forward", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER)).Unstretch();
				UIElementTemplates(forwardShadingSwitch).TickBox([this, &selectPreviewButton, selectPreviewButtonText, &scenePreviewQuad](bool val) {
					selectPreviewButton.SetDisableInput(val); 
					selectPreviewButton.GetButtonModel()->SetHide(val);
					selectPreviewButtonText->SetHide(val);
					/*if (val)
					{
						Material* mat = new Material("GEE_3D_SCENE_PREVIEW_MATERIAL", 0.0f, RenderEng.FindShader("Forward_NoLight"));
						mat->AddTexture(std::make_shared<NamedTexture>(ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorTexture(0), "albedo1"));
						scenePreviewQuad.OverrideInstancesMaterial(mat);
						selectPreviewButtonText->SetContent("Final");
					}*/
					 EEForceForwardShading = val; }, [this]() { return EEForceForwardShading; });
			}

			UIActorDefault& scaleActor = editorScene.CreateActorAtRoot<UIActorDefault>("TextTestButton", Transform(Vec2f(0.0f, 0.0f), Vec2f(0.1f, 0.1f)));

			UIWindowActor& window1 = editorScene.CreateActorAtRoot<UIWindowActor>("MyTestWindow");
			window1.SetTransform(Transform(Vec2f(0.0, -0.5f), Vec2f(0.2f)));

			UIButtonActor& bigButtonActor = window1.CreateChild<UIButtonActor>("MojTestowyButton", "big button");
			bigButtonActor.SetTransform(Transform(Vec2f(0.0f, 1.5f), Vec2f(0.4f)));

			// Main buttons
			{
				auto mainIconsMat = MakeShared<AtlasMaterial>(Material("GEE_E_MainIconsMaterial", 0.0f, RenderEng.FindShader("Forward_NoLight")), Vec2i(3, 2));
				mainIconsMat->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("EditorAssets/main_icons.png", Texture::Format::RGBA(), true), "albedo1"));
				RenderEng.AddMaterial(mainIconsMat);

				auto mainButtonTheme = [&](UIButtonActor& button, float iconID) {
					button.GetRoot()->GetComponent("ButtonText")->GetTransform().SetPosition(Vec2f(0.0f, -0.6f));
					button.CreateComponent<ModelComponent>("SettingsIcon", Transform(Vec2f(0.0f, 0.3f), Vec2f(0.75f))).AddMeshInst(MeshInstance(RenderEng.GetBasicShapeMesh(EngineBasicShape::QUAD), MakeShared<MaterialInstance>(*mainIconsMat, mainIconsMat->GetTextureIDInterpolatorTemplate(iconID))));
					button.GetRoot()->GetComponent<ModelComponent>("SettingsIcon")->OverrideInstancesMaterialInstances(MakeShared<MaterialInstance>(*mainIconsMat, mainIconsMat->GetTextureIDInterpolatorTemplate(iconID)));
				};
			
			
				auto& hierarchyTreesButton = editorScene.CreateActorAtRoot<UIButtonActor>("HierarchyTreesButton", "HierarchyTrees", [this, &editorScene]() {
					UIWindowActor& window = editorScene.CreateActorAtRoot<UIWindowActor>("MeshTreesWindow");
					window.GetTransform()->SetScale(Vec2f(0.5f));

					UIAutomaticListActor& meshTreesList = window.CreateChild<UIAutomaticListActor>("MeshTreesList");
					for (auto& scene : Scenes)
						for (auto& meshTree : scene->HierarchyTrees)
							meshTreesList.CreateChild<UIButtonActor>("MeshTreeButton", meshTree->GetName(), [this, &meshTree]() { PreviewHierarchyTree(*meshTree); });

					meshTreesList.Refresh();

				}, Transform(Vec2f(-0.8f, -0.8f), Vec2f(0.1f)));
				mainButtonTheme(hierarchyTreesButton, 1.0f);

				auto& materialsButton = editorScene.CreateActorAtRoot<UIButtonActor>("MaterialsButton", "Materials", nullptr, Transform(Vec2f(-0.5f, -0.8f), Vec2f(0.1f)));
				materialsButton.SetOnClickFunc([this, &editorScene, &materialsButton]() {
					UIWindowActor& window = editorScene.CreateActorAtRoot<UIWindowActor>("MaterialsPreviewWindow");
					UIAutomaticListActor& list = window.CreateChild<UIAutomaticListActor>("MaterialsList");

					UIButtonActor& addMaterialButton = list.CreateChild<UIButtonActor>("CreateMaterialButton", [&]() {
						const auto& view = window.GetViewT();
						GetRenderEngineHandle()->AddMaterial(MakeShared<AtlasMaterial>("CreatedMaterial" + std::to_string(IDSystem<Material>::GenerateID())));
						window.MarkAsKilled();
						materialsButton.OnClick();
						if (auto newWindow = dynamic_cast<UIWindowActor*>(editorScene.FindActor("MaterialsPreviewWindow")))
							newWindow->SetCanvasView(view);
					});
					uiButtonActorUtil::ButtonMatsFromAtlas(addMaterialButton, *dynamic_cast<AtlasMaterial*>(GetRenderEngineHandle()->FindMaterial("GEE_E_Add_Icon_Mat").get()), 0.0f, 1.0f, 2.0f);

					for (auto& it : RenderEng.Materials)
					{
						std::string name = (it) ? (it->GetLocalization().GetFullStr()) : ("NULLPTR???");
						UIButtonActor& button = list.CreateChild<UIButtonActor>(name + "Button", name, [this, &window, &it, name]() { UIWindowActor& matWindow = window.CreateChildCanvas<UIWindowActor>(name + "MaterialWindow"); EditorDescriptionBuilder descBuilder(*this, matWindow, matWindow); it->GetEditorDescription(descBuilder); matWindow.AutoClampView(); matWindow.RefreshFieldsList(); });
						std::cout << name << '\n';
					}

					list.Refresh();
				});
				mainButtonTheme(materialsButton, 0.0f);

				auto& settingsButton = editorScene.CreateActorAtRoot<UIButtonActor>("SettingsButton", "Settings", [this, &editorScene]() {
					UIWindowActor& window = editorScene.CreateActorAtRoot<UIWindowActor>("MainSceneSettingsWindow");
					window.GetTransform()->SetScale(Vec2f(0.5f));
					window.AddField("Bloom").GetTemplates().TickBox([this](bool bloom) { GetGameSettings()->Video.bBloom = bloom; UpdateGameSettings(); }, [this]() -> bool { return GetGameSettings()->Video.bBloom; });

					UIInputBoxActor& gammaInputBox = window.AddField("Gamma").CreateChild<UIInputBoxActor>("GammaInputBox");
					gammaInputBox.SetOnInputFunc([this](float val) { GetGameSettings()->Video.MonitorGamma = val; UpdateGameSettings(); }, [this]()->float { return GetGameSettings()->Video.MonitorGamma; });

					window.AddField("Default font").GetTemplates().PathInput([this](const std::string& path) { Fonts.push_back(MakeShared<Font>(*DefaultFont)); *DefaultFont = *EngineDataLoader::LoadFont(*this, path); }, [this]() {return GetDefaultFont()->GetPath(); }, { "*.ttf", "*.otf" });
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
						auto& viewTexField = window.AddField("View texture");
						SharedPtr<unsigned int> texID = MakeShared<unsigned int>(0);

						viewTexField.CreateChild<UIInputBoxActor>("PreviewTexIDInputBox").SetOnInputFunc([texID](float val) { *texID = val; }, [texID]() { return *texID; });
						viewTexField.CreateChild<UIButtonActor>("OKButton", "OK", [this, texID, &window]() {
							if (!glIsTexture(*texID))
								return;
							UIWindowActor& texPreviewWindow = window.CreateChildCanvas<UIWindowActor>("PreviewTexWindow");
							ModelComponent& texPreviewQuad = texPreviewWindow.CreateChild<UIActorDefault>("TexPreviewActor").CreateComponent<ModelComponent>("TexPreviewQuad");
							Material* mat = new Material("TexturePreviewMat", 0.0f, GetRenderEngineHandle()->FindShader("Forward_NoLight"));
							mat->AddTexture(MakeShared<NamedTexture>(Texture::FromGeneratedGlId(Vec2u(0), GL_TEXTURE_2D, *texID, Texture::Format::RGBA()), "albedo1"));
							texPreviewQuad.AddMeshInst(MeshInstance(GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), mat));

							texPreviewWindow.AutoClampView();

							}).GetTransform()->Move(Vec2f(2.0f, 0.0f));
					}

					window.AddField("Render debug icons").GetTemplates().TickBox(bDebugRenderComponents);

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
						list.CreateChild<UIButtonActor>(it->GetName() + "Button", it->GetName(), [this, &window, &it]() { UIWindowActor& shaderWindow = window.CreateChildCanvas<UIWindowActor>("ShdaerPreviewWindow"); EditorDescriptionBuilder descBuilder(*this, shaderWindow, shaderWindow); it->GetEditorDescription(descBuilder); shaderWindow.AutoClampView(); shaderWindow.RefreshFieldsList(); });
					}
					list.Refresh();
				}, Transform(Vec2f(0.1f, -0.8f), Vec2f(0.1f)));
				mainButtonTheme(shadersButton, 2.0f);

				// Profiling
				Shader& graphShader = *RenderEng.AddShader(ShaderLoader::LoadShaders("GraphShader", "Shaders/graph/graph.vs", "Shaders/graph/graph.fs"), true);
				graphShader.SetExpectedMatrices({ MatrixType::MVP });

				auto& profilerButton = editorScene.CreateActorAtRoot<UIButtonActor>("SIEMANDZIOBUTTON", "Profile", nullptr, Transform(Vec2f(0.4f, -0.8f), Vec2f(0.1f)));
				profilerButton.SetOnClickFunc([&]() {
					auto& profilerWindow = editorScene.CreateActorAtRoot<UIWindowActor>("Profiler window");

					// Additional info
					auto& additionalInfoActor = profilerWindow.CreateChild<UIActorDefault>("AdditionalFPSData", Transform(Vec2f(1.5f, 0.0f), Vec2f(1.0f)));

					// Actual graph
					auto graphMaterial = MakeShared<Material>("GraphMaterial", 0.0f, &graphShader);
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
					graphButton.CreateComponent<TextComponent>("XUnitMain", Transform(Vec2f(1.0f, -1.0f), Vec2f(0.1f)), "[seconds]", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::TOP));
					graphButton.CreateComponent<TextComponent>("YUnitMain", Transform(Vec2f(-1.0f, 1.0f), Vec2f(0.1f)), "[FPS]", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::RIGHT, TextAlignment::BOTTOM));

					graphButton.CreateComponent<TextComponent>("XUnitText1", Transform(Vec2f(-1.0f, -1.0f), Vec2f(0.1f)), "-30", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::TOP));
					graphButton.CreateComponent<TextComponent>("XUnitText2", Transform(Vec2f(0.0f, -1.0f), Vec2f(0.1f)), "-15", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::TOP));
					graphButton.CreateComponent<TextComponent>("XUnitText3", Transform(Vec2f(1.0f, -1.0f), Vec2f(0.1f)), "0", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::RIGHT, TextAlignment::TOP));

					graphButton.CreateComponent<TextComponent>("YUnitText1", Transform(Vec2f(-1.0f, -1.0f), Vec2f(0.1f)), "0", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::RIGHT, TextAlignment::BOTTOM));
					graphButton.CreateComponent<TextComponent>("YUnitText2", Transform(Vec2f(-1.0f, 0.0f), Vec2f(0.1f)), "150", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::RIGHT, TextAlignment::CENTER));
					graphButton.CreateComponent<TextComponent>("YUnitText3", Transform(Vec2f(-1.0f, 1.0f), Vec2f(0.1f)), "300", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::RIGHT, TextAlignment::TOP));

					auto& infoText = graphButton.CreateComponent<TextComponent>("InfoText", Transform(Vec2f(-0.0f, 1.0f), Vec2f(0.05f)), "FPS - the lower the better\n<Hold SHIFT while dragging mouse to inspect a range>", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::BOTTOM));
					auto greyMaterial = MakeShared<Material>("Grey", 0.0f, RenderEng.FindShader("TextShader"));
					greyMaterial->SetColor(Vec4f(Vec3f(0.5f), 1.0f));
					infoText.SetMaterialInst(*RenderEng.AddMaterial(greyMaterial));

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

			SharedPtr<Material> scenePreviewMaterial = MakeShared<Material>("GEE_3D_SCENE_PREVIEW_MATERIAL", 0.0f, GetGameHandle()->GetRenderEngineHandle()->FindShader("Forward_NoLight"));
			RenderEng.AddMaterial(scenePreviewMaterial);
			scenePreviewMaterial->AddTexture(MakeShared<NamedTexture>(ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorTexture(0), "albedo1"));
			EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->OverrideInstancesMaterial(scenePreviewMaterial.get());
		}

		void GameEngineEngineEditor::SetupMainMenu()
		{
			GameScene& mainMenuScene = CreateUIScene("GEE_Main_Menu", *Window);

			//SetCanvasContext(&mainMenuScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Canvas_Context"));

			{
				Actor& engineTitleActor = mainMenuScene.CreateActorAtRoot<Actor>("EngineTitleActor", Transform(Vec2f(0.0f, 0.6f)));
				TextComponent& engineTitleTextComp = engineTitleActor.CreateComponent<TextComponent>("EngineTitleTextComp", Transform(), "Game Engine Engine", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
				engineTitleTextComp.SetTransform(Transform(Vec2f(0.0f), Vec2f(0.09f)));

				SharedPtr<Material> titleMaterial = MakeShared<Material>("GEE_Engine_Title");
				RenderEng.AddMaterial(titleMaterial);
				titleMaterial->SetColor(hsvToRgb(Vec3f(300.0f, 1.0f, 1.0f)));

				engineTitleTextComp.SetMaterialInst(MaterialInstance(*titleMaterial));

				TextComponent& engineSubtitleTextComp = engineTitleActor.CreateComponent<TextComponent>("EngineSubtitleTextComp", Transform(), "made by Michal Andryskowski", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
				engineSubtitleTextComp.SetTransform(Transform(Vec2f(0.0f, -0.15f), Vec2f(0.045f)));
				Material* subtitleMaterial = RenderEng.AddMaterial(MakeShared<Material>("GEE_Engine_Subtitle"));
				subtitleMaterial->SetColor(hsvToRgb(Vec3f(300.0f, 0.4f, 0.3f)));
				engineSubtitleTextComp.SetMaterialInst(*subtitleMaterial);
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


				window.AddField("Project filepath").GetTemplates().PathInput([this](const std::string& filepath) { ProjectFilepath = filepath;  extractDirectoryAndFilename(filepath, ProjectName, std::string()); }, [this]() { return ProjectFilepath; }, { "*.json", "*.geeproject", "*.geeprojectold" });

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
					recentProjectsActor.CreateComponent<TextComponent>("RecentsText", Transform(Vec2f(0.0f, 1.6f), Vec2f(0.5f)), "Recent projects", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
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
					else if (ActiveScene == EditorScene && keyEventCast.GetKeyCode() == Key::S && GetInputRetriever().IsKeyPressed(Key::LeftControl))
						SaveProject();
				}

				if (ActiveScene)
					ActiveScene->HandleEventAll(*polledEvent);
			}

			for (auto& popup : OpenPopups)
			{
				while (SharedPtr<Event> polledEvent = EventHolderObj.PollEvent(popup.Window))
				{
					popup.Scene.get().HandleEventAll(*polledEvent);
				}
			}
		}

		void GameEngineEngineEditor::SelectComponent(Component* comp, GameScene& editorScene)
		{
			if (comp && comp->IsBeingKilled())	//Components and Actors that are being killed should never be chosen - doing it is a perfect opportunity for runtime crashes to occur.
				comp = nullptr;


			bool sameComp = SelectedComp == comp;
			SelectedComp = comp;
			Transform previousCanvasView;
			if (const Actor* found = editorScene.GetRootActor()->FindActor("GEE_E_Components_Info_Canvas"))
			{
				previousCanvasView = dynamic_cast<UICanvasActor*>(const_cast<Actor*>(found))->CanvasView;
				const_cast<Actor*>(found)->MarkAsKilled();
			}

			if (!comp)
				return;

			UICanvasActor& canvas = editorScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Components_Info_Canvas");
			canvas.SetTransform(Transform(Vec2f(-0.75f, 0.75f), Vec2f(0.2f)));
			UIActorDefault* scaleActor = canvas.GetScaleActor();

			comp->GetEditorDescription(EditorDescriptionBuilder(*this, *scaleActor));

			canvas.RefreshFieldsList();

			//Scenes.back()->RootActor->DebugHierarchy();

			if (!previousCanvasView.IsEmpty() && sameComp)
				canvas.SetCanvasView(previousCanvasView);
			else
				canvas.AutoClampView();
		}

		void GameEngineEngineEditor::SelectActor(Actor* actor, GameScene& editorScene)
		{
			if (actor && actor->IsBeingKilled())	//Components and Actors that are being killed should never be chosen - doing it is a perfect opportunity for runtime crashes to occur.
				actor = nullptr;

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

			UICanvasActor& canvas = editorScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Actors_Components_Canvas", Transform(Vec2f(-0.68f, 0.0f), Vec2f(0.32f, 0.5f)));
			canvas.SetViewScale(Vec2f(1.0f, glm::sqrt(0.5f / 0.32f)));
			canvas.SetPopupCreationFunc([this](PopupDescription desc)
				{
					desc.AddSubmenu("Create component", [this](PopupDescription desc) { desc.AddOption("Component", nullptr, IconData(*GetGameHandle()->GetRenderEngineHandle(), "EditorAssets/component_icon.png", Vec2i(3, 1), 0.0f)); desc.AddOption("Light", nullptr); });
					desc.AddSubmenu("Refresh", [](PopupDescription desc) { desc.AddOption("Type 1", nullptr); desc.AddOption("Type 2", nullptr); }, IconData(*GetGameHandle()->GetRenderEngineHandle(), "EditorAssets/refresh_icon.png", Vec2i(3, 1), 0.0f));
					desc.AddSubmenu("Types", [](PopupDescription desc) { desc.AddOption("Type 1", nullptr); desc.AddOption("Type 2", nullptr); });
					desc.AddSubmenu("Delete", [](PopupDescription desc) { desc.AddOption("Type 1", nullptr); desc.AddOption("Type 2", nullptr); });
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

			actor->GetEditorDescription(EditorDescriptionBuilder(*this, *scaleActor));

			if (canvas.FieldsList)
				canvas.FieldsList->Refresh();

			UIButtonActor& refreshButton = scaleActor->CreateChild<UIButtonActor>("GEE_E_Scene_Refresh_Button", [this, actor, &editorScene]() { this->SelectActor(actor, editorScene); }, Transform(Vec2f(6.0f, 0.0f), Vec2f(0.5f)));
			uiButtonActorUtil::ButtonMatsFromAtlas(refreshButton, *dynamic_cast<AtlasMaterial*>(RenderEng.FindMaterial("GEE_E_Refresh_Icon_Mat").get()), 0.0f, 1.0f, 2.0f);

			UIButtonActor& addActorButton = scaleActor->CreateChild<UIButtonActor>("GEE_E_Scene_Add_Actor_Button", [this, &editorScene, &refreshButton, actor, &canvas]() {
				UIWindowActor& compSelectWindow = editorScene.CreateActorAtRoot<UIWindowActor>(&canvas, "GEE_E_Component_Selection_Window", Transform(Vec2f(0.0f, -0.5f), Vec2f(0.3f)));

				auto createCompButton = [this, &compSelectWindow, &editorScene, &refreshButton, actor](int i, std::function<Component& ()> createCompFunc) {
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
				createCompButton(1, [this, &editorScene, getSelectedComp]() -> Component& { LightComponent& comp = getSelectedComp().CreateComponent<LightComponent>("A LightComponent", POINT, GetMainScene()->GetRenderData()->GetAvailableLightIndex(), GetMainScene()->GetRenderData()->GetAvailableLightIndex()); comp.CalculateLightRadius(); return comp;  });
				createCompButton(2, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent<Audio::SoundSourceComponent>("A SoundSourceComponent"); });
				createCompButton(3, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent<CameraComponent>("A CameraComponent"); });
				createCompButton(4, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent<TextComponent>("A TextComponent", Transform(), "", ""); });
				createCompButton(5, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent<LightProbeComponent>("A LightProbeComponent"); });

			}, Transform(Vec2f(8.0f, 0.0f), Vec2f(0.5f)));

			AtlasMaterial& addIconMat = *dynamic_cast<AtlasMaterial*>(GetRenderEngineHandle()->FindMaterial("GEE_E_Add_Icon_Mat").get());
			addIconMat.AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("EditorAssets/add_icon.png", Texture::Format::RGBA(), false, Texture::MinFilter::NearestInterpolateMipmap()), "albedo1"));
			uiButtonActorUtil::ButtonMatsFromAtlas(addActorButton, addIconMat, 0.0f, 1.0f, 2.0f);

			if (!previousCanvasView.IsEmpty() && sameActor)
				canvas.SetCanvasView(previousCanvasView);
			else
			{
				canvas.AutoClampView();
				canvas.SetViewScale(static_cast<Vec2f>(canvas.CanvasView.GetScale()) * Vec2f(1.0f, glm::sqrt(0.5f / 0.32f)));
			}
		}

		void GameEngineEngineEditor::SelectScene(GameScene* selectedScene, GameScene& editorScene)
		{
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

			auto& aaa = editorScene.CreateActorAtRoot<UICanvasFieldCategory>("AAAAA");
			aaa.SetTransform(Transform(Vec2f(0.68f, 0.1f), Vec2f(0.0272f, 0.0272f)));

			auto& plswork = aaa.CreateChild<UIActorDefault>("eeafksnflak");

			UICanvasActor& canvas = plswork.CreateChild<UICanvasActor>("GEE_E_Scene_Actors_Canvas", Transform(Vec2f(0.0f), Vec2f(10.0f)));
			if (!previousCanvasView.IsEmpty() && sameScene)
				canvas.CanvasView = previousCanvasView;
			canvas.SetPopupCreationFunc([](PopupDescription desc)
				{
					desc.AddOption("Refresh", nullptr);
				});

			UIButtonActor& refreshButton = canvas.CreateChild<UIButtonActor>("GEE_E_Scene_Refresh_Button", [this, selectedScene, &editorScene]() { this->SelectScene(selectedScene, editorScene); }, Transform(Vec2f(6.0f, 0.0f), Vec2f(0.5f)));
			uiButtonActorUtil::ButtonMatsFromAtlas(refreshButton, *dynamic_cast<AtlasMaterial*>(RenderEng.FindMaterial("GEE_E_Refresh_Icon_Mat").get()), 0.0f, 1.0f, 2.0f);



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

			AtlasMaterial& addActorMat = *new AtlasMaterial(Material("GEE_E_Add_Actor_Material", 0.0f, RenderEng.FindShader("Forward_NoLight")), glm::ivec2(3, 1));
			addActorMat.AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("EditorAssets/add_icon.png", Texture::Format::RGBA(), false, Texture::MinFilter::NearestInterpolateMipmap()), "albedo1"));
			uiButtonActorUtil::ButtonMatsFromAtlas(addActorButton, addActorMat, 0.0f, 1.0f, 2.0f);

			UIAutomaticListActor& listActor = canvas.CreateChild<UIAutomaticListActor>("ListActor");
			AddActorToList<Actor>(editorScene, *selectedScene->GetRootActor(), listActor, canvas);

			listActor.Refresh();

			EditorScene->GetRootActor()->DebugActorHierarchy();

			canvas.AutoClampView();
			canvas.SetViewScale(static_cast<Vec2f>(canvas.CanvasView.GetScale()) * Vec2f(1.0f, glm::sqrt(0.32f / 0.32f)));
		}

		void GameEngineEngineEditor::PreviewHierarchyTree(HierarchyTemplate::HierarchyTreeT& tree)
		{
			/*Actor& actor = GetMainScene()->CreateActorAtRoot<Actor>("Added"));
			std::cout << "Added actor creating done\n";
			EngineDataLoader::InstantiateTree(*actor.GetRoot(), tree);
			std::cout << "Added tree instantiated succesfully\n";

			SelectScene(GetMainScene(), *EditorScene);	//refresh
			SelectActor(&actor, *EditorScene);*/

			GameScene& editorScene = *EditorScene;
			UIWindowActor& previewWindow = editorScene.CreateActorAtRoot<UIWindowActor>("Mesh node preview window");
			previewWindow.SetTransform(Transform(Vec2f(0.0f, -0.5f), Vec2f(0.4f)));

			UICanvasActor& nodesCanvas = previewWindow.CreateChild<UICanvasActor>("Nodes canvas");
			nodesCanvas.SetTransform(Transform(Vec2f(-0.2f, 0.2f), Vec2f(0.8f, 0.8f)));

			UIButtonActor& instantiateButton = previewWindow.CreateChild<UIButtonActor>("InstantiateButton", "Instantiate", [this, &tree, &editorScene]() {
				std::function<Component* ()> getSelectedComp = [this]() -> Component* { return ((SelectedComp) ? (SelectedComp) : ((SelectedActor) ? (SelectedActor->GetRoot()) : (nullptr))); };

				if (Component* selectedComp = getSelectedComp())
					EngineDataLoader::InstantiateTree(*selectedComp, tree);

				SelectActor(SelectedActor, editorScene);
			});
			instantiateButton.SetTransform(Transform(Vec2f(0.875f, 0.8f), Vec2f(0.1f)));

			previewWindow.KillScrollBars(); //for now

			std::function<void(UIAutomaticListActor&, const HierarchyTemplate::HierarchyNodeBase&, int)> nodeCreatorFunc = [&editorScene, &nodesCanvas, &nodeCreatorFunc](UIAutomaticListActor& parent, const HierarchyTemplate::HierarchyNodeBase& correspondingNode, int level) {


				UIButtonActor& nodeButton = parent.CreateChild<UIButtonActor>("OGAR");//correspondingNode.GetName()));
				TextConstantSizeComponent& nodeText = nodeButton.CreateComponent<TextConstantSizeComponent>("TEXTTEXT", Transform(), correspondingNode.GetCompBaseType().GetName(), "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
				nodeButton.SetTransform(Transform(Vec2f(0.0f, -3.0f), Vec2f(1.0f)));

				//std::cout << "NODE: " << correspondingNode.GetName() << ". CHILDREN COUNT: " << correspondingNode.GetChildCount() << '\n';

				for (int i = 0; i < correspondingNode.GetChildCount(); i++)
				{
					UIAutomaticListActor& nodeChildrenList = parent.CreateChild<UIAutomaticListActor>(correspondingNode.GetCompBaseType().GetName() + "children list", Vec3f(3.0f, 0.0f, 0.0f));
					if (i == 0)
						nodeChildrenList.GetTransform()->Move(((float)correspondingNode.GetChildCount() - 1.0f) * -Vec3f(3.0f, 0.0f, 0.0f) / 2.0f);
					nodeCreatorFunc(nodeChildrenList, *correspondingNode.GetChild(i), level + 1);
				}

				//nodeChildrenList.GetTransform()->Move(((float)correspondingNode.GetChildCount() - 1.0f) * -Vec3f(10.0f, 0.0f, 0.0f) / 2.0f);
			};

			UIAutomaticListActor& rootList = previewWindow.CreateChild<UIAutomaticListActor>("Root children list", Vec3f(3.0f, 0.0f, 0.0f));
			rootList.GetTransform()->SetScale(Vec2f(0.1f));

			nodeCreatorFunc(rootList, tree.GetRoot(), 0);
			rootList.Refresh();

			std::function<void(Actor&)> func = [&previewWindow, &func](Actor& actor) {
				if (TextComponent* text = actor.GetRoot()->GetComponent<TextComponent>("TEXTTEXT"))
				{
					std::string str1 = std::to_string(previewWindow.ToCanvasSpace(actor.GetTransform()->GetWorldTransform()).GetPos().x);
					str1 = str1.erase(str1.find_last_not_of('0') + 1, std::string::npos);
					if (str1.back() == '.')
						str1 = str1.substr(0, str1.length() - 1);

					std::string str2 = std::to_string(previewWindow.ToCanvasSpace(actor.GetTransform()->GetWorldTransform()).GetPos().y);
					str2 = str2.erase(str2.find_last_not_of('0') + 1, std::string::npos);
					if (str2.back() == '.')
						str2 = str2.substr(0, str2.length() - 1);
					text->SetContent(text->GetContent() + str1 + ", " + str2);
				}

				for (Actor* child : actor.GetChildren())
					func(*child);
			};

			func(rootList);

			for (int i = 0; i < tree.GetAnimationCount(); i++)
			{
				UIButtonActor& animationButton = previewWindow.CreateChild<UIButtonActor>(tree.GetAnimation(i).Localization.Name + " animation button", tree.GetAnimation(i).Localization.Name);
				animationButton.SetTransform(Transform(Vec2f(-0.8f + static_cast<float>(i) * 0.3f, -0.8f), Vec2f(0.1f)));
			}

			previewWindow.DebugActorHierarchy(0);
		}

		void GameEngineEngineEditor::PassMouseControl(Controller* controller)
		{
			if (ActiveScene == EditorScene)
				GameController = controller;
			else
				Game::PassMouseControl(controller);
		}

		void GameEngineEngineEditor::Render()
		{
			//////////////////////POPUPS
			OpenPopups.erase(std::remove_if(OpenPopups.begin(), OpenPopups.end(), [](Popup& popup) { if (glfwWindowShouldClose(&popup.Window.get())) { glfwDestroyWindow(&popup.Window.get()); return true; } return false; }), OpenPopups.end());

			////////////////////////MAIN WINDOW
			if ((GetMainScene() && !GetMainScene()->ActiveCamera) || (EditorScene && !EditorScene->ActiveCamera))
			{
				if (auto camActor = GetMainScene()->FindActor("CameraActor"))
					if (auto comp = camActor->GetRoot()->GetComponent<CameraComponent>("Camera"))
						GetMainScene()->BindActiveCamera(comp);
				if (auto camActor = EditorScene->FindActor("OrthoCameraActor"))
					if (auto comp = camActor->GetRoot()->GetComponent<CameraComponent>("OrthoCameraComp"))
						EditorScene->BindActiveCamera(comp);
			}

			RenderEng.PrepareFrame();


			if (GetMainScene() && GetMainScene()->ActiveCamera)
			{
				//Render 3D scene
				RenderEng.PrepareScene(0, *ViewportRenderCollection, GetMainScene()->GetRenderData());
				SceneRenderer(RenderEng, &ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer()).FullRender(GetMainScene()->ActiveCamera->GetRenderInfo(0, *ViewportRenderCollection), Viewport(Vec2f(0.0f)), true, false,
					[&](GEE_FB::Framebuffer& mainFramebuffer) {
					if (bDebugRenderComponents)
					{
						mainFramebuffer.SetDrawSlot(0);
						RenderEng.FindShader("Forward_NoLight")->Use();
						GetMainScene()->GetRootActor()->DebugRenderAll(GetMainScene()->ActiveCamera->GetRenderInfo(0, *ViewportRenderCollection), RenderEng.FindShader("Forward_NoLight"));
						mainFramebuffer.SetDrawSlots();
					}
					});// , Viewport(Vec2f(0.0f), Settings->Video.Resolution)/*Viewport((static_cast<Vec2f>(Settings->WindowSize) - Settings->Video.Resolution) / Vec2f(2.0f, 1.0f), Vec2f(Settings->Video.Resolution.x, Settings->Video.Resolution.y))*/);

				if (SelectedActor && !GetInputRetriever().IsKeyPressed(Key::F2))
				{
					std::function<void(Component&)> renderAllColObjs = [this, &renderAllColObjs](Component& comp) {
						if (comp.GetCollisionObj() && comp.GetName() != "Counter")
							CollisionObjRendering(GetMainScene()->ActiveCamera->GetRenderInfo(0, *ViewportRenderCollection), *this, *comp.GetCollisionObj(), comp.GetTransform().GetWorldTransform(), (SelectedComp && SelectedComp == &comp) ? (Vec3f(0.1f, 0.6f, 0.3f)) : (Vec3f(0.063f, 0.325f, 0.071f)));

						for (auto& child : comp.GetChildren())
							renderAllColObjs(*child);
					};

					renderAllColObjs(*SelectedActor->GetRoot());
				}

				RenderEng.RenderText(SceneMatrixInfo(*ViewportRenderCollection, *GetMainScene()->GetRenderData()), *GetDefaultFont(), "(C) twoja babka studios", Transform(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f), Vec3f(32.0f)), glm::pow(Vec3f(0.0f, 0.73f, 0.84f), Vec3f(1.0f / 2.2f)), nullptr, true);
			}
			else if (GetMainScene())
			{
				ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().Bind(true);
				glClear(GL_COLOR_BUFFER_BIT);

				RenderEng.FindShader("Forward_NoLight")->Use();
				Renderer(RenderEng).StaticMeshInstances(SceneMatrixInfo(*ViewportRenderCollection, *GetMainScene()->GetRenderData()), { MeshInstance(RenderEng.GetBasicShapeMesh(EngineBasicShape::QUAD), RenderEng.FindMaterial("GEE_No_Camera_Icon").get()) }, Transform(), *RenderEng.FindShader("Forward_NoLight"));

				RenderEng.RenderText(SceneMatrixInfo(*ViewportRenderCollection, *GetMainScene()->GetRenderData()), *DefaultFont, "No camera", Transform(Vec2f(0.0f, -0.8f), Vec2f(0.1f)), Vec3f(1.0f, 0.0f, 0.0f), nullptr, false, std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));

			}

			bool renderEditorScene = EditorScene && EditorScene->ActiveCamera;
			if (renderEditorScene)
			{
				///Render editor scene
				SceneMatrixInfo info = EditorScene->ActiveCamera->GetRenderInfo(0, *HUDRenderCollection);
				RenderEng.PrepareScene(0, *HUDRenderCollection, EditorScene->GetRenderData());
				SceneRenderer(RenderEng).FullRender(info, Viewport(Vec2f(0.0f), Vec2f(Settings->WindowSize)), true, true);
			}

			if (GameScene* mainMenuScene = GetScene("GEE_Main_Menu"))
				if (mainMenuScene->GetActiveCamera())
				{
					SceneMatrixInfo info = mainMenuScene->ActiveCamera->GetRenderInfo(0, *HUDRenderCollection);
					RenderEng.PrepareScene(0, *HUDRenderCollection, mainMenuScene->GetRenderData());
					SceneRenderer(RenderEng).FullRender(info, Viewport(Vec2f(0.0f), Vec2f(Settings->WindowSize)), !renderEditorScene, true);
				}
			
			if (GameScene* meshPreviewScene = GetScene("GEE_Mesh_Preview_Scene"))
				if (meshPreviewScene->GetActiveCamera())
				{
					RenderToolboxCollection& renderTbCollection = **std::find_if(RenderEng.RenderTbCollections.begin(), RenderEng.RenderTbCollections.end(), [](const UniquePtr<RenderToolboxCollection>& tbCol) { return tbCol->GetName() == "GEE_E_Mesh_Preview_Toolbox_Collection"; });
					SceneMatrixInfo info = meshPreviewScene->ActiveCamera->GetRenderInfo(0, renderTbCollection);
					RenderEng.PrepareScene(0, renderTbCollection, meshPreviewScene->GetRenderData());
					SceneRenderer(RenderEng, &renderTbCollection.GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer()).FullRender(info);
				}

			if (EditorScene)
				if (UIButtonActor* cast = dynamic_cast<UIButtonActor*>(EditorScene->GetRootActor()->FindActor("SceneViewportActor")))
					;// cast->OnClick();


			glfwSwapBuffers(Window);

			for (auto& popup : OpenPopups)
			{
				glfwMakeContextCurrent(&popup.Window.get());
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
				glDebugMessageCallbackARB(DebugCallbacks::OpenGLDebug, nullptr);
				glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

				glEnable(GL_DEPTH_TEST);
				glDisable(GL_CULL_FACE);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, 400, 400);
				glDrawBuffer(GL_BACK);
				glClearColor(glm::abs(glm::cos((this->GetProgramRuntime()))), 0.3f, 0.8f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				SceneMatrixInfo info(popup.ID, *ViewportRenderCollection, *popup.Scene.get().GetRenderData(), Mat4f(1.0f), glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -2550.0f), Vec3f(0.0f));
				info.SetCareAboutShader(true);
				Material mat("", 0.0f, RenderEng.FindShader("Forward_NoLight"));
				mat.SetColor(Vec4f(1.0f, 0.0f, 0.2f, 1.0f));

				RenderEng.FindShader("Forward_NoLight")->Use();
				mat.UpdateWholeUBOData(RenderEng.FindShader("Forward_NoLight"), RenderEng.GetEmptyTexture());

				RenderEng.GetBasicShapeMesh(EngineBasicShape::QUAD).Bind(popup.ID);
				//Renderer(RenderEng).StaticMeshInstances(MatrixInfoExt(1, Mat4f(1.0f), Mat4f(1.0f), Vec3f(0.0f)), { MeshInstance(RenderEng.GetBasicShapeMesh(EngineBasicShape::QUAD), &mat) }, Transform(), *RenderEng.FindShader("Forward_NoLight"));

				//GetScene("GEE_E_Popup_Window")->FindActor("CreateComponentButton")->GetRoot()->GetComponent<RenderableComponent>("CreateComponentButton's_Button_Model")->Render(info, RenderEng.FindShader("Forward_NoLight"));
				//SceneRenderer(RenderEng).FullRender());
				SceneRenderer(RenderEng).RawUIScene(info);

				glfwSwapBuffers(&popup.Window.get());
			}

			glfwMakeContextCurrent(Window);
		}

		void GameEngineEngineEditor::LoadProject(const std::string& filepath)
		{
			ProjectFilepath = filepath;

			std::cout << "Opening project with filepath: " << filepath << '\n';
			SetupEditorScene();
			EditorScene = GetScene("GEE_Editor");
			if (GameScene* mainMenuScene = GetScene("GEE_Main_Menu"))
				mainMenuScene->MarkAsKilled();
			LoadSceneFromFile(filepath, "GEE_Main");
			SetMainScene(GetScene("GEE_Main"));
			SetActiveScene(EditorScene);
			SelectScene(GetMainScene(), *EditorScene);
		
			SceneRenderer(RenderEng, 0).PreRenderLoopPassStatic(0, GetSceneRenderDatas());

			UpdateRecentProjects();

			glfwRequestWindowAttention(Window);
		}

		void GameEngineEngineEditor::SaveProject()
		{
			if (getFilepathExtension(ProjectFilepath) == ".geeprojectold")
				ProjectFilepath = ProjectFilepath.substr(0, ProjectFilepath.find(".geeprojectold")) + ".json";
			std::cout << "Saving project to path " << ProjectFilepath << "\n";

			//CheckSerializationErrors();

			std::ofstream serializationStream(ProjectFilepath);

			if (Logs.find(EditorScene) == Logs.end())
				Logs.emplace(std::make_pair(EditorScene, EditorMessageLogger(*this, *EditorScene)));
			Logs.find(EditorScene)->second.Log("Saving project" + ToStringPrecision(IDSystem<int>::GenerateID(), 0), EditorMessageLogger::MessageType::Information);

			{
				try
				{
					cereal::JSONOutputArchive archive(serializationStream);
					GetMainScene()->GetRenderData()->SaveSkeletonBatches(archive);
					GetMainScene()->GetRootActor()->Save(archive);
					archive.serializeDeferments();
				}
				catch (cereal::Exception& ex)
				{
					std::cout << "ERROR While saving scene: " << ex.what() << '\n';
					Logs.emplace(std::make_pair(EditorScene, EditorMessageLogger(*this, *EditorScene)));
					Logs.find(EditorScene)->second.Log(ex.what(), EditorMessageLogger::MessageType::Error);
				}
			}
			serializationStream.close();

			UpdateRecentProjects();

			std::cout << "Project " + ProjectName + " (" + ProjectFilepath + ") saved successfully.\n";
		//	Logs.find(EditorScene)->second.Log("Project saved", EditorMessageLogger::MessageType::Success);
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

	}
}