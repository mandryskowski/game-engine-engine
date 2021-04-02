#include <game/Game.h>
#include <assetload/FileLoader.h>
#include <rendering/LightProbe.h>
#include <UI/Font.h>
#include <scene/Controller.h>
#include <scene/Actor.h>
#include <input/InputDevicesStateRetriever.h>
#include <thread>

bool PrimitiveDebugger::bDebugMeshTrees = false;
bool PrimitiveDebugger::bDebugProbeLoading = false;
bool PrimitiveDebugger::bDebugCubemapFromTex = false;
bool PrimitiveDebugger::bDebugFramebuffers = true;
bool PrimitiveDebugger::bDebugHierarchy = false;

Controller* mouseController = nullptr;  //there are 2 similiar camera variables: ActiveCamera and global MouseController. the first one is basically the camera we use to see the world (view mat); the second one is updated by mouse controls.
										//this is a shitty comment that doesnt fit since a few months ago but i dont want to erase it
EventHolder* GLFWEventProcessor::TargetHolder = nullptr;

void APIENTRY debugOutput(GLenum source,	//Copied from learnopengl.com - I don't think it's worth it to rewrite a bunch of couts.
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

Game::Game(const ShadingModel& shading, const GameSettings& settings) :
	AudioEng(static_cast<GameManager*>(this)), RenderEng(static_cast<GameManager*>(this)), PhysicsEng(&DebugMode), Shading(shading), Settings(nullptr), GameStarted(false)
{

	glDisable(GL_MULTISAMPLE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	 
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	glDebugMessageCallbackARB(debugOutput, nullptr);
	glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

	DebugMode = true;
	LoopBeginTime = 0.0f;
	TimeAccumulator = 0.0f;

	Fonts.push_back(EngineDataLoader::LoadFont(*this, "fonts/expressway rg.ttf"));

	GEE_FB::Framebuffer DefaultFramebuffer;
}

void Game::Init(GLFWwindow* window)
{
	Window = window;
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

	RenderEng.Init(glm::uvec2(Settings->Video.Resolution.x, Settings->Video.Resolution.y));
	PhysicsEng.Init();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Game::LoadSceneFromFile(const std::string& path, const std::string& name)
{
	EngineDataLoader::SetupSceneFromFile(this, path, name);
}

GameScene& Game::CreateScene(const std::string& name)
{
	AddScene(std::make_unique<GameScene>(GameScene(*this, name)));
	return *Scenes.back();
}

void Game::AddScene(std::unique_ptr<GameScene> scene)
{
	Scenes.push_back(std::unique_ptr<GameScene>(scene.release()));		//"scene" does not work anymore
	RenderEng.AddSceneRenderDataPtr(Scenes.back()->GetRenderData());
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

void Game::BindAudioListenerTransformPtr(Transform* transform)
{
	AudioEng.SetListenerTransformPtr(transform);
}

void Game::PassMouseControl(Controller* controller)
{
	mouseController = controller;

	if (!mouseController)
		glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	else
	{
		glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetCursorPos(Window, (double)Settings->WindowSize.x / 2.0, (double)Settings->WindowSize.y / 2.0);
	}
}

const Controller* Game::GetCurrentMouseController() const
{
	return mouseController;
}

bool Game::HasStarted() const
{
	return GameStarted;
}

InputDevicesStateRetriever Game::GetInputRetriever()
{
	return InputDevicesStateRetriever(*Window);
}

PhysicsEngineManager* Game::GetPhysicsHandle()
{
	return &PhysicsEng;
}

RenderEngineManager* Game::GetRenderEngineHandle()
{
	return &RenderEng;
}

AudioEngineManager* Game::GetAudioEngineHandle()
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
		if (it->GetName() == name)
			return it.get();

	return nullptr;
}


Actor* Game::GetRootActor(GameScene* scene)
{
	if (!scene)
		scene = GetScene("GEE_Main");

	return scene->RootActor.get();
}

HierarchyTemplate::HierarchyTreeT* Game::FindHierarchyTree(const std::string& name, HierarchyTemplate::HierarchyTreeT* treeToIgnore)
{
	for (auto& it : Scenes)
		if (auto found = it->FindHierarchyTree(name, treeToIgnore))
			return found;

	return nullptr;
}

std::shared_ptr<Font> Game::FindFont(const std::string& path)
{
	auto found = std::find_if(Fonts.begin(), Fonts.end(), [path](std::shared_ptr<Font> font) {return font->GetPath() == path; });
	if (found != Fonts.end())
		return *found;

	return nullptr;
}

unsigned int ticks = 1;
bool wasSecondFPS = false;
float timeSum = 0.0f;

void Game::PreGameLoop()
{
	std::cout << "Render pass started.\n";
	RenderEng.PreLoopPass();
	std::cout << "Render pass done.\n";

	float lastUpdateTime = (float)glfwGetTime();
	const float timeStep = 1.0f / 60.0f;
	float deltaTime = 0.0f;
	TimeAccumulator = 0.0f;

	LoopBeginTime = (float)glfwGetTime();

	//AddActorToScene(std::make_shared<Controller>(Controller(this, "MojTestowyController")));
	//Scenes[0]->AddActorToRoot(std::make_shared<UIButtonActor>(UIButtonActor(Scenes[0].get(), "MojTestowyButton")));
	for (int i = 0; i < static_cast<int>(Scenes.size()); i++)
		Scenes[i]->RootActor->OnStartAll();

	GameStarted = true;
}

bool Game::GameLoopIteration(float timeStep, float deltaTime)
{
	if (glfwWindowShouldClose(Window))
		return true;

	glfwPollEvents();


	if (glfwGetKey(Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(Window, true);

	if (deltaTime > 0.25f)
		deltaTime = 0.25f;

	float fraction = fmod((float)glfwGetTime(), 1.0f);
	if (fraction < 0.1f && !wasSecondFPS && 0)
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
	}

	Render();
	ticks++;

	return false;
}

void Game::HandleEvents()
{
	while (std::shared_ptr<Event> polledEvent = EventHolderObj.PollEvent())
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
		Scenes[i]->RootActor->UpdateAll(deltaTime);
	
	AudioEng.Update();
}
float pBegin = 0.0f;
bool DUPA = false;

void GLFWEventProcessor::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	TargetHolder->Events.push(std::make_shared<CursorMoveEvent>(CursorMoveEvent(EventType::MOUSE_MOVED, glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos)))));

	if (!mouseController || !TargetHolder)
		return;

	glm::ivec2 halfWindowSize(0);
	glfwGetWindowSize(window, &halfWindowSize.x, &halfWindowSize.y);
	halfWindowSize /= 2;
	glfwSetCursorPos(window, (double)halfWindowSize.x, (double)halfWindowSize.y);
	mouseController->RotateWithMouse(glm::vec2(xpos - (float)halfWindowSize.x, ypos - (float)halfWindowSize.y)); //Implement it as an Event instead! TODO
}

void GLFWEventProcessor::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	TargetHolder->Events.push(std::make_shared<MouseButtonEvent>(MouseButtonEvent((action == GLFW_PRESS) ? (EventType::MOUSE_PRESSED) : (EventType::MOUSE_RELEASED), static_cast<MouseButton>(button), mods)));
}

