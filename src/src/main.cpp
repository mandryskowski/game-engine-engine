#define STB_IMAGE_IMPLEMENTATION

#include <editor/GameEngineEngineEditor.h>
#include <scene/LightProbeComponent.h>
#include <UI/UICanvasField.h>
#include <scene/GunActor.h>
#include <scene/Controller.h>
#include <editor/EditorActions.h>


using namespace GEE;

#ifdef GEE_OS_WINDOWS
extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}

extern "C"
{
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif //GEE_OS_WINDOWS

Editor::EditorManager* Editor::EditorEventProcessor::EditorHandle = nullptr;


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
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GEE_GL_VERSION_MAJOR);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GEE_GL_VERSION_MINOR);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	SystemWindow* programWindow = glfwCreateWindow(800, 600, "Game Engine Engine", nullptr, nullptr);

	if (!programWindow)
	{
		std::cerr << "CRITICAL ENGINE ERROR: Cannot create GLFW window. Please check that your system supports OpenGL " <<
			GEE_GL_VERSION_MAJOR << "." << GEE_GL_VERSION_MINOR << ".\n";
		return -1;
	}
	
	glfwMakeContextCurrent(programWindow);
	
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		std::cerr << "CRITICAL ENGINE ERROR: Cannot load GL functions.";
		return -1;
	}

	GameSettings settings;
	
	Editor::GameEngineEngineEditor editor(programWindow, settings);
	editor.SetupMainMenu();

	editor.SetActiveScene(editor.GetScene("GEE_Main_Menu"));
	editor.PassMouseControl(nullptr);

	if (!projectFilepathArgument.empty())
		editor.LoadProject(projectFilepathArgument);

	editor.PreGameLoop();

	auto lastUpdateTime = glfwGetTime();
	bool endGame = false;

	while (!endGame)
	{
		const Time deltaTime = glfwGetTime() - lastUpdateTime;
		lastUpdateTime = glfwGetTime();

		endGame = editor.GameLoopIteration(1.0 / 60.0, deltaTime);
	}

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