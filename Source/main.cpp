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

class Obiekt
{
	glm::vec3 Pozycja;
	
public:
	Obiekt(glm::vec3 poz)
	{
		Pozycja = poz;
	}
	glm::vec3 GetPozycja()
	{
		return Pozycja;
	}
};

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

/*
	ENGINE TEXTURE UNITS
	0-9 free
	10 2D shadow map array
	11 3D shadow map array
	12+ free
*/