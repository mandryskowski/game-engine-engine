#define STB_IMAGE_IMPLEMENTATION
#define CEREAL_LOAD_FUNCTION_NAME Load
#define CEREAL_SAVE_FUNCTION_NAME Save
#define CEREAL_SERIALIZE_FUNCTION_NAME Serialize
#include <editor/GameEngineEngineEditor.h>
#include <utility/Log.h>
 
using namespace GEE;
Editor::EditorManager* Editor::EditorEventProcessor::EditorHandle = nullptr;

class EightBallPoolGame : public Editor::GameEngineEngineEditor
{
public:
	EightBallPoolGame(SystemWindow* window, const GameSettings& settings) : GameEngineEngineEditor(window, settings) {}
};

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GEE_GL_VERSION_MAJOR);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	
	SystemWindow* programWindow = glfwCreateWindow(800, 600, "Eight Ball Pool", nullptr, nullptr);

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


	EightBallPoolGame game(programWindow, GameSettings());
	game.SetupMainMenu();

	game.SetActiveScene(game.GetScene("GEE_Main_Menu"));
	game.PassMouseControl(nullptr);

	game.PreGameLoop();

	auto lastUpdateTime = glfwGetTime();
	bool endGame = false;

	while (!endGame)
	{
		const auto deltaTime = glfwGetTime() - lastUpdateTime;
		lastUpdateTime = glfwGetTime();

		endGame = game.GameLoopIteration(1.0 / 60.0, deltaTime);
	}

	return 0;
}