#define STB_IMAGE_IMPLEMENTATION
#include "Game.h"
#include <map>

class KulkiGame : public Game
{
public:
	KulkiGame(GLFWwindow* window, const ShadingModel& shading) :
		Game(shading)
	{
		Init(window);
	} 
	virtual void Init(GLFWwindow* window) override
	{
		Game::Init(window);
		LoadLevel("level.txt");
	}
};

class Base
{
public:
	Base()
	{
		method();
	}
	virtual void method()
	{
		std::cout << "jedynka\n";
	}
};

class Child : public Base
{
public:
	Child():
		Base()
	{}
	virtual void method() override
	{
		Base::method();
		std::cout << "dwojka\n";
	}
};

int main()
{
	Child xd;
	int max_v_uniforms;
	glm::quat myRot = toQuat(glm::vec3(0.0f, 20.0f, 0.0f));
	glm::quat shapeRot = toQuat(glm::vec3(0.0f, 45.0f, 0.0f));
	myRot = shapeRot * myRot;
	printVector(myRot * glm::inverse(shapeRot), "test");
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

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

	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_v_uniforms);
	std::cout << "max: " << max_v_uniforms << "\n";
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &max_v_uniforms);
	std::cout << "max2: " << max_v_uniforms << "\n";

	KulkiGame game(programWindow, ShadingModel::SHADING_PBR_COOK_TORRANCE);
	game.Run();
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