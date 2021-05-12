#define STB_IMAGE_IMPLEMENTATION
#define CEREAL_LOAD_FUNCTION_NAME Load
#define CEREAL_SAVE_FUNCTION_NAME Save
#define CEREAL_SERIALIZE_FUNCTION_NAME Serialize
#include <game/Game.h>
#include <scene/TextComponent.h>
#include <scene/CameraComponent.h>
#include <assetload/FileLoader.h>
#include <UI/UICanvasActor.h>
#include <scene/UIInputBoxActor.h>
#include <scene/UIWindowActor.h>
#include <scene/ModelComponent.h>
#include <scene/LightComponent.h>
#include <scene/LightProbeComponent.h>
#include <UI/UICanvasField.h>
#include <UI/UIListActor.h>
#include <scene/GunActor.h>
#include <scene/PawnActor.h>
#include <scene/Controller.h>
#include <input/InputDevicesStateRetriever.h>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/archives/json.hpp>
#include <map>

//Po 11 minutach i 7 sekundach crashuje przez C:\Users\48511\Desktop\PhysX-4.1\physx\source\foundation\include\PsBroadcast.h (199) : abort : User allocator returned NULL. Czemu?
//ODPOWIEDZ MEMORY LEAKS!!!!! we leak about 3 MB/s

extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}

