#include "Controller.h"
#include "Component.h"
#include "CameraComponent.h"
#include "CollisionObject.h"
#include <PhysX/PxPhysicsAPI.h>

Controller::Controller(GameManager* gameHandle, std::string name):
	Actor(gameHandle, name)
{
	PxController = gameHandle->GetPhysicsHandle()->CreateController();
}

void Controller::OnStart()
{
	PossessedActor = GameHandle->GetSearchEngine()->FindActor("CameraActor");

	CollisionObject* colObject = new CollisionObject(false, CollisionShapeType::COLLISION_CAPSULE);
	colObject->TransformPtr = PossessedActor->GetTransform();
	colObject->ActorPtr = PxController->getActor();
	GameHandle->GetPhysicsHandle()->AddCollisionObject(colObject);
}

void Controller::HandleInputs(GLFWwindow* window, float deltaTime)
{
	/*
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		PxController->move(physx::PxVec3(0.0f, 0.0f, -1.0f), 0.001f, deltaTime, physx::PxControllerFilters());
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		PxController->move(physx::PxVec3(0.0f, 0.0f, 1.0f), 0.001f, deltaTime, physx::PxControllerFilters());
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		PxController->move(physx::PxVec3(-1.0f, 0.0f, 0.0f), 0.001f, deltaTime, physx::PxControllerFilters());
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		PxController->move(physx::PxVec3(1.0f, 0.0f, 0.0f), 0.001f, deltaTime, physx::PxControllerFilters());
	}*/
}
