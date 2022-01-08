#include <scene/Controller.h>
#include <game/GameScene.h>
#include <scene/Component.h>
#include <physics/CollisionObject.h>
#include <physics/DynamicPhysicsObjects.h>
#include <animation/Animation.h>
#include <input/InputDevicesStateRetriever.h>
#include <scene/TextComponent.h>
#include <scene/CameraComponent.h>
#include <scene/GunActor.h>
#include <math/Transform.h>

#include <scene/hierarchy/HierarchyTree.h>
#include <scene/hierarchy/HierarchyNode.h>
#include <scene/hierarchy/HierarchyNodeInstantiation.h>

#include <input/Event.h>
#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>

namespace GEE
{
	Controller::Controller(GameScene& scene, Actor* parentActor, const std::string& name) :
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

	FreeRoamingController::FreeRoamingController(GameScene& scene, Actor* parentActor, const std::string& name) :
		Controller(scene, parentActor, name),
		RotationEuler(Vec3f(0.0f))
	{
		SetHideCursor(true);
		SetLockMouseAtCenter(true);
	}

	void FreeRoamingController::SetPossessedActor(Actor* actor)
	{
		Controller::SetPossessedActor(actor);
		RotationEuler = toEuler(actor->GetTransform()->GetRot());
		RotationEuler.z = 0.0f;
		std::cout << "Possessed actor rot euler " << RotationEuler << "\n";
	}

	Vec3i FreeRoamingController::GetMovementDirFromPressedKeys()
	{
		Vec3i movementDir(0);
		if (GameHandle->GetInputRetriever().IsKeyPressed(Key::W))
			movementDir.z -= 1;
		if (GameHandle->GetInputRetriever().IsKeyPressed(Key::S))
			movementDir.z += 1;
		if (GameHandle->GetInputRetriever().IsKeyPressed(Key::A))
			movementDir.x -= 1;
		if (GameHandle->GetInputRetriever().IsKeyPressed(Key::D))
			movementDir.x += 1;
		if (GameHandle->GetInputRetriever().IsKeyPressed(Key::Space))
			movementDir.y += 1;
		if (GameHandle->GetInputRetriever().IsKeyPressed(Key::LeftControl))
			movementDir.y -= 1;

		return movementDir;
	}

	void FreeRoamingController::Update(float deltaTime)
	{
		Controller::Update(deltaTime);

		if (!PossessedActor || &Scene != GameHandle->GetActiveScene())
			return;

		const Vec3i& movementDir = GetMovementDirFromPressedKeys();
		const Vec3f rotatedMovementDir = PossessedActor->GetTransform()->GetWorldTransform().GetRotationMatrix() * Vec3f(static_cast<float>(movementDir.x), 0.0f, static_cast<float>(movementDir.z)) // account for rotation on XZ
										+ Vec3f(0.0f, movementDir.y, 0.0f);	// ignore rotation on Y
		if (rotatedMovementDir == Vec3f(0.0f))
			return;

		PossessedActor->GetTransform()->Move(glm::normalize(rotatedMovementDir) * deltaTime);
	}

	void FreeRoamingController::OnMouseMovement(const Vec2f& previousPosPx, const Vec2f& currentPosPx, const Vec2u& windowSize)
	{
		if (!PossessedActor)
			return;

		Vec2f mouseOffset = currentPosPx - previousPosPx;
		mouseOffset.y *= -1.0f;

		float sensitivity = 0.15f;
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
	}

	FPSController::FPSController(GameScene& scene, Actor* parentActor, const std::string& name) :
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
		std::cout << "Tworze kontroler.\n";

		PxController = GameHandle->GetPhysicsHandle()->CreateController(*Scene.GetPhysicsData(), PossessedActor->GetTransform()->GetWorldTransform());

