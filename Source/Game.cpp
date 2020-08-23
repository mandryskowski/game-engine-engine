#include "Game.h"
#include "LightComponent.h"
#include "ModelComponent.h"
#include "CameraComponent.h"
#include "FileLoader.h"
#include "LightProbe.h"
#include <thread>

bool PrimitiveDebugger::bDebugMeshTrees = false;
bool PrimitiveDebugger::bDebugProbeLoading = false;
bool PrimitiveDebugger::bDebugCubemapFromTex = false;
bool PrimitiveDebugger::bDebugFramebuffers = false;
bool PrimitiveDebugger::bDebugHierarchy = false;

CameraComponent* mouseContextCamera = nullptr;

void APIENTRY debugOutput(GLenum source,
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

Game::Game(const ShadingModel& shading) :
	RootActor(static_cast<GameManager*>(this), "Root"), Settings(std::make_unique<GameSettings>("Settings.ini")), AudioEng(static_cast<GameManager*>(this)), RenderEng(static_cast<GameManager*>(this), shading), PhysicsEng(&DebugMode), Shading(shading), PreviousFrameView(glm::mat4(1.0f))
{
	glDisable(GL_MULTISAMPLE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	glDebugMessageCallbackARB(debugOutput, nullptr);
	glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

	ActiveCamera = nullptr;

	DebugMode = false;
}

void Game::Init(GLFWwindow* window)
{
	Window = window;
	glfwSetWindowSize(Window, Settings->WindowSize.x, Settings->WindowSize.y);
	glfwSetWindowTitle(Window, Settings->WindowTitle.c_str());
	if (Settings->bWindowFullscreen)
		glfwSetWindowMonitor(Window, glfwGetPrimaryMonitor(), 0, 0, Settings->WindowSize.x, Settings->WindowSize.y, 60);

	if (!Settings->bVSync)
		glfwSwapInterval(0);

	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPos(Window, (double)Settings->WindowSize.x / 2.0, (double)Settings->WindowSize.y / 2.0);
	glfwSetCursorPosCallback(Window, cursorPosCallback);


	MatricesBuffer.Generate(0, sizeof(glm::mat4) * 2);

	RenderEng.Init(Settings->WindowSize);
	PhysicsEng.Init();
	Searcher.Setup(&RenderEng, &AudioEng, &RootActor);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Game::LoadLevel(std::string path)
{
	EngineDataLoader::LoadLevelFile(static_cast<GameManager*>(this), path);

	PhysicsEng.Setup();
	std::cout << "Collision objects setup finished.\n";

	RootActor.Setup(&Searcher);
	std::cout << "Data loading finished.\n";

	if (PrimitiveDebugger::bDebugHierarchy)
		RootActor.DebugHierarchy();
}

void Game::BindActiveCamera(CameraComponent* cam)
{
	if (!cam)
		return;
	ActiveCamera = cam;
	mouseContextCamera = cam;
	MatricesBuffer.SubDataMatrix4fv(ActiveCamera->GetProjectionMat(), sizeof(glm::mat4));
	AudioEng.SetListenerTransformPtr(&ActiveCamera->GetTransform());
}

std::shared_ptr<Actor> Game::AddActorToScene(std::shared_ptr<Actor> actor)
{
	RootActor.AddChild(actor);
	return actor;
}

SearchEngine* Game::GetSearchEngine()
{
	return &Searcher;
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

const GameSettings* Game::GetGameSettings()
{
	return Settings.get();
}

unsigned int ticks = 1;
bool wasSecondFPS = false;
float timeSum = 0.0f;
void Game::Run()
{
	if (!ActiveCamera)
	{
		std::cerr << "ERROR: Camera not bound!\n";
		return;
	}

	RenderEng.PreRenderPass();

	float lastUpdateTime = (float)glfwGetTime();
	const float timeStep = 1.0f / 60.0f;
	float deltaTime = 0.0f;
	float timeAccumulator = 0.0f;

	float beginning = (float)glfwGetTime();

	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();

		if (glfwGetKey(Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(Window, true);

		deltaTime = (float)glfwGetTime() - lastUpdateTime;
		lastUpdateTime = (float)glfwGetTime();

		if (deltaTime > 0.25f)
			deltaTime = 0.25f;

		float fraction = fmod((float)glfwGetTime(), 1.0f);
		if (fraction < 0.1f && !wasSecondFPS)
		{
			std::cout << "******Frames per second: " << (float)ticks / (glfwGetTime() - beginning) << "			Milliseconds per frame: " << (glfwGetTime() - beginning) * 1000.0f / (float)ticks << '\n';
			std::cout << "Update%: " << timeSum / (glfwGetTime() - beginning) * 100.0f << "%\n";
			std::cout << "Unstable ms/frame: " << deltaTime * 1000.0f << ".\n";
			std::cout << "Badany: " << Transform::test() / (glfwGetTime() - beginning) * 100.0f << "%.\n";
			wasSecondFPS = true;
			ticks = 0;
			timeSum = 0.0f;
			beginning = glfwGetTime();
		}
		else if (fraction > 0.9f)
			wasSecondFPS = false;

		if (GLenum error = glGetError())
		{
			std::cout << "OpenGL Error: " << error << ".\n";
		}

		RootActor.HandleInputsAll(Window, deltaTime);

		timeAccumulator += deltaTime;

		while (timeAccumulator >= timeStep)
		{
			//RootActor.GetTransform()->Rotation.y += deltaTime;
			Update(timeStep);
			timeAccumulator -= timeStep;
		}

		//RenderEng.FindShader("Geometry")->Use();
		//RenderEng.FindShader("Geometry")->Uniform1f("velocityScale", timeStep / deltaTime);
		Render();
		ticks++;
	}
}

void Game::Update(float deltaTime)
{
	PhysicsEng.Update(deltaTime);
	RootActor.UpdateAll(deltaTime);
	MatricesBuffer.SubDataMatrix4fv(ActiveCamera->GetTransform().GetWorldTransform().GetViewMatrix(), 0);
	AudioEng.Update();
}
float pBegin = 0.0f;
bool DUPA = false;
void Game::Render()
{
	Transform camWorld = ActiveCamera->GetTransform().GetWorldTransform();
	glm::vec3 camPos = ActiveCamera->GetTransform().GetWorldTransform().PositionRef;
	glm::mat4 view = camWorld.GetViewMatrix();
	glm::mat4 projection = ActiveCamera->GetProjectionMat();
	glm::mat4 VP = ActiveCamera->GetVP(&camWorld);
	RenderInfo info(&view, &projection, &VP);

	info.camPos = &camPos;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	RenderEng.FullRender(info);
	
	glfwSwapBuffers(Window);
}


void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	glm::ivec2 halfWindowSize;
	glfwGetWindowSize(window, &halfWindowSize.x, &halfWindowSize.y);
	halfWindowSize /= 2;
	glfwSetCursorPos(window, (double)halfWindowSize.x, (double)halfWindowSize.y);
	mouseContextCamera->RotateWithMouse(glm::vec2(xpos - (float)halfWindowSize.x, ypos - (float)halfWindowSize.y));
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