#define STB_IMAGE_IMPLEMENTATION
#define CEREAL_LOAD_FUNCTION_NAME Load
#define CEREAL_SAVE_FUNCTION_NAME Save
#define CEREAL_SERIALIZE_FUNCTION_NAME Serialize
#include <scene/Actor.h>
#include <editor/GameEngineEngineEditor.h>
#include <scene/TextComponent.h>
#include <scene/CameraComponent.h>
#include <scene/ModelComponent.h>
#include <scene/LightProbeComponent.h>
#include <UI/UICanvasField.h>
#include <scene/GunActor.h>
#include <scene/Controller.h>
#include <input/InputDevicesStateRetriever.h>
#include <scene/UIButtonActor.h>

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
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	
	SystemWindow* programWindow = glfwCreateWindow(800, 600, "Eight Ball Pool", nullptr, nullptr);

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


	EightBallPoolGame game(programWindow, GameSettings());
	game.SetupMainMenu();

	game.SetActiveScene(game.GetScene("GEE_Main_Menu"));
	game.PassMouseControl(nullptr);

	game.PreGameLoop();

	float deltaTime = 0.0f;
	float lastUpdateTime = glfwGetTime();
	bool endGame = false;

	do
	{
		deltaTime = glfwGetTime() - lastUpdateTime;
		lastUpdateTime = glfwGetTime();

		if (Material* found = game.GetRenderEngineHandle()->FindMaterial("GEE_Engine_Title").get())	//dalem tu na chama; da sie to zrobic duzo ladniej ale musialbym zmodyfikowac klase Interpolator zeby obslugiwala lambdy ale mi sie nie chce
			found->SetColor(hsvToRgb(Vec3f(glm::mod((float)glfwGetTime() * 10.0f, 360.0f), 0.6f, 0.6f)));

		endGame = game.GameLoopIteration(1.0f / 60.0f, deltaTime);
	} while (!endGame);

	return 0;
}