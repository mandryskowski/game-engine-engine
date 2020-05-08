#include "GunActor.h"

GunActor::GunActor(std::string name):
	Actor(name)
{
	RootComponent = new ModelComponent("GunModel");
	FireCooldown = 2.0f;
	CooldownLeft = 0.0f;
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

	GunBlast = searcher->FindSoundSource("DoubleBarrelBlast");

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
	CooldownLeft -= deltaTime;

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && CooldownLeft <= 0.0f)
	{
		ParticleMeshInst->GetMaterialInst()->ResetAnimation();
		GunBlast->Play();

		GunModel->GetTransform()->AddInterpolator("rotation", 0.0f, 0.25f, glm::vec3(0.0f), glm::vec3(30.0f, 0.0f, 0.0f), InterpolationType::QUINTIC, true);
		GunModel->GetTransform()->AddInterpolator("rotation", 0.25f, 1.25f, glm::vec3(0.0f), InterpolationType::QUADRATIC, true);
		 
		CooldownLeft = FireCooldown;
	}
}