extern "C"
{
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

struct EditorEventProcessor
{
	static void FileDropCallback(GLFWwindow* window, int count, const char** paths)
	{
		if (count > 0)
		{
			std::cout << "Dropped " << paths[0] << '\n';
			EditorHandle->PreviewHierarchyTree(*EngineDataLoader::LoadHierarchyTree(*EditorHandle->GetGameHandle()->GetMainScene(), paths[0]));
		}
	}
	static void Resize(GLFWwindow* window, int width, int height)
	{
		EditorHandle->GetGameHandle()->GetGameSettings()->WindowSize = glm::uvec2(width, height);
		EditorHandle->UpdateSettings();
	}

	static EditorManager* EditorHandle;
};
EditorManager* EditorEventProcessor::EditorHandle = nullptr;
class GameEngineEngineEditor : public Game, public EditorManager
{
public:
	GameEngineEngineEditor(GLFWwindow* window, const GameSettings& settings) :
		Game(settings.Video.Shading, settings),
		EditorScene(nullptr),
		ActiveScene(nullptr),
		SelectedComp(nullptr),
		SelectedActor(nullptr),
		SelectedScene(nullptr),
		CanvasContext(nullptr)
	{
		Settings = std::make_unique<GameSettings>(GameSettings(EngineDataLoader::LoadSettingsFromFile<GameSettings>("Settings.ini")));
		glm::vec2 res = static_cast<glm::vec2>(Settings->WindowSize);
		//Settings->ViewportData = glm::uvec4(res.x * 0.3f, res.y * 0.4f, res.x * 0.4, res.y * 0.6f);
		Settings->Video.Resolution = glm::vec2(res.x * 0.4f, res.y * 0.6f);
		Settings->Video.Shading = ShadingModel::SHADING_PBR_COOK_TORRANCE;
		Init(window);
	} 
	virtual GameManager* GetGameHandle() override
	{
		return this;
	}
	virtual GameScene* GetSelectedScene() override
	{
		return SelectedScene;
	}
	virtual std::vector<GameScene*> GetScenes() override
	{
		std::vector<GameScene*> scenes(Scenes.size());
		std::transform(Scenes.begin(), Scenes.end(), scenes.begin(), [](std::unique_ptr<GameScene>& sceneVec) {return sceneVec.get(); });
		return scenes;
	}
	virtual void Init(GLFWwindow* window) override
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
	virtual void UpdateSettings() override
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
	void SetupEditorScene()
	{
		GameScene& editorScene = CreateScene("GEE_Editor");

		//settings.ViewportData = glm::uvec4(res.x * 0.3f, res.y * 0.4f, res.x * 0.4, res.y * 0.6f);

		Actor& scenePreviewActor = editorScene.CreateActorAtRoot<Actor>("SceneViewportActor");
		ModelComponent& scenePreviewQuad = scenePreviewActor.CreateComponent<ModelComponent>("SceneViewportQuad");
		 
		/*UICanvasActor& exampleCanvas = editorScene.CreateActorAtRoot<UICanvasActor>("ExampleCanvas");
		exampleCanvas.SetTransform(Transform(glm::vec3(0.7f, -0.4f, 0.0f), glm::vec3(0.0f), glm::vec3(0.3f)));
		UIButtonActor& lolBackgroundButton = exampleCanvas.CreateChild<UIButtonActor>("VeryImportantButton");
		lolBackgroundButton.SetTransform(Transform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f)));

		TextComponent& sampleText = exampleCanvas.CreateComponent<TextComponent>("SampleTextComp", Transform(glm::vec3(0.9f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f)), "Sample Text");
		exampleCanvas.AddUIElement(sampleText);
		sampleText.SetAlignment(TextAlignment::CENTER, TextAlignment::CENTER);

		TextComponent& sampleText2 = exampleCanvas.CreateComponent<TextComponent>("SampleTextComp", Transform(glm::vec3(1.9f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f)), "Very important text");
		exampleCanvas.AddUIElement(sampleText2);

		TextComponent& sampleText3 = exampleCanvas.CreateComponent<TextComponent>("SampleTextComp", Transform(glm::vec3(1.9f, 1.5f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f)), "Mega ultra important text");
		exampleCanvas.AddUIElement(sampleText3);

		for (int y = -3; y <= 3; y++)
		{
			for (int x = -3; x <= 3; x++)
			{
				TextComponent& gridSampleText = exampleCanvas.CreateComponent<TextComponent>("SampleTextComp", Transform(glm::vec3(x, y, 0.0f), glm::vec3(0.0f), glm::vec3(0.05f)), "Sample Text " + std::to_string(x) + "|||" + std::to_string(y));
				exampleCanvas.AddUIElement(gridSampleText);
				gridSampleText.SetAlignment(TextAlignment::RIGHT, TextAlignment::TOP);
			}
		}*/

		UIWindowActor& window1 = editorScene.CreateActorAtRoot<UIWindowActor>("MyTestWindow");
		window1.SetTransform(Transform(glm::vec2(0.0, -0.5f), glm::vec2(0.2f)));

		UIButtonActor& bigButtonActor = window1.CreateChild<UIButtonActor>("MojTestowyButton");
		bigButtonActor.SetTransform(Transform(glm::vec2(0.0f, 1.5f), glm::vec2(0.4f)));
		TextComponent& bigButtonText = bigButtonActor.CreateComponent<TextComponent>("BigButtonText", Transform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f)), "big button", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
		window1.AddUIElement(bigButtonActor);

		editorScene.CreateActorAtRoot<UIButtonActor>("MeshTreesButton", "MeshTrees", [this, &editorScene]() {
			UIWindowActor& window = editorScene.CreateActorAtRoot<UIWindowActor>("MeshTreesWindow");
			window.GetTransform()->SetScale(glm::vec2(0.5f));

			UIAutomaticListActor& meshTreesList = window.CreateChild<UIAutomaticListActor>("MeshTreesList");
			for (auto& scene : Scenes)
				for (auto& meshTree : scene->HierarchyTrees)
					meshTreesList.CreateChild<UIButtonActor>("MeshTreeButton", meshTree->GetName(), [this, &meshTree]() { PreviewHierarchyTree(*meshTree); });

			window.AddUIElement(meshTreesList);
			meshTreesList.Refresh();

			}).SetTransform(Transform(Vec2f(-0.8f, -0.8f), Vec2f(0.1f)));

		editorScene.CreateActorAtRoot<UIButtonActor>("MaterialsButton", "Materials", [this, &editorScene]() {
			UIWindowActor& window = editorScene.CreateActorAtRoot<UIWindowActor>("MaterialsPreviewWindow");
			window.GetTransform()->SetScale(glm::vec2(0.5f));
			UIAutomaticListActor& list = window.CreateChild<UIAutomaticListActor>("MaterialsList");

			std::cout << "Nr of materials: " << RenderEng.Materials.size() << '\n';
			for (auto& it : RenderEng.Materials)
			{
				std::string name = (it) ? (it->GetLocalization().GetFullStr()) : ("NULLPTR???");
				UIButtonActor& button = list.CreateChild<UIButtonActor>(name + "Button", name);
				window.AddUIElement(button);
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

			window.AddUIElement(*window.GetScaleActor());
			window.FieldsList->Refresh();
			window.AutoClampView();
			
		}).SetTransform(Transform(Vec2f(-0.2f, -0.8f), Vec2f(0.1f)));

		editorScene.CreateActorAtRoot<UIButtonActor>("ShadersButton", "Shaders", [this, &editorScene]() {
			UIWindowActor& window = editorScene.CreateActorAtRoot<UIWindowActor>("ShadersPreviewWindow");
			window.GetTransform()->SetScale(glm::vec2(0.5f));

			UIButtonActor& addShaderButton = window.CreateChild<UIButtonActor>("AddShaderButton");

			AtlasMaterial& addIconMat = *dynamic_cast<AtlasMaterial*>(GetRenderEngineHandle()->FindMaterial("GEE_E_Add_Icon_Mat").get());
			addShaderButton.SetMatIdle(MaterialInstance(addIconMat, addIconMat.GetTextureIDInterpolatorTemplate(0.0f)));
			addShaderButton.SetMatHover(MaterialInstance(addIconMat, addIconMat.GetTextureIDInterpolatorTemplate(1.0f)));
			addShaderButton.SetMatClick(MaterialInstance(addIconMat, addIconMat.GetTextureIDInterpolatorTemplate(2.0f)));

			window.AddUIElement(addShaderButton);

			}).SetTransform(Transform(Vec2f(0.1f, -0.8f), Vec2f(0.1f)));


		Actor& cameraActor = editorScene.CreateActorAtRoot<Actor>("OrthoCameraActor");
		CameraComponent& orthoCameraComp = cameraActor.CreateComponent<CameraComponent>("OrthoCameraComp");

		editorScene.BindActiveCamera(&orthoCameraComp);


		EditorScene = &editorScene;

		EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->AddMeshInst(GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
		EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->SetTransform(Transform(glm::vec2(0.0f, 0.4f), glm::vec2(0.4f, 0.6f)));

		std::shared_ptr<Material> scenePreviewMaterial = std::make_shared<Material>("GEE_3D_SCENE_PREVIEW_MATERIAL", 0.0f, GetGameHandle()->GetRenderEngineHandle()->FindShader("Forward_NoLight"));
		RenderEng.AddMaterial(scenePreviewMaterial);
		scenePreviewMaterial->AddTexture(std::make_shared<NamedTexture>(*ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorBuffer(0), "albedo1"));
		EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->OverrideInstancesMaterial(scenePreviewMaterial.get());
	}
	void SetCanvasContext(UICanvasActor* context)
	{
		CanvasContext = context;
		if (ActiveScene)
			ActiveScene->GetRootActor()->HandleEventAll(Event(EventType::FOCUS_SWITCHED));
	}
	void SetupMainMenu()
	{
		GameScene& mainMenuScene = CreateScene("GEE_Main_Menu");

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

				window.AddField("Project folder").GetTemplates().FolderInput([this](const std::string& str) { ProjectFileDirectory = str + "/"; }, [this]() { return ProjectFileDirectory; });

				UIInputBoxActor& projectNameInputBox = window.AddField("Project name").CreateChild<UIInputBoxActor>("ProjectNameInputBox");
				projectNameInputBox.SetOnInputFunc([this](const std::string& input) { ProjectName = input; }, [this]() { return ProjectName; });
				
				UIButtonActor& okButton = window.AddField("").CreateChild<UIButtonActor>("OKButton", "OK", [this, &window]() { window.MarkAsKilled(); LoadProject(ProjectFileDirectory + ProjectName); });

				window.AddUIElement(*window.GetScaleActor());
				window.RefreshFieldsList();
				window.AutoClampView();

				SetCanvasContext(&window);
			}).SetTransform(Transform(Vec2f(-0.3f, 0.0f), Vec2f(0.2f)));

		mainMenuScene.CreateActorAtRoot<UIButtonActor>("LoadProjectButton", "Load project", [this, &mainMenuScene]() {
			UIWindowActor& window = mainMenuScene.CreateActorAtRoot<UIWindowActor>("ProjectLoaderWindow");
			window.SetTransform(Transform(Vec2f(0.0f), Vec2f(0.5f)));

			window.AddField("Project filepath").GetTemplates().PathInput([this](const std::string& filepath) { extractDirectoryAndFilename(filepath, ProjectName, ProjectFileDirectory); }, [this]() { return ProjectFileDirectory + ProjectName; }, { "*.json", "*.geeproject", "*.geeprojectold" });

			UIButtonActor& okButton = window.AddField("").CreateChild<UIButtonActor>("OKButton", "OK", [this, &window]() { window.MarkAsKilled(); LoadProject(ProjectFileDirectory + ProjectName); });

			window.AddUIElement(*window.GetScaleActor());
			window.RefreshFieldsList();
			window.AutoClampView();

			SetCanvasContext(&window);
			}).SetTransform(Transform(Vec2f(0.3f, 0.0f), Vec2f(0.2f)));

		Actor& cameraActor = mainMenuScene.CreateActorAtRoot<Actor>("OrthoCameraActor");
		CameraComponent& orthoCameraComp = cameraActor.CreateComponent<CameraComponent>("OrthoCameraComp");

		mainMenuScene.BindActiveCamera(&orthoCameraComp);
		SetActiveScene(&mainMenuScene);
	}
	virtual void HandleEvents() override
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

			if (ActiveScene)
				if (CanvasContext && ActiveScene == GetScene("GEE_Main_Menu"))
					CanvasContext->HandleEventAll(*polledEvent);
				else
					ActiveScene->RootActor->HandleEventAll(*polledEvent);
		}
		if (SelectedComp)
			;//SelectedComp->GetTransform().SetRotation(glm::normalize(EditedRotation));
	}
	virtual void SelectComponent(Component* comp, GameScene& editorScene) override
	{
		if (comp && comp->IsBeingKilled())	//Components and Actors that are being killed should never be chosen - doing it is a perfect opportunity for runtime crashes to occur.
			comp = nullptr;


		bool sameActor = SelectedComp == comp;
		SelectedComp = comp;
		Transform previousCanvasView;
		if (const Actor* found = editorScene.GetRootActor()->FindActor("GEE_E_Components_Info_Canvas"))
		{
			previousCanvasView = dynamic_cast<UICanvasActor*>(const_cast<Actor*>(found))->CanvasView;
			const_cast<Actor*>(found)->MarkAsKilled();
		}

		if (!comp)
			return;

		UICanvasActor& selectCompCanvas = editorScene.CreateActorAtRoot<UICanvasActor>("GEE_E_Components_Info_Canvas");
		selectCompCanvas.SetTransform(Transform(glm::vec2(-0.75f, 0.75f), glm::vec2(0.2f)));
		UIActor* scaleActor = selectCompCanvas.GetScaleActor();

		comp->GetEditorDescription(EditorDescriptionBuilder(*this, *scaleActor));
		for (auto& it : scaleActor->GetChildren())
			selectCompCanvas.AddUIElement(*it);	//add all created actors/components to the UI
		
		selectCompCanvas.RefreshFieldsList();

		Scenes.back()->RootActor->DebugHierarchy();

		if (!previousCanvasView.IsEmpty() && sameActor)
			selectCompCanvas.CanvasView = previousCanvasView;
		else
			selectCompCanvas.AutoClampView();
	}

	virtual void SelectActor(Actor* actor, GameScene& editorScene) override
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
		UIActor* scaleActor = canvas.GetScaleActor();

		UICanvasField& componentsListField = canvas.AddField("List of components");
		UIAutomaticListActor& listActor = componentsListField.CreateChild<UIAutomaticListActor>("ListActor");
		canvas.FieldsList->SetListElementOffset(canvas.FieldsList->GetListElementCount() - 1, [&listActor, &componentsListField]() { std::cout << "List offset: " << (static_cast<glm::mat3>(componentsListField.GetTransform()->GetMatrix()) * listActor.GetListOffset()).y << '\n';  return static_cast<glm::mat3>(componentsListField.GetTransform()->GetMatrix()) * (listActor.GetListOffset()); });
		canvas.FieldsList->SetListCenterOffset(canvas.FieldsList->GetListElementCount() - 1, [&componentsListField]() -> glm::vec3 { return glm::vec3(0.0f, -componentsListField.GetTransform()->ScaleRef.y, 0.0f); });
		listActor.GetTransform()->Move(glm::vec2(0.5f, 0.0f));

		AddActorToList<Component>(editorScene, *actor->GetRoot(), listActor, canvas);

		listActor.Refresh();
		canvas.RefreshFieldsList();

		actor->GetEditorDescription(EditorDescriptionBuilder(*this, *scaleActor));
		for (auto& it : scaleActor->GetChildren())	//add all created actors/components to the UI
			if (UIActor* cast = dynamic_cast<UIActor*>(it))
				canvas.AddUIElement(*cast);
			else
				canvas.AddUIElement(*it);

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

		canvas.AddUIElement(refreshButton);



		//UIButtonActor& addActorButton = scaleActor.NowyCreateChild<UIButtonActor>("GEE_E_Scene_Add_Actor_Button", [this, actor, &refreshButton]() {  actor->NowyCreateComponent<Component>("CREATED_COMP", Transform())); refreshButton.OnClick(); });
		UIButtonActor& addActorButton = scaleActor->CreateChild<UIButtonActor>("GEE_E_Scene_Add_Actor_Button", [this, &editorScene, &refreshButton, actor]() {
			UIWindowActor& compSelectWindow = editorScene.CreateActorAtRoot<UIWindowActor>("Component_Selection_Window");
			compSelectWindow.SetTransform(Transform(glm::vec2(0.0f, -0.5f), glm::vec2(0.3f)));

			auto addCompCreateButton = [this, &compSelectWindow, &editorScene, &refreshButton, actor](int i, std::function<Component& ()> createCompFunc) {
				UIButtonActor& compButton = compSelectWindow.CreateChild<UIButtonActor>("GEE_E_Scene_Add_Comp_Button", [this, &editorScene, &refreshButton, actor, createCompFunc]() { createCompFunc(); SelectActor(actor, editorScene); });

				compButton.GetTransform()->SetPosition(glm::vec2(-0.7f + static_cast<float>(i) * 0.2f, 0.7f));
				compButton.GetTransform()->SetScale(glm::vec2(0.1f, 0.1f));

				Component& exampleComponent = createCompFunc();

				compButton.SetMatIdle(exampleComponent.GetDebugMatInst(EditorIconState::IDLE));
				compButton.SetMatHover(exampleComponent.GetDebugMatInst(EditorIconState::HOVER));
				compButton.SetMatClick(exampleComponent.GetDebugMatInst(EditorIconState::BEING_CLICKED_INSIDE));

				compSelectWindow.AddUIElement(compButton);
				actor->GetRoot()->DetachChild(exampleComponent);
			};

			std::function<Component&()> getSelectedComp = [this, actor]() -> Component& { return ((SelectedComp) ? (*SelectedComp) : (*actor->GetRoot())); };

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

		canvas.AddUIElement(addActorButton);

		if (!previousCanvasView.IsEmpty() && sameActor)
			canvas.CanvasView = previousCanvasView;
		else
		{
			canvas.AutoClampView();
			canvas.SetViewScale(static_cast<Vec2f>(canvas.CanvasView.ScaleRef) * glm::vec2(1.0f, glm::sqrt(0.5f / 0.32f)));
		}
	}
	virtual void SelectScene(GameScene* selectedScene, GameScene& editorScene) override
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

		canvas.AddUIElement(refreshButton);


		UIButtonActor& addActorButton = canvas.CreateChild<UIButtonActor>("GEE_E_Scene_Add_Actor_Button", [this, &editorScene, &refreshButton, selectedScene]() {
			UIWindowActor& actorSelectWindow = editorScene.CreateActorAtRoot<UIWindowActor>("Component_Selection_Window");
			actorSelectWindow.SetTransform(Transform(glm::vec2(0.0f, -0.5f), glm::vec2(0.3f)));

			auto addActorCreateButton = [this, &actorSelectWindow, &editorScene, &refreshButton, selectedScene](int i, std::function<Actor&()> createActorFunc, const std::string& className) {
				UIButtonActor& actorButton = actorSelectWindow.CreateChild<UIButtonActor>("GEE_E_Scene_Add_Comp_Button", className, [this, &editorScene, &refreshButton, &actorSelectWindow, selectedScene, createActorFunc]() { Actor& createdActor = createActorFunc(); actorSelectWindow.MarkAsKilled();  std::cout << "CLICKED " << createdActor.GetName() << '\n'; SelectScene(selectedScene, editorScene); SelectActor(&createdActor, editorScene); });

				actorButton.GetTransform()->SetPosition(glm::vec2(-0.7f, 0.7f - static_cast<float>(i) * 0.2f));
				actorButton.GetTransform()->SetScale(glm::vec2(0.1f, 0.1f));

				actorSelectWindow.AddUIElement(actorButton);
			};

			std::function<Actor& ()> getSelectedActor = [this, selectedScene]() -> Actor& { return ((SelectedActor) ? (*SelectedActor) : (const_cast<Actor&>(*selectedScene->GetRootActor()))); };

			addActorCreateButton(0, [this, &editorScene, getSelectedActor]() -> Actor& { return getSelectedActor().CreateChild<Actor>("An Actor"); }, "Actor");
			addActorCreateButton(1, [this, &editorScene, getSelectedActor]() -> GunActor& { return getSelectedActor().CreateChild<GunActor>("A GunActor"); }, "GunActor");
			addActorCreateButton(2, [this, &editorScene, getSelectedActor]() -> PawnActor& { return getSelectedActor().CreateChild<PawnActor>("A PawnActor"); }, "PawnActor");
			addActorCreateButton(3, [this, &editorScene, getSelectedActor]() -> Controller& { return getSelectedActor().CreateChild<Controller>("A Controller"); }, "Controller");
			});

		addActorButton.GetTransform()->SetPosition(glm::vec2(8.0f, 0.0f));
		addActorButton.GetTransform()->SetScale(glm::vec2(0.5f));

		AtlasMaterial& addActorMat = *new AtlasMaterial(Material("GEE_E_Add_Actor_Material", 0.0f, RenderEng.FindShader("Forward_NoLight")), glm::ivec2(3, 1));
		addActorMat.AddTexture(std::make_shared<NamedTexture>(textureFromFile("EditorAssets/add_icon.png", GL_RGB, GL_LINEAR, GL_NEAREST_MIPMAP_LINEAR), "albedo1"));
		addActorButton.SetMatIdle(MaterialInstance(addActorMat));
		addActorButton.SetMatHover(MaterialInstance(addActorMat, addActorMat.GetTextureIDInterpolatorTemplate(1.0f)));
		addActorButton.SetMatClick(MaterialInstance(addActorMat, addActorMat.GetTextureIDInterpolatorTemplate(2.0f)));

		canvas.AddUIElement(addActorButton);

		UIAutomaticListActor& listActor = canvas.CreateChild<UIAutomaticListActor>("ListActor");
		//listActor.GetTransform()->SetScale(glm::vec3(0.5f));

		//refreshButton.NowyCreateComponent<TextComponent>("GEE_E_Scene_Refresh_Text", Transform(glm::vec2(0.0f), glm::vec3(0.0f), glm::vec2(1.0f / 8.0f, 1.0f)), "Refresh", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));

		AddActorToList<Actor>(editorScene, const_cast<Actor&>(*selectedScene->GetRootActor()), listActor, canvas);

		listActor.Refresh();

		const_cast<Actor*>(EditorScene->GetRootActor())->DebugActorHierarchy();

		canvas.AutoClampView();
		canvas.SetViewScale(static_cast<Vec2f>(canvas.CanvasView.ScaleRef) * glm::vec2(1.0f, glm::sqrt(0.32f / 0.32f)));
	}
	virtual void PreviewHierarchyTree(HierarchyTemplate::HierarchyTreeT& tree) override
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

		UIButtonActor& instantiateButton = previewWindow.CreateChild<UIButtonActor>("InstantiateButton", "Instantiate", [this, &tree]() {
			std::function<Component*()> getSelectedComp = [this]() -> Component* { return ((SelectedComp) ? (SelectedComp) : ((SelectedActor) ? (SelectedActor->GetRoot()) : (nullptr))); };

			if (Component* selectedComp = getSelectedComp())
				EngineDataLoader::InstantiateTree(*selectedComp, tree);
			});
		instantiateButton.SetTransform(Transform(glm::vec2(0.875f, 0.8f), glm::vec2(0.1f)));

		previewWindow.HideScrollBars(); //for now

		std::function<void(UIAutomaticListActor&, const HierarchyTemplate::HierarchyNodeBase&, int)> nodeCreatorFunc = [&editorScene, &nodesCanvas, &nodeCreatorFunc](UIAutomaticListActor& parent, const HierarchyTemplate::HierarchyNodeBase& correspondingNode, int level) {


			UIButtonActor& nodeButton = parent.CreateChild<UIButtonActor>("OGAR");//correspondingNode.GetName()));
			TextConstantSizeComponent& nodeText = nodeButton.CreateComponent<TextConstantSizeComponent>("TEXTTEXT", Transform(), correspondingNode.GetCompBaseType().GetName(), "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
			nodeButton.SetTransform(Transform(glm::vec2(0.0f, -3.0f), glm::vec2(1.0f)));
			nodesCanvas.AddUIElement(nodeButton);

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
	template <typename T> void AddActorToList(GameScene& editorScene, T& obj, UIAutomaticListActor& listParent, UICanvas& canvas)
	{
		if (obj.IsBeingKilled())
			return;

		UIButtonActor& element = listParent.CreateChild<UIButtonActor>(obj.GetName() + "'s Button", [this, &obj, &editorScene ]() {this->Select(&obj, editorScene); });//*this, obj, &EditorManager::Select<T>));
		element.SetTransform(Transform(glm::vec2(1.5f, 0.0f), glm::vec2(3.0f, 1.0f)));
		TextConstantSizeComponent& elementText = element.CreateComponent<TextConstantSizeComponent>(obj.GetName() + "'s Text", Transform(glm::vec2(0.0f), glm::vec2(0.4f / 3.0f, 0.4f)), "", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
		elementText.SetMaxSize(Vec2f(0.9f));
		elementText.SetContent(obj.GetName());
		canvas.AddUIElement(element);

		std::vector<T*> children = obj.GetChildren();
		std::cout << "liczba dzieciakow of " << obj.GetName() << ": " << children.size() << '\n';
		for (auto& child : children)
		{
			//AddActorToList(editorScene, *child, listParent.CreateChild(UIListActor(editorScene, "NextLevel")), canvas);
			UIAutomaticListActor& nestedList = listParent.CreateChild<UIAutomaticListActor>(child->GetName() + "'s nestedlist");
			AddActorToList(editorScene, *child, nestedList, canvas);
		}
	}
	virtual void Render() override
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

			Material* scenePreviewMaterial = new Material("GEE_3D_SCENE_PREVIEW_MATERIAL", 0.0f, GetGameHandle()->GetRenderEngineHandle()->FindShader("Forward_NoLight"));

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
			//scenePreviewMaterial->AddTexture(new NamedTexture(Texture(GL_TEXTURE_2D, ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorBuffer(0)->GetID()), "albedo1"));
			//EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->OverrideInstancesMaterial(scenePreviewMaterial);
			//EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->OverrideInstancesMaterial(RenderEng.FindMaterial("GEE_Button_Idle"));
			//RenderEng.RenderText(*MyFont, "Time: " + std::to_string(glfwGetTime()));
			//RenderEng.RenderText(RenderInfo(*RenderEng.GetCurrentTbCollection()), *MyFont, "Viewport " + std::to_string(Settings->ViewportData.z) + "x" + std::to_string(Settings->ViewportData.w), Transform(glm::vec3(0.0f, 632.0f, 0.0f), glm::vec3(0.0f), glm::vec3(16.0f)), glm::pow(glm::vec3(0.0f, 0.73f, 0.84f), glm::vec3(1.0f / 2.2f)), nullptr, true);
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
			RenderEng.FullSceneRender(info, EditorScene->GetRenderData(), nullptr, Viewport(glm::vec2(0.0f), glm::vec2(Settings->WindowSize)));
		}

		if (GameScene* mainMenuScene = GetScene("GEE_Main_Menu"))
			if (mainMenuScene->GetActiveCamera())
			{
				RenderInfo info = mainMenuScene->ActiveCamera->GetRenderInfo(*HUDRenderCollection);
				RenderEng.PrepareScene(*HUDRenderCollection, mainMenuScene->GetRenderData());
				RenderEng.FullSceneRender(info, mainMenuScene->GetRenderData(), nullptr, Viewport(glm::vec2(0.0f), glm::vec2(Settings->WindowSize)), !renderEditorScene);
			}

		glfwSwapBuffers(Window);
	}
	void PrintResources()
	{

	}
	void SetActiveScene(GameScene* scene)
	{
		ActiveScene = scene;
	}
	void LoadProject(const std::string& filepath)
	{
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
	}
	void SaveProject()
	{
		std::string filepath = ProjectFileDirectory + toValidFilepath(ProjectName);
		if (filepath.find(".json") == std::string::npos)
			filepath += ".json";
		std::cout << "Saving to path " << filepath << "\n";

		std::ofstream serializationStream(filepath);
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
	}

private:
	RenderToolboxCollection *ViewportRenderCollection, *HUDRenderCollection;
	GameScene* EditorScene;
	GameScene* ActiveScene;	//which scene currently handles events
	GameSettings EditorSettings;

	std::string ProjectName, ProjectFileDirectory;

	UICanvasActor* CanvasContext;

	Component* SelectedComp;
	Actor* SelectedActor;
	GameScene* SelectedScene;
};

