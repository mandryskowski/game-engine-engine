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

	Controller* mouseController = nullptr; 
											
	EventHolder* WindowEventProcessor::TargetHolder = nullptr;
	std::unordered_map<SystemWindow*, Vec2i> WindowEventProcessor::PrevCursorPositions;

	Game::Game(const ShadingAlgorithm& shading, const GameSettings& settings) :
		bGameTerminated(false),
		MainScene(nullptr),
		ActiveScene(nullptr),
		Settings(nullptr),
		DefaultFont(nullptr),
		RenderEng(static_cast<GameManager*>(this)),
		PhysicsEng(&DebugMode),
		AudioEng(static_cast<GameManager*>(this)),
		Shading(shading),
		GameStarted(false),
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
		LoopBeginTime = 0.0;
		TimeAccumulator = 0.0;
	}

	void Game::Init(SystemWindow* window, const Vec2u& windowSize)
	{
		GameWindow = window;
		glfwSetWindowUserPointer(GameWindow, nullptr);
		glfwSetWindowSize(GameWindow, windowSize.x, windowSize.y);

		if (Settings->bWindowFullscreen)
			glfwSetWindowMonitor(GameWindow, glfwGetPrimaryMonitor(), 0, 0, windowSize.x, windowSize.y, 60);

		if (!Settings->Video.bVSync)
			glfwSwapInterval(0);

		WindowEventProcessor::TargetHolder = &EventHolderObj;

		glfwSetInputMode(GameWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetCursorPos(GameWindow, (double)windowSize.x / 2.0, (double)windowSize.y / 2.0);

		glfwSetCursorPosCallback(GameWindow, WindowEventProcessor::CursorPosCallback);
		glfwSetCursorEnterCallback(window, WindowEventProcessor::CursorLeaveEnterCallback);
		glfwSetMouseButtonCallback(GameWindow, WindowEventProcessor::MouseButtonCallback);
		glfwSetKeyCallback(GameWindow, WindowEventProcessor::KeyPressedCallback);
		glfwSetCharCallback(GameWindow, WindowEventProcessor::CharEnteredCallback);
		glfwSetScrollCallback(GameWindow, WindowEventProcessor::ScrollCallback);
		glfwSetDropCallback(GameWindow, WindowEventProcessor::FileDropCallback);
		glfwSetWindowCloseCallback(GameWindow, [](GLFWwindow* window) { static_cast<Game*>(glfwGetWindowUserPointer(window))->TerminateGame(); });

		RenderEng.Init(Vec2u(Settings->Video.Resolution.x, Settings->Video.Resolution.y));
		PhysicsEng.Init();

		DefaultFont = EngineDataLoader::LoadFont(*this, "Assets/Editor/fonts/Atkinson-Hyperlegible-Regular-102.otf", "Assets/Editor/fonts/Atkinson-Hyperlegible-Bold-102.otf", "Assets/Editor/fonts/Atkinson-Hyperlegible-Italic-102.otf", "Assets/Editor/fonts/Atkinson-Hyperlegible-BoldItalic-102.otf");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	bool Game::LoadSceneFromFile(const std::string& path, const std::string& name)
	{
		std::cout << "Loading scene " + name + " from filepath " + path + "\n";
		return EngineDataLoader::SetupSceneFromFile(this, path, name);
	}

	GameScene& Game::CreateScene(String name, bool disallowChangingNameIfTaken)
	{
		if (GetScene(name))
		{
			GEE_CORE_ASSERT(!disallowChangingNameIfTaken);
			name = GetUniqueName(Scenes.begin(), Scenes.end(), [](UniquePtr<GameScene>& scene) { return scene->GetName(); }, name);
		}
		AddScene(MakeUnique<GameScene>(*this, name));
		return *Scenes.back();
	}

	GameScene& Game::CreateUIScene(String name, SystemWindow& associateWindow, bool disallowChangingNameIfTaken)
	{
		if (GetScene(name))
		{
			GEE_CORE_ASSERT(!disallowChangingNameIfTaken);
			name = GetUniqueName(Scenes.begin(), Scenes.end(), [](UniquePtr<GameScene>& scene) { return scene->GetName(); }, name);
		}
		AddScene(MakeUnique<GameScene>(*this, name, true, associateWindow));
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
			glfwSetInputMode(GameWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		else
		{
			if (controller->GetHideCursor())
				glfwSetInputMode(GameWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			else
				glfwSetInputMode(GameWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

			Vec2i windowSize = WindowData(*GameWindow).GetWindowSize();
			WindowEventProcessor::PrevCursorPositions[GameWindow] = WindowData(*GameWindow).GetMousePositionPx(false);
			//glfwSetCursorPos(GameWindow, (double)windowSize.x / 2.0, (double)windowSize.y / 2.0);
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

	InputDevicesStateRetriever Game::GetDefInputRetriever()
	{
		return InputDevicesStateRetriever(*GameWindow);
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

	GameScene* Game::GetActiveScene()
	{
		return ActiveScene;
	}

	void Game::SetActiveScene(GameScene* scene)
	{
		ActiveScene = scene;
		glfwSetWindowUserPointer(GameWindow, scene);
	}

	void Game::SetCursorIcon(DefaultCursorIcon icon)
	{
		glfwSetCursor(GameWindow, glfwCreateStandardCursor(static_cast<int>(icon)));
	}

	Hierarchy::Tree* Game::FindHierarchyTree(const std::string& name, Hierarchy::Tree* treeToIgnore)
	{
		for (auto& it : Scenes)
			if (auto found = it->FindHierarchyTree(name, treeToIgnore))
				return found;

		return nullptr;
	}

	SharedPtr<Font> Game::FindFont(const std::string& path)
	{
		auto found = std::find_if(Fonts.begin(), Fonts.end(), [path](SharedPtr<Font> font) {return font->GetVariation(FontStyle::Regular)->GetPath() == path; });
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

	bool Game::GameLoopIteration(Time timeStep, Time deltaTime)
	{
		if (bGameTerminated)
			return true;

		glfwPollEvents();

		if (deltaTime > 0.25)
			deltaTime = 0.25;

		float fraction = fmod((float)glfwGetTime(), 1.0f);
		if (fraction < 0.1 && !wasSecondFPS)
		{
			std::cout << "******Frames per second: " << (float)ticks / (glfwGetTime() - LoopBeginTime) << "			Milliseconds per frame: " << (glfwGetTime() - LoopBeginTime) * 1000.0f / (float)ticks << '\n';
			wasSecondFPS = true;
			ticks = 0;
			timeSum = 0.0;
			LoopBeginTime = glfwGetTime();
		}
		else if (fraction > 0.9)
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
		for (auto& scene : Scenes)
		{
			while (SharedPtr<Event> polledEvent = EventHolderObj.PollEvent(*scene))
			{
				if (polledEvent->GetEventRoot() && &polledEvent->GetEventRoot()->GetScene() == scene.get())
					polledEvent->GetEventRoot()->HandleEventAll(*polledEvent);
				else
					scene->RootActor->HandleEventAll(*polledEvent);
			}
		}
	}

	void Game::Update(Time deltaTime)
	{
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

	EventPusher Game::GetEventPusher(GameScene& scene)
	{
		return EventPusher(scene, EventHolderObj);
	}

	void Game::DeleteScene(GameScene& scene)
	{
		std::cout << "Deleting scene " << scene.GetName() << '\n';
		scene.GetRootActor()->Delete();
		Scenes.erase(std::remove_if(Scenes.begin(), Scenes.end(), [&scene](UniquePtr<GameScene>& sceneVec) { return sceneVec.get() == &scene; }), Scenes.end());
	}

	std::vector<GameSceneRenderData*> Game::GetSceneRenderDatas()
	{
		std::vector<GameSceneRenderData*> renderDatas;
		renderDatas.reserve(Scenes.size());
		std::transform(Scenes.begin(), Scenes.end(), std::back_inserter(renderDatas), [](UniquePtr<GameScene>& scene) { return scene->GetRenderData(); });
		return renderDatas;
	}

	void WindowEventProcessor::CursorPosCallback(SystemWindow* window, double xpos, double ypos)
	{
		Vec2i windowSize(0);
		glfwGetWindowSize(window, &windowSize.x, &windowSize.y);

		// ypos is measured from the top. We want to measure all positions from the bottom (so 0 is at the very bottom and windowSize.y is at the very top).
		ypos = windowSize.y - ypos;

		GameScene& gameScene = *GetGameSceneFromWindow(*window);

		TargetHolder->PushEvent(gameScene, CursorMoveEvent(Vec2f(xpos, ypos), windowSize));

		if (!mouseController || !TargetHolder)
			return;


		mouseController->OnMouseMovement(
			Vec2i(PrevCursorPositions[window].x, windowSize.y - PrevCursorPositions[window].y),
			Vec2f(xpos, ypos), windowSize); //Implement it as an Event instead! TODO

		if (mouseController->GetLockMouseAtCenter())
			glfwSetCursorPos(window, PrevCursorPositions[window].x,  PrevCursorPositions[window].y);
		else
			PrevCursorPositions[window] = static_cast<Vec2i>(Vec2d(xpos, windowSize.y - ypos));
	}

	void WindowEventProcessor::MouseButtonCallback(SystemWindow* window, int button, int action, int mods)
	{
		GameScene& gameScene = *GetGameSceneFromWindow(*window);
		TargetHolder->PushEvent(gameScene, MouseButtonEvent(static_cast<MouseButton>(button), mods, action != GLFW_PRESS));
	}

	void WindowEventProcessor::KeyPressedCallback(SystemWindow* window, int key, int scancode, int action, int mods)
	{
		GameScene& gameScene = *GetGameSceneFromWindow(*window);
		TargetHolder->PushEvent(gameScene, KeyEvent(static_cast<Key>(key), mods, (action == GLFW_PRESS) ? (KeyEvent::KeyAction::Pressed) : ((action == GLFW_REPEAT) ? (KeyEvent::KeyAction::Repeated) : (KeyEvent::KeyAction::Released))));
	}

	void WindowEventProcessor::CharEnteredCallback(SystemWindow* window, unsigned int codepoint)
	{
		GameScene& gameScene = *GetGameSceneFromWindow(*window);
		TargetHolder->PushEvent(gameScene, CharEnteredEvent(codepoint));
	}

	void WindowEventProcessor::ScrollCallback(SystemWindow* window, double offsetX, double offsetY)
	{
		GameScene& gameScene = *GetGameSceneFromWindow(*window);
		Vec2d cursorPos(0.0);
		glfwGetCursorPos(window, &cursorPos.x, &cursorPos.y);

		Vec2i windowSize(0);
		glfwGetWindowSize(window, &windowSize.x, &windowSize.y);

		TargetHolder->PushEvent(gameScene, MouseScrollEvent(Vec2f(static_cast<float>(-offsetX), static_cast<float>(offsetY))));
		TargetHolder->PushEvent(gameScene, CursorMoveEvent(static_cast<Vec2f>(cursorPos), windowSize));	//When we scroll (e.g. a canvas), it is possible that some buttons or other objects relying on cursor position might be scrolled (moved) as well, so we need to create a CursorMoveEvent.
	}

	void WindowEventProcessor::FileDropCallback(SystemWindow* window, int count, const char** paths)
	{

	}

	void WindowEventProcessor::CursorLeaveEnterCallback(SystemWindow* window, int enter)
	{
		Vec2d cursorPos;
		glfwGetCursorPos(window, &cursorPos.x, &cursorPos.y);
		if (!enter)
		{
			CursorPosCallback(window, cursorPos.x, cursorPos.y);
		}
	}

	GameScene* WindowEventProcessor::GetGameSceneFromWindow(SystemWindow& window)
	{
		GameScene* gameScene = static_cast<GameScene*>(glfwGetWindowUserPointer(&window));

		if (!gameScene)
			std::cout << "Window is not configured for receiving events." << '\n';

		return gameScene;
	}

	void APIENTRY DebugCallbacks::OpenGLDebug(GLenum source,	//Copied from learnopengl.com - I don't think it's worth it to rewrite a bunch of couts.
		GLenum type,
		unsigned int id,
		GLenum severity,
		GLsizei length,
		const char* message,
		const void* userParam)
	{
		// ignore insignificant error/warning codes
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
