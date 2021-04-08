#define STB_IMAGE_IMPLEMENTATION
#include <game/Game.h>
#include <scene/TextComponent.h>
#include <scene/CameraComponent.h>
#include <assetload/FileLoader.h>
#include <UI/UICanvasActor.h>
#include <scene/UIInputBoxActor.h>
#include <scene/UIWindowActor.h>
#include <scene/ModelComponent.h>
#include <scene/LightComponent.h>
#include <UI/UICanvasField.h>
#include <UI/UIListActor.h>
#include <scene/GunActor.h>
#include <scene/PawnActor.h>
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
			EditorHandle->PreviewHierarchyTree(*EngineDataLoader::LoadHierarchyTree(*EditorHandle->GetGameHandle()->GetScene("GEE_Main"), paths[0]));
		}
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
		SelectedActor(nullptr)
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
	virtual void Init(GLFWwindow* window) override
	{
		Game::Init(window);

		EditorEventProcessor::EditorHandle = this;
		glfwSetDropCallback(Window, EditorEventProcessor::FileDropCallback);

		LoadSceneFromFile("level.txt", "GEE_Main");

		SetupEditorScene();
		EditorScene = GetScene("GEE_Editor");

		BindAudioListenerTransformPtr(&GetScene("GEE_Main")->GetActiveCamera()->GetTransform());

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

		ActiveScene = GetScene("GEE_Main");

		SelectScene(ActiveScene, *EditorScene);

		EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->AddMeshInst(GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
		EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->SetTransform(Transform(glm::vec2(0.0f, 0.4f), glm::vec2(0.4f, 0.6f)));

		Material* scenePreviewMaterial = new Material("GEE_3D_SCENE_PREVIEW_MATERIAL", 0.0f, 0.0f, GetGameHandle()->GetRenderEngineHandle()->FindShader("Forward_NoLight"));
		RenderEng.AddMaterial(scenePreviewMaterial);
		//scenePreviewMaterial->AddTexture(new NamedTexture(Texture(GL_TEXTURE_2D, ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorBuffer(0)->GetID()), "albedo1"));
		//scenePreviewMaterial->AddTexture(new NamedTexture(*reserveTexture(Settings->Video.Resolution, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR, GL_TEXTURE_2D, 0, "albedo1"), "albedo1"));
		NamedTexture* zjeb = new NamedTexture(/**reserveTexture(Settings->Video.Resolution, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR, GL_TEXTURE_2D, 0, "albedo1")*/*ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorBuffer(0), "albedo1");
		std::cout << "ZJEBANA TEKSTURA: " << zjeb->GetID() << '\n';
		scenePreviewMaterial->AddTexture(new NamedTexture(/**reserveTexture(Settings->Video.Resolution, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR, GL_TEXTURE_2D, 0, "albedo1")*/*ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorBuffer(0), "albedo1"));
		EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->OverrideInstancesMaterial(scenePreviewMaterial);
		//SelectComponent(GetScene("GEE_Main")->FindActor("Januszek")->GetRoot());
		//SelectComponent(GetScene("GEE_Main")->FindActor("AStolenRevolver")->GetRoot());

		/*RenderTbCollections.push_back(std::make_unique<RenderToolboxCollection>(RenderToolboxCollection("WholeWindowRenderCollection", glm::vec2(GameHandle->GetGameSettings()->ViewportData.z, GameHandle->GetGameSettings()->ViewportData.w), *GameHandle->GetGameSettings())));
		RenderTbCollections.back()->AddTb<DeferredShadingToolbox>(DeferredShadingToolbox(settings, Shading, resolution));
		RenderTbCollections.back()->AddTb<MainFramebufferToolbox>(MainFramebufferToolbox(settings, Shading, glm::vec2(resolution), RenderTbCollections.back()->GetTb<DeferredShadingToolbox>()));

		CurrentTbCollection = RenderTbCollections[0].get();*/
	}
	void SetupEditorScene()
	{
		GameScene& editorScene = CreateScene("GEE_Editor");

		//settings.ViewportData = glm::uvec4(res.x * 0.3f, res.y * 0.4f, res.x * 0.4, res.y * 0.6f);

		Actor& scenePreviewActor = editorScene.CreateActorAtRoot(Actor(editorScene, "SceneViewportActor"));
		ModelComponent& scenePreviewQuad = scenePreviewActor.CreateComponent(ModelComponent(editorScene, "SceneViewportQuad"));
		 
		UICanvasActor& exampleCanvas = editorScene.CreateActorAtRoot(UICanvasActor(editorScene, "ExampleCanvas"));
		exampleCanvas.SetTransform(Transform(glm::vec3(0.7f, -0.4f, 0.0f), glm::vec3(0.0f), glm::vec3(0.3f)));
		UIButtonActor& lolBackgroundButton = exampleCanvas.CreateChild(UIButtonActor(editorScene, "VeryImportantButton"));
		lolBackgroundButton.SetTransform(Transform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f)));

		TextComponent& sampleText = exampleCanvas.CreateComponent(TextComponent(editorScene, "SampleTextComp", Transform(glm::vec3(0.9f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f)), "Sample Text", "fonts/expressway rg.ttf"));
		exampleCanvas.AddUIElement(sampleText);
		sampleText.SetAlignment(TextAlignment::CENTER, TextAlignment::CENTER);

		TextComponent& sampleText2 = exampleCanvas.CreateComponent(TextComponent(editorScene, "SampleTextComp", Transform(glm::vec3(1.9f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f)), "Very important text", "fonts/expressway rg.ttf"));
		exampleCanvas.AddUIElement(sampleText2);

		TextComponent& sampleText3 = exampleCanvas.CreateComponent(TextComponent(editorScene, "SampleTextComp", Transform(glm::vec3(1.9f, 1.5f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f)), "Mega ultra important text", "fonts/expressway rg.ttf"));
		exampleCanvas.AddUIElement(sampleText3);

		for (int y = -3; y <= 3; y++)
		{
			for (int x = -3; x <= 3; x++)
			{
				TextComponent& gridSampleText = exampleCanvas.CreateComponent(TextComponent(editorScene, "SampleTextComp", Transform(glm::vec3(x, y, 0.0f), glm::vec3(0.0f), glm::vec3(0.05f)), "Sample Text " + std::to_string(x) + "|||" + std::to_string(y), "fonts/expressway rg.ttf"));
				exampleCanvas.AddUIElement(gridSampleText);
				gridSampleText.SetAlignment(TextAlignment::RIGHT, TextAlignment::TOP);
			}
		}

		UIWindowActor& window1 = editorScene.CreateActorAtRoot(UIWindowActor(editorScene, "MyTestWindow"));
		window1.SetTransform(Transform(glm::vec2(0.0, -0.5f), glm::vec2(0.2f)));

		UIButtonActor& bigButtonActor = window1.CreateChild(UIButtonActor(editorScene, "MojTestowyButton"));
		bigButtonActor.SetTransform(Transform(glm::vec2(0.0f, 1.5f), glm::vec2(0.4f)));
		TextComponent& bigButtonText = bigButtonActor.CreateComponent(TextComponent(editorScene, "BigButtonText", Transform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f)), "big button", "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER)));
		window1.AddUIElement(bigButtonActor);

		Actor& cameraActor = editorScene.CreateActorAtRoot(Actor(editorScene, "OrthoCameraActor"));
		CameraComponent& orthoCameraComp = cameraActor.CreateComponent(CameraComponent(editorScene, "OrthoCameraComp"));

		editorScene.BindActiveCamera(&orthoCameraComp);
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
				ActiveScene = EditorScene;
				for (auto& it : Scenes)
					it->RootActor->HandleEventAll(Event(EventType::FOCUS_SWITCHED));
			}
			else if (polledEvent->GetType() == EventType::KEY_RELEASED && dynamic_cast<KeyEvent&>(*polledEvent).GetKeyCode() == Key::TAB)
			{
				ActiveScene = GetScene("GEE_Main");
				for (auto& it : Scenes)
					it->RootActor->HandleEventAll(Event(EventType::FOCUS_SWITCHED));
			}

			ActiveScene->RootActor->HandleEventAll(*polledEvent);
		}
		if (SelectedComp)
			;//SelectedComp->GetTransform().SetRotation(glm::normalize(EditedRotation));
	}
	virtual void SelectComponent(Component* comp, GameScene& editorScene) override
	{
		SelectedComp = comp;

		if (const Actor* found = editorScene.GetRootActor()->FindActor("GEE_E_Components_Info_Canvas"))
			const_cast<Actor*>(found)->MarkAsKilled();

		if (!comp)
			return;

		UICanvasActor& selectCompCanvas = editorScene.CreateActorAtRoot(UICanvasActor(editorScene, "GEE_E_Components_Info_Canvas"));
		selectCompCanvas.SetTransform(Transform(glm::vec2(-0.75f, 0.75f), glm::vec2(0.2f)));
		UIActor& scaleActor = *selectCompCanvas.GetScaleActor();

		comp->GetEditorDescription(scaleActor, editorScene);
		for (auto& it : scaleActor.GetChildren())
			selectCompCanvas.AddUIElement(*it);	//add all created actors/components to the UI
		
		if (selectCompCanvas.FieldsList)
			selectCompCanvas.FieldsList->Refresh();

		Scenes.back()->RootActor->DebugHierarchy();
	}
	virtual void SelectActor(Actor* actor, GameScene& editorScene) override
	{
		if (const Actor* found = editorScene.GetRootActor()->FindActor("GEE_E_Actors_Components_Canvas"))
			const_cast<Actor*>(found)->MarkAsKilled();

		SelectedActor = actor;
		SelectComponent(nullptr, editorScene);

		if (!actor)
			return;

		UICanvasActor& canvas = editorScene.CreateActorAtRoot(UICanvasActor(editorScene, "GEE_E_Actors_Components_Canvas"));
		UIActor& scaleActor = *canvas.GetScaleActor();
		canvas.SetTransform(Transform(glm::vec3(-0.68f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.32f, 0.5f, 1.0f)));

		//scaleActor.GetTransform()->SetScale(glm::vec2(1.0f, 0.04f / 0.5f));
		canvas.SetViewScale(glm::vec2(1.0f, glm::sqrt(0.5f / 0.32f)));

		UICanvasField& componentsListField = canvas.AddField("List of components");
		UIAutomaticListActor& listActor = componentsListField.CreateChild(UIAutomaticListActor(editorScene, "ListActor"));
		canvas.FieldsList->SetListElementOffset(canvas.FieldsList->GetListElementCount() - 1, [&listActor, &componentsListField]() { std::cout << "List offset: " << (static_cast<glm::mat3>(componentsListField.GetTransform()->GetMatrix()) * listActor.GetListOffset()).y << '\n';  return static_cast<glm::mat3>(componentsListField.GetTransform()->GetMatrix()) * (listActor.GetListOffset()); });
		canvas.FieldsList->SetListCenterOffset(canvas.FieldsList->GetListElementCount() - 1, [&componentsListField]() -> glm::vec3 { return glm::vec3(0.0f, -componentsListField.GetTransform()->ScaleRef.y, 0.0f); });
		listActor.GetTransform()->Move(glm::vec2(1.5f, 0.0f));

		AddActorToList<Component>(editorScene, *actor->GetRoot(), listActor, canvas);

		listActor.Refresh();
		if (canvas.FieldsList)
			canvas.FieldsList->Refresh();


		actor->GetEditorDescription(scaleActor, editorScene);
		for (auto& it : scaleActor.GetChildren())	//add all created actors/components to the UI
			if (UIActor* cast = dynamic_cast<UIActor*>(it))
				canvas.AddUIElement(*cast);
			else
				canvas.AddUIElement(*it);

		if (canvas.FieldsList)
			canvas.FieldsList->Refresh();

		UIButtonActor& refreshButton = scaleActor.CreateChild(UIButtonActor(editorScene, "GEE_E_Scene_Refresh_Button", [this, actor, &editorScene]() { this->SelectActor(actor, editorScene); }));

	//	refreshButton.GetTransform()->SetPosition(glm::vec2(1.75f, 0.0f));
		refreshButton.GetTransform()->SetPosition(glm::vec2(6.0f, 0.0f));
		refreshButton.GetTransform()->SetScale(glm::vec2(0.5f));

		AtlasMaterial& refreshIconMat = *new AtlasMaterial("GEE_E_Refresh_Icon_Material", glm::ivec2(3, 1), 0.0f, 0.0f, RenderEng.FindShader("Forward_NoLight"));
		refreshIconMat.AddTexture(new NamedTexture(textureFromFile("EditorAssets/refresh_icon.png", GL_RGB, GL_LINEAR, GL_NEAREST_MIPMAP_LINEAR), "albedo1"));
		refreshButton.SetMatIdle(MaterialInstance(refreshIconMat));
		refreshButton.SetMatHover(MaterialInstance(refreshIconMat, refreshIconMat.GetTextureIDInterpolatorTemplate(1.0f)));
		refreshButton.SetMatClick(MaterialInstance(refreshIconMat, refreshIconMat.GetTextureIDInterpolatorTemplate(2.0f)));

		canvas.AddUIElement(refreshButton);



		//UIButtonActor& addActorButton = scaleActor.CreateChild(UIButtonActor(editorScene, "GEE_E_Scene_Add_Actor_Button", [this, actor, &refreshButton]() {  actor->CreateComponent(Component(actor->GetScene(), "CREATED_COMP", Transform())); refreshButton.OnClick(); }));
		UIButtonActor& addActorButton = scaleActor.CreateChild(UIButtonActor(editorScene, "GEE_E_Scene_Add_Actor_Button", [this, &editorScene, &refreshButton, actor]() {
			UIWindowActor& compSelectWindow = editorScene.CreateActorAtRoot(UIWindowActor(editorScene, "Component_Selection_Window"));
			compSelectWindow.SetTransform(Transform(glm::vec2(0.0f, -0.5f), glm::vec2(0.3f)));

			auto func = [this, &compSelectWindow, &editorScene, &refreshButton, actor](int i, std::function<Component& ()> createCompFunc) {
				UIButtonActor& compButton = compSelectWindow.CreateChild(UIButtonActor(editorScene, "GEE_E_Scene_Add_Comp_Button", [this, &editorScene, &refreshButton, actor, createCompFunc]() { createCompFunc(); SelectActor(actor, editorScene); }));

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

			func(0, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent(Component(getSelectedComp().GetScene(), "A Component", Transform())); });
			func(1, [this, &editorScene, getSelectedComp]() -> Component& { LightComponent& comp = getSelectedComp().CreateComponent(LightComponent(getSelectedComp().GetScene(), "A LightComponent", POINT, GetScene("GEE_Main")->GetRenderData()->GetAvailableLightIndex(), GetScene("GEE_Main")->GetRenderData()->GetAvailableLightIndex())); comp.CalculateLightRadius(); return comp;  });
			func(2, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent(SoundSourceComponent(getSelectedComp().GetScene(), "A SoundSourceComponent")); });
			func(3, [this, &editorScene, getSelectedComp]() -> Component& { return getSelectedComp().CreateComponent(CameraComponent(getSelectedComp().GetScene(), "A CameraComponent")); });

			}));

		addActorButton.GetTransform()->SetPosition(glm::vec2(8.0f, 0.0f));
		addActorButton.GetTransform()->SetScale(glm::vec2(0.5f));

		AtlasMaterial& addActorMat = *new AtlasMaterial("GEE_E_Add_Actor_Material", glm::ivec2(3, 1), 0.0f, 0.0f, RenderEng.FindShader("Forward_NoLight"));
		addActorMat.AddTexture(new NamedTexture(textureFromFile("EditorAssets/add_icon.png", GL_RGB, GL_LINEAR, GL_NEAREST_MIPMAP_LINEAR), "albedo1"));
		addActorButton.SetMatIdle(MaterialInstance(addActorMat));
		addActorButton.SetMatHover(MaterialInstance(addActorMat, addActorMat.GetTextureIDInterpolatorTemplate(1.0f)));
		addActorButton.SetMatClick(MaterialInstance(addActorMat, addActorMat.GetTextureIDInterpolatorTemplate(2.0f)));

		canvas.AddUIElement(addActorButton);
	}
	virtual void SelectScene(GameScene* selectedScene, GameScene& editorScene) override
	{
		if (const Actor* found = editorScene.GetRootActor()->FindActor("GEE_E_Scene_Actors_Canvas"))
			const_cast<Actor*>(found)->MarkAsKilled();

		SelectActor(nullptr, editorScene);

		if (!selectedScene)
			return;

		UICanvasActor& canvas = editorScene.CreateActorAtRoot(UICanvasActor(editorScene, "GEE_E_Scene_Actors_Canvas")); 
		canvas.SetTransform(Transform(glm::vec2(0.68f, 0.1f), glm::vec2(0.32f, 0.04f)));
		canvas.SetViewScale(glm::vec2(glm::sqrt(0.32f / 0.04f), 1.0f));
		//std::shared_ptr<Actor> header = std::make_shared<Actor>(EditorScene.get(), "SelectedSceneHeaderActor");

		UIButtonActor& refreshButton = canvas.CreateChild(UIButtonActor(editorScene, "GEE_E_Scene_Refresh_Button", [this, selectedScene, &editorScene]() { this->SelectScene(selectedScene, editorScene); }));

		refreshButton.GetTransform()->SetPosition(glm::vec2(6.0f, 0.0f));
		refreshButton.GetTransform()->SetScale(glm::vec2(0.5f));

		AtlasMaterial& refreshIconMat = *new AtlasMaterial("GEE_E_Refresh_Icon_Material", glm::ivec2(3, 1), 0.0f, 0.0f, RenderEng.FindShader("Forward_NoLight"));
		refreshIconMat.AddTexture(new NamedTexture(textureFromFile("EditorAssets/refresh_icon.png", GL_RGB, GL_LINEAR, GL_NEAREST_MIPMAP_LINEAR), "albedo1"));
		refreshButton.SetMatIdle(MaterialInstance(refreshIconMat));
		refreshButton.SetMatHover(MaterialInstance(refreshIconMat, refreshIconMat.GetTextureIDInterpolatorTemplate(1.0f)));
		refreshButton.SetMatClick(MaterialInstance(refreshIconMat, refreshIconMat.GetTextureIDInterpolatorTemplate(2.0f)));

		canvas.AddUIElement(refreshButton);


		UIButtonActor& addActorButton = canvas.CreateChild(UIButtonActor(editorScene, "GEE_E_Scene_Add_Actor_Button", [this, &editorScene, &refreshButton, selectedScene]() {
			UIWindowActor& actorSelectWindow = editorScene.CreateActorAtRoot(UIWindowActor(editorScene, "Component_Selection_Window"));
			actorSelectWindow.SetTransform(Transform(glm::vec2(0.0f, -0.5f), glm::vec2(0.3f)));

			auto func = [this, &actorSelectWindow, &editorScene, &refreshButton, selectedScene](int i, std::function<Actor&()> createActorFunc, const std::string& className) {
				UIButtonActor& actorButton = actorSelectWindow.CreateChild(UIButtonActor(editorScene, "GEE_E_Scene_Add_Comp_Button", className, [this, &editorScene, &refreshButton, selectedScene, createActorFunc]() { Actor& createdActor = createActorFunc(); SelectScene(selectedScene, editorScene); SelectActor(&createdActor, editorScene); }));

				actorButton.GetTransform()->SetPosition(glm::vec2(-0.7f, 0.7f - static_cast<float>(i) * 0.2f));
				actorButton.GetTransform()->SetScale(glm::vec2(0.1f, 0.1f));

				actorSelectWindow.AddUIElement(actorButton);
			};

			std::function<Actor& ()> getSelectedActor = [this, selectedScene]() -> Actor& { return ((SelectedActor) ? (*SelectedActor) : (const_cast<Actor&>(*selectedScene->GetRootActor()))); };

			func(0, [this, &editorScene, getSelectedActor]() -> Actor& { return getSelectedActor().CreateChild(Actor(getSelectedActor().GetScene(), "An Actor")); }, "Actor");
			func(1, [this, &editorScene, getSelectedActor]() -> GunActor& { return getSelectedActor().CreateChild(GunActor(getSelectedActor().GetScene(), "A GunActor")); }, "GunActor");
			func(2, [this, &editorScene, getSelectedActor]() -> PawnActor& { return getSelectedActor().CreateChild(PawnActor(getSelectedActor().GetScene(), "A PawnActor")); }, "PawnActor");
			}));

		addActorButton.GetTransform()->SetPosition(glm::vec2(8.0f, 0.0f));
		addActorButton.GetTransform()->SetScale(glm::vec2(0.5f));

		AtlasMaterial& addActorMat = *new AtlasMaterial("GEE_E_Add_Actor_Material", glm::ivec2(3, 1), 0.0f, 0.0f, RenderEng.FindShader("Forward_NoLight"));
		addActorMat.AddTexture(new NamedTexture(textureFromFile("EditorAssets/add_icon.png", GL_RGB, GL_LINEAR, GL_NEAREST_MIPMAP_LINEAR), "albedo1"));
		addActorButton.SetMatIdle(MaterialInstance(addActorMat));
		addActorButton.SetMatHover(MaterialInstance(addActorMat, addActorMat.GetTextureIDInterpolatorTemplate(1.0f)));
		addActorButton.SetMatClick(MaterialInstance(addActorMat, addActorMat.GetTextureIDInterpolatorTemplate(2.0f)));

		canvas.AddUIElement(addActorButton);

		UIAutomaticListActor& listActor = canvas.CreateChild(UIAutomaticListActor(editorScene, "ListActor"));
		//listActor.GetTransform()->SetScale(glm::vec3(0.5f));

		//refreshButton.CreateComponent(TextComponent(editorScene, "GEE_E_Scene_Refresh_Text", Transform(glm::vec2(0.0f), glm::vec3(0.0f), glm::vec2(1.0f / 8.0f, 1.0f)), "Refresh", "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER)));

		AddActorToList<Actor>(editorScene, const_cast<Actor&>(*selectedScene->GetRootActor()), listActor, canvas);

		listActor.Refresh();

		const_cast<Actor*>(EditorScene->GetRootActor())->DebugActorHierarchy();
	}
	virtual void PreviewHierarchyTree(HierarchyTemplate::HierarchyTreeT& tree) override
	{
		/*Actor& actor = GetScene("GEE_Main")->CreateActorAtRoot(Actor(*GetScene("GEE_Main"), "Added"));
		std::cout << "Added actor creating done\n";
		EngineDataLoader::InstantiateTree(*actor.GetRoot(), tree);
		std::cout << "Added tree instantiated succesfully\n";

		SelectScene(GetScene("GEE_Main"), *EditorScene);	//refresh
		SelectActor(&actor, *EditorScene);*/

		GameScene& editorScene = *EditorScene;
		UIWindowActor& previewWindow = editorScene.CreateActorAtRoot(UIWindowActor(editorScene, "Mesh node preview window"));
		previewWindow.SetTransform(Transform(glm::vec2(0.0f, -0.5f), glm::vec2(0.4f)));

		UICanvasActor& nodesCanvas = previewWindow.CreateChild(UICanvasActor(editorScene, "Nodes canvas"));
		nodesCanvas.SetTransform(Transform(glm::vec2(-0.2f, 0.2f), glm::vec2(0.8f, 0.8f)));

		UIButtonActor& instantiateButton = previewWindow.CreateChild(UIButtonActor(editorScene, "InstantiateButton", "Instantiate", [this, &tree]() {
			std::function<Component*()> getSelectedComp = [this]() -> Component* { return ((SelectedComp) ? (SelectedComp) : ((SelectedActor) ? (SelectedActor->GetRoot()) : (nullptr))); };

			if (Component* selectedComp = getSelectedComp())
				EngineDataLoader::InstantiateTree(*selectedComp, tree);
			}));
		instantiateButton.SetTransform(Transform(glm::vec2(0.875f, 0.8f), glm::vec2(0.1f)));

		previewWindow.HideScrollBars(); //for now

		std::function<void(UIAutomaticListActor&, const HierarchyTemplate::HierarchyNodeBase&, int)> nodeCreatorFunc = [&editorScene, &nodesCanvas, &nodeCreatorFunc](UIAutomaticListActor& parent, const HierarchyTemplate::HierarchyNodeBase& correspondingNode, int level) {


			UIButtonActor& nodeButton = parent.CreateChild(UIButtonActor(editorScene, "OGAR"));//correspondingNode.GetName()));
			TextConstantSizeComponent& nodeText = nodeButton.CreateComponent(TextConstantSizeComponent(editorScene, "TEXTTEXT", Transform(), correspondingNode.GetCompBaseType().GetName(), "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER)));
			nodeButton.SetTransform(Transform(glm::vec2(0.0f, -3.0f), glm::vec2(1.0f)));
			nodesCanvas.AddUIElement(nodeButton);

			//std::cout << "NODE: " << correspondingNode.GetName() << ". CHILDREN COUNT: " << correspondingNode.GetChildCount() << '\n';

			for (int i = 0; i < correspondingNode.GetChildCount(); i++)
			{
				UIAutomaticListActor& nodeChildrenList = parent.CreateChild(UIAutomaticListActor(editorScene, correspondingNode.GetCompBaseType().GetName() + "children list", glm::vec3(3.0f, 0.0f, 0.0f)));
				if (i == 0)
					nodeChildrenList.GetTransform()->Move(((float)correspondingNode.GetChildCount() - 1.0f) * -glm::vec3(3.0f, 0.0f, 0.0f) / 2.0f);
				parent.NestList(nodeChildrenList);
				nodeCreatorFunc(nodeChildrenList, *correspondingNode.GetChild(i), level + 1);
			}

			//nodeChildrenList.GetTransform()->Move(((float)correspondingNode.GetChildCount() - 1.0f) * -glm::vec3(10.0f, 0.0f, 0.0f) / 2.0f);
		};

		UIAutomaticListActor& rootList = previewWindow.CreateChild(UIAutomaticListActor(editorScene, "Root children list", glm::vec3(3.0f, 0.0f, 0.0f)));
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
			UIButtonActor& animationButton = previewWindow.CreateChild(UIButtonActor(editorScene, tree.GetAnimation(i).Name + " animation button", tree.GetAnimation(i).Name));
			animationButton.SetTransform(Transform(glm::vec2(-0.8f + static_cast<float>(i) * 0.3f, -0.8f), glm::vec2(0.1f)));
		}

		previewWindow.DebugActorHierarchy(0);
	}
	template <typename T> void AddActorToList(GameScene& editorScene, T& obj, UIAutomaticListActor& listParent, UICanvas& canvas)
	{
		UIButtonActor& element = listParent.CreateChild<UIButtonActor>(UIButtonActor(editorScene, obj.GetName() + "'s Button", [this, &obj, &editorScene ]() {this->Select(&obj, editorScene); }));//*this, obj, &EditorManager::Select<T>));
		element.SetTransform(Transform(glm::vec2(1.5f, 0.0f), glm::vec2(3.0f, 1.0f)));
		TextConstantSizeComponent& elementText = element.CreateComponent(TextConstantSizeComponent(editorScene, obj.GetName() + "'s Text", Transform(glm::vec2(0.0f), glm::vec2(0.4f / 3.0f, 0.4f)), obj.GetName(), "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER)));
		elementText.SetMaxSize(glm::vec2(1.0f, 0.5f));
		elementText.SetContent(obj.GetName());
		canvas.AddUIElement(element);

		std::vector<T*> children = obj.GetChildren();
		std::cout << "liczba dzieciakow of " << obj.GetName() << ": " << children.size() << '\n';
		for (auto& child : children)
		{
			//AddActorToList(editorScene, *child, listParent.CreateChild(UIListActor(editorScene, "NextLevel")), canvas);
			UIAutomaticListActor& nestedList = listParent.CreateChild(UIAutomaticListActor(element.GetScene(), child->GetName() + "'s nestedlist"));
			listParent.NestList(nestedList);
			AddActorToList(editorScene, *child, nestedList, canvas);
		}
	}
	virtual void Render() override
	{
		if (!GetScene("GEE_Main")->ActiveCamera || !EditorScene->ActiveCamera)
		{
			std::cerr << "ERROR! Camera not bound in one of the scenes.\n";
			return;
		}

		RenderEng.PrepareFrame();



		//Render 3D scene
		RenderEng.PrepareScene(*ViewportRenderCollection, GetScene("GEE_Main")->GetRenderData());
		RenderEng.FullSceneRender(GetScene("GEE_Main")->ActiveCamera->GetRenderInfo(*ViewportRenderCollection), GetScene("GEE_Main")->GetRenderData(), &ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer());// , Viewport(glm::vec2(0.0f), Settings->Video.Resolution)/*Viewport((static_cast<glm::vec2>(Settings->WindowSize) - Settings->Video.Resolution) / glm::vec2(2.0f, 1.0f), glm::vec2(Settings->Video.Resolution.x, Settings->Video.Resolution.y))*/);
		RenderEng.FindShader("Forward_NoLight")->Use();
		GetScene("GEE_Main")->GetRootActor()->DebugRenderAll(GetScene("GEE_Main")->ActiveCamera->GetRenderInfo(*ViewportRenderCollection), RenderEng.FindShader("Forward_NoLight"));

		Material* scenePreviewMaterial = new Material("GEE_3D_SCENE_PREVIEW_MATERIAL", 0.0f, 0.0f, GetGameHandle()->GetRenderEngineHandle()->FindShader("Forward_NoLight"));
		//scenePreviewMaterial->AddTexture(new NamedTexture(Texture(GL_TEXTURE_2D, ViewportRenderCollection->GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorBuffer(0)->GetID()), "albedo1"));
		//EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->OverrideInstancesMaterial(scenePreviewMaterial);
		//EditorScene->FindActor("SceneViewportActor")->GetRoot()->GetComponent<ModelComponent>("SceneViewportQuad")->OverrideInstancesMaterial(RenderEng.FindMaterial("GEE_Button_Idle"));
		//RenderEng.RenderText(*MyFont, "Time: " + std::to_string(glfwGetTime()));
		//RenderEng.RenderText(RenderInfo(*RenderEng.GetCurrentTbCollection()), *MyFont, "Viewport " + std::to_string(Settings->ViewportData.z) + "x" + std::to_string(Settings->ViewportData.w), Transform(glm::vec3(0.0f, 632.0f, 0.0f), glm::vec3(0.0f), glm::vec3(16.0f)), glm::pow(glm::vec3(0.0f, 0.73f, 0.84f), glm::vec3(1.0f / 2.2f)), nullptr, true);
		RenderEng.RenderText(RenderInfo(*ViewportRenderCollection), *Fonts[0], "(C) twoja babka studios", Transform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(32.0f)), glm::pow(glm::vec3(0.0f, 0.73f, 0.84f), glm::vec3(1.0f / 2.2f)), nullptr, true);

		///Render editor scene
		RenderInfo info = EditorScene->ActiveCamera->GetRenderInfo(*HUDRenderCollection);
		RenderEng.PrepareScene(*HUDRenderCollection, EditorScene->GetRenderData());
		RenderEng.FullSceneRender(info, EditorScene->GetRenderData(), nullptr, Viewport(glm::vec2(0.0f), glm::vec2(Settings->WindowSize)));

		glfwSwapBuffers(Window);
	}
	void PrintResources()
	{

	}

private:
	RenderToolboxCollection *ViewportRenderCollection, *HUDRenderCollection;
	GameScene* EditorScene;
	GameScene* ActiveScene;	//which scene currently handles events
	GameSettings EditorSettings;

	Component* SelectedComp;
	Actor* SelectedActor;
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
int main()
{
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

	editor.PreGameLoop();

	float deltaTime = 0.0f;
	float lastUpdateTime = glfwGetTime();
	bool endGame = false;

	const char* xDPath = "Argon/Argon.glb";
	EditorEventProcessor::FileDropCallback(programWindow, 1, &xDPath);
	do
	{
		deltaTime = glfwGetTime() - lastUpdateTime;
		lastUpdateTime = glfwGetTime();

		endGame = editor.GameLoopIteration(1.0f / 60.0f, deltaTime);
		const char* path = "Argon/Argon.glb";
		//EditorEventProcessor::FileDropCallback(programWindow, 1, &path);
	} while (!endGame);
	//game.Run();
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