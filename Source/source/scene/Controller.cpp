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

#include <input/Event.h>
#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>

namespace GEE
{
	Controller::Controller(GameScene& scene, Actor* parentActor, std::string name) :
		Actor(scene, parentActor, name),
		PxController(nullptr),
		PossessedActor(nullptr),
		Directions{ false, false, false, false, false },
		Velocity(Vec3f(0.0f)),
		PreviousFramePos(Vec3f(0.0f)),
		RotationEuler(Vec3f(0.0f))
	{
		int i = 0;
		std::generate(MovementAxises.begin(), MovementAxises.end(),
			[&i]()
			{
				MovementAxis dir;
				dir.MovementInterpolator = nullptr;// std::make_unique<Interpolator<float>>(Interpolator<float>(0.0f, 0.15f, 0.0f, 1.0f, InterpolationType::LINEAR));
				dir.Inversed = false;

				switch (static_cast<MovementDirections>(i))
				{
				case DIRECTION_FORWARD: dir.Direction = Vec3f(0.0f, 0.0f, -1.0f); break;
				case DIRECTION_BACKWARD: dir.Direction = Vec3f(0.0f, 0.0f, 1.0f); break;
				case DIRECTION_LEFT: dir.Direction = Vec3f(-1.0f, 0.0f, 0.0f); break;
				case DIRECTION_RIGHT: dir.Direction = Vec3f(1.0f, 0.0f, 0.0f); break;
				}
				i++;
				return dir;
			});
	}

	void Controller::SetPossessedActor(Actor* actor)
	{
		if (PossessedActor)
			PossessedActor->GetRoot()->SetCollisionObject(nullptr);	//PxController will probably be released here, so we set it to Nullptr in the next line

		PxController = nullptr;
		PossessedActor = actor;

		if (!PossessedActor)
			return;
		std::cout << "Tworze kontroler.\n";

		RotationEuler = Vec3f(0.0f);

		PxController = GameHandle->GetPhysicsHandle()->CreateController(*Scene.GetPhysicsData(), PossessedActor->GetTransform()->GetWorldTransform());

		std::unique_ptr<Physics::CollisionObject> colObject = std::make_unique<Physics::CollisionObject>(false);
		colObject->ActorPtr = PxController->getActor();
		colObject->IgnoreRotation = true;
		PossessedActor->GetRoot()->SetCollisionObject(std::move(colObject));
	}

	void Controller::HandleEvent(const Event& ev)
	{
		Actor::HandleEvent(ev);
	}

	void Controller::ReadMovementKeys()
	{
		for (int i = 0; i < DIRECTION_COUNT; i++)
			Directions[i] = false;

		if (!PossessedActor || GameHandle->GetCurrentMouseController() != this)
			return;

		if (Actor* gameLevelActor = Scene.FindActor("GameLevel"))
			if (Component* forkliftComp = gameLevelActor->GetRoot()->GetComponent("Forklift.001"))
			{
				if (GameHandle->GetInputRetriever().IsKeyPressed(Key::E))
					forkliftComp->GetTransform().SetParentTransform(PossessedActor->GetTransform(), true);
				if (GameHandle->GetInputRetriever().IsKeyPressed(Key::F))
					forkliftComp->GetTransform().SetParentTransform(&gameLevelActor->GetRoot()->GetTransform(), true);
			}

		if (GameHandle->GetInputRetriever().IsKeyPressed(Key::W))
			Directions[DIRECTION_FORWARD] = true;
		if (GameHandle->GetInputRetriever().IsKeyPressed(Key::S))
			Directions[DIRECTION_BACKWARD] = true;
		if (GameHandle->GetInputRetriever().IsKeyPressed(Key::A))
			Directions[DIRECTION_LEFT] = true;
		if (GameHandle->GetInputRetriever().IsKeyPressed(Key::D))
			Directions[DIRECTION_RIGHT] = true;
		if (GameHandle->GetInputRetriever().IsKeyPressed(Key::Space))
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

		if (PossessedActor->IsBeingKilled())
		{
			PossessedActor = nullptr;
			return;
		}

		if (PxController->getActor()->getNbShapes() == 0)
			return;

		ReadMovementKeys();

		//bool isOnGround = PxController->getActor()->getScene()->raycast(physx::toVec3(PxController->getFootPosition()), PxController->getScene()->getGravity().getNormalized(), 0.03f, dupa);

		physx::PxRaycastBuffer dupa;
		physx::PxSweepBuffer sweepBuffer;
		physx::PxShape** shapes = new physx::PxShape * [16];
		PxController->getActor()->getShapes(shapes, 16);

		shapes[0]->acquireReference();
		PxController->getActor()->detachShape(*shapes[0]);
		//toTransform(PxController->getActor()->getGlobalPose() * shapes[0]->getLocalPose()).Print("pose");
		bool groundSweep = PxController->getScene()->sweep(shapes[0]->getGeometry().capsule(), PxController->getActor()->getGlobalPose() * shapes[0]->getLocalPose(), PxController->getScene()->getGravity().getNormalized(), 0.5f, sweepBuffer, physx::PxHitFlag::eDEFAULT, physx::PxQueryFilterData(), nullptr);
		bool isOnGround = (groundSweep) && ((PxController->getFootPosition().y - sweepBuffer.block.position.y) < 0.01f);
		PxController->getActor()->attachShape(*shapes[0]);
		shapes[0]->release();

		Velocity.y -= 9.81f * deltaTime;

		if (isOnGround && Directions[DIRECTION_UP])
		{
			Velocity.y = 0.0f;
			Velocity += Physics::Util::toGlm(PxController->getUpDirection()) * 2.45f;
			isOnGround = false;
		}

		if (isOnGround)
			;// std::cout << "GROUND!!!\n";

		Vec3f wishVec(0.0f);
		for (int i = 0; i < 4; i++)
		{
			//	MovementAxises[i].MovementInterpolator->Update(deltaTime);
			Vec3f dir = PossessedActor->GetTransform()->GetWorldTransform().GetRotationMatrix() * MovementAxises[i].Direction;
			dir = glm::normalize(Vec3f(dir.x, 0.0f, dir.z));
			wishVec += dir * static_cast<float>(Directions[i]);
		}

		if (wishVec != Vec3f(0.0f))
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
			float currentProjectedSpeed = (isOnGround) ? (glm::length(Vec3f(Velocity.x, 0.0f, Velocity.z))) : (glm::dot(Vec3f(Velocity.x, 0.0f, Velocity.z), wishVec));
			float addedSpeed = wishSpeed - currentProjectedSpeed;
			if (addedSpeed > 0.0f)
			{
				Velocity += wishVec * addedSpeed;
			}
		}

