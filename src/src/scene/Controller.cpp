#include <scene/Controller.h>
#include <game/GameScene.h>
#include <scene/Component.h>
#include <physics/CollisionObject.h>
#include <physics/DynamicPhysicsObjects.h>
#include <input/InputDevicesStateRetriever.h>
#include <scene/TextComponent.h>
#include <scene/GunActor.h>
#include <math/Transform.h>

#include <scene/hierarchy/HierarchyNode.h>

#include <input/Event.h>
#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>

namespace GEE
{
	Controller::Controller(GameScene& scene, Actor* parentActor, const String& name) :
		Actor(scene, parentActor, name),
		PossessedActor(nullptr),
		bHideCursor(false),
		bLockMouseAtCenter(false)
	{
	}

	bool Controller::GetHideCursor() const
	{
		return bHideCursor;
	}

	bool Controller::GetLockMouseAtCenter() const
	{
		return bLockMouseAtCenter;
	}

	void Controller::SetHideCursor(bool hide)
	{
		bHideCursor = hide;
	}

	void Controller::SetLockMouseAtCenter(bool lock)
	{
		bLockMouseAtCenter = lock;
	}

	void Controller::SetPossessedActor(Actor* actor)
	{
		PossessedActor = actor;
	}

	void Controller::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Actor::GetEditorDescription(descBuilder);

