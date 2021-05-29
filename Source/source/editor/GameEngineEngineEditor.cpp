#include <editor/GameEngineEngineEditor.h>
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
#include <whereami.h>
#include <map>

namespace GEE
{
	void EditorEventProcessor::FileDropCallback(GLFWwindow* window, int count, const char** paths)
	{
		if (count > 0)
		{
			std::cout << "Dropped " << paths[0] << '\n';
			EditorHandle->PreviewHierarchyTree(*EngineDataLoader::LoadHierarchyTree(*EditorHandle->GetGameHandle()->GetMainScene(), paths[0]));
		}
	}

	void EditorEventProcessor::Resize(GLFWwindow* window, int width, int height)
	{
		EditorHandle->GetGameHandle()->GetGameSettings()->WindowSize = glm::uvec2(width, height);
		EditorHandle->UpdateSettings();
	}


	GameEngineEngineEditor::GameEngineEngineEditor(GLFWwindow* window, const GameSettings& settings) :
		Game(settings.Video.Shading, settings),
		EditorScene(nullptr),
		SelectedComp(nullptr),
		SelectedActor(nullptr),
		SelectedScene(nullptr)
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

		Settings = std::make_unique<GameSettings>(GameSettings(EngineDataLoader::LoadSettingsFromFile<GameSettings>("Settings.ini")));
		glm::vec2 res = static_cast<glm::vec2>(Settings->WindowSize);
		//Settings->ViewportData = glm::uvec4(res.x * 0.3f, res.y * 0.4f, res.x * 0.4, res.y * 0.6f);
		Settings->Video.Resolution = glm::vec2(res.x * 0.4f, res.y * 0.6f);
		Settings->Video.Shading = ShadingModel::SHADING_PBR_COOK_TORRANCE;
		Init(window);
	}

	GameManager* GameEngineEngineEditor::GetGameHandle()
	{
		return this;
	}

	GameScene* GameEngineEngineEditor::GetSelectedScene()
	{
		return SelectedScene;
	}

	std::vector<GameScene*> GameEngineEngineEditor::GetScenes()
	{
		std::vector<GameScene*> scenes(Scenes.size());
		std::transform(Scenes.begin(), Scenes.end(), scenes.begin(), [](std::unique_ptr<GameScene>& sceneVec) {return sceneVec.get(); });
		return scenes;
	}

	void GameEngineEngineEditor::Init(GLFWwindow* window)
	{
		Game::Init(window);

		EditorEventProcessor::EditorHandle = this;
		glfwSetDropCallback(Window, EditorEventProcessor::FileDropCallback);
		glfwSetFramebufferSizeCallback(Window, EditorEventProcessor::Resize);


		std::shared_ptr<AtlasMaterial> addIconMat = std::make_shared<AtlasMaterial>(Material("GEE_E_Add_Icon_Mat", 0.0f, RenderEng.FindShader("Forward_NoLight")), glm::ivec2(3, 1));
		GetRenderEngineHandle()->AddMaterial(addIconMat);
		addIconMat->AddTexture(std::make_shared<NamedTexture>(textureFromFile("EditorAssets/add_icon.png", GL_RGB, GL_LINEAR, GL_NEAREST_MIPMAP_LINEAR), "albedo1"));

		std::shared_ptr<AtlasMaterial> refreshIconMat = std::make_shared<AtlasMaterial>(Material("GEE_E_Refresh_Icon_Mat", 0.0f, RenderEng.FindShader("Forward_NoLight")), glm::ivec2(3, 1));
		GetRenderEngineHandle()->AddMaterial(refreshIconMat);
		refreshIconMat->AddTexture(std::make_shared<NamedTexture>(textureFromFile("EditorAssets/refresh_icon.png", GL_RGB, GL_LINEAR, GL_NEAREST_MIPMAP_LINEAR), "albedo1"));

		{
			std::shared_ptr<Material> material = std::make_shared<Material>(Material("GEE_Default_Text_Material", 0.0f, RenderEng.FindShader("TextShader")));
			//material->SetColor(Vec4f(0.0f, 0.607057f, 0.663284f, 1.0f));
			material->SetColor(Vec4f(1.0f));
			GetRenderEngineHandle()->AddMaterial(material);
		}
		{
			std::shared_ptr<Material> material = std::make_shared<Material>(Material("GEE_Default_X_Axis", 0.0f, RenderEng.FindShader("Forward_NoLight")));
			material->SetColor(Vec4f(0.39f, 0.18f, 0.18f, 1.0f));
			GetRenderEngineHandle()->AddMaterial(material);
		}
		{
			std::shared_ptr<Material> material = std::make_shared<Material>(Material("GEE_Default_Y_Axis", 0.0f, RenderEng.FindShader("Forward_NoLight")));
			material->SetColor(Vec4f(0.18f, 0.39f, 0.25f, 1.0f));
			GetRenderEngineHandle()->AddMaterial(material);
		}
		{
			std::shared_ptr<Material> material = std::make_shared<Material>(Material("GEE_Default_Z_Axis", 0.0f, RenderEng.FindShader("Forward_NoLight")));
			material->SetColor(Vec4f(0.18f, 0.25f, 0.39f, 1.0f));
			GetRenderEngineHandle()->AddMaterial(material);
		}
		{
			std::shared_ptr<Material> material = std::make_shared<Material>(Material("GEE_Default_W_Axis", 0.0f, RenderEng.FindShader("Forward_NoLight")));
			material->SetColor(Vec4f(0.45f, 0.46f, 0.5f, 1.0f));
			GetRenderEngineHandle()->AddMaterial(material);
		}
		{
			std::shared_ptr<Material> material = std::make_shared<Material>(Material("GEE_E_Canvas_Background_Material", 0.0f, RenderEng.FindShader("Forward_NoLight")));
			material->SetColor(Vec4f(0.26f, 0.34f, 0.385f, 1.0f));
			GetRenderEngineHandle()->AddMaterial(material);
		}

		{
			std::shared_ptr<Material> material = std::make_shared<Material>(Material("GEE_No_Camera_Icon", 0.0f, RenderEng.FindShader("Forward_NoLight")));
			GetRenderEngineHandle()->AddMaterial(material);
			material->AddTexture(std::make_shared<NamedTexture>(textureFromFile("EditorAssets/no_camera_icon.png", GL_RGB, GL_LINEAR, GL_NEAREST_MIPMAP_LINEAR, true), "albedo1"));
		}

		std::shared_ptr<AtlasMaterial> kopecMat = std::make_shared<AtlasMaterial>(Material("Kopec", 0.0f, RenderEng.FindShader("Forward_NoLight")), glm::ivec2(2, 1));
		GetRenderEngineHandle()->AddMaterial(kopecMat);
		kopecMat->AddTexture(std::make_shared<NamedTexture>(textureFromFile("kopec_tekstura.png", GL_RGB, GL_LINEAR, GL_NEAREST_MIPMAP_LINEAR, true), "albedo1"));

		//LoadSceneFromFile("level.txt", "GEE_Main");
		//SetMainScene(GetScene("GEE_Main"));

		GameSettings::VideoSettings& renderSettings = EditorSettings.Video;
		renderSettings.AAType = AA_NONE;
		renderSettings.AmbientOcclusionSamples = 0;
		renderSettings.bBloom = false;
		renderSettings.DrawToWindowFBO = true;
		renderSettings.POMLevel = SettingLevel::SETTING_NONE;
		renderSettings.Resolution = Settings->WindowSize;
		renderSettings.Shading = ShadingModel::SHADING_FULL_LIT;
		renderSettings.ShadowLevel = SettingLevel::SETTING_NONE;
		renderSettings.TMType = ToneMappingType::TM_NONE;
		renderSettings.MonitorGamma = 1.0f;

		ViewportRenderCollection = &RenderEng.AddRenderTbCollection(RenderToolboxCollection("GameSceneRenderCollection", Settings->Video));

		HUDRenderCollection = &RenderEng.AddRenderTbCollection(RenderToolboxCollection("HUDRenderCollection", renderSettings));

		GameManager::DefaultScene = GetMainScene();
		SetActiveScene(GetMainScene());

		//SelectScene(ActiveScene, *EditorScene);
	}

	void GameEngineEngineEditor::UpdateSettings()
	{
		ViewportRenderCollection->AddTbsRequiredBySettings();

		Material* previewMaterial = RenderEng.FindMaterial("GEE_3D_SCENE_PREVIEW_MATERIAL").get();
		if (!previewMaterial)
		{
			std::cout << "ERROR: No scene preview material detected.\n";
			return;
		}
		previewMaterial->Textures.clear();
		previewMaterial->AddTexture(std::make_shared<NamedTexture>(*ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorBuffer(0), "albedo1"));
	}

	void GameEngineEngineEditor::SetupEditorScene()
	{
		GameScene& editorScene = CreateScene("GEE_Editor", true);

		//settings.ViewportData = glm::uvec4(res.x * 0.3f, res.y * 0.4f, res.x * 0.4, res.y * 0.6f);

		Actor& scenePreviewActor = editorScene.CreateActorAtRoot<Actor>("SceneViewportActor");
		ModelComponent& scenePreviewQuad = scenePreviewActor.CreateComponent<ModelComponent>("SceneViewportQuad");

		//SetCanvasContext(&editorScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Canvas_Context"));

		/*UICanvasActor& exampleCanvas = editorScene.CreateActorAtRoot<UICanvasActor>("ExampleCanvas");
		exampleCanvas.SetTransform(Transform(glm::vec3(0.7f, -0.4f, 0.0f), glm::vec3(0.0f), glm::vec3(0.3f)));
		UIButtonActor& lolBackgroundButton = exampleCanvas.CreateChild<UIButtonActor>("VeryImportantButton");
		lolBackgroundButton.SetTransform(Transform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f)));

		TextComponent& sampleText = exampleCanvas.CreateComponent<TextComponent>("SampleTextComp", Transform(glm::vec3(0.9f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f)), "Sample Text");
		sampleText.SetAlignment(TextAlignment::CENTER, TextAlignment::CENTER);

		TextComponent& sampleText2 = exampleCanvas.CreateComponent<TextComponent>("SampleTextComp", Transform(glm::vec3(1.9f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f)), "Very important text");

		TextComponent& sampleText3 = exampleCanvas.CreateComponent<TextComponent>("SampleTextComp", Transform(glm::vec3(1.9f, 1.5f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f)), "Mega ultra important text");

		for (int y = -3; y <= 3; y++)
		{
			for (int x = -3; x <= 3; x++)
			{
				TextComponent& gridSampleText = exampleCanvas.CreateComponent<TextComponent>("SampleTextComp", Transform(glm::vec3(x, y, 0.0f), glm::vec3(0.0f), glm::vec3(0.05f)), "Sample Text " + std::to_string(x) + "|||" + std::to_string(y));
				gridSampleText.SetAlignment(TextAlignment::RIGHT, TextAlignment::TOP);
			}
		}*/

		UIWindowActor& window1 = editorScene.CreateActorAtRoot<UIWindowActor>("MyTestWindow");
		window1.SetTransform(Transform(glm::vec2(0.0, -0.5f), glm::vec2(0.2f)));

		UIButtonActor& bigButtonActor = window1.CreateChild<UIButtonActor>("MojTestowyButton");
		bigButtonActor.SetTransform(Transform(glm::vec2(0.0f, 1.5f), glm::vec2(0.4f)));
		TextComponent& bigButtonText = bigButtonActor.CreateComponent<TextComponent>("BigButtonText", Transform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f)), "big button", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));

		editorScene.CreateActorAtRoot<UIButtonActor>("MeshTreesButton", "MeshTrees", [this, &editorScene]() {
			UIWindowActor& window = editorScene.CreateActorAtRoot<UIWindowActor>("MeshTreesWindow");
			window.GetTransform()->SetScale(glm::vec2(0.5f));

			UIAutomaticListActor& meshTreesList = window.CreateChild<UIAutomaticListActor>("MeshTreesList");
			for (auto& scene : Scenes)
				for (auto& meshTree : scene->HierarchyTrees)
					meshTreesList.CreateChild<UIButtonActor>("MeshTreeButton", meshTree->GetName(), [this, &meshTree]() { PreviewHierarchyTree(*meshTree); });

			meshTreesList.Refresh();

			}).SetTransform(Transform(Vec2f(-0.8f, -0.8f), Vec2f(0.1f)));

			editorScene.CreateActorAtRoot<UIButtonActor>("MaterialsButton", "Materials", [this, &editorScene]() {
				UIWindowActor& window = editorScene.CreateActorAtRoot<UIWindowActor>("MaterialsPreviewWindow");
				UIAutomaticListActor& list = window.CreateChild<UIAutomaticListActor>("MaterialsList");

				std::cout << "Nr of materials: " << RenderEng.Materials.size() << '\n';
				for (auto& it : RenderEng.Materials)
				{
					std::string name = (it) ? (it->GetLocalization().GetFullStr()) : ("NULLPTR???");
					UIButtonActor& button = list.CreateChild<UIButtonActor>(name + "Button", name, [this, &window, &it, name]() { UIWindowActor& matWindow = window.CreateChildCanvas<UIWindowActor>(name + "MaterialWindow"); EditorDescriptionBuilder descBuilder(*this, matWindow, matWindow); it->GetEditorDescription(descBuilder); matWindow.AutoClampView(); matWindow.RefreshFieldsList(); });
					std::cout << name << '\n';
				}

				list.Refresh();

				}).SetTransform(Transform(Vec2f(-0.5f, -0.8f), Vec2f(0.1f)));

				editorScene.CreateActorAtRoot<UIButtonActor>("SettingsButton", "Settings", [this, &editorScene]() {
					UIWindowActor& window = editorScene.CreateActorAtRoot<UIWindowActor>("MainSceneSettingsWindow");
					window.GetTransform()->SetScale(glm::vec2(0.5f));
					window.AddField("Bloom").GetTemplates().TickBox([this]() -> bool { bool updated = (GetGameSettings()->Video.bBloom = !GetGameSettings()->Video.bBloom); UpdateSettings(); return updated; });

					UIInputBoxActor& gammaInputBox = window.AddField("Gamma").CreateChild<UIInputBoxActor>("GammaInputBox");
					gammaInputBox.SetOnInputFunc([this](float val) { GetGameSettings()->Video.MonitorGamma = val; UpdateSettings(); }, [this]()->float { return GetGameSettings()->Video.MonitorGamma; });

					window.AddField("Default font").GetTemplates().PathInput([this](const std::string& path) { Fonts.push_back(std::make_shared<Font>(*DefaultFont)); *DefaultFont = *EngineDataLoader::LoadFont(*this, path); }, [this]() {return GetDefaultFont()->GetPath(); }, { "*.ttf", "*.otf" });
					window.AddField("Rebuild light probes").CreateChild<UIButtonActor>("RebuildProbesButton", "Rebuild", [this]() { RenderEng.PreLoopPass(); });

					window.FieldsList->Refresh();
					window.AutoClampView();

					}).SetTransform(Transform(Vec2f(-0.2f, -0.8f), Vec2f(0.1f)));

					editorScene.CreateActorAtRoot<UIButtonActor>("ShadersButton", "Shaders", [this, &editorScene]() {
						UIWindowActor& window = editorScene.CreateActorAtRoot<UIWindowActor>("ShadersPreviewWindow");
						UIAutomaticListActor& list = window.CreateChild<UIAutomaticListActor>("ShadersList");
						for (auto& it : RenderEng.Shaders)
						{
							list.CreateChild<UIButtonActor>(it->GetName() + "Button", it->GetName(), [this, &window, &it]() { UIWindowActor& shaderWindow = window.CreateChildCanvas<UIWindowActor>("ShdaerPreviewWindow"); EditorDescriptionBuilder descBuilder(*this, shaderWindow, shaderWindow); it->GetEditorDescription(descBuilder); shaderWindow.AutoClampView(); shaderWindow.RefreshFieldsList(); });
						}
						list.Refresh();
						}).SetTransform(Transform(Vec2f(0.1f, -0.8f), Vec2f(0.1f)));


						Actor& cameraActor = editorScene.CreateActorAtRoot<Actor>("OrthoCameraActor");
						CameraComponent& orthoCameraComp = cameraActor.CreateComponent<CameraComponent>("OrthoCameraComp", glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -2550.0f));

						editorScene.BindActiveCamera(&orthoCameraComp);


						EditorScene = &editorScene;

						EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->AddMeshInst(GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
						EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->SetTransform(Transform(glm::vec2(0.0f, 0.4f), glm::vec2(0.4f, 0.6f)));

						std::shared_ptr<Material> scenePreviewMaterial = std::make_shared<Material>("GEE_3D_SCENE_PREVIEW_MATERIAL", 0.0f, GetGameHandle()->GetRenderEngineHandle()->FindShader("Forward_NoLight"));
						RenderEng.AddMaterial(scenePreviewMaterial);
						scenePreviewMaterial->AddTexture(std::make_shared<NamedTexture>(*ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorBuffer(0), "albedo1"));
						EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->OverrideInstancesMaterial(scenePreviewMaterial.get());
	}

	void GameEngineEngineEditor::SetupMainMenu()
	{
		GameScene& mainMenuScene = CreateScene("GEE_Main_Menu", true);

		//SetCanvasContext(&mainMenuScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Canvas_Context"));

		{
			Actor& engineTitleActor = mainMenuScene.CreateActorAtRoot<Actor>("EngineTitleActor");
			TextComponent& engineTitleTextComp = engineTitleActor.CreateComponent<TextComponent>("EngineTitleTextComp", Transform(), "Game Engine Engine", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
			engineTitleTextComp.SetTransform(Transform(Vec2f(0.0f, 0.6f), Vec2f(0.12f)));

			std::shared_ptr<Material> purpleTextMaterial = std::make_shared<Material>("GEE_Engine_Title");
			RenderEng.AddMaterial(purpleTextMaterial);
			purpleTextMaterial->SetColor(hsvToRgb(Vec3f(300.0f, 1.0f, 1.0f)));

			engineTitleTextComp.SetMaterialInst(MaterialInstance(*purpleTextMaterial));
		}

		mainMenuScene.CreateActorAtRoot<UIButtonActor>("NewProjectButton", "New project", [this, &mainMenuScene]() {
			UIWindowActor& window = mainMenuScene.CreateActorAtRoot<UIWindowActor>("ProjectCreatorWindow");
			window.SetTransform(Transform(Vec2f(0.0f), Vec2f(0.5f)));

			ProjectFilepath = "Projects/";
			window.AddField("Project folder").GetTemplates().FolderInput([this](const std::string& str) { ProjectFilepath = str + "/"; }, [this]() { return ProjectFilepath; });

			UIInputBoxActor& projectNameInputBox = window.AddField("Project name").CreateChild<UIInputBoxActor>("ProjectNameInputBox");
			//UIInputBoxActor& projectNameInputBox

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
						UIElementTemplates(mainMenuScene.CreateActorAtRoot<UIActorDefault>("Recent", Transform(Vec2f(0.0f, -0.5f), Vec2f(0.1f)))).ListSelection<std::string>(filepaths.begin(), filepaths.end(), [this](UIAutomaticListActor& listActor, std::string& filepath) { listActor.CreateChild<UIButtonActor>("RecentFilepathButton", filepath, [this, filepath]() { LoadProject(filepath); }); });
				}

				Actor& cameraActor = mainMenuScene.CreateActorAtRoot<Actor>("OrthoCameraActor");
				CameraComponent& orthoCameraComp = cameraActor.CreateComponent<CameraComponent>("OrthoCameraComp", glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -2550.0f));

				mainMenuScene.BindActiveCamera(&orthoCameraComp);
				SetActiveScene(&mainMenuScene);
	}

	void GameEngineEngineEditor::HandleEvents()
	{
		if (!ActiveScene)
		{
			std::cerr << "ERROR! No scene was set as active. Events will not be handled by any scene.\n";
			return;
		}

		while (std::shared_ptr<Event> polledEvent = EventHolderObj.PollEvent())
		{
			if (polledEvent->GetType() == EventType::KEY_PRESSED && dynamic_cast<KeyEvent&>(*polledEvent).GetKeyCode() == Key::TAB)
			{
				std::cout << "Pressed TAB!\n";
				bool sceneChanged = false;
				if (ActiveScene == EditorScene && GetMainScene())
				{
					Controller* controller = dynamic_cast<Controller*>(GetMainScene()->FindActor("MojTestowyController"));
					if (!controller)
						controller = dynamic_cast<Controller*>(GetMainScene()->FindActor("A Controller"));
					if (controller)
					{
						ActiveScene = GetMainScene();
						PassMouseControl(controller);
						sceneChanged = true;
					}
				}
				else if (EditorScene)
				{
					ActiveScene = EditorScene;
					PassMouseControl(nullptr);
					sceneChanged = true;
				}

				if (sceneChanged)
					for (auto& it : Scenes)
						it->RootActor->HandleEventAll(Event(EventType::FOCUS_SWITCHED));
			}
			else if (polledEvent->GetType() == EventType::KEY_PRESSED && dynamic_cast<KeyEvent&>(*polledEvent).GetKeyCode() == Key::S && GetInputRetriever().IsKeyPressed(Key::LEFT_CONTROL))
			{
				SaveProject();
				std::cout << "Project " << ProjectName << " saved.\n";
			}
			if (ActiveScene)
				ActiveScene->HandleEventAll(*polledEvent);
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
		canvas.SetTransform(Transform(glm::vec2(-0.75f, 0.75f), glm::vec2(0.2f)));
		UIActorDefault* scaleActor = canvas.GetScaleActor();

		comp->GetEditorDescription(EditorDescriptionBuilder(*this, *scaleActor));

		canvas.RefreshFieldsList();

		Scenes.back()->RootActor->DebugHierarchy();

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

		UICanvasActor& canvas = editorScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Actors_Components_Canvas");
		canvas.SetTransform(Transform(glm::vec3(-0.68f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.32f, 0.5f, 1.0f)));
		canvas.SetViewScale(glm::vec2(1.0f, glm::sqrt(0.5f / 0.32f)));
		UIActorDefault* scaleActor = canvas.GetScaleActor();

		UICanvasField& componentsListField = canvas.AddField("List of components");
		UIAutomaticListActor& listActor = componentsListField.CreateChild<UIAutomaticListActor>("ListActor");
		canvas.FieldsList->SetListElementOffset(canvas.FieldsList->GetListElementCount() - 1, [&listActor, &componentsListField]() { std::cout << "List offset: " << (static_cast<glm::mat3>(componentsListField.GetTransform()->GetMatrix()) * listActor.GetListOffset()).y << '\n';  return static_cast<glm::mat3>(componentsListField.GetTransform()->GetMatrix()) * (listActor.GetListOffset()); });
		canvas.FieldsList->SetListCenterOffset(canvas.FieldsList->GetListElementCount() - 1, [&componentsListField]() -> glm::vec3 { return glm::vec3(0.0f, -componentsListField.GetTransform()->ScaleRef.y, 0.0f); });
		listActor.GetTransform()->Move(glm::vec2(0.5f, 0.0f));

		AddActorToList<Component>(editorScene, *actor->GetRoot(), listActor, canvas);

		listActor.Refresh();
		canvas.RefreshFieldsList();

		actor->GetEditorDescription(EditorDescriptionBuilder(*this, *scaleActor));

		if (canvas.FieldsList)
			canvas.FieldsList->Refresh();

		UIButtonActor& refreshButton = scaleActor->CreateChild<UIButtonActor>("GEE_E_Scene_Refresh_Button", [this, actor, &editorScene]() { this->SelectActor(actor, editorScene); });

		//	refreshButton.GetTransform()->SetPosition(glm::vec2(1.75f, 0.0f));
		refreshButton.GetTransform()->SetPosition(glm::vec2(6.0f, 0.0f));
		refreshButton.GetTransform()->SetScale(glm::vec2(0.5f));

		AtlasMaterial& refreshIconMat = *dynamic_cast<AtlasMaterial*>(RenderEng.FindMaterial("GEE_E_Refresh_Icon_Mat").get());
		refreshButton.SetMatIdle(MaterialInstance(refreshIconMat));
		refreshButton.SetMatHover(MaterialInstance(refreshIconMat, refreshIconMat.GetTextureIDInterpolatorTemplate(1.0f)));
		refreshButton.SetMatClick(MaterialInstance(refreshIconMat, refreshIconMat.GetTextureIDInterpolatorTemplate(2.0f)));



		//UIButtonActor& addActorButton = scaleActor.NowyCreateChild<UIButtonActor>("GEE_E_Scene_Add_Actor_Button", [this, actor, &refreshButton]() {  actor->NowyCreateComponent<Component>("CREATED_COMP", Transform())); refreshButton.OnClick(); });
		UIButtonActor& addActorButton = scaleActor->CreateChild<UIButtonActor>("GEE_E_Scene_Add_Actor_Button", [this, &editorScene, &refreshButton, actor, &canvas]() {
			UIWindowActor& compSelectWindow = editorScene.CreateActorAtRoot<UIWindowActor>(&canvas, "GEE_E_Component_Selection_Window");
			compSelectWindow.SetTransform(Transform(glm::vec2(0.0f, -0.5f), glm::vec2(0.3f)));

			auto addCompCreateButton = [this, &compSelectWindow, &editorScene, &refreshButton, actor](int i, std::function<Component& ()> createCompFunc) {
				UIButtonActor& compButton = compSelectWindow.CreateChild<UIButtonActor>("GEE_E_Scene_Add_Comp_Button", [this, &editorScene, &refreshButton, actor, createCompFunc]() { createCompFunc(); SelectActor(actor, editorScene); });

				compButton.GetTransform()->SetPosition(glm::vec2(-0.7f + static_cast<float>(i) * 0.2f, 0.7f));
				compButton.GetTransform()->SetScale(glm::vec2(0.1f, 0.1f));

				Component& exampleComponent = createCompFunc();

				compButton.SetMatIdle(exampleComponent.GetDebugMatInst(EditorIconState::IDLE));
				compButton.SetMatHover(exampleComponent.GetDebugMatInst(EditorIconState::HOVER));
				compButton.SetMatClick(exampleComponent.GetDebugMatInst(EditorIconState::BEING_CLICKED_INSIDE));

				actor->GetRoot()->DetachChild(exampleComponent);
			};

			std::function<Component& ()> getSelectedComp = [this, actor]() -> Component& { return ((SelectedComp) ? (*SelectedComp) : (*actor->GetRoot())); };

			addCompCreateButton(0, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent<Component>("A Component", Transform()); });
			addCompCreateButton(1, [this, &editorScene, getSelectedComp]() -> Component& { LightComponent& comp = getSelectedComp().CreateComponent<LightComponent>("A LightComponent", POINT, GetMainScene()->GetRenderData()->GetAvailableLightIndex(), GetMainScene()->GetRenderData()->GetAvailableLightIndex()); comp.CalculateLightRadius(); return comp;  });
			addCompCreateButton(2, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent<SoundSourceComponent>("A SoundSourceComponent"); });
			addCompCreateButton(3, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent<CameraComponent>("A CameraComponent"); });
			addCompCreateButton(4, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent<TextComponent>("A TextComponent", Transform(), "", ""); });
			addCompCreateButton(5, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent<LightProbeComponent>("A LightProbeComponent"); });

			});

		addActorButton.GetTransform()->SetPosition(glm::vec2(8.0f, 0.0f));
		addActorButton.GetTransform()->SetScale(glm::vec2(0.5f));

		AtlasMaterial& addIconMat = *dynamic_cast<AtlasMaterial*>(GetRenderEngineHandle()->FindMaterial("GEE_E_Add_Icon_Mat").get());
		addIconMat.AddTexture(std::make_shared<NamedTexture>(textureFromFile("EditorAssets/add_icon.png", GL_RGB, GL_LINEAR, GL_NEAREST_MIPMAP_LINEAR), "albedo1"));
		addActorButton.SetMatIdle(MaterialInstance(addIconMat));
		addActorButton.SetMatHover(MaterialInstance(addIconMat, addIconMat.GetTextureIDInterpolatorTemplate(1.0f)));
		addActorButton.SetMatClick(MaterialInstance(addIconMat, addIconMat.GetTextureIDInterpolatorTemplate(2.0f)));

		if (!previousCanvasView.IsEmpty() && sameActor)
			canvas.SetCanvasView(previousCanvasView);
		else
		{
			canvas.AutoClampView();
			canvas.SetViewScale(static_cast<Vec2f>(canvas.CanvasView.ScaleRef) * glm::vec2(1.0f, glm::sqrt(0.5f / 0.32f)));
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

		UICanvasActor& canvas = editorScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Scene_Actors_Canvas");
		canvas.SetTransform(Transform(glm::vec2(0.68f, 0.1f), glm::vec2(0.272f, 0.272f)));
		if (!previousCanvasView.IsEmpty() && sameScene)
			canvas.CanvasView = previousCanvasView;
		//std::shared_ptr<Actor> header = std::make_shared<Actor>(EditorScene.get(), "SelectedSceneHeaderActor");

		UIButtonActor& refreshButton = canvas.CreateChild<UIButtonActor>("GEE_E_Scene_Refresh_Button", [this, selectedScene, &editorScene]() { this->SelectScene(selectedScene, editorScene); });

		refreshButton.GetTransform()->SetPosition(glm::vec2(6.0f, 0.0f));
		refreshButton.GetTransform()->SetScale(glm::vec2(0.5f));

		AtlasMaterial& refreshIconMat = *dynamic_cast<AtlasMaterial*>(RenderEng.FindMaterial("GEE_E_Refresh_Icon_Mat").get());
		refreshButton.SetMatIdle(MaterialInstance(refreshIconMat));
		refreshButton.SetMatHover(MaterialInstance(refreshIconMat, refreshIconMat.GetTextureIDInterpolatorTemplate(1.0f)));
		refreshButton.SetMatClick(MaterialInstance(refreshIconMat, refreshIconMat.GetTextureIDInterpolatorTemplate(2.0f)));



		UIButtonActor& addActorButton = canvas.CreateChild<UIButtonActor>("GEE_E_Scene_Add_Actor_Button", [this, &editorScene, &refreshButton, selectedScene, &canvas]() {
			UIWindowActor& actorSelectWindow = canvas.CreateChildCanvas<UIWindowActor>("GEE_E_Actor_Selection_Window");
			actorSelectWindow.SetTransform(Transform(glm::vec2(0.0f, -0.5f), glm::vec2(0.3f)));

			auto addActorCreateButton = [this, &actorSelectWindow, &editorScene, &refreshButton, selectedScene](int i, std::function<Actor& ()> createActorFunc, const std::string& className) {
				UIButtonActor& actorButton = actorSelectWindow.CreateChild<UIButtonActor>("GEE_E_Scene_Add_Comp_Button", className, [this, &editorScene, &refreshButton, &actorSelectWindow, selectedScene, createActorFunc]() { Actor& createdActor = createActorFunc(); actorSelectWindow.MarkAsKilled();  std::cout << "CLICKED " << createdActor.GetName() << '\n'; SelectScene(selectedScene, editorScene); SelectActor(&createdActor, editorScene); });

				actorButton.GetTransform()->SetPosition(glm::vec2(-0.7f, 0.7f - static_cast<float>(i) * 0.2f));
				actorButton.GetTransform()->SetScale(glm::vec2(0.1f, 0.1f));
			};

			std::function<Actor& ()> getSelectedActor = [this, selectedScene]() -> Actor& { return ((SelectedActor) ? (*SelectedActor) : (const_cast<Actor&>(*selectedScene->GetRootActor()))); };

			addActorCreateButton(0, [this, &editorScene, &selectedScene, getSelectedActor]() -> Actor& { return getSelectedActor().CreateChild<Actor>(selectedScene->GetUniqueActorName("An Actor")); }, "Actor");
			addActorCreateButton(1, [this, &editorScene, &selectedScene, getSelectedActor]() -> GunActor& { return getSelectedActor().CreateChild<GunActor>(selectedScene->GetUniqueActorName("A GunActor")); }, "GunActor");
			addActorCreateButton(2, [this, &editorScene, &selectedScene, getSelectedActor]() -> PawnActor& { return getSelectedActor().CreateChild<PawnActor>(selectedScene->GetUniqueActorName("A PawnActor")); }, "PawnActor");
			addActorCreateButton(3, [this, &editorScene, &selectedScene, getSelectedActor]() -> Controller& { return getSelectedActor().CreateChild<Controller>(selectedScene->GetUniqueActorName("A Controller")); }, "Controller");
			});

		addActorButton.GetTransform()->SetPosition(glm::vec2(8.0f, 0.0f));
		addActorButton.GetTransform()->SetScale(glm::vec2(0.5f));

		AtlasMaterial& addActorMat = *new AtlasMaterial(Material("GEE_E_Add_Actor_Material", 0.0f, RenderEng.FindShader("Forward_NoLight")), glm::ivec2(3, 1));
		addActorMat.AddTexture(std::make_shared<NamedTexture>(textureFromFile("EditorAssets/add_icon.png", GL_RGB, GL_LINEAR, GL_NEAREST_MIPMAP_LINEAR), "albedo1"));
		addActorButton.SetMatIdle(MaterialInstance(addActorMat));
		addActorButton.SetMatHover(MaterialInstance(addActorMat, addActorMat.GetTextureIDInterpolatorTemplate(1.0f)));
		addActorButton.SetMatClick(MaterialInstance(addActorMat, addActorMat.GetTextureIDInterpolatorTemplate(2.0f)));

		UIAutomaticListActor& listActor = canvas.CreateChild<UIAutomaticListActor>("ListActor");
		//listActor.GetTransform()->SetScale(glm::vec3(0.5f));

		//refreshButton.NowyCreateComponent<TextComponent>("GEE_E_Scene_Refresh_Text", Transform(glm::vec2(0.0f), glm::vec3(0.0f), glm::vec2(1.0f / 8.0f, 1.0f)), "Refresh", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));

		AddActorToList<Actor>(editorScene, const_cast<Actor&>(*selectedScene->GetRootActor()), listActor, canvas);

		listActor.Refresh();

		const_cast<Actor*>(EditorScene->GetRootActor())->DebugActorHierarchy();

		canvas.AutoClampView();
		canvas.SetViewScale(static_cast<Vec2f>(canvas.CanvasView.ScaleRef) * glm::vec2(1.0f, glm::sqrt(0.32f / 0.32f)));
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
		previewWindow.SetTransform(Transform(glm::vec2(0.0f, -0.5f), glm::vec2(0.4f)));

		UICanvasActor& nodesCanvas = previewWindow.CreateChild<UICanvasActor>("Nodes canvas");
		nodesCanvas.SetTransform(Transform(glm::vec2(-0.2f, 0.2f), glm::vec2(0.8f, 0.8f)));

		UIButtonActor& instantiateButton = previewWindow.CreateChild<UIButtonActor>("InstantiateButton", "Instantiate", [this, &tree, &editorScene]() {
			std::function<Component* ()> getSelectedComp = [this]() -> Component* { return ((SelectedComp) ? (SelectedComp) : ((SelectedActor) ? (SelectedActor->GetRoot()) : (nullptr))); };

			if (Component* selectedComp = getSelectedComp())
				EngineDataLoader::InstantiateTree(*selectedComp, tree);

			SelectActor(SelectedActor, editorScene);
			});
		instantiateButton.SetTransform(Transform(glm::vec2(0.875f, 0.8f), glm::vec2(0.1f)));

		previewWindow.HideScrollBars(); //for now

		std::function<void(UIAutomaticListActor&, const HierarchyTemplate::HierarchyNodeBase&, int)> nodeCreatorFunc = [&editorScene, &nodesCanvas, &nodeCreatorFunc](UIAutomaticListActor& parent, const HierarchyTemplate::HierarchyNodeBase& correspondingNode, int level) {


			UIButtonActor& nodeButton = parent.CreateChild<UIButtonActor>("OGAR");//correspondingNode.GetName()));
			TextConstantSizeComponent& nodeText = nodeButton.CreateComponent<TextConstantSizeComponent>("TEXTTEXT", Transform(), correspondingNode.GetCompBaseType().GetName(), "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
			nodeButton.SetTransform(Transform(glm::vec2(0.0f, -3.0f), glm::vec2(1.0f)));

			//std::cout << "NODE: " << correspondingNode.GetName() << ". CHILDREN COUNT: " << correspondingNode.GetChildCount() << '\n';

			for (int i = 0; i < correspondingNode.GetChildCount(); i++)
			{
				UIAutomaticListActor& nodeChildrenList = parent.CreateChild<UIAutomaticListActor>(correspondingNode.GetCompBaseType().GetName() + "children list", glm::vec3(3.0f, 0.0f, 0.0f));
				if (i == 0)
					nodeChildrenList.GetTransform()->Move(((float)correspondingNode.GetChildCount() - 1.0f) * -glm::vec3(3.0f, 0.0f, 0.0f) / 2.0f);
				nodeCreatorFunc(nodeChildrenList, *correspondingNode.GetChild(i), level + 1);
			}

			//nodeChildrenList.GetTransform()->Move(((float)correspondingNode.GetChildCount() - 1.0f) * -glm::vec3(10.0f, 0.0f, 0.0f) / 2.0f);
		};

		UIAutomaticListActor& rootList = previewWindow.CreateChild<UIAutomaticListActor>("Root children list", glm::vec3(3.0f, 0.0f, 0.0f));
		rootList.GetTransform()->SetScale(glm::vec2(0.1f));

		nodeCreatorFunc(rootList, tree.GetRoot(), 0);
		rootList.Refresh();

		std::function<void(Actor&)> func = [&previewWindow, &func](Actor& actor) {
			if (TextComponent* text = actor.GetRoot()->GetComponent<TextComponent>("TEXTTEXT"))
			{
				std::string str1 = std::to_string(previewWindow.ToCanvasSpace(actor.GetTransform()->GetWorldTransform()).PositionRef.x);
				str1 = str1.erase(str1.find_last_not_of('0') + 1, std::string::npos);
				if (str1.back() == '.')
					str1 = str1.substr(0, str1.length() - 1);

				std::string str2 = std::to_string(previewWindow.ToCanvasSpace(actor.GetTransform()->GetWorldTransform()).PositionRef.y);
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
			animationButton.SetTransform(Transform(glm::vec2(-0.8f + static_cast<float>(i) * 0.3f, -0.8f), glm::vec2(0.1f)));
		}

		previewWindow.DebugActorHierarchy(0);
	}

	void GameEngineEngineEditor::Render()
	{
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
			RenderEng.PrepareScene(*ViewportRenderCollection, GetMainScene()->GetRenderData());
			RenderEng.FullSceneRender(GetMainScene()->ActiveCamera->GetRenderInfo(*ViewportRenderCollection), GetMainScene()->GetRenderData(), &ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer());// , Viewport(glm::vec2(0.0f), Settings->Video.Resolution)/*Viewport((static_cast<glm::vec2>(Settings->WindowSize) - Settings->Video.Resolution) / glm::vec2(2.0f, 1.0f), glm::vec2(Settings->Video.Resolution.x, Settings->Video.Resolution.y))*/);
			RenderEng.FindShader("Forward_NoLight")->Use();
			GetMainScene()->GetRootActor()->DebugRenderAll(GetMainScene()->ActiveCamera->GetRenderInfo(*ViewportRenderCollection), RenderEng.FindShader("Forward_NoLight"));

			if (SelectedActor && !GetInputRetriever().IsKeyPressed(Key::P))
			{
				std::function<void(Component&)> renderAllColObjs = [this, &renderAllColObjs](Component& comp) {
					if (comp.GetCollisionObj() && comp.GetName() != "Counter")
						CollisionObjRendering(GetMainScene()->ActiveCamera->GetRenderInfo(*ViewportRenderCollection), *this, *comp.GetCollisionObj(), comp.GetTransform().GetWorldTransform(), (SelectedComp && SelectedComp == &comp) ? (glm::vec3(0.1f, 0.6f, 0.3f)) : (glm::vec3(0.063f, 0.325f, 0.071f)));

					for (auto& child : comp.GetChildren())
						renderAllColObjs(*child);
				};

				renderAllColObjs(*SelectedActor->GetRoot());
			}

			RenderEng.RenderText(RenderInfo(*ViewportRenderCollection), *GetDefaultFont(), "(C) twoja babka studios", Transform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(32.0f)), glm::pow(glm::vec3(0.0f, 0.73f, 0.84f), glm::vec3(1.0f / 2.2f)), nullptr, true);
		}
		else
		{
			ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().Bind();
			RenderEng.FindShader("Forward_NoLight")->Use();
			RenderEng.RenderStaticMesh(RenderInfo(*ViewportRenderCollection), MeshInstance(RenderEng.GetBasicShapeMesh(EngineBasicShape::QUAD), RenderEng.FindMaterial("GEE_No_Camera_Icon").get()), Transform(), RenderEng.FindShader("Forward_NoLight"));

		}

		bool renderEditorScene = EditorScene && EditorScene->ActiveCamera;
		if (renderEditorScene)
		{
			///Render editor scene
			RenderInfo info = EditorScene->ActiveCamera->GetRenderInfo(*HUDRenderCollection);
			RenderEng.PrepareScene(*HUDRenderCollection, EditorScene->GetRenderData());
			RenderEng.FullSceneRender(info, EditorScene->GetRenderData(), nullptr, Viewport(glm::vec2(0.0f), glm::vec2(Settings->WindowSize)), true, true);
		}

		if (GameScene* mainMenuScene = GetScene("GEE_Main_Menu"))
			if (mainMenuScene->GetActiveCamera())
			{
				RenderInfo info = mainMenuScene->ActiveCamera->GetRenderInfo(*HUDRenderCollection);
				RenderEng.PrepareScene(*HUDRenderCollection, mainMenuScene->GetRenderData());
				RenderEng.FullSceneRender(info, mainMenuScene->GetRenderData(), nullptr, Viewport(glm::vec2(0.0f), glm::vec2(Settings->WindowSize)), !renderEditorScene, true);
			}

		if (GameScene* meshPreviewScene = GetScene("GEE_Mesh_Preview_Scene"))
			if (meshPreviewScene->GetActiveCamera())
			{
				RenderToolboxCollection& renderTbCollection = **std::find_if(RenderEng.RenderTbCollections.begin(), RenderEng.RenderTbCollections.end(), [](const std::unique_ptr<RenderToolboxCollection>& tbCol) { return tbCol->GetName() == "GEE_E_Mesh_Preview_Toolbox_Collection"; });
				RenderInfo info = meshPreviewScene->ActiveCamera->GetRenderInfo(renderTbCollection);
				RenderEng.PrepareScene(renderTbCollection, meshPreviewScene->GetRenderData());
				RenderEng.FullSceneRender(info, meshPreviewScene->GetRenderData(), &renderTbCollection.GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer());
			}

		glfwSwapBuffers(Window);
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
		RenderEng.PreLoopPass();

		UpdateRecentProjects();
	}

	void GameEngineEngineEditor::SaveProject()
	{
		if (getFilepathExtension(ProjectFilepath) == ".geeprojectold")
			ProjectFilepath = ProjectFilepath.substr(0, ProjectFilepath.find(".geeprojectold")) + ".json";
		std::cout << "Saving to path " << ProjectFilepath << "\n";

		std::ofstream serializationStream(ProjectFilepath);
		{
			try
			{
				cereal::JSONOutputArchive archive(serializationStream);
				GetMainScene()->GetRenderData()->SaveSkeletonBatches(archive);
				const_cast<Actor*>(GetMainScene()->GetRootActor())->Save(archive);
				archive.serializeDeferments();
			}
			catch (cereal::Exception& ex)
			{
				std::cout << "ERROR While saving scene: " << ex.what() << '\n';
			}
		}
		serializationStream.close();

		UpdateRecentProjects();
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