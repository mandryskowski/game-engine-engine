#include "Game.h"
#include <thread>

CameraComponent* mouseContextCamera = nullptr;

Game::Game(GLFWwindow *window) :
	RootActor("Root"), Settings("Settings.ini")
{
	Window = window;
	glfwSetWindowSize(Window, Settings.WindowSize.x, Settings.WindowSize.y);
	glfwSetWindowTitle(Window, Settings.WindowTitle.c_str());
	if (Settings.bWindowFullscreen)
		glfwSetWindowMonitor(Window, glfwGetPrimaryMonitor(), 0, 0, Settings.WindowSize.x, Settings.WindowSize.y, 60);

	if (!Settings.bVSync)
		glfwSwapInterval(0);

	glDisable(GL_MULTISAMPLE);

	DepthBufferData* sharedDepthStencil = new DepthBufferData(GL_TEXTURE_2D, GL_DEPTH24_STENCIL8, GL_NEAREST, GL_NEAREST);	//we share this buffer between the geometry and main framebuffer - we want the deferred-rendered objects depth to influence light volumes&forward rendered objects
	GFramebuffer.Load(Settings.WindowSize, std::vector<ColorBufferData> {
		ColorBufferData(GL_TEXTURE_2D, GL_RGB16F, GL_FLOAT, GL_NEAREST, GL_NEAREST),	//gPosition texture
		ColorBufferData(GL_TEXTURE_2D, GL_RGB16F, GL_FLOAT, GL_NEAREST, GL_NEAREST),	//gNormal texture
		ColorBufferData(GL_TEXTURE_2D, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST)},		//gAlbedoSpec texture
		sharedDepthStencil); //depth+stencil
	MainFramebuffer.Load(Settings.WindowSize, std::vector<ColorBufferData> {ColorBufferData(GL_TEXTURE_2D, GL_RGBA16F, GL_FLOAT), ColorBufferData(GL_TEXTURE_2D, GL_RGBA16F, GL_FLOAT, GL_LINEAR, GL_LINEAR)}, sharedDepthStencil);

	ActiveCamera = nullptr;
	
	MatricesBuffer.Generate(0, sizeof(glm::mat4) * 2); 

	RenderEng.SetupShadowmaps(glm::uvec2(1024));
	RenderEng.SetupAmbientOcclusion(Settings.WindowSize, 64);

	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPos(Window, (double)Settings.WindowSize.x / 2.0, (double)Settings.WindowSize.y / 2.0);
	glfwSetCursorPosCallback(Window, cursorPosCallback);

	GamePostprocess.LoadSettings(&Settings);
}

void Game::LoadLevel(std::string path)
{
	std::ifstream file(path);
	std::stringstream filestr;
	filestr << file.rdbuf();

	std::string type;
	Actor* currentActor = nullptr;
	while (filestr >> type)
	{
		if (type == "newactor")
		{
			std::string actorName;
			filestr >> actorName;

			currentActor = new Actor(actorName);
			AddActorToScene(currentActor);
		}
		if (type == "newcomp")
		{
			if (!currentActor)
			{
				std::cerr << "ERROR! Component defined without an actor\n";
				break;
			}
			LoadComponentData(filestr, currentActor);
		}
		if (type == "materialsfile")
		{
			std::string path;
			filestr >> path;

			RenderEng.LoadMaterials(path);
		}
	}

	RenderEng.LoadModels();
}