		descBuilder.AddField("Active").GetTemplates().TickBox([this](bool val) { if (val) GameHandle->PassMouseControl(this); }, [this]() { return GameHandle->GetCurrentMouseController() == this; });
	}

	FreeRoamingController::FreeRoamingController(GameScene& scene, Actor* parentActor, const String& name) :
		Controller(scene, parentActor, name),
		RotationEuler(Vec3f(0.0f)),
		ControllerSpeed(1.0f)
	{
		SetHideCursor(true);
		SetLockMouseAtCenter(true);
	}

	void FreeRoamingController::SetPossessedActor(Actor* actor)
	{
		Controller::SetPossessedActor(actor);
		if (!actor)
			return;
		RotationEuler = toEuler(actor->GetTransform()->GetRot());
		RotationEuler.z = 0.0f;
		std::cout << "Possessed actor rot euler " << RotationEuler << "\n";
	}

	Vec3i FreeRoamingController::GetMovementDirFromPressedKeys()
	{
		Vec3i movementDir(0);
		if (GameHandle->GetDefInputRetriever().IsKeyPressed(Key::W))
			movementDir.z -= 1;
		if (GameHandle->GetDefInputRetriever().IsKeyPressed(Key::S))
			movementDir.z += 1;
		if (GameHandle->GetDefInputRetriever().IsKeyPressed(Key::A))
			movementDir.x -= 1;
		if (GameHandle->GetDefInputRetriever().IsKeyPressed(Key::D))
			movementDir.x += 1;
		if (GameHandle->GetDefInputRetriever().IsKeyPressed(Key::Space))
			movementDir.y += 1;
		if (GameHandle->GetDefInputRetriever().IsKeyPressed(Key::LeftControl))
			movementDir.y -= 1;

		return movementDir;
	}

	void FreeRoamingController::Update(Time dt)
	{
		Controller::Update(dt);

		if (!PossessedActor || GameHandle->GetCurrentMouseController() != this)
			return;

		const Vec3i& movementDir = GetMovementDirFromPressedKeys();
		const Vec3f rotatedMovementDir = PossessedActor->GetTransform()->GetWorldTransform().GetRotationMatrix() * Vec3f(static_cast<float>(movementDir.x), 0.0f, static_cast<float>(movementDir.z)) // account for rotation on XZ
										+ Vec3f(0.0f, movementDir.y, 0.0f);	// ignore rotation on Y
		if (rotatedMovementDir == Vec3f(0.0f))
			return;

		PossessedActor->GetTransform()->Move(glm::normalize(rotatedMovementDir) * static_cast<float>(dt) * ControllerSpeed);
	}

	void FreeRoamingController::OnMouseMovement(const Vec2f& previousPosPx, const Vec2f& currentPosPx, const Vec2u& windowSize)
	{
		if (!PossessedActor)
			return;

		Vec2f mouseOffset = currentPosPx - previousPosPx;
		mouseOffset.y *= -1.0f;

		const float sensitivity = 0.15f;
		RotationEuler.x -= mouseOffset.y * sensitivity;
		RotationEuler.y -= mouseOffset.x * sensitivity;

		RotationEuler.x = glm::clamp(RotationEuler.x, -89.9f, 89.9f);
		RotationEuler.y = fmod(RotationEuler.y, 360.0f);

		PossessedActor->GetTransform()->SetRotation(RotationEuler);
	}

	void FreeRoamingController::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Controller::GetEditorDescription(descBuilder);

		descBuilder.AddField("Target camera").GetTemplates().ObjectInput<Actor>(
			[this]() { std::vector<Actor*> actors; Scene.GetRootActor()->GetAllActors(&actors); return actors; },
			[this](Actor* actor) { SetPossessedActor(actor); });
		descBuilder.AddField("Controller speed").GetTemplates().SliderRawInterval(0.0, 10.0, [this](float speed) { ControllerSpeed = speed; }, ControllerSpeed);
	}

	FPSController::FPSController(GameScene& scene, Actor* parentActor, const String& name) :
		FreeRoamingController(scene, parentActor, name),
		PxController(nullptr),
		Velocity(Vec3f(0.0f))
	{
	}

	void FPSController::SetPossessedActor(Actor* actor)
	{
		if (PossessedActor)
			PossessedActor->GetRoot()->SetCollisionObject(nullptr);	//PxController will probably be released here, so we set it to Nullptr in the next line

		PxController = nullptr;
		
		FreeRoamingController::SetPossessedActor(actor);

		if (!PossessedActor)
			return;

		PxController = GameHandle->GetPhysicsHandle()->CreateController(*Scene.GetPhysicsData(), PossessedActor->GetTransform()->GetWorldTransform());

		UniquePtr<Physics::CollisionObject> colObject = MakeUnique<Physics::CollisionObject>(false);
		colObject->ActorPtr = PxController->getActor();
		colObject->IgnoreRotation = true;
		PossessedActor->GetRoot()->SetCollisionObject(std::move(colObject));
	}

	Vec3i FPSController::GetMovementDirFromPressedKeys()
	{
		if (GameHandle->GetCurrentMouseController() != this)
			return Vec3i(0);
		const Vec3i& movementDir = FreeRoamingController::GetMovementDirFromPressedKeys();

		// Cancel out the alteration of movementDir due to pressing LeftControl.
		if (GameHandle->GetDefInputRetriever().IsKeyPressed(Key::LeftControl))
			return movementDir + Vec3i(0, 1, 0);

		// If LeftControl is not pressed, there is no need to change anything.
		return movementDir;
	}

	void FPSController::Update(Time dt)
	{
		if (!PossessedActor || &Scene != GameHandle->GetActiveScene())
			return;

		if (PossessedActor->IsBeingKilled())
		{
			PossessedActor = nullptr;
			return;
		}

		if (PxController->getActor()->getNbShapes() == 0)
			return;

		const Vec3i& movementDir = GetMovementDirFromPressedKeys();

		physx::PxSweepBuffer sweepBuffer;
		auto** shapes = new physx::PxShape * [16];
		PxController->getActor()->getShapes(shapes, 16);

		shapes[0]->acquireReference();
		PxController->getActor()->detachShape(*shapes[0]);

		const bool groundSweep = PxController->getScene()->sweep(shapes[0]->getGeometry().capsule(), PxController->getActor()->getGlobalPose() * shapes[0]->getLocalPose(), PxController->getScene()->getGravity().getNormalized(), 0.5f, sweepBuffer, physx::PxHitFlag::eDEFAULT, physx::PxQueryFilterData(), nullptr);
		bool isOnGround = (groundSweep) && PxController->getFootPosition().y - sweepBuffer.block.position.y < 0.01;
		PxController->getActor()->attachShape(*shapes[0]);
		shapes[0]->release();

		Velocity.y -= static_cast<float>(9.81 * dt);

		if (isOnGround && movementDir.y == 1)
		{
			Velocity.y = 0.0f;
			Velocity += Physics::Util::toGlm(PxController->getUpDirection()) * 2.45f;
			isOnGround = false;
		}

		Vec3f wishVec = PossessedActor->GetTransform()->GetWorldTransform().GetRotationMatrix() * static_cast<Vec3f>(movementDir);
		wishVec = Vec3f(wishVec.x, 0.0f, wishVec.z);

		if (wishVec != Vec3f(0.0f))
			wishVec = glm::normalize(wishVec);

		if (isOnGround)
		{
			const auto previousSpeed = glm::length(Velocity);

			if (previousSpeed > 0.0f)
			{
				const auto friction = 10.0;
				const auto drop = friction * dt * previousSpeed;
				Velocity *= glm::max(previousSpeed - drop, 0.0) / previousSpeed;
			}
		}

		{
			const float wishSpeed = (isOnGround) ? (3.0f) : (0.5f);
			const float currentProjectedSpeed = (isOnGround) ? (glm::length(Vec3f(Velocity.x, 0.0f, Velocity.z))) : (glm::dot(Vec3f(Velocity.x, 0.0f, Velocity.z), wishVec));
			const float addedSpeed = wishSpeed - currentProjectedSpeed;
			if (addedSpeed > 0.0f)
			{
				Velocity += wishVec * addedSpeed;
			}
		}

		const physx::PxExtendedVec3 prevPos = PxController->getPosition();

		PxController->move(Physics::Util::toPx(Velocity * static_cast<float>(dt)), static_cast<physx::PxF32>(0.001), static_cast<physx::PxF32>(dt), physx::PxControllerFilters());

		if (isOnGround)
			PxController->move(Physics::Util::toPx(Vec3f(0.0f, -0.2f, 0.0f)), static_cast<physx::PxF32>(0.001), static_cast<physx::PxF32>(dt), physx::PxControllerFilters());
		Velocity = Physics::Util::toGlm(PxController->getPosition() - prevPos) / static_cast<float>(dt);

		PossessedActor->GetTransform()->SetPositionWorld(Physics::Util::toGlm(PxController->getPosition()));

		if (TextComponent* found = dynamic_cast<TextComponent*>(PossessedActor->GetRoot()->GetComponent("CameraText")))
			found->SetContent("Velocity: " + std::to_string(glm::length(Vec3f(Velocity.x, 0.0f, Velocity.z))) + " " + std::to_string(Velocity.y) + ((isOnGround) ? (" ON-GROUND") : (" MID-AIR")));
	}

	ShootingController::ShootingController(GameScene& scene, Actor* parentActor, const String& name) :
		FPSController(scene, parentActor, name),
		PossessedGunActor(nullptr)
	{
	}

	void ShootingController::HandleEvent(const Event& ev)
	{
		FPSController::HandleEvent(ev);
		if (ev.GetType() != EventType::MousePressed || GameHandle->GetCurrentMouseController() != this || !PossessedGunActor)
			return;

		const MouseButtonEvent& pressedEv = *dynamic_cast<const MouseButtonEvent*>(&ev);
		if (pressedEv.GetButton() == MouseButton::Right)
		{
			printVector(PossessedGunActor->GetTransform()->GetRot(), "Prawdziwa rotacja");
			printVector(PossessedGunActor->GetTransform()->GetWorldTransform().GetPos(), "Pozycja gracza");
			printVector(PossessedGunActor->GetTransform()->GetWorldTransform().GetFrontVec(), "Front");
		}
		if (pressedEv.GetButton() == MouseButton::Left)
		{

			
			PossessedGunActor->FireWeapon();
		}
	}

	void ShootingController::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		FPSController::GetEditorDescription(descBuilder);

		descBuilder.AddField("PossessedGunActor").GetTemplates().ObjectInput<Actor, GunActor>(*Scene.GetRootActor(), PossessedGunActor);
	}

}