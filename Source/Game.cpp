#include "Game.h"

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

	GLenum mainFBOTexType = GL_TEXTURE_2D;
	if (Settings.AntiAliasingSamples > 0)
	{
		glEnable(GL_MULTISAMPLE);			//this is not necessary - it should be enabled by default, but i'm not sure if i can count on it
		mainFBOTexType = GL_TEXTURE_2D_MULTISAMPLE;
	}

	MainFramebuffer.Load(Settings.WindowSize, std::vector<ColorBufferData> {ColorBufferData(true, true, mainFBOTexType), ColorBufferData(true, true, mainFBOTexType, GL_LINEAR, GL_LINEAR)}, new DepthBufferData(mainFBOTexType), Settings.AntiAliasingSamples);
	BlitFramebuffer.Load(Settings.WindowSize, std::vector<ColorBufferData> {ColorBufferData(true, true), ColorBufferData(true, true, GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR)});

	ActiveCamera = nullptr;
	
	MatricesBuffer.Generate(0, sizeof(glm::mat4) * 2);

	DebugShader.LoadShaders("debug.vs", "debug.fs");
	DebugShader.UniformBlockBinding("Matrices", 0);

	RenderEng.SetupShadowmaps(glm::uvec2(1024));

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
		Lights[i]->UpdateUBOData(&LightsBuffer);
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
	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();

		deltaTime = (float)glfwGetTime() - lastUpdateTime;
		lastUpdateTime = (float)glfwGetTime();

		if (deltaTime > 0.25f)
			deltaTime = 0.25f;

		RootActor.HandleInputsAll(Window, deltaTime);

		timeAccumulator += deltaTime;

		while (timeAccumulator >= timeStep)
		{
			//RootActor.GetTransform()->Rotation.y += deltaTime;
			Update(timeStep);
			timeAccumulator -= timeStep;
		}
		
		Render();
	}
}

void Game::Update(float deltaTime)
{
	RootActor.UpdateAll(deltaTime);
	MatricesBuffer.SubDataMatrix4fv(ActiveCamera->GetTransform()->GetWorldTransform().GetViewMatrix(), 0);
	LightsBuffer.SubData4fv(ActiveCamera->GetTransform()->GetWorldTransform().Position, 16);
	UpdateLightUniforms();		//TODO: Optimize this - it takes too much time to update every light each frame.
}

void Game::Render()
{
	float time1, time2;
	time1 = glfwGetTime();
	RenderEng.RenderShadowMaps(Lights);
	time2 = glfwGetTime();
	//std::cout << "Zjadlo mi " << (time2 - time1) * 1000.0f << "ms.\n";

	MainFramebuffer.Bind();
	glViewport(0, 0, Settings.WindowSize.x, Settings.WindowSize.y);
	glEnable(GL_DEPTH_TEST);
	glClearBufferfv(GL_COLOR, 0, glm::value_ptr(glm::vec3(0.1f, 0.15f, 0.15f)));
	glClearBufferfv(GL_COLOR, 1, glm::value_ptr(glm::vec3(0.0f)));
	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_CULL_FACE);
	
	RenderEng.RenderScene(ActiveCamera->GetTransform()->GetWorldTransform().GetViewMatrix());

	//CollisionEng.Draw(&DebugShader);		//uncomment to debug collision system

	//if (Settings.AntiAliasingSamples > 0)
	MainFramebuffer.BlitToFBO(BlitFramebuffer.GetFBO());

	GamePostprocess.Render(BlitFramebuffer.GetColorBuffer(0).OpenGLBuffer, GamePostprocess.GaussianBlur(BlitFramebuffer.GetColorBuffer(1).OpenGLBuffer, 10));

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