		float beforePxSpeed = glm::length(Velocity);
		physx::PxExtendedVec3 prevPos = PxController->getPosition();

		PxController->move(Physics::Util::toPx(Velocity * deltaTime), 0.001f, deltaTime, physx::PxControllerFilters());

		if (isOnGround)
			PxController->move(Physics::Util::toPx(Vec3f(0.0f, -0.2f, 0.0f)), 0.001f, deltaTime, physx::PxControllerFilters());
		Velocity = Physics::Util::toGlm(PxController->getPosition() - prevPos) / deltaTime;

		if (TextComponent* found = dynamic_cast<TextComponent*>(PossessedActor->GetRoot()->GetComponent("CameraText")))
			found->SetContent("Velocity: " + std::to_string(glm::length(Vec3f(Velocity.x, 0.0f, Velocity.z))) + " " + std::to_string(Velocity.y) + ((isOnGround) ? (" ON-GROUND") : (" MID-AIR")));

		/*physx::PxRaycastBuffer dupa;
		bool isOnGround = PxController->getActor()->getScene()->raycast(physx::toVec3(PxController->getFootPosition()), PxController->getScene()->getGravity().getNormalized(), 0.1f, dupa);
		Vec3f movementVec(0.0f);

		Velocity.x *= 0.5f;
		Velocity.z *= 0.5f;

		for (int i = 0; i < 4; i++)
		{
			MovementAxises[i].MovementInterpolator->Update(deltaTime);
			Vec3f dir = PossessedActor->GetTransform()->GetWorldTransform().GetRotationMatrix() * MovementAxises[i].Direction;
			dir = glm::normalize(Vec3f(dir.x, 0.0f, dir.z));
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

	void Controller::RotateWithMouse(Vec2f mouseOffset)
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

	void Controller::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Actor::GetEditorDescription(descBuilder);

		descBuilder.AddField("Target camera").GetTemplates().ObjectInput<Actor, Actor>([this]() { std::vector<Actor*> actors; Scene.GetRootActor()->GetAllActors(&actors); return actors; }, [this](Actor* actor) { SetPossessedActor(actor); });
	}

	ShootingController::ShootingController(GameScene& scene, Actor* parentActor, const std::string& name) :
		Controller(scene, parentActor, name),
		PossessedGunActor(nullptr)
	{
	}

	void ShootingController::HandleEvent(const Event& ev)
	{
		Controller::HandleEvent(ev);
		if (ev.GetType() != EventType::MousePressed || GameHandle->GetCurrentMouseController() != this || !PossessedGunActor)
			return;

		const MouseButtonEvent& pressedEv = *dynamic_cast<const MouseButtonEvent*>(&ev);
		if (pressedEv.GetButton() == MouseButton::Right)
		{
			printVector(PossessedGunActor->GetTransform()->Rot(), "Prawdziwa rotacja");
			printVector(PossessedGunActor->GetTransform()->GetWorldTransform().Pos(), "Pozycja gracza");
			printVector(PossessedGunActor->GetTransform()->GetWorldTransform().GetFrontVec(), "Front");
		}
		if (pressedEv.GetButton() == MouseButton::Left)
			PossessedGunActor->FireWeapon();
	}

	void ShootingController::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Controller::GetEditorDescription(descBuilder);

		descBuilder.AddField("PossessedGunActor").GetTemplates().ObjectInput<Actor, GunActor>(*Scene.GetRootActor(), PossessedGunActor);
	}
}