Component* Game::LoadComponentData(std::stringstream& filestr, Actor* currentActor)
{
	///////////////////////////////1. Load the type and the name of the current component
	Component* comp = nullptr;
	std::string type, name;
	filestr >> type >> name;

	///////////////////////////////2. Load info about its position in hierarchy
	
	std::string familyWord, parentName;
	familyWord = getNextWord(filestr);
	if (familyWord == "child")
		filestr >> familyWord >> parentName;	///skip familyword and load parentname
	else if (familyWord == "root")
		filestr >> familyWord;	///skip familyword
	else
		familyWord = "attachToRoot";

	///////////////////////////////3. Load component's data

	if (type == "mesh")
	{
		MeshComponent* meshPtr = nullptr;
		std::string loadType, materialName;
		filestr >> loadType;

		if (loadType == "load")
		{
			std::string path;
			filestr >> path;
			meshPtr = new MeshComponent(name);
			meshPtr->LoadFromFile(path, &materialName);
		}
		else if (loadType == "copy")
		{
			std::string targetName;
			filestr >> targetName;

			MeshComponent* target = RenderEng.FindMesh(targetName);
			if (!target)
			{
				std::cerr << "Can't find mesh " << targetName << '\n';
				return nullptr;
			}

			meshPtr = new MeshComponent(*target);
			meshPtr->SetName(name);
		}

		if (isNextWordEqual(filestr, "material"))
			filestr >> materialName;

		if (!materialName.empty())
		{
			Material* material = RenderEng.FindMaterial(materialName);

			if (!material)
			{
				std::cerr << "Can't find material " << materialName << '\n';
				return nullptr;
			}
			meshPtr->SetMaterial(RenderEng.FindMaterial(materialName));
		}

		AddMeshToScene(meshPtr);
		comp = meshPtr;
	}
	else if (type == "model")
	{
		ModelComponent* modelPtr = nullptr;
		std::string path, materialName;

		filestr >> path;
		modelPtr = new ModelComponent(name);
		modelPtr->SetFilePath(path);

		if (isNextWordEqual(filestr, "material"))
			filestr >> materialName;

		RenderEng.AddModel(modelPtr);
		comp = modelPtr;
	}
	else if (type == "light")
	{
		std::string lightType;
		filestr >> lightType;
		glm::mat4 projection;
		if (lightType == "point" || lightType == "spot")
			projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		else
			projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 10.0f);

		LightComponent* lightPtr = new LightComponent(name, toLightType(lightType), Lights.size(), 10.0f, projection);
		for (unsigned int vector = 0; vector < 3; vector++)
			for (unsigned int axis = 0; axis < 3; axis++)
				filestr >> (*lightPtr)[vector][axis];	//TODO: sprawdzic czy sie wywala

		glm::vec3 additionalData(0.0f);
		for (unsigned int i = 0; i < 3; i++)
			filestr >> additionalData[i];
		lightPtr->SetAdditionalData(additionalData);

		lightPtr->CalculateLightRadius();

		AddLightToScene(lightPtr);
		comp = lightPtr;
	}
	else if (type == "camera")
	{
		float speedPerSec;
		filestr >> speedPerSec;

		comp = new CameraComponent(name, Settings.WindowSize, &CollisionEng, speedPerSec);

		if (isNextWordEqual(filestr, "active"))
			BindActiveCamera(dynamic_cast<CameraComponent*>(comp));
	}
	else if (type == "collision")
	{
		CollisionComponent* colPtr = nullptr;
		std::string shapeType;
		filestr >> shapeType;

		if (shapeType == "cube")
			colPtr = new BBox(name);
		else if (shapeType == "sphere")
			colPtr = new BSphere(name);
		else
			return nullptr;

		std::string flipCollision;
		filestr >> flipCollision;
		if (flipCollision == "true")
			colPtr->bFlipCollisionSide = true;

		CollisionEng.AddCollisionInstance(colPtr);
		comp = colPtr;
	}
	else
		return nullptr;

	///////////////////////////////4. Load its tranformations

	if (isNextWordEqual(filestr, "transform"))
		loadTransform(filestr, comp->GetTransform());

	///////////////////////////////5.  After the component has been created, we can take care of its hierarchy stuff (it was loaded in 2.)

	if (familyWord == "child")
	{
		Component* parent = currentActor->GetRoot()->SearchForComponent(parentName);
		if (parent)
			parent->AddComponent(comp);
		else
			std::cerr << "ERROR! Can't find target parent " << parentName << "!\n";
	}
	else if (familyWord == "root")
		currentActor->ReplaceRoot(comp);
	else if (familyWord == "attachToRoot")
		currentActor->AddComponent(comp);


	//////Check for an error

	std::string lastWord;
	filestr >> lastWord;
	if (lastWord != "end")
		std::cerr << "ERROR: There is no ''end'' after component's " << name << " definition! Detected word: " << lastWord << ".\n";

	return comp;
}

void Game::SetupLights()
{
	LightsBuffer.Generate(1, sizeof(glm::vec4) * 2 + Lights.size() * 192);
	LightsBuffer.SubData1i(Lights.size(), 0);

	UpdateLightUniforms();
}

void Game::UpdateLightUniforms()
{
	LightsBuffer.offsetCache = sizeof(glm::vec4) * 2;
	for (unsigned int i = 0; i < Lights.size(); i++)
		Lights[i]->UpdateUBOData(&LightsBuffer, ActiveCamera->GetTransform()->GetWorldTransform().GetViewMatrix());
}

void Game::BindActiveCamera(CameraComponent* cam)
{
	if (!cam)
		return;
	ActiveCamera = cam;
	mouseContextCamera = cam;
	MatricesBuffer.SubDataMatrix4fv(ActiveCamera->GetProjectionMat(), sizeof(glm::mat4));
}

void Game::AddActorToScene(Actor* actor)
{
	RootActor.AddChild(actor);
}

