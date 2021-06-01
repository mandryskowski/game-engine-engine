#define STB_IMAGE_IMPLEMENTATION
#define CEREAL_LOAD_FUNCTION_NAME Load
#define CEREAL_SAVE_FUNCTION_NAME Save
#define CEREAL_SERIALIZE_FUNCTION_NAME Serialize
#include <editor/GameEngineEngineEditor.h>
#include <scene/TextComponent.h>
#include <scene/CameraComponent.h>
#include <scene/UIWindowActor.h>
#include <scene/ModelComponent.h>
#include <scene/LightComponent.h>
#include <scene/LightProbeComponent.h>
#include <scene/UIInputBoxActor.h>
#include <UI/UICanvasField.h>
#include <UI/UIListActor.h>
#include <scene/GunActor.h>
#include <scene/PawnActor.h>
#include <scene/Controller.h>
#include <input/InputDevicesStateRetriever.h>
#include <whereami.h>
#include <scene/BoneComponent.h>
#include <map>

using namespace GEE;

extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}

extern "C"
{
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

EditorManager* EditorEventProcessor::EditorHandle = nullptr;

int main(int argc, char** argv)
{
	std::string programFilepath;	//do not rely on this; if called from cmd, for example, it may not actually contain the program filepath
	std::string projectFilepathArgument;
	for (int i = 0; i < argc; i++)
	{
		std::cout << argv[i] << '\n';
		switch (i)
		{
		case 0: programFilepath = argv[i]; break;
		case 1: projectFilepathArgument = argv[i]; break;
		}
	}
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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
	editor.SetupMainMenu();

	editor.SetActiveScene(editor.GetScene("GEE_Main_Menu"));
	editor.PassMouseControl(nullptr);

	if (!projectFilepathArgument.empty())
		editor.LoadProject(projectFilepathArgument);

	editor.PreGameLoop();

	float deltaTime = 0.0f;
	float lastUpdateTime = glfwGetTime();
	bool endGame = false;

	do
	{
		deltaTime = glfwGetTime() - lastUpdateTime;
		lastUpdateTime = glfwGetTime();

		if (Material* found = editor.GetRenderEngineHandle()->FindMaterial("GEE_Engine_Title").get())	//dalem tu na chama; da sie to zrobic duzo ladniej ale musialbym zmodyfikowac klase Interpolator zeby obslugiwala lambdy ale mi sie nie chce
			found->SetColor(hsvToRgb(Vec3f(glm::mod((float)glfwGetTime() * 10.0f, 360.0f), 0.6f, 0.6f)));

		endGame = editor.GameLoopIteration(1.0f / 60.0f, deltaTime);
	} while (!endGame);

	editor.SaveProject();

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