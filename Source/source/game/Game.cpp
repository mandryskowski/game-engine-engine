#include <game/Game.h>
#include <assetload/FileLoader.h>
#include <rendering/LightProbe.h>
#include <UI/Font.h>
#include <scene/Controller.h>
#include <scene/Actor.h>
#include <input/InputDevicesStateRetriever.h>
#include <thread>

namespace GEE
{
	bool PrimitiveDebugger::bDebugMeshTrees = false;
	bool PrimitiveDebugger::bDebugProbeLoading = false;
	bool PrimitiveDebugger::bDebugCubemapFromTex = false;
	bool PrimitiveDebugger::bDebugFramebuffers = false;
	bool PrimitiveDebugger::bDebugHierarchy = false;

	Controller* mouseController = nullptr;  //there are 2 similiar camera variables: ActiveCamera and global MouseController. the first one is basically the camera we use to see the world (view mat); the second one is updated by mouse controls.
											//this is a shitty comment that doesnt fit since a few months ago but i dont want to erase it
	EventHolder* GLFWEventProcessor::TargetHolder = nullptr;

	Game::Game(const ShadingAlgorithm& shading, const GameSettings& settings) :
		bGameTerminated(false),
		AudioEng(static_cast<GameManager*>(this)),
		RenderEng(static_cast<GameManager*>(this)),
		PhysicsEng(&DebugMode),
		Shading(shading),
		Settings(nullptr),
		GameStarted(false),
		DefaultFont(nullptr),
		MainScene(nullptr),
		ActiveScene(nullptr),
		TotalTickCount(0),
		TotalFrameCount(0)
	{
		GameManager::GamePtr = this;
		glDisable(GL_MULTISAMPLE);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		glDebugMessageCallbackARB(DebugCallbacks::OpenGLDebug, nullptr);
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

		DebugMode = true;
		LoopBeginTime = 0.0f;
		TimeAccumulator = 0.0f;

		GEE_FB::Framebuffer DefaultFramebuffer;
	}

