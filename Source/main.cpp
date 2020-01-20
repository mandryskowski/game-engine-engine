#define STB_IMAGE_IMPLEMENTATION
#include "Game.h"

class KulkiGame : public Game
{
public:
	KulkiGame(GLFWwindow *window) :
		Game(window)
	{
		Init();
	} 
	virtual void Init()
	{
		LoadLevel("level.txt");
	}
};

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	GLFWwindow* programWindow = glfwCreateWindow(1920, 1080, "c00lki", nullptr, nullptr);

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

	KulkiGame game(programWindow);
	game.Run();
	
	return 0;
}