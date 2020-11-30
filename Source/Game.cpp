#include "Game.h"
#include "LightComponent.h"
#include "ModelComponent.h"
#include "CameraComponent.h"
#include "FileLoader.h"
#include "LightProbe.h"
#include "Font.h"
#include "Controller.h"
#include "ButtonActor.h"
#include <thread>

bool PrimitiveDebugger::bDebugMeshTrees = false;
bool PrimitiveDebugger::bDebugProbeLoading = false;
bool PrimitiveDebugger::bDebugCubemapFromTex = false;
bool PrimitiveDebugger::bDebugFramebuffers = false;
bool PrimitiveDebugger::bDebugHierarchy = true;

Controller* mouseController = nullptr;  //there are 2 similiar camera variables: ActiveCamera and global MouseController. the first one is basically the camera we use to see the world (view mat); the second one is updated by mouse controls.
										//this is a shitty comment that doesnt fit since a few months ago but i dont want to erase it
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
	AudioEng(static_cast<GameManager*>(this)), RenderEng(static_cast<GameManager*>(this)), PhysicsEng(&DebugMode), Shading(shading), Settings(nullptr)
{

	glDisable(GL_MULTISAMPLE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	 
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	glDebugMessageCallbackARB(debugOutput, nullptr);
	glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

	DebugMode = true;
	LoopBeginTime = 0.0f;
	TimeAccumulator = 0.0f;
	MyFont = EngineDataLoader::LoadFont("fonts/expressway rg.ttf");

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

	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPos(Window, (double)Settings->WindowSize.x / 2.0, (double)Settings->WindowSize.y / 2.0);
	glfwSetCursorPosCallback(Window, cursorPosCallback);

	RenderEng.Init(glm::uvec2(Settings->Video.Resolution.x, Settings->Video.Resolution.y));
	PhysicsEng.Init();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Game::LoadSceneFromFile(std::string path)
{
	AddScene(EngineDataLoader::LoadSceneFromFile(static_cast<GameManager*>(this), path));
}

void Game::AddScene(std::unique_ptr<GameScene> scene)
{
	Scenes.push_back(std::unique_ptr<GameScene>(scene.release()));
	RenderEng.AddSceneRenderDataPtr(Scenes.back()->GetRenderData());
	PhysicsEng.AddScenePhysicsDataPtr(Scenes.back()->GetPhysicsData());
	std::cout << "level loaded.\n";

	std::cout << "Setting up collision\n";
	PhysicsEng.SetupScene(Scenes.back()->GetPhysicsData());
	std::cout << "Collision objects setup finished.\n";

	std::cout << "Setting up actors & components.\n";
	Scenes.back()->RootActor->Setup();
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

GameScene* Game::GetMainScene()
{
	return Scenes[0].get();
}

Actor* Game::GetRootActor(GameScene* scene)
{
	if (!scene)
		scene = GetMainScene();

	return scene->RootActor.get();
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
	//Scenes[0]->AddActorToRoot(std::make_shared<ButtonActor>(ButtonActor(Scenes[0].get(), "MojTestowyButton")));
	for (int i = 0; i < static_cast<int>(Scenes.size()); i++)
		Scenes[i]->RootActor->OnStartAll();
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
	if (fraction < 0.1f && !wasSecondFPS)
	{
		std::cout << "******Frames per second: " << (float)ticks / (glfwGetTime() - LoopBeginTime) << "			Milliseconds per frame: " << (glfwGetTime() - LoopBeginTime) * 1000.0f / (float)ticks << '\n';
		std::cout << "Update%: " << timeSum / (glfwGetTime() - LoopBeginTime) * 100.0f << "%\n";
		std::cout << "Unstable ms/frame: " << deltaTime * 1000.0f << ".\n";
		std::cout << "Badany: " << Transform::test() / (glfwGetTime() - LoopBeginTime) * 100.0f << "%.\n";
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

	for (int i = 0; i < static_cast<int>(Scenes.size()); i++)
		Scenes[i]->RootActor->HandleInputsAll(Window);

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

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (!mouseController)
		return;

	glm::ivec2 halfWindowSize;
	glfwGetWindowSize(window, &halfWindowSize.x, &halfWindowSize.y);
	halfWindowSize /= 2;
	glfwSetCursorPos(window, (double)halfWindowSize.x, (double)halfWindowSize.y);
	mouseController->RotateWithMouse(glm::vec2(xpos - (float)halfWindowSize.x, ypos - (float)halfWindowSize.y));
}

void loadTransform(std::stringstream& filestr, Transform* transform)
{
	std::string input;

	for (int transformField = 0; transformField < 4; transformField++)
	{
		glm::vec3 value(0.0f);
		bool skipped = false;

		for (int i = 0; i < 3; i++)
		{
			size_t pos = (size_t)filestr.tellg();
			filestr >> input;
			if (input == "skip")
			{
				skipped = true;
				break;
			}

			else if (input == "end")
			{
				filestr.seekg(pos);	//go back to the position before the component's end, so other functions aren't alarmed
				return;
			}

			value[i] = (float)atof(input.c_str());
		}

		if (!skipped)	//be sure to not override the default value if we skip a field; f.e. scale default value is vec3(1.0), not vec3(0.0)
			transform->Set(transformField, value);
	}
}