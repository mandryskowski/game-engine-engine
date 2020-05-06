#include "GunActor.h"

GunActor::GunActor(std::string name):
	Actor(name)
{
	RootComponent = new ModelComponent("GunModel");
}

void GunActor::LoadDataFromLevel(SearchEngine* searcher)
{
	if (!LoadStream)
	{
		std::cerr << "ERROR! Can't load GunActor " << Name << " data.\n";
		return;
	}

	std::string str;
	(*LoadStream) >> str;

	if (str == "FireMaterial")
	{
		(*LoadStream) >> str;
		std::cout << "Firematerial = " << searcher->FindMaterial(str) << ".\n";
	}

	ParticleMeshInst = searcher->FindModel("FireParticle")->FindMeshInstance("ENG_QUAD");
	ParticleMeshInst->GetMaterialInst()->SetInterp(new Interpolation(0.0f, 0.25f, InterpolationType::LINEAR));
	
	GunModel = searcher->FindModel("MyDoubleBarrel");
	GunModel->GetTransform()->AddInterpolator(new Interpolator<glm::vec3>(10.0f, 20.0f, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -10.0f), InterpolationType::LINEAR), "position");
	GunModel->GetTransform()->AddInterpolator(new Interpolator<glm::vec3>(10.0f, 5.0f, glm::vec3(0.0f), glm::vec3(360.0f, 0.0f, 0.0f), InterpolationType::QUADRATIC, AnimBehaviour::STOP, AnimBehaviour::EXTRAPOLATE), "rotation");
	GunModel->GetTransform()->AddInterpolator(new Interpolator<glm::vec3>(10.0f, 10.0f, glm::vec3(0.075f, 0.075f, 0.05f), glm::vec3(0.3f, 0.3f, 0.2f), InterpolationType::CONSTANT, AnimBehaviour::STOP, AnimBehaviour::STOP), "scale");

	Actor::LoadDataFromLevel(searcher);
}

void GunActor::Update(float deltaTime)
{
	ParticleMeshInst->GetMaterialInst()->Update(deltaTime);
	GunModel->GetTransform()->Update(deltaTime);
	Actor::Update(deltaTime);
}

void GunActor::HandleInputs(GLFWwindow* window, float deltaTime)
{
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		ParticleMeshInst->GetMaterialInst()->ResetAnimation();
}