void Game::AddLightToScene(LightComponent* light)
{
	Lights.push_back(light);
}
void Game::AddMeshToScene(MeshComponent* mesh)
{
	RenderEng.AddMesh(mesh);
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
	const float timeStep = 1.0f / 30.0f;
	float deltaTime = 0.0f;
	float timeAccumulator = 0.0f;

	float beginning = glfwGetTime();

	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();
		if (glfwGetKey(Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(Window, true);

		deltaTime = (float)glfwGetTime() - lastUpdateTime;
		lastUpdateTime = (float)glfwGetTime();

		if (deltaTime > 0.25f)
			deltaTime = 0.25f;

		float fraction = fmod(glfwGetTime(), 1.0f);
		if (fraction < 0.1f && !wasSecondFPS)
		{
			std::cout << "******Frames per second: " << (float)ticks / (glfwGetTime() - beginning) << "			Milliseconds per frame: " << (glfwGetTime() - beginning) * 1000.0f / (float)ticks << '\n';
			std::cout << "Update%: " << timeSum / (glfwGetTime() - beginning) * 100.0f << "%\n";
			std::cout << "Matrix%: " << Transform::test() / (glfwGetTime() - beginning) * 100.0f << "%\n";
			wasSecondFPS = true;
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

		Render();
		ticks++;
	}
}

void Game::Update(float deltaTime)
{
	RootActor.UpdateAll(deltaTime);
	MatricesBuffer.SubDataMatrix4fv(ActiveCamera->GetTransform()->GetWorldTransform().GetViewMatrix(),0);
	LightsBuffer.SubData4fv(ActiveCamera->GetTransform()->GetWorldTransform().PositionRef, 16);
}
bool wasSecond = false;
float pBegin = 0.0f;
void Game::Render()
{
	Transform camWorld = ActiveCamera->GetTransform()->GetWorldTransform();
	glm::mat4 view = camWorld.GetViewMatrix();
	glm::mat4 projection = ActiveCamera->GetProjectionMat();
	glm::mat4 VP = ActiveCamera->GetVP(&camWorld);
	RenderInfo info(&view, &projection, &VP);

	////////////////////1. Shadow maps pass
	RenderEng.RenderShadowMaps(Lights);
	//std::cout << "Zjadlo mi " << (time2 - time1) * 1000.0f << "ms.\n";

	////////////////////2. Geometry pass
	GFramebuffer.Bind();
	glViewport(0, 0, Settings.WindowSize.x, Settings.WindowSize.y);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Shader* gShader = RenderEng.GetShader("geometry");
	gShader->Use();
	gShader->Uniform3fv("camPos", camWorld.PositionRef);
	
	RenderEng.RenderScene(info, gShader, MeshType::MESH_ALL, true);
	
	////////////////////2.5 SSAO pass

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, GFramebuffer.GetColorBuffer(0).OpenGLBuffer);	//gPosition
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, GFramebuffer.GetColorBuffer(1).OpenGLBuffer);	//gNormal

	RenderEng.RenderSSAO(info, Settings.WindowSize);
	GamePostprocess.GaussianBlur(RenderEng.GetSSAOTex(), 4);


	////////////////////3. Lighting pass
	UpdateLightUniforms();
	MainFramebuffer.Bind();
	glClearBufferfv(GL_COLOR, 0, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.0f)));
	glClearBufferfv(GL_COLOR, 1, glm::value_ptr(glm::vec3(0.0f)));

	for (int i = 0; i < 3; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, GFramebuffer.GetColorBuffer(i).OpenGLBuffer);
	}
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, RenderEng.GetSSAOTex());
	RenderEng.RenderLightVolumes(info, Lights, Settings.WindowSize);

	////////////////////3.5 Forward rendering pass
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	
	Shader* forwardShader = RenderEng.GetShader("forward");
	//RenderEng.RenderScene(info, forwardShader, MeshType::MESH_FORWARD, true);

	
	/*glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	std::vector <std::pair<EngineObjectTypes, const Transform*>> collisionDebugData = CollisionEng.GetCollisionInstancesDebugData();		//Uncomment to debug bounding boxes.
	RenderEng.RenderEngineObjects(info, collisionDebugData);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);*/
	

	////////////////////4. Postprocessing pass (Blur + Tonemapping & Gamma Correction)
	GamePostprocess.Render(MainFramebuffer.GetColorBuffer(0).OpenGLBuffer, GamePostprocess.GaussianBlur(MainFramebuffer.GetColorBuffer(1).OpenGLBuffer, 10));
	//GamePostprocess.Render(RenderEng.GetSSAOTex(), 0);

	if (pBegin == 0.0f)
		pBegin = glfwGetTime();
	float time1, time2;
	time1 = glfwGetTime() - pBegin;
	glfwSwapBuffers(Window);
	time2 = glfwGetTime() - pBegin;
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
	std::string value;
	for (unsigned int vector = 0; vector < 4; vector++)
	{
		for (unsigned int i = 0; i < 3; i++)
		{
			size_t pos = (size_t)filestr.tellg();
			filestr >> value;
			if (value == "skip")
				break;
			else if (value == "true")
			{
				transform->bConstrain = true;
				continue;
			}
			else if (value == "end")
			{
				filestr.seekg(pos);	//go back to the position before the component's end, so other functions aren't alarmed
				return;
			}
			(*transform)[vector][i] = (float)atof(value.c_str());
		}
	}

	if (isNextWordEqual(filestr, "true"))
		transform->bConstrain = true;
}