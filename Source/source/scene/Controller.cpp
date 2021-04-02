#include <scene/Controller.h>
#include <scene/Component.h>
#include <scene/CameraComponent.h>
#include <physics/CollisionObject.h>
#include <animation/Animation.h>
#include <input/InputDevicesStateRetriever.h>
#include <PhysX/PxPhysicsAPI.h>
#include <scene/TextComponent.h>
#include <scene/GunActor.h>
#include <math/Transform.h>

Controller::Controller(GameScene& scene, std::string name):
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
			dir.MovementInterpolator = nullptr;// std::make_unique<Interpolator<float>>(Interpolator<float>(0.0f, 0.15f, 0.0f, 1.0f, InterpolationType::LINEAR));
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
	Actor::OnStart();

	PxController = GameHandle->GetPhysicsHandle()->CreateController(*Scene.GetPhysicsData());
	PossessedActor = GameHandle->GetRootActor()->FindActor("CameraActor");
	std::cout << "Tworze kontroler.\n";

	CollisionObject* colObject = new CollisionObject(false, CollisionShapeType::COLLISION_CAPSULE);
	colObject->ActorPtr = PxController->getActor();
	colObject->IgnoreRotation = true;
	Scene.GetPhysicsData()->AddCollisionObject(*colObject, *PossessedActor->GetTransform());
}

#include <input/Event.h>

void Controller::HandleEvent(const Event& ev)
{
	Actor::HandleEvent(ev);
}