class Base
{
public:
	int Pierwsza = 5;
	static std::unique_ptr<Base> Get()
	{
		return std::make_unique<Base>();
	}
	virtual void AA() { std::cout << Pierwsza << "|!!!!!\n"; };
};

class Inheriting : public Base
{
public:
	float Druga = 3.0f;
	static std::unique_ptr<Base> Get()
	{
		return static_unique_pointer_cast<Base>(std::make_unique<Inheriting>());
	}
	virtual void AA() override { std::cout << Druga << "|!!!!!\n"; };
};

class Different
{
public:
	Different(std::unique_ptr<Base>&& base):
		BaseObj(std::move(base))
	{
		BaseObj->AA();
	}

private:
	std::unique_ptr<Base> BaseObj;
};

class BaseAndInheritingFactory
{
	typedef std::unique_ptr<Base>(*CreateObjFun)(void);
	typedef std::map<std::string, CreateObjFun> FactoryMap;
	std::map<std::string, CreateObjFun> CreateMap;
public:
	BaseAndInheritingFactory()
	{
		Register("Base", Base::Get);
		Register("Inheriting", Inheriting::Get);
	}
	static BaseAndInheritingFactory& Get()
	{
		static BaseAndInheritingFactory factory;
		return factory;
	}
	std::unique_ptr<Base> CreateBase(const std::string& name)
	{
		FactoryMap::iterator it = CreateMap.find(name);
		if (it != CreateMap.end())
			return (*it->second)();
		return nullptr;
	}
	void Register(std::string name, CreateObjFun fun)
	{
		CreateMap[name] = fun;
	}
private:
};

