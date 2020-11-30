#include "GunActor.h"
#include "SoundSourceComponent.h"
#include "FileLoader.h"

GunActor::GunActor(GameScene* scene, std::string name):
	Actor(scene, name)
{
	FireCooldown = 2.0f;
	CooldownLeft = 0.0f;
	FiredBullets = 0;
}

void GunActor::Setup()
{
	if (!SetupStream)
	{
		std::cerr << "ERROR! Can't setup GunActor " << Name << " dla " << this << ".\n";
		return;
	}

	std::string str;
	(*SetupStream) >> str;

	if (str == "FireMaterial")
	{
		(*SetupStream) >> str;
		std::cout << "Firematerial = " << GameHandle->GetRenderEngineHandle()->FindMaterial(str) << ".\n";
	}

	ModelComponent* found = dynamic_cast<ModelComponent*>(Scene->GetRenderData()->FindRenderable("FireParticle"));
	std::cout << "Wyszukalem se: " << found << ".\n";
	dynamic_cast<ModelComponent*>(found->SearchForComponent("Quad"))->SetRenderAsBillboard(true);
	ParticleMeshInst = found->FindMeshInstance("Quad");
	ParticleMeshInst->GetMaterialInst()->SetInterp(new Interpolation(0.0f, 0.25f, InterpolationType::LINEAR));
	
	GunModel = dynamic_cast<ModelComponent*>(Scene->GetRenderData()->FindRenderable("MyDoubleBarrel"));
	GunModel->GetTransform().SetFront(glm::vec3(0.0f, 0.0f, -1.0f));

	/*
	GunModel->GetTransform()->AddInterpolator("position", 10.0f, 20.0f, glm::vec3(0.0f, 0.0f, -10.0f), InterpolationType::LINEAR);
	GunModel->GetTransform()->AddInterpolator("rotation", 10.0f, 5.0f, glm::vec3(360.0f, 0.0f, 0.0f), InterpolationType::QUADRATIC, false, AnimBehaviour::STOP, AnimBehaviour::EXTRAPOLATE);
	GunModel->GetTransform()->AddInterpolator("scale", 10.0f, 10.0f, glm::vec3(0.3f, 0.3f, 0.2f), InterpolationType::CONSTANT);*/

	GunBlast = Scene->GetAudioData()->FindSource("DoubleBarrelBlast");
	
	Actor::Setup();
}

void GunActor::Update(float deltaTime)
{
	ParticleMeshInst->GetMaterialInst()->Update(deltaTime);
	GunModel->GetTransform().Update(deltaTime);
	CooldownLeft -= deltaTime;
	Actor::Update(deltaTime);
}

void GunActor::HandleInputs(GLFWwindow* window)
{
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		printVector(GunModel->GetTransform().RotationRef, "Prawdziwa rotacja");
		printVector(GunModel->GetTransform().GetWorldTransform().PositionRef, "Pozycja gracza");
	}
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && CooldownLeft <= 0.0f)
	{
		ParticleMeshInst->GetMaterialInst()->ResetAnimation();
		GunBlast->Play();
		
		GunModel->GetTransform().AddInterpolator<glm::quat>("rotation", 0.0f, 0.25f, glm::quat(glm::vec3(0.0f)), toQuat(glm::vec3(30.0f, 0.0f, 0.0f)), InterpolationType::QUINTIC, true);
		GunModel->GetTransform().AddInterpolator<glm::quat>("rotation", 0.25f, 1.25f, glm::quat(glm::vec3(0.0f)), InterpolationType::QUADRATIC, true);

		Actor* actor = Scene->AddActorToRoot(std::make_shared<Actor>(Scene, "Bullet" + std::to_string(FiredBullets)));
		ModelComponent* bulletModel = ModelComponent::Of(ModelComponent(Scene, "BulletModel" + std::to_string(FiredBullets++), Transform(GetTransform()->GetWorldTransform().PositionRef, glm::vec3(0.0f), glm::vec3(0.1f)))).get();
		EngineDataLoader::LoadModel("hqSphere/hqSphere.obj", bulletModel, MeshTreeInstancingType::ROOTTREE, GameHandle->GetRenderEngineHandle()->FindMaterial("RustedIron"));

		actor->ReplaceRoot(bulletModel);

		std::unique_ptr<CollisionObject> dupa = std::make_unique<CollisionObject>(CollisionObject(false, CollisionShapeType::COLLISION_SPHERE));
		CollisionObject* col = bulletModel->SetCollisionObject(dupa);
		GameHandle->GetPhysicsHandle()->ApplyForce(col, GunModel->GetTransform().GetWorldTransform().FrontRef * 0.25f);

		actor->DebugHierarchy();

		
		CooldownLeft = FireCooldown;
	}
}