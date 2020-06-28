#define STB_IMAGE_IMPLEMENTATION
#include "Game.h"

class KulkiGame : public Game
{
public:
	KulkiGame(GLFWwindow* window) :
		Game()
	{
		Init(window);
	} 
	virtual void Init(GLFWwindow* window) override
	{
		Game::Init(window);
		LoadLevel("level.txt");
	}
};

int main()
{
	glm::quat myRot = toQuat(glm::vec3(0.0f, 20.0f, 0.0f));
	glm::quat shapeRot = toQuat(glm::vec3(0.0f, 45.0f, 0.0f));
	myRot = shapeRot * myRot;
	printVector(myRot * glm::inverse(shapeRot), "test");
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

	KulkiGame game(programWindow);
	game.Run();
	return 0;
}

/*
	ENGINE TEXTURE UNITS
	0-9 free
	10 2D shadow map array
	11 3D shadow map array
	12+ free
*/