CEREAL_REGISTER_TYPE(GunActor)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Actor, GunActor)
CEREAL_REGISTER_TYPE(PawnActor)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Actor, PawnActor)
CEREAL_REGISTER_TYPE(Controller)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Actor, Controller)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Controller, ShootingController)
CEREAL_REGISTER_TYPE(ShootingController)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Actor, ShootingController)
CEREAL_REGISTER_TYPE(CameraComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, CameraComponent)
CEREAL_REGISTER_TYPE(ModelComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ModelComponent)
CEREAL_REGISTER_TYPE(TextComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, TextComponent)
CEREAL_REGISTER_TYPE(BoneComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, BoneComponent)
CEREAL_REGISTER_TYPE(LightComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, LightComponent)
CEREAL_REGISTER_TYPE(SoundSourceComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, SoundSourceComponent)
CEREAL_REGISTER_TYPE(AnimationManagerComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, AnimationManagerComponent)
CEREAL_REGISTER_TYPE(LightProbeComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, LightProbeComponent)

CEREAL_REGISTER_TYPE(Material)
CEREAL_REGISTER_TYPE(AtlasMaterial)



int main(int argc, char** argv)
{
	std::string programFilepath;
	std::string projectFilepathArgument;
	for (int i = 0; i < argc; i++)
	{
		std::cout << argv[i] << '\n';
		switch (i)
		{
		case 0: programFilepath = argv[i]; break;
		case 1: projectFilepathArgument = argv[i]; break;
		}
	}
	Different(BaseAndInheritingFactory::Get().CreateBase("Inheriting"));
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

	glm::dvec3 front(-0.80989, - 0.15385, - 0.56604);
	glm::dvec3 rotationVec = glm::cross(front, glm::dvec3(0.0f, 0.0f, -1.0f));
	double angle = std::atan2((double)glm::dot(front, glm::dvec3(0.0f, 1.0f, 0.0f)), 0.0);
	glm::quat rot(glm::angleAxis(angle, rotationVec));
	printVector(rot, "quick maths");

	GLFWwindow* programWindow = glfwCreateWindow(800, 600, "c00lki", nullptr, nullptr);

	if (!programWindow)
	{
		std::cerr << "nie dziala glfw :('\n";
		return -1;
	}

	glfwMakeContextCurrent(programWindow);
	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))	//zaladuj glad (openGL); jesli sie nie udalo, to konczymy program
	{
		std::cerr << "zjebalo sie.";
		return -1;
	}

	int max_v_uniforms;
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_v_uniforms);
	std::cout << "max: " << max_v_uniforms << "\n";
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &max_v_uniforms);
	std::cout << "max2: " << max_v_uniforms << "\n";

	GameSettings settings;
	//settings.LoadFromFile("Settings.ini");
	
	//glm::vec2 res = static_cast<glm::vec2>(settings.WindowSize);
	//settings.ViewportData = glm::uvec4(res.x * 0.3f, res.y * 0.4f, res.x * 0.4, res.y * 0.6f);
	//settings.Video.Resolution = glm::vec2(res.x * 0.4, res.y * 0.6f);

	GameEngineEngineEditor editor(programWindow, settings);
	editor.SetupMainMenu();

	editor.SetActiveScene(editor.GetScene("GEE_Main_Menu"));
	editor.PassMouseControl(nullptr);

	if (!projectFilepathArgument.empty())
		editor.LoadProject(projectFilepathArgument);

	/*std::ifstream deserializationStream("zjebany.json");

	std::cout << "Serializing...\n";
	if (deserializationStream.good() && editor.GetMainScene())
	{
		try
		{
			cereal::JSONInputArchive archive(deserializationStream);
			cereal::LoadAndConstruct<Actor>::ScenePtr = editor.GetMainScene();
			editor.GetMainScene()->GetRenderData()->LoadSkeletonBatches(archive);
			const_cast<Actor*>(editor.GetMainScene()->GetRootActor())->Load(archive);

			archive.serializeDeferments();

			editor.GetMainScene()->Load();
		}
		catch (cereal::Exception& ex)
		{
			std::cout << "ERROR: While loading scene: " << ex.what() << '\n';
		}
		const_cast<Actor*>(editor.GetMainScene()->GetRootActor())->DebugHierarchy();
	}
	deserializationStream.close();*/


	editor.PreGameLoop();

	float deltaTime = 0.0f;
	float lastUpdateTime = glfwGetTime();
	bool endGame = false;

	do
	{
		deltaTime = glfwGetTime() - lastUpdateTime;
		lastUpdateTime = glfwGetTime();

		if (Material* found = editor.GetRenderEngineHandle()->FindMaterial("GEE_Engine_Title").get())
			found->SetColor(hsvToRgb(Vec3f(glm::mod((float)glfwGetTime() * 10.0f, 360.0f), 1.0f, 1.0f)));

		endGame = editor.GameLoopIteration(1.0f / 60.0f, deltaTime);
	} while (!endGame);
	//game.Run();

	editor.SaveProject();

	return 0;
}

/*
	ENGINE TEXTURE UNITS
	0-9 free
	10 2D shadow map array
	11 3D shadow map array
	12 irradiance map array
	13 prefilter map array
	14 BRDF lookup texture
	15+ free
	TODO: Environment map jest mipmapowany; czy te mipmapy sa wgl uzyte???
*/