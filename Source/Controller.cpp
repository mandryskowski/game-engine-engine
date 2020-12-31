#include "Controller.h"
#include "Component.h"
#include "CameraComponent.h"
#include "CollisionObject.h"
#include "Animation.h"
#include "InputDevicesStateRetriever.h"
#include <PhysX/PxPhysicsAPI.h>

Controller::Controller(GameScene* scene, std::string name):
	Actor(scene, name),
	PxController(nullptr),
	Directions{false, false, false, false, false},
	Velocity(glm::vec3(0.0f)),
	PreviousFramePos(glm::vec3(0.0f)),
	RotationEuler(glm::vec3(0.0f))
{

	std::generate(MovementAxises.begin(), MovementAxises.end(),
		[]()
		{
			static int i = 0;
			MovementAxis dir;
			dir.MovementInterpolator = std::make_unique<Interpolator<float>>(Interpolator<float>(0.0f, 0.15f, 0.0f, 1.0f, InterpolationType::LINEAR));
			dir.Inversed = false;

			switch (static_cast<MovementDirections>(i))
			{
			case DIRECTION_FORWARD: dir.Direction = glm::vec3(0.0f, 0.0f, -1.0f); break;
			case DIRECTION_BACKWARD: dir.Direction = glm::vec3(0.0f, 0.0f, 1.0f); break;
			case DIRECTION_LEFT: dir.Direction = glm::vec3(-1.0f, 0.0f, 0.0f); break;
			case DIRECTION_RIGHT: dir.Direction = glm::vec3(1.0f, 0.0f, 0.0f); break;
			}
			i++;
			return dir;
		});
}

void Controller::OnStart()
{
	PxController = GameHandle->GetPhysicsHandle()->CreateController(Scene->GetPhysicsData());
	PossessedActor = GameHandle->GetRootActor()->FindActor("CameraActor");
	std::cout << "Tworze kontroler.\n";

	CollisionObject* colObject = new CollisionObject(false, CollisionShapeType::COLLISION_CAPSULE);
	colObject->TransformPtr = PossessedActor->GetTransform();
	colObject->ActorPtr = PxController->getActor();
	colObject->IgnoreRotation = true;
	GameHandle->GetPhysicsHandle()->AddCollisionObject(Scene->GetPhysicsData(), colObject);
}

void Controller::HandleEvent(const Event& ev)
{
	// put shooting here
}

void Controller::ReadMovementKeys()
{
	if (!PossessedActor)
		return;

	for (int i = 0; i < DIRECTION_COUNT; i++)
		Directions[i] = false;

	if (GameHandle->GetInputRetriever().IsKeyPressed(Key::W))
		Directions[DIRECTION_FORWARD] = true;
	if (GameHandle->GetInputRetriever().IsKeyPressed(Key::S))
		Directions[DIRECTION_BACKWARD] = true;
	if (GameHandle->GetInputRetriever().IsKeyPressed(Key::A))
		Directions[DIRECTION_LEFT] = true;
	if (GameHandle->GetInputRetriever().IsKeyPressed(Key::D))
		Directions[DIRECTION_RIGHT] = true;
	if (GameHandle->GetInputRetriever().IsKeyPressed(Key::SPACE))
		Directions[DIRECTION_UP] = true;
	if (GameHandle->GetInputRetriever().IsKeyPressed(Key::TAB))
	{
		GameHandle->PassMouseControl(nullptr);
	}
	else
		GameHandle->PassMouseControl(this);

	HandleMovementAxis(Directions[DIRECTION_FORWARD], MovementAxises[DIRECTION_FORWARD]);
	HandleMovementAxis(Directions[DIRECTION_BACKWARD], MovementAxises[DIRECTION_BACKWARD]);
	HandleMovementAxis(Directions[DIRECTION_LEFT], MovementAxises[DIRECTION_LEFT]);
	HandleMovementAxis(Directions[DIRECTION_RIGHT], MovementAxises[DIRECTION_RIGHT]);
}

void Controller::Update(float deltaTime)
{
	if (!PossessedActor)
		return;

	ReadMovementKeys();

	physx::PxRaycastBuffer dupa;
	bool isOnGround = PxController->getActor()->getScene()->raycast(physx::toVec3(PxController->getFootPosition()), PxController->getScene()->getGravity().getNormalized(), 0.1f, dupa);
	glm::vec3 movementVec(0.0f);
	
	for (int i = 0; i < 4; i++)
	{
		MovementAxises[i].MovementInterpolator->Update(deltaTime);
		glm::vec3 dir = PossessedActor->GetTransform()->GetWorldTransform().GetRotationMatrix() * MovementAxises[i].Direction;
		dir = glm::normalize(glm::vec3(dir.x, 0.0f, dir.z));
		movementVec += dir * MovementAxises[i].MovementInterpolator->GetCurrentValue();
	}

	//PossessedActor->GetTransform()->Move(movementVec * deltaTime * 3.0f);
	PxController->move(toPx(movementVec * deltaTime * 3.0f), 0.001f, deltaTime, physx::PxControllerFilters());

	if (isOnGround)
	{
		if (Directions[DIRECTION_UP])
			Velocity = toGlm(PxController->getUpDirection()) * 2.45f;
		else
			Velocity.y = 0.0f;
	}
	else
	{
		if (Velocity.y > 0 && PxController->getActor()->getGlobalPose().p.y == PreviousFramePos.y)
		{
			Velocity.y = 0.0f;
			std::cout << "zajebales sie\n";
		}
		else
			Velocity += toGlm(PxController->getScene()->getGravity()) * deltaTime;
	}

	PxController->move(toPx(Velocity * deltaTime), 0.001f, deltaTime, physx::PxControllerFilters());

	PreviousFramePos = toGlm(PxController->getActor()->getGlobalPose().p);
}

void Controller::RotateWithMouse(glm::vec2 mouseOffset)
{
	if (!PossessedActor)
		return;

	float sensitivity = 0.15f;
	RotationEuler.x -= mouseOffset.y * sensitivity;
	RotationEuler.y -= mouseOffset.x * sensitivity;

	RotationEuler.x = glm::clamp(RotationEuler.x, -89.9f, 89.9f);
	RotationEuler.y = fmod(RotationEuler.y, 360.0f);

	PossessedActor->GetTransform()->SetRotation(RotationEuler);
}

void Controller::HandleMovementAxis(bool pressed, MovementAxis& axis)
{
	if ((pressed && axis.Inversed) || (!pressed && !axis.Inversed))
	{
		axis.MovementInterpolator->Inverse();
		axis.Inversed = !axis.Inversed;
	}
}