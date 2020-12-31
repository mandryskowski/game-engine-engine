#pragma once
#include "Actor.h"
#include <array>

template<class T> class Interpolator;

namespace physx
{
	class PxController;
}

enum MovementDirections
{
	DIRECTION_FORWARD,
	DIRECTION_BACKWARD,
	DIRECTION_LEFT,
	DIRECTION_RIGHT,
	DIRECTION_UP,
	DIRECTION_COUNT
};

class Controller : public Actor
{

	struct MovementAxis
	{
		std::unique_ptr<Interpolator<float>> MovementInterpolator;
		bool Inversed;
		glm::vec3 Direction;
	};

	physx::PxController* PxController;
	Actor* PossessedActor;
	bool Directions[DIRECTION_COUNT];
	glm::vec3 Velocity;
	glm::vec3 PreviousFramePos;
	glm::vec3 RotationEuler;

	std::array<MovementAxis, 4> MovementAxises;

public:
	Controller(GameScene* scene, std::string name);

	virtual void OnStart() override;

	virtual void HandleEvent(const Event& ev) override;
	virtual void ReadMovementKeys();
	virtual void Update(float deltaTime) override;
	void RotateWithMouse(glm::vec2);	//rotates camera - you should pass the mouse offset from the center
	void HandleMovementAxis(bool pressed, MovementAxis& axis);
};