void GLFWEventProcessor::KeyPressedCallback(GLFWwindow*, int key, int scancode, int action, int mods)
{
	TargetHolder->Events.push(std::make_shared<KeyEvent>(KeyEvent((action == GLFW_PRESS) ? (EventType::KEY_PRESSED) : ((action == GLFW_REPEAT) ? (EventType::KEY_REPEATED) : (EventType::KEY_RELEASED)), static_cast<Key>(key), mods)));
}

void GLFWEventProcessor::CharEnteredCallback(GLFWwindow*, unsigned int codepoint)
{
	TargetHolder->Events.push(std::make_shared<CharEnteredEvent>(CharEnteredEvent(EventType::CHARACTER_ENTERED, codepoint)));
}

void GLFWEventProcessor::ScrollCallback(GLFWwindow* window, double offsetX, double offsetY)
{
	std::cout << "Scrolled " << offsetX << ", " << offsetY << "\n";
	glm::dvec2 cursorPos(0.0);
	glfwGetCursorPos(window, &cursorPos.x, &cursorPos.y);
	TargetHolder->Events.push(std::make_shared<MouseScrollEvent>(MouseScrollEvent(EventType::MOUSE_SCROLLED, glm::vec2(static_cast<float>(-offsetX), static_cast<float>(offsetY)))));
	TargetHolder->Events.push(std::make_shared<CursorMoveEvent>(CursorMoveEvent(EventType::MOUSE_MOVED, glm::vec2(cursorPos))));	//When we scroll (e.g. a canvas), it is possible that some buttons or other objects relying on cursor position might be scrolled (moved) as well, so we need to create a CursorMoveEvent.
}

void GLFWEventProcessor::FileDropCallback(GLFWwindow* window, int count, const char** paths)
{

}
