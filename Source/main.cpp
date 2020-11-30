#define STB_IMAGE_IMPLEMENTATION
#include "Game.h"
#include "ButtonActor.h"
#include "TextComponent.h"
#include "CameraComponent.h"
#include "FileLoader.h"
#include <map>

class GameEngineEngineEditor : public Game
{
public:
	GameEngineEngineEditor(GLFWwindow* window, const GameSettings& settings) :
		Game(settings.Video.Shading, settings)
	{
		Settings = std::make_unique<GameSettings>(GameSettings(EngineDataLoader::LoadSettingsFromFile<GameSettings>("Settings.ini")));
		glm::vec2 res = static_cast<glm::vec2>(Settings->WindowSize);
		//Settings->ViewportData = glm::uvec4(res.x * 0.3f, res.y * 0.4f, res.x * 0.4, res.y * 0.6f);
		Settings->Video.Resolution = glm::vec2(res.x * 0.4f, res.y * 0.6f);
		Settings->Video.Shading = ShadingModel::SHADING_PBR_COOK_TORRANCE;
		Init(window);
	} 
	virtual void Init(GLFWwindow* window) override
	{
		Game::Init(window);
		LoadSceneFromFile("level.txt");
		AddScene(GetEditorScene());

		BindAudioListenerTransformPtr(&GetMainScene()->GetActiveCamera()->GetTransform());

		ViewportRenderCollection = &RenderEng.AddRenderTbCollection(RenderToolboxCollection("GameSceneRenderCollection", Settings->Video));

		GameSettings::VideoSettings hudRenderSettings;
		hudRenderSettings.AAType = AA_NONE;
		hudRenderSettings.AmbientOcclusionSamples = 0;
		hudRenderSettings.bBloom = false;
		hudRenderSettings.POMLevel = SettingLevel::SETTING_NONE;
		hudRenderSettings.Resolution = Settings->WindowSize;
		hudRenderSettings.Shading = ShadingModel::SHADING_FULL_LIT;
		hudRenderSettings.ShadowLevel = SettingLevel::SETTING_NONE;

		HUDRenderCollection = &RenderEng.AddRenderTbCollection(RenderToolboxCollection("HUDRenderCollection", hudRenderSettings));

		/*RenderTbCollections.push_back(std::make_unique<RenderToolboxCollection>(RenderToolboxCollection("WholeWindowRenderCollection", glm::vec2(GameHandle->GetGameSettings()->ViewportData.z, GameHandle->GetGameSettings()->ViewportData.w), *GameHandle->GetGameSettings())));
		RenderTbCollections.back()->AddTb<DeferredShadingToolbox>(DeferredShadingToolbox(settings, Shading, resolution));
		RenderTbCollections.back()->AddTb<MainFramebufferToolbox>(MainFramebufferToolbox(settings, Shading, glm::vec2(resolution), RenderTbCollections.back()->GetTb<DeferredShadingToolbox>()));

		CurrentTbCollection = RenderTbCollections[0].get();*/
	}
	std::unique_ptr<GameScene> GetEditorScene()
	{
		std::unique_ptr<GameScene> editorScene = std::make_unique<GameScene>(GameScene(this));
		editorScene->AddActorToRoot(std::make_shared<ButtonActor>(ButtonActor(editorScene.get(), "MojTestowyButton")))->SetTransform(Transform(glm::vec3(-0.6f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f)));
		editorScene->AddActorToRoot(std::make_shared<ButtonActor>(ButtonActor(editorScene.get(), "smol button")))->SetTransform(Transform(glm::vec3(-0.75f, -0.75f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f)));
		
		Actor* textActor = editorScene->AddActorToRoot(std::make_shared<Actor>(Actor(editorScene.get(), "ViewportText")));
		TextComponent* textComp = TextComponent::Of(TextComponent(editorScene.get(), "ViewportTextComp", Transform(glm::vec3(0.0f, 0.75f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f)), "siemano", "fonts/expressway rg.ttf")).get();
		textActor->AddComponent(textComp);

		Actor* cameraActor = editorScene->AddActorToRoot(std::make_shared<Actor>(Actor(editorScene.get(), "OrthoCameraActor")));
		CameraComponent* orthoCameraComp = new CameraComponent(editorScene.get(), "OrthoCameraComp");
		cameraActor->AddComponent(orthoCameraComp);

		editorScene->BindActiveCamera(orthoCameraComp);

		textComp->GetTransform().GetWorldTransform().Print("Scene 2 Text transform");
		return editorScene;
	}
	virtual void Render() override
	{
		if (!Scenes[0]->ActiveCamera || !Scenes[1]->ActiveCamera)
		{
			std::cerr << "ERROR! Camera not bound in one of the scenes.\n";
			return;
		}

		RenderEng.PrepareFrame();

		glViewport(0, 0, 800, 600);
		RenderInfo info = Scenes[1]->ActiveCamera->GetRenderInfo(*HUDRenderCollection);
		RenderEng.PrepareScene(*HUDRenderCollection, Scenes[1]->GetRenderData());
		RenderEng.FullSceneRender(info, Scenes[1]->GetRenderData(), nullptr, Viewport(glm::vec2(0.0f), glm::vec2(Settings->WindowSize)));


		RenderEng.PrepareScene(*ViewportRenderCollection, Scenes[0]->GetRenderData());
		RenderEng.FullSceneRender(Scenes[0]->ActiveCamera->GetRenderInfo(*ViewportRenderCollection), Scenes[0]->GetRenderData(), nullptr, Viewport((static_cast<glm::vec2>(Settings->WindowSize) - Settings->Video.Resolution) / glm::vec2(2.0f, 1.0f), glm::vec2(Settings->Video.Resolution.x, Settings->Video.Resolution.y)));
		RenderEng.FindShader("Forward_NoLight")->Use();
		Scenes[0]->GetRootActor()->DebugRenderAll(Scenes[0]->ActiveCamera->GetRenderInfo(*ViewportRenderCollection), RenderEng.FindShader("Forward_NoLight"));

		//RenderEng.RenderText(*MyFont, "Time: " + std::to_string(glfwGetTime()));
		//RenderEng.RenderText(RenderInfo(*RenderEng.GetCurrentTbCollection()), *MyFont, "Viewport " + std::to_string(Settings->ViewportData.z) + "x" + std::to_string(Settings->ViewportData.w), Transform(glm::vec3(0.0f, 632.0f, 0.0f), glm::vec3(0.0f), glm::vec3(16.0f)), glm::pow(glm::vec3(0.0f, 0.73f, 0.84f), glm::vec3(1.0f / 2.2f)), nullptr, true);
		RenderEng.RenderText(RenderInfo(*ViewportRenderCollection), *MyFont, "(C) twoja babka studios", Transform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(64.0f)), glm::pow(glm::vec3(0.0f, 0.73f, 0.84f), glm::vec3(1.0f / 2.2f)), nullptr, true);

		glfwSwapBuffers(Window);
	}

private:
	RenderToolboxCollection *ViewportRenderCollection, *HUDRenderCollection;
};

class Base
{
public:
	Base()
	{
	}
	void method()
	{
		method2();
	}
	void method2()
	{
		std::cout << "jedynka\n";
	}
};

void foo(std::vector<Base*>::iterator it, int size)
{
	for (int i = 0; i < size; i++)
	{
		(*it)->method();
		if (i==0)
		it++;
	}
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

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

	do
	{
		deltaTime = glfwGetTime() - lastUpdateTime;
		lastUpdateTime = glfwGetTime();

		endGame = editor.GameLoopIteration(1.0f / 60.0f, deltaTime);
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