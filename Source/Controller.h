#pragma once
#include "Actor.h"

namespace physx
{
	class PxController;
}

class Controller : public Actor
{
	physx::PxController* PxController;
	Actor* PossessedActor;
public:
	Controller(GameManager* gameHandle, std::string name);

	virtual void OnStart() override;

	virtual void HandleInputs(GLFWwindow* window, float deltaTime) override;
};