	void Game::Init(SystemWindow* window)
	{
		Window = window;
		glfwSetWindowUserPointer(Window, this);
		glfwSetWindowSize(Window, Settings->WindowSize.x, Settings->WindowSize.y);

		if (Settings->bWindowFullscreen)
			glfwSetWindowMonitor(Window, glfwGetPrimaryMonitor(), 0, 0, Settings->WindowSize.x, Settings->WindowSize.y, 60);

		if (!Settings->Video.bVSync)
			glfwSwapInterval(0);

		GLFWEventProcessor::TargetHolder = &EventHolderObj;

		glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetCursorPos(Window, (double)Settings->WindowSize.x / 2.0, (double)Settings->WindowSize.y / 2.0);

		glfwSetCursorPosCallback(Window, GLFWEventProcessor::CursorPosCallback);
		glfwSetMouseButtonCallback(Window, GLFWEventProcessor::MouseButtonCallback);
		glfwSetKeyCallback(Window, GLFWEventProcessor::KeyPressedCallback);
		glfwSetCharCallback(Window, GLFWEventProcessor::CharEnteredCallback);
		glfwSetScrollCallback(Window, GLFWEventProcessor::ScrollCallback);
		glfwSetDropCallback(Window, GLFWEventProcessor::FileDropCallback);
		glfwSetWindowCloseCallback(Window, [](GLFWwindow* window) { static_cast<Game*>(glfwGetWindowUserPointer(window))->TerminateGame(); });

		RenderEng.Init(Vec2u(Settings->Video.Resolution.x, Settings->Video.Resolution.y));
		PhysicsEng.Init();

		DefaultFont = EngineDataLoader::LoadFont(*this, "fonts/Atkinson-Hyperlegible-Regular-102.otf");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Game::LoadSceneFromFile(const std::string& path, const std::string& name)
	{
		std::cout << "Loading scene " + name + " from filepath " + path + "\n";
		EngineDataLoader::SetupSceneFromFile(this, path, name);
	}

	GameScene& Game::CreateScene(const std::string& name, bool isAnUIScene)
	{
		AddScene(MakeUnique<GameScene>(*this, name, isAnUIScene));
		return *Scenes.back();
	}

	void Game::AddScene(UniquePtr<GameScene> scene)
	{
		Scenes.push_back(UniquePtr<GameScene>(scene.release()));		//"scene" does not work anymore
		RenderEng.AddSceneRenderDataPtr(*Scenes.back()->GetRenderData());
		std::cout << "level loaded.\n";

		std::cout << "Setting up collision\n";
		PhysicsEng.SetupScene(*Scenes.back()->GetPhysicsData());
		std::cout << "Collision objects setup finished.\n";

		std::cout << "Setting up actors & components.\n";
		if (PrimitiveDebugger::bDebugHierarchy)
			Scenes.back()->RootActor->DebugHierarchy();
		std::cout << "Actors & components setup finished.\n";

		std::cout << "Data loading finished.\n";
	}

	void Game::AddInterpolation(const Interpolation& interp)
	{
		Interpolations.push_back(MakeUnique<Interpolation>(interp));
	}

	void Game::BindAudioListenerTransformPtr(Transform* transform)
	{
		AudioEng.SetListenerTransformPtr(transform);
		std::cout << "SETTING AUDIO LISTENER TRANSFORM TO " << transform << '\n';
	}

	void Game::UnbindAudioListenerTransformPtr(Transform* transform)
	{
		if (AudioEng.GetListenerTransformPtr() == transform)
			AudioEng.SetListenerTransformPtr(nullptr);
	}

	void Game::PassMouseControl(Controller* controller)
	{
		mouseController = controller;

		if (!mouseController)
			glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		else
		{
			if (controller->GetHideCursor())
				glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwSetCursorPos(Window, (double)Settings->WindowSize.x / 2.0, (double)Settings->WindowSize.y / 2.0);
		}
	}

	const Controller* Game::GetCurrentMouseController() const
	{
		return mouseController;
	}

	SharedPtr<Font> Game::GetDefaultFont()
	{
		return DefaultFont;
	}

	bool Game::HasStarted() const
	{
		return GameStarted;
	}

	InputDevicesStateRetriever Game::GetInputRetriever()
	{
		return InputDevicesStateRetriever(*Window);
	}

	double Game::GetProgramRuntime() const
	{
		return glfwGetTime();
	}

	Physics::PhysicsEngineManager* Game::GetPhysicsHandle()
	{
		return &PhysicsEng;
	}

	RenderEngineManager* Game::GetRenderEngineHandle()
	{
		return &RenderEng;
	}

	Audio::AudioEngineManager* Game::GetAudioEngineHandle()
	{
		return &AudioEng;
	}

	GameSettings* Game::GetGameSettings()
	{
		return Settings.get();
	}

	GameScene* Game::GetScene(const std::string& name)
	{
		for (auto& it : Scenes)
			if (!it->IsBeingKilled() && it->GetName() == name)
				return it.get();

		return nullptr;
	}

	GameScene* Game::GetMainScene()
	{
		return MainScene;
	}


	Actor* Game::GetRootActor(GameScene* scene)
	{
		if (!scene)
			scene = GetMainScene();

		return scene->RootActor.get();
	}

	unsigned long long Game::GetTotalTickCount() const
	{
		return TotalTickCount;
	}

	unsigned long long Game::GetTotalFrameCount() const
	{
		return TotalFrameCount;
	}

	void Game::SetActiveScene(GameScene* scene)
	{
		ActiveScene = scene;
	}

	void Game::SetCursorIcon(DefaultCursorIcon icon)
	{
		glfwSetCursor(Window, glfwCreateStandardCursor(static_cast<int>(icon)));
	}

	HierarchyTemplate::HierarchyTreeT* Game::FindHierarchyTree(const std::string& name, HierarchyTemplate::HierarchyTreeT* treeToIgnore)
	{
		for (auto& it : Scenes)
			if (auto found = it->FindHierarchyTree(name, treeToIgnore))
				return found;

		return nullptr;
	}

	SharedPtr<Font> Game::FindFont(const std::string& path)
	{
		auto found = std::find_if(Fonts.begin(), Fonts.end(), [path](SharedPtr<Font> font) {return font->GetPath() == path; });
		if (found != Fonts.end())
			return *found;

		return nullptr;
	}

	unsigned int ticks = 1;
	bool wasSecondFPS = false;
	float timeSum = 0.0f;

	void Game::PreGameLoop()
	{
		float lastUpdateTime = (float)glfwGetTime();
		const float timeStep = 1.0f / 60.0f;
		float deltaTime = 0.0f;
		TimeAccumulator = 0.0f;

		LoopBeginTime = (float)glfwGetTime();

		//AddActorToScene(MakeShared<FPSController>(FPSController(this, "MojTestowyController")));
		//Scenes[0]->AddActorToRoot(MakeShared<UIButtonActor>(UIButtonActor(Scenes[0].get(), "MojTestowyButton")));
		for (int i = 0; i < static_cast<int>(Scenes.size()); i++)
			Scenes[i]->RootActor->OnStartAll();

		GameStarted = true;
	}

	bool Game::GameLoopIteration(float timeStep, float deltaTime)
	{
		if (bGameTerminated)
			return true;

		glfwPollEvents();

		if (deltaTime > 0.25f)
			deltaTime = 0.25f;

		float fraction = fmod((float)glfwGetTime(), 1.0f);
		if (fraction < 0.1f && !wasSecondFPS)
		{
			std::cout << "******Frames per second: " << (float)ticks / (glfwGetTime() - LoopBeginTime) << "			Milliseconds per frame: " << (glfwGetTime() - LoopBeginTime) * 1000.0f / (float)ticks << '\n';
			std::cout << "Update%: " << timeSum / (glfwGetTime() - LoopBeginTime) * 100.0f << "%\n";
			std::cout << "Unstable ms/frame: " << deltaTime * 1000.0f << ".\n";
			std::cout << "Renderable count: " << Scenes[0]->GetRenderData()->Renderables.size() << '\n';
			std::cout << "Light probe count: " << Scenes[0]->GetRenderData()->LightProbes.size() << '\n';
			//	std::cout << "Badany: " << Transform::test() / (glfwGetTime() - LoopBeginTime) * 100.0f << "%.\n";
			wasSecondFPS = true;
			ticks = 0;
			timeSum = 0.0f;
			LoopBeginTime = glfwGetTime();
		}
		else if (fraction > 0.9f)
			wasSecondFPS = false;

		if (GLenum error = glGetError())
		{
			std::cout << "OpenGL Error: " << error << ".\n";
		}

		HandleEvents();

		TimeAccumulator += deltaTime;

		while (TimeAccumulator >= timeStep)
		{
			Update(timeStep);

			TimeAccumulator -= timeStep;
			TotalTickCount++;
		}

		Render();
		ticks++;
		TotalFrameCount++;

		return false;
	}

	void Game::HandleEvents()
	{
		while (SharedPtr<Event> polledEvent = EventHolderObj.PollEvent(*Window))
		{
			for (int i = 0; i < static_cast<int>(Scenes.size()); i++)
				Scenes[i]->RootActor->HandleEventAll(*polledEvent);
		}
	}

	void Game::Update(float deltaTime)
	{
		DUPA::AnimTime += deltaTime;
		PhysicsEng.Update(deltaTime);

		for (int i = 0; i < static_cast<int>(Scenes.size()); i++)
			Scenes[i]->Update(deltaTime);

		AudioEng.Update();

		Interpolations.erase(std::remove_if(Interpolations.begin(), Interpolations.end(), [deltaTime](UniquePtr<Interpolation>& interp) { return interp->UpdateT(deltaTime); }), Interpolations.end());
	}
	void Game::TerminateGame()
	{
		bGameTerminated = true;
	}
	void Game::SetMainScene(GameScene* scene)
	{
		MainScene = scene;
		GameManager::DefaultScene = MainScene;
	}
	void Game::DeleteScene(GameScene& scene)
	{
		std::cout << "Deleting scene " << scene.GetName() << '\n';
		Scenes.erase(std::remove_if(Scenes.begin(), Scenes.end(), [&scene](UniquePtr<GameScene>& sceneVec) { return sceneVec.get() == &scene; }), Scenes.end());
	}

	std::vector<GameSceneRenderData*> Game::GetSceneRenderDatas()
	{
		std::vector<GameSceneRenderData*> renderDatas;
		renderDatas.reserve(Scenes.size());
		std::transform(Scenes.begin(), Scenes.end(), std::back_inserter(renderDatas), [](UniquePtr<GameScene>& scene) { return scene->GetRenderData(); });
		return renderDatas;
	}

	float pBegin = 0.0f;
	bool DUPA = false;

	void GLFWEventProcessor::CursorPosCallback(SystemWindow* window, double xpos, double ypos)
	{
		TargetHolder->PushEvent(*window, CursorMoveEvent(EventType::MouseMoved, Vec2f(static_cast<float>(xpos), static_cast<float>(ypos))));

		if (!mouseController || !TargetHolder)
			return;

		Vec2i windowSize(0);
		glfwGetWindowSize(window, &windowSize.x, &windowSize.y);

		if (mouseController->GetLockMouseAtCenter())
			glfwSetCursorPos(window, windowSize.x / 2, windowSize.y / 2);

		mouseController->OnMouseMovement(static_cast<Vec2f>(windowSize / 2), Vec2f(xpos, windowSize.y - ypos)); //Implement it as an Event instead! TODO
	}

	void GLFWEventProcessor::MouseButtonCallback(SystemWindow* window, int button, int action, int mods)
	{
		TargetHolder->PushEvent(*window, MouseButtonEvent((action == GLFW_PRESS) ? (EventType::MousePressed) : (EventType::MouseReleased), static_cast<MouseButton>(button), mods));
	}

	void GLFWEventProcessor::KeyPressedCallback(SystemWindow* window, int key, int scancode, int action, int mods)
	{
		TargetHolder->PushEvent(*window, KeyEvent((action == GLFW_PRESS) ? (EventType::KeyPressed) : ((action == GLFW_REPEAT) ? (EventType::KeyRepeated) : (EventType::KeyReleased)), static_cast<Key>(key), mods));
	}

	void GLFWEventProcessor::CharEnteredCallback(SystemWindow* window, unsigned int codepoint)
	{
		TargetHolder->PushEvent(*window, CharEnteredEvent(EventType::CharacterEntered, codepoint));
	}

	void GLFWEventProcessor::ScrollCallback(SystemWindow* window, double offsetX, double offsetY)
	{
		std::cout << "Scrolled " << offsetX << ", " << offsetY << "\n";
		Vec2d cursorPos(0.0);
		glfwGetCursorPos(window, &cursorPos.x, &cursorPos.y);
		TargetHolder->PushEvent(*window, MouseScrollEvent(EventType::MouseScrolled, Vec2f(static_cast<float>(-offsetX), static_cast<float>(offsetY))));
		TargetHolder->PushEvent(*window, CursorMoveEvent(EventType::MouseMoved, Vec2f(cursorPos)));	//When we scroll (e.g. a canvas), it is possible that some buttons or other objects relying on cursor position might be scrolled (moved) as well, so we need to create a CursorMoveEvent.
	}

	void GLFWEventProcessor::FileDropCallback(SystemWindow* window, int count, const char** paths)
	{

	}

	void APIENTRY DebugCallbacks::OpenGLDebug(GLenum source,	//Copied from learnopengl.com - I don't think it's worth it to rewrite a bunch of couts.
		GLenum type,
		unsigned int id,
		GLenum severity,
		GLsizei length,
		const char* message,
		const void* userParam)
	{
		// ignore non-significant error/warning codes
		if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
		if (type == GL_DEBUG_TYPE_PERFORMANCE_ARB)
			return;

		std::cout << "---------------" << std::endl;
		std::cout << "Debug message (" << id << "): " << message << std::endl;

		switch (source)
		{
		case GL_DEBUG_SOURCE_API_ARB:             std::cout << "Source: API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:   std::cout << "Source: Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: std::cout << "Source: Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:     std::cout << "Source: Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION_ARB:     std::cout << "Source: Application"; break;
		case GL_DEBUG_SOURCE_OTHER_ARB:           std::cout << "Source: Other"; break;
		} std::cout << std::endl;

		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR_ARB:               std::cout << "Type: Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: std::cout << "Type: Deprecated Behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:  std::cout << "Type: Undefined Behaviour"; break;
		case GL_DEBUG_TYPE_PORTABILITY_ARB:         std::cout << "Type: Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE_ARB:         std::cout << "Type: Performance"; break;
			//case GL_DEBUG_TYPE_MARKER_ARB:              std::cout << "Type: Marker"; break;
			//case GL_DEBUG_TYPE_PUSH_GROUP_ARB:          std::cout << "Type: Push Group"; break;
			//case GL_DEBUG_TYPE_POP_GROUP_ARB:           std::cout << "Type: Pop Group"; break;
		case GL_DEBUG_TYPE_OTHER_ARB:               std::cout << "Type: Other"; break;
		} std::cout << std::endl;

		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH_ARB:         std::cout << "Severity: high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM_ARB:       std::cout << "Severity: medium"; break;
		case GL_DEBUG_SEVERITY_LOW_ARB:          std::cout << "Severity: low"; break;
			//case GL_DEBUG_SEVERITY_NOTIFICATION_ARB: std::cout << "Severity: notification"; break;
		} std::cout << std::endl;
		std::cout << std::endl;
	}
}