void Controller::ReadMovementKeys()
{
	if (GameHandle->GetInputRetriever().IsKeyPressed(Key::TAB))
	{
		GameHandle->PassMouseControl(nullptr);
		for (int i = 0; i < DIRECTION_COUNT; i++)
			Directions[i] = false;
	}
	else
		GameHandle->PassMouseControl(this);

	if (!PossessedActor || GameHandle->GetCurrentMouseController() != this)
		return;

	for (int i = 0; i < DIRECTION_COUNT; i++)
		Directions[i] = false;

	if (GameHandle->GetInputRetriever().IsKeyPressed(Key::E))
		Scene.FindActor("GameLevel")->GetRoot()->GetComponent("Forklift.001")->GetTransform().SetParentTransform(PossessedActor->GetTransform(), true);
	if (GameHandle->GetInputRetriever().IsKeyPressed(Key::F))
		Scene.FindActor("GameLevel")->GetRoot()->GetComponent("Forklift.001")->GetTransform().SetParentTransform(&Scene.FindActor("GameLevel")->GetRoot()->GetTransform(), true);

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

	//bool isOnGround = PxController->getActor()->getScene()->raycast(physx::toVec3(PxController->getFootPosition()), PxController->getScene()->getGravity().getNormalized(), 0.03f, dupa);

	physx::PxRaycastBuffer dupa;
	physx::PxSweepBuffer sweepBuffer;
	physx::PxShape** shapes = new physx::PxShape*[16];
	PxController->getActor()->getShapes(shapes, 16);
	shapes[0]->acquireReference();
	PxController->getActor()->detachShape(*shapes[0]);
	bool groundSweep = PxController->getScene()->sweep(shapes[0]->getGeometry().capsule(), PxController->getActor()->getGlobalPose() * shapes[0]->getLocalPose(), PxController->getScene()->getGravity().getNormalized(), 0.5f, sweepBuffer, physx::PxHitFlag::eDEFAULT, physx::PxQueryFilterData(), nullptr);
	bool isOnGround = (groundSweep) && ((PxController->getFootPosition().y - sweepBuffer.block.position.y) < 0.01f);
	PxController->getActor()->attachShape(*shapes[0]);
	shapes[0]->release();

	Velocity.y -= 9.81f * deltaTime;

	if (isOnGround && Directions[DIRECTION_UP])
	{
		Velocity.y = 0.0f;
		Velocity += toGlm(PxController->getUpDirection()) * 2.45f;
		isOnGround = false;
	}

	if (isOnGround)
		;// std::cout << "GROUND!!!\n";

	glm::vec3 wishVec(0.0f);
	for (int i = 0; i < 4; i++)
	{
	//	MovementAxises[i].MovementInterpolator->Update(deltaTime);
		glm::vec3 dir = PossessedActor->GetTransform()->GetWorldTransform().GetRotationMatrix() * MovementAxises[i].Direction;
		dir = glm::normalize(glm::vec3(dir.x, 0.0f, dir.z));
		wishVec += dir * static_cast<float>(Directions[i]);
	}

	if (wishVec != glm::vec3(0.0f))
		wishVec = glm::normalize(wishVec);

	if (isOnGround)
	{
		const float friction = 10.0f;
		float previousSpeed = glm::length(Velocity);

		if (previousSpeed > 0.0f)
		{
			float drop = friction * deltaTime * previousSpeed;
			Velocity *= glm::max(previousSpeed - drop, 0.0f) / previousSpeed;
		}
	}

	{
		const float wishSpeed = (isOnGround) ? (3.0f) : (0.5f);
		float currentProjectedSpeed = (isOnGround) ? (glm::length(glm::vec3(Velocity.x, 0.0f, Velocity.z))) : (glm::dot(glm::vec3(Velocity.x, 0.0f, Velocity.z), wishVec));
		float addedSpeed = wishSpeed - currentProjectedSpeed;
		if (addedSpeed > 0.0f)
		{
			Velocity += wishVec * addedSpeed;
		}
	}

	float beforePxSpeed = glm::length(Velocity);
	physx::PxExtendedVec3 prevPos = PxController->getPosition();
	PxController->move(toPx(Velocity * deltaTime), 0.001f, deltaTime, physx::PxControllerFilters());
	if (isOnGround)
		PxController->move(toPx(glm::vec3(0.0f, -0.2f, 0.0f)), 0.001f, 0.0f, physx::PxControllerFilters());
	Velocity = toGlm(PxController->getPosition() - prevPos) / deltaTime;

	dynamic_cast<TextComponent*>(PossessedActor->GetRoot()->GetComponent("CameraText"))->SetContent("Velocity: " + std::to_string(glm::length(glm::vec3(Velocity.x, 0.0f, Velocity.z))) + " " + std::to_string(Velocity.y) + ((isOnGround) ? (" ON-GROUND") : (" MID-AIR")));

	/*physx::PxRaycastBuffer dupa;
	bool isOnGround = PxController->getActor()->getScene()->raycast(physx::toVec3(PxController->getFootPosition()), PxController->getScene()->getGravity().getNormalized(), 0.1f, dupa);
	glm::vec3 movementVec(0.0f);

	Velocity.x *= 0.5f;
	Velocity.z *= 0.5f;
	
	for (int i = 0; i < 4; i++)
	{
		MovementAxises[i].MovementInterpolator->Update(deltaTime);
		glm::vec3 dir = PossessedActor->GetTransform()->GetWorldTransform().GetRotationMatrix() * MovementAxises[i].Direction;
		dir = glm::normalize(glm::vec3(dir.x, 0.0f, dir.z));
		movementVec += dir * static_cast<float>(Directions[i]);
	}

	const float maxRunSpeed = 3.0f;
	float currentSpeedProjected = glm::dot(Velocity, movementVec);
	float addedSpeed = maxRunSpeed - currentSpeedProjected;
	addedSpeed = glm::max(addedSpeed / deltaTime, 0.0f);
	Velocity += movementVec * addedSpeed;

	//PossessedActor->GetTransform()->Move(movementVec * deltaTime * 3.0f);
	//PxController->move(toPx(movementVec * deltaTime * 3.0f), 0.001f, deltaTime, physx::PxControllerFilters());

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
	std::cout << "SPEED: " << glm::length(Velocity) << "\n";

	PreviousFramePos = toGlm(PxController->getActor()->getGlobalPose().p);*/
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
		//axis.MovementInterpolator->Inverse();
		axis.Inversed = !axis.Inversed;
	}
}

ShootingController::ShootingController(GameScene& scene, const std::string& name):
	Controller(scene, name),
	PossesedActorGun(nullptr)
{
}

void ShootingController::OnStart()
{
	Controller::OnStart();
	PossesedActorGun = dynamic_cast<GunActor*>(GameHandle->GetRootActor()->FindActor("Gun"));;
}

void ShootingController::HandleEvent(const Event& ev)
{
	Controller::HandleEvent(ev);
	if (ev.GetType() != EventType::MOUSE_PRESSED || GameHandle->GetCurrentMouseController() != this)
		return;

	const MouseButtonEvent& pressedEv = *dynamic_cast<const MouseButtonEvent*>(&ev);
	if (pressedEv.GetButton() == MouseButton::RIGHT)
	{
		printVector(PossesedActorGun->GetTransform()->RotationRef, "Prawdziwa rotacja");
		printVector(PossesedActorGun->GetTransform()->GetWorldTransform().PositionRef, "Pozycja gracza");
		printVector(PossesedActorGun->GetTransform()->GetWorldTransform().GetFrontVec(), "Front");
	}
	if (pressedEv.GetButton() == MouseButton::LEFT)
		PossesedActorGun->FireWeapon();
}