		UniquePtr<Physics::CollisionObject> colObject = MakeUnique<Physics::CollisionObject>(false);
		colObject->ActorPtr = PxController->getActor();
		colObject->IgnoreRotation = true;
		PossessedActor->GetRoot()->SetCollisionObject(std::move(colObject));
	}

	Vec3i FPSController::GetMovementDirFromPressedKeys()
	{
		const Vec3i& movementDir = FreeRoamingController::GetMovementDirFromPressedKeys();

		// Cancel out the alteration of movementDir due to pressing LeftControl.
		if (GameHandle->GetInputRetriever().IsKeyPressed(Key::LeftControl))
			return movementDir + Vec3i(0, 1, 0);

		// If LeftControl is not pressed, there is no need to change anything.
		return movementDir;
	}

	void FPSController::Update(float deltaTime)
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

		physx::PxRaycastBuffer dupa;
		physx::PxSweepBuffer sweepBuffer;
		physx::PxShape** shapes = new physx::PxShape * [16];
		PxController->getActor()->getShapes(shapes, 16);

		shapes[0]->acquireReference();
		PxController->getActor()->detachShape(*shapes[0]);

		bool groundSweep = PxController->getScene()->sweep(shapes[0]->getGeometry().capsule(), PxController->getActor()->getGlobalPose() * shapes[0]->getLocalPose(), PxController->getScene()->getGravity().getNormalized(), 0.5f, sweepBuffer, physx::PxHitFlag::eDEFAULT, physx::PxQueryFilterData(), nullptr);
		bool isOnGround = (groundSweep) && ((PxController->getFootPosition().y - sweepBuffer.block.position.y) < 0.01f);
		PxController->getActor()->attachShape(*shapes[0]);
		shapes[0]->release();

		Velocity.y -= 9.81f * deltaTime;

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

		PossessedActor->GetTransform()->SetPositionWorld(Physics::Util::toGlm(PxController->getPosition()));

		if (TextComponent* found = dynamic_cast<TextComponent*>(PossessedActor->GetRoot()->GetComponent("CameraText")))
			found->SetContent("Velocity: " + std::to_string(glm::length(Vec3f(Velocity.x, 0.0f, Velocity.z))) + " " + std::to_string(Velocity.y) + ((isOnGround) ? (" ON-GROUND") : (" MID-AIR")));
	}

	ShootingController::ShootingController(GameScene& scene, Actor* parentActor, const std::string& name) :
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
			PossessedGunActor->FireWeapon();
	}

	void ShootingController::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		FPSController::GetEditorDescription(descBuilder);

		descBuilder.AddField("PossessedGunActor").GetTemplates().ObjectInput<Actor, GunActor>(*Scene.GetRootActor(), PossessedGunActor);
	}
	CueController::CueController(GameScene& scene, Actor* parentActor, const std::string& name):
		Controller(scene, parentActor, name),
		WhiteBallActor(nullptr),
		CueHitPower(0.2f),
		ConstrainedBallMovement(false),
		BallStaticFriction(1.0f),
		BallDynamicFriction(0.5f),
		BallRestitution(1.0f),
		BallLinearDamping(0.5f),
		BallAngularDamping(0.5f),
		MinCueDistance(2.0f),
		MaxCueDistance(2.4f),
		CueDistanceAnim(0.0f, 1.0f),
		PowerInversed(false)
	{
		CueDistanceAnim.SetOnUpdateFunc([](float CompType) { return CompType == 1.0f;  });
	}
	void CueController::OnStart()
	{
		std::cout << "Started pool ball game\n";
		ResetBalls();
	}
	void CueController::HandleEvent(const Event& ev)
	{
		Controller::HandleEvent(ev);

		if (ev.GetType() != EventType::MouseReleased || !WhiteBallActor || !PossessedActor || &Scene != GameHandle->GetActiveScene() || !WhiteBallActor->GetRoot()->GetCollisionObj())
			return;

		if (auto cast = dynamic_cast<const MouseButtonEvent*>(&ev))
			if (cast->GetButton() == MouseButton::Left && CueDistanceAnim.GetT() > 0.0f)
			{
				Vec3f cueDir = PossessedActor->GetTransform()->GetWorldTransform().GetPos() - WhiteBallActor->GetTransform()->GetWorldTransform().GetPos();
				cueDir.y = 0.0f;
				cueDir = glm::normalize(cueDir);

				Vec3f force = (CueHitPower * CueDistanceAnim.GetT()) * -cueDir;
				Vec3f torque = (CueHitPower * CueDistanceAnim.GetT()) * glm::cross(cueDir, -cueDir);
				std::cout << "#POOL#> Launching cue ball with force " << force << " and torque" << torque << "\n";
				
				Physics::ApplyForce(*WhiteBallActor->GetRoot()->GetCollisionObj(), force);
				//WhiteBallActor->GetRoot()->GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>()->addTorque(Physics::Util::toPx(torque));

				WhiteBallActor->GetRoot()->GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>()->setStabilizationThreshold(0.0f);
				WhiteBallActor->GetRoot()->GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>()->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
				//WhiteBallActor->GetRoot()->GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>()->setRigidDynamicLockFlags(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y);

				if (auto poolBallsActor = Scene.FindActor("Poolballs"))
				{
					std::vector<Component*> components;
					poolBallsActor->GetRoot()->GetAllComponents(&components);

					for (auto comp : components)
						if (comp->GetCollisionObj())
						{
						//	Physics::SetLinearDamping(*comp->GetCollisionObj(), 0.2f);
						//	Physics::SetAngularDamping(*comp->GetCollisionObj(), 0.2f);
							comp->GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>()->setStabilizationThreshold(0.0f);
							//comp->GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>()->setRigidDynamicLockFlags(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y);
						}
				}

				CueDistanceAnim.Reset();
			}
	}
	void CueController::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Controller::GetEditorDescription(descBuilder);

		auto forAllBallsFunc = [this](std::function<void(Component*)> iterationFunc)
		{
			if (auto poolBallsActor = Scene.FindActor("Poolballs"))
			{
				std::vector<Component*> components;
				poolBallsActor->GetRoot()->GetAllComponents(&components);
				if (WhiteBallActor) components.push_back(WhiteBallActor->GetRoot());

				for (auto comp : components)
					iterationFunc(comp);
			}
		};

		descBuilder.AddField("White ball").GetTemplates().ObjectInput<Actor, Actor>(*Scene.GetRootActor(), WhiteBallActor);
		descBuilder.AddField("Cue").GetTemplates().ObjectInput<Actor, Actor>(*Scene.GetRootActor(), [this](Actor* cue) { SetPossessedActor(cue); });
		descBuilder.AddField("Cue hit power").CreateChild<UIInputBoxActor>("CueHitPowerInputBox", [this](float val) { CueHitPower = val; }, [this]() { return CueHitPower; });
		descBuilder.AddField("Reset balls").CreateChild<UIButtonActor>("ResetBallsButton", "Reset", [this]() { ResetBalls(); });
		descBuilder.AddField("Constrain Y axis").GetTemplates().TickBox([this, forAllBallsFunc](bool val)
		{
			forAllBallsFunc([val](Component* comp) { comp->GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>()->setRigidDynamicLockFlags((val) ? (physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y) : (static_cast<physx::PxRigidDynamicLockFlag::Enum>(0)));  });
			ConstrainedBallMovement = val;
		}, [this](){ return ConstrainedBallMovement;});

		auto updateMatFunc = [this, forAllBallsFunc](physx::PxMaterial* mat)
		{
			if (auto poolBallsActor = Scene.FindActor("Poolballs"))
			{
				physx::PxMaterial* prevMat = nullptr;

				forAllBallsFunc([this, mat, &prevMat](Component* comp)
				{
					physx::PxShape* shapePtr;

					comp->GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>()->getShapes(&shapePtr, 1);
					shapePtr->getMaterials(&prevMat, 1);

					std::cout << "Shape actor before: " << shapePtr->getReferenceCount() << '\n';
					comp->GetCollisionObj()->ActorPtr->detachShape(*shapePtr);
					std::cout << "Shape actor after: " << shapePtr->getReferenceCount() << '\n';
					shapePtr->setMaterials(&mat, 1);
					comp->GetCollisionObj()->ActorPtr->attachShape(*shapePtr);

				});

				if (prevMat)
					prevMat->release();
			}
		};
		descBuilder.AddField("Ball static friction").GetTemplates().SliderUnitInterval([this, updateMatFunc](float coeff) {	updateMatFunc(GameHandle->GetPhysicsHandle()->CreateMaterial(BallStaticFriction = coeff, BallDynamicFriction, BallRestitution));}, BallStaticFriction);
		descBuilder.AddField("Ball dynamic friction").GetTemplates().SliderUnitInterval([this, updateMatFunc](float coeff) { updateMatFunc(GameHandle->GetPhysicsHandle()->CreateMaterial(BallStaticFriction, BallDynamicFriction = coeff, BallRestitution));}, BallDynamicFriction);
		descBuilder.AddField("Ball restitution").GetTemplates().SliderUnitInterval([this, updateMatFunc](float coeff) {	updateMatFunc(GameHandle->GetPhysicsHandle()->CreateMaterial(BallStaticFriction, BallDynamicFriction, BallRestitution = coeff));}, BallRestitution);


		descBuilder.AddField("Ball linear damping").GetTemplates().SliderUnitInterval([this, forAllBallsFunc](float coeff) { forAllBallsFunc([this, coeff](Component* comp) { Physics::SetLinearDamping(*comp->GetCollisionObj(), BallLinearDamping = coeff); });	}, BallLinearDamping);
		descBuilder.AddField("Ball angular damping").GetTemplates().SliderUnitInterval([this, forAllBallsFunc](float coeff) { forAllBallsFunc([this, coeff](Component* comp) { Physics::SetAngularDamping(*comp->GetCollisionObj(), BallAngularDamping = coeff); });	}, BallAngularDamping);
	}
	void CueController::OnMouseMovement(const Vec2f&, const Vec2f& currentPosPx, const Vec2u& windowSize)
	{
		if (!PossessedActor || !WhiteBallActor || !Scene.GetActiveCamera())
			return;

		const Vec2f currentPosViewportSpace = static_cast<Vec2f>(glm::inverse(GameHandle->GetScene("GEE_Editor")->FindActor("SceneViewportActor")->GetTransform()->GetWorldTransformMatrix()) * Vec4f(pxConversion::PxToNDC(currentPosPx, windowSize), 0.0f, 1.0f));

		const Vec4f whiteBallProj = Scene.GetActiveCamera()->GetProjectionMat() * Scene.GetActiveCamera()->GetViewMat() *  Vec4f(WhiteBallActor->GetTransform()->GetWorldTransform().GetPos(), 1.0f);
		const Vec2f whiteBallViewportSpace = (static_cast<Vec3f>(whiteBallProj) / whiteBallProj.z);
		
		if (currentPosViewportSpace == whiteBallViewportSpace)
			return;

		Vec2f mouseDir = glm::normalize(whiteBallViewportSpace - currentPosViewportSpace);
		float cueRotation = std::atan2(mouseDir.y, mouseDir.x);

		ResetCue(cueRotation);
	}
	void CueController::Update(float deltaTime)
	{
		Controller::Update(deltaTime);

		if (WhiteBallActor && WhiteBallActor->IsBeingKilled())
			WhiteBallActor = nullptr;

		if (WhiteBallActor && WhiteBallActor->GetRoot()->GetCollisionObj() && WhiteBallActor->GetTransform()->GetPos().y < 0.95f)
		{
			WhiteBallActor->SetTransform(Scene.FindHierarchyTree("Pooltable")->GetRoot().FindNode("PoolBall_White")->GetCompBaseType().GetTransform());
			Physics::SetLinearVelocity(*WhiteBallActor->GetRoot()->GetCollisionObj(), Vec3f(0.0f));
			Physics::SetAngularVelocity(*WhiteBallActor->GetRoot()->GetCollisionObj(), Vec3f(0.0f));
		}

		if (auto poolBallsActor = Scene.FindActor("Poolballs"))
		{
			std::vector<Component*> components;
			poolBallsActor->GetRoot()->GetAllComponents(&components);

			for (auto comp : components)
				if (comp->GetCollisionObj() && comp->GetTransform().GetWorldTransform().GetPos().y < 0.95f)
					comp->MarkAsKilled();

			if (components.empty())
				ResetBalls();
		}

		if (&Scene != GameHandle->GetActiveScene())
			return;

		if (GameHandle->GetInputRetriever().IsMouseButtonPressed(MouseButton::Right))
		{
			CueDistanceAnim.UpdateT(-deltaTime);
			ResetCue();
		}
		else if (GameHandle->GetInputRetriever().IsMouseButtonPressed(MouseButton::Left))
		{
			CueDistanceAnim.UpdateT(deltaTime);
			ResetCue();
		}

	}
	void CueController::ResetBalls()
	{
		if (auto poolBallsActor = Scene.FindActor("Poolballs"))
		{
			std::vector<Component*> components;
			poolBallsActor->GetRoot()->GetAllComponents(&components);

			for (auto comp : components)
				comp->MarkAsKilled();

			if (auto tableHierarchyTree = Scene.FindHierarchyTree("Pooltable"))
			{
				for (int i = 0; i < static_cast<int>(tableHierarchyTree->GetRoot().GetChildCount()); i++)
				{
					auto child = tableHierarchyTree->GetRoot().GetChild(i);
					std::string stringName = child->GetCompBaseType().GetName();
					std::transform(stringName.begin(), stringName.end(), stringName.begin(), [](unsigned char ch) { return std::tolower(ch); });
					if (stringName.find("ball") != std::string::npos)
					{
						auto rigidDynamicPtr = (stringName.find("white") == std::string::npos) ? (poolBallsActor->AddComponent(std::move(child->GenerateComp(MakeShared<HierarchyTemplate::NodeInstantiation::Data>(*tableHierarchyTree), *poolBallsActor))).GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>()) : (WhiteBallActor->GetRoot()->GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>());

						// Set ball material
						physx::PxShape* shapePtr;
						physx::PxMaterial* mat = GameHandle->GetPhysicsHandle()->CreateMaterial(BallStaticFriction, BallDynamicFriction, BallRestitution);

						rigidDynamicPtr->getShapes(&shapePtr, 1);

						rigidDynamicPtr->detachShape(*shapePtr);
						shapePtr->setContactOffset(0.001f);
						shapePtr->setMaterials(&mat, 1);
						rigidDynamicPtr->attachShape(*shapePtr);

						// Set damping
						rigidDynamicPtr->setLinearDamping(BallLinearDamping);
						rigidDynamicPtr->setAngularDamping(BallAngularDamping);

						rigidDynamicPtr->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
						rigidDynamicPtr->setRigidDynamicLockFlags((ConstrainedBallMovement) ? (physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y) : (static_cast<physx::PxRigidDynamicLockFlag::Enum>(0)));
					}
				}
			}
		}

		WhiteBallActor->SetTransform(Scene.FindHierarchyTree("Pooltable")->GetRoot().FindNode("PoolBall_White")->GetCompBaseType().GetTransform());
		Physics::SetLinearVelocity(*WhiteBallActor->GetRoot()->GetCollisionObj(), Vec3f(0.0f));
		Physics::SetAngularVelocity(*WhiteBallActor->GetRoot()->GetCollisionObj(), Vec3f(0.0f));
	}
	void CueController::ResetCue(float rotationRad)
	{
		/*float cueDistance = MinCueDistance + (MaxCueDistance - MinCueDistance) * CueDistanceAnim.GetT();
		Mat4f cueTransform = glm::translate(Mat4f(1.0f), WhiteBallActor->GetTransform()->GetWorldTransform().GetPos()) * glm::rotate(Mat4f(1.0f), rotationRad, Vec3f(0.0f, 1.0f, 0.0f));

		PossessedActor->GetTransform()->SetPosition(Vec3f(cueTransform * Vec4f(0.0f, 0.0f, -cueDistance, 1.0f)));
		PossessedActor->GetTransform()->SetRotation(Vec3f(0.0f, glm::degrees(rotationRad) - 90.0f, 0.0f));*/
		ResetCue(toQuat(Vec3f(0.0f, glm::degrees(rotationRad), 0.0f)));
	}
	void CueController::ResetCue(Quatf rotation)
	{
		if (rotation == Quatf())
			rotation = PossessedActor->GetTransform()->GetWorldTransform().GetRot() * toQuat(Vec3f(0.0f, 90.0f, 0.0f));
		float cueDistance = MinCueDistance + (MaxCueDistance - MinCueDistance) * CueDistanceAnim.GetT();
		Mat4f cueTransform = glm::translate(Mat4f(1.0f), WhiteBallActor->GetTransform()->GetWorldTransform().GetPos()) * static_cast<Mat4f>(rotation);

		PossessedActor->GetTransform()->SetPosition(Vec3f(cueTransform * Vec4f(0.0f, 0.0f, -cueDistance, 1.0f)));
		PossessedActor->GetTransform()->SetRotation(rotation * toQuat(Vec3f(0.0f, -90.0f, 0.0f)));
	}
}