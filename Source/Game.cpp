#include "Game.h"
#include "LightComponent.h"
#include "ModelComponent.h"
#include "CameraComponent.h"
#include "FileLoader.h"
#include <thread>

CameraComponent* mouseContextCamera = nullptr;

Game::Game() :
	RootActor(static_cast<GameManager*>(this), "Root"), Settings(std::make_unique<GameSettings>("Settings.ini")), AudioEng(static_cast<GameManager*>(this)), RenderEng(static_cast<GameManager*>(this)), PhysicsEng(&DebugMode)
{
	glDisable(GL_MULTISAMPLE);

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

	std::shared_ptr<GEE_FB::DepthStencilBuffer> sharedDepthStencil = std::make_shared<GEE_FB::DepthStencilBuffer>(GL_TEXTURE_2D, GL_DEPTH24_STENCIL8, GL_UNSIGNED_INT_24_8, GL_NEAREST, GL_NEAREST);	//we share this buffer between the geometry and main framebuffer - we want deferred-rendered objects depth to influence light volumes&forward rendered objects
	std::shared_ptr<GEE_FB::ColorBuffer> velocityBuffer = std::make_shared<GEE_FB::ColorBuffer>(GL_TEXTURE_2D, GL_RGB16F, GL_FLOAT, GL_NEAREST, GL_NEAREST);
	std::vector<std::shared_ptr<GEE_FB::ColorBuffer>> gColorBuffers = {
		std::make_shared<GEE_FB::ColorBuffer>(GL_TEXTURE_2D, GL_RGB16F, GL_FLOAT, GL_NEAREST, GL_NEAREST),	//gPosition texture
		std::make_shared<GEE_FB::ColorBuffer>(GL_TEXTURE_2D, GL_RGB16F, GL_FLOAT, GL_NEAREST, GL_NEAREST),	//gNormal texture
		std::make_shared<GEE_FB::ColorBuffer>(GL_TEXTURE_2D, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST)		//gAlbedoSpec texture
		//std::make_shared<GEE_FB::ColorBuffer>(GL_TEXTURE_2D, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST)		//roughness, metallic, ao texture
	};
	if (Settings->IsVelocityBufferNeeded())
		gColorBuffers.push_back(velocityBuffer);
	GFramebuffer.Load(Settings->WindowSize, gColorBuffers, sharedDepthStencil);

	std::vector<std::shared_ptr<GEE_FB::ColorBuffer>> colorBuffers;
	colorBuffers.push_back(std::make_shared<GEE_FB::ColorBuffer>(GL_TEXTURE_2D, GL_RGBA16F, GL_FLOAT, GL_LINEAR, GL_LINEAR));	//add color buffer
	if (Settings->bBloom)
		colorBuffers.push_back(std::make_shared<GEE_FB::ColorBuffer>(GL_TEXTURE_2D, GL_RGBA16F, GL_FLOAT, GL_LINEAR, GL_LINEAR));	//add blur buffer
	if (Settings->IsVelocityBufferNeeded())
		colorBuffers.push_back(velocityBuffer);
	MainFramebuffer.Load(Settings->WindowSize, colorBuffers, sharedDepthStencil);	//color and blur buffers

	MatricesBuffer.Generate(0, sizeof(glm::mat4) * 2);

	RenderEng.Init();
	PhysicsEng.Init();
	Searcher.Setup(&RenderEng, &AudioEng, &RootActor);
	GamePostprocess.Init(Settings.get());

	RenderEng.SetupShadowmaps();
}

void Game::LoadLevel(std::string path)
{
	EngineDataLoader::LoadLevelFile(static_cast<GameManager*>(this), path);

	PhysicsEng.Setup();
	std::cout << "Collision objects setup finished.\n";

	RootActor.Setup(&Searcher);
	std::cout << "Data loading finished.\n";

	RootActor.DebugHierarchy();
}


void Game::SetupLights()
{
	LightsBuffer.Generate(1, sizeof(glm::vec4) * 2 + Lights.size() * 192);
	LightsBuffer.SubData1i((int)Lights.size(), (size_t)0);

	UpdateLightUniforms();
}

void Game::UpdateLightUniforms()
{
	LightsBuffer.offsetCache = sizeof(glm::vec4) * 2;
	for (unsigned int i = 0; i < Lights.size(); i++)
		Lights[i]->UpdateUBOData(&LightsBuffer);
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

void Game::AddLightToScene(LightComponent* light)
{
	Lights.push_back(light);
}

int Game::GetAvailableLightIndex()
{
	return Lights.size();
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

PostprocessManager* Game::GetPostprocessHandle()
{
	return &GamePostprocess;
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

	SetupLights();
	 
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

		RootActor.HandleInputsAll(Window, deltaTime);

		timeAccumulator += deltaTime;

		while (timeAccumulator >= timeStep)
		{
			//RootActor.GetTransform()->Rotation.y += deltaTime;
			Update(timeStep);
			timeAccumulator -= timeStep;
		}

		RenderEng.FindShader("geometry")->Use();
		RenderEng.FindShader("geometry")->Uniform1f("velocityScale", timeStep / deltaTime);
		Render();
		ticks++;
	}
}

void Game::Update(float deltaTime)
{
	PhysicsEng.Update(deltaTime);
	RootActor.UpdateAll(deltaTime);
	MatricesBuffer.SubDataMatrix4fv(ActiveCamera->GetTransform().GetWorldTransform().GetViewMatrix(),0);
	LightsBuffer.SubData4fv(ActiveCamera->GetTransform().GetWorldTransform().PositionRef, 16);
	AudioEng.Update();
}
float pBegin = 0.0f;
bool DUPA = false;
void Game::Render()
{
	Transform camWorld = ActiveCamera->GetTransform().GetWorldTransform();
	glm::mat4 view = camWorld.GetViewMatrix();
	glm::mat4 projection = ActiveCamera->GetProjectionMat();
	glm::mat4 VP = ActiveCamera->GetVP(&camWorld);
	RenderInfo info(&view, &projection, &VP);
	info.MainPass = true;

	if (DebugMode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	////////////////////1. Shadow maps pass
	if (!DUPA || (Settings->ShadowLevel > SettingLevel::SETTING_MEDIUM))
	{
		RenderEng.RenderShadowMaps(Lights);
		DUPA = true;
	}
	//std::cout << "Zjadlo mi " << (time2 - time1) * 1000.0f << "ms.\n";

	////////////////////2. Geometry pass
	GFramebuffer.Bind();
	glViewport(0, 0, Settings->WindowSize.x, Settings->WindowSize.y);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Shader* gShader = RenderEng.FindShader("geometry");
	gShader->Use();
	gShader->Uniform3fv("camPos", camWorld.PositionRef);
	
	RenderEng.RenderScene(info, gShader);
	
	////////////////////2.5 SSAO pass

	const Texture* SSAOtex = nullptr;
	if (Settings->AmbientOcclusionSamples > 0)
		//SSAOtex = RenderEng.RenderSSAO(info, GFramebuffer.GetColorBuffer(0)->GetID(), GFramebuffer.GetColorBuffer(1)->GetID(), GamePostprocess);	//pass gPosition and gNormal
		SSAOtex = GamePostprocess.SSAOPass(info, GFramebuffer.GetColorBuffer(0).get(), GFramebuffer.GetColorBuffer(1).get());	//pass gPosition and gNormal
	

	////////////////////3. Lighting pass
	UpdateLightUniforms();
	MainFramebuffer.Bind();
	glClearBufferfv(GL_COLOR, 0, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.0f)));
	glClearBufferfv(GL_COLOR, 1, glm::value_ptr(glm::vec3(0.0f)));

	for (int i = 0; i < 3; i++)
	{
		GFramebuffer.GetColorBuffer(i)->Bind(i);
	}

	if (SSAOtex)
		SSAOtex->Bind(3);
	RenderEng.RenderLightVolumes(info, Lights, Settings->WindowSize);

	////////////////////3.5 Forward rendering pass
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	
	std::vector<Shader*> forwardShaders = RenderEng.GetForwardShaders();
	for (unsigned int i = 0; i < forwardShaders.size(); i++)
		RenderEng.RenderScene(info, forwardShaders[i]);

	if (DebugMode)
		PhysicsEng.DebugRender(&RenderEng, info);
	

	////////////////////4. Postprocessing pass (Blur + Tonemapping & Gamma Correction)
	GamePostprocess.Render(MainFramebuffer.GetColorBuffer(0).get(), (Settings->bBloom) ? (MainFramebuffer.GetColorBuffer(1).get()) : (nullptr), MainFramebuffer.DepthBuffer.get(), (Settings->IsVelocityBufferNeeded()) ? (GFramebuffer.GetColorBuffer(3).get()) : (nullptr));
	//GamePostprocess.Render(GFramebuffer.GetColorBuffer(3)->GetID(), 0, MainFramebuffer.DepthBuffer->GetID(), (Settings->IsVelocityBufferNeeded()) ? (GFramebuffer.GetColorBuffer(3)->GetID()) : (0));

	if (pBegin == 0.0f)
		pBegin = (float)glfwGetTime();
	float time1, time2;
	time1 = (float)glfwGetTime() - pBegin;
	glfwSwapBuffers(Window);
	time2 = (float)glfwGetTime() - pBegin;
	timeSum += time2 - time1;
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