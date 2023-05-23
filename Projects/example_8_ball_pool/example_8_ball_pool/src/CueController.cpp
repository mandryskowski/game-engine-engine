#include "CueController.h"
#include <physics/DynamicPhysicsObjects.h>
#include <game/GameScene.h>
#include <input/InputDevicesStateRetriever.h>
#include <PhysX/PxRigidDynamic.h>
#include <scene/CameraComponent.h>
#include "PoolBallActor.h"

#include <scene/UIInputBoxActor.h>
#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>

#include "physics/CollisionObject.h"
#include "physics/PhysicsEngineManager.h"

namespace GEE
{
	CueController::CueController(GameScene& scene, Actor* parentActor, const std::string& name) :
		Controller(scene, parentActor, name),
		WhiteBallActor(nullptr),
		CueMaxGivenSpeed(12.0f),
		BallStaticFriction(1.0f),
		BallDynamicFriction(0.5f),
		BallRestitution(1.0f),
		BallLinearDamping(0.5f),
		BallAngularDamping(0.5f),
		ConstrainedBallMovement(false),
		MinCueDistance(2.0f),
		MaxCueDistance(2.4f),
		PowerInverted(false),
		CueDistanceAnim(0.0f, 1.0f)
	{
		CueDistanceAnim.SetOnUpdateFunc([](double T) { return T == 1.0;  });
	}
	void CueController::OnStart()
	{
		std::cout << "Started pool ball game\n";
		ResetBalls();
	}
	void CueController::HandleEvent(const Event& ev)
	{
		Controller::HandleEvent(ev);

		if (ev.GetType() != EventType::MouseReleased || !WhiteBallActor || !PossessedActor || &Scene != GameHandle->GetActiveScene() || !WhiteBallActor->GetRoot()->GetCollisionObj() || WhiteBallActor->GetRoot()->GetCollisionObj()->IsStatic)
			return;

		if (auto cast = dynamic_cast<const MouseButtonEvent*>(&ev))
			if (cast->GetButton() == MouseButton::Left && CueDistanceAnim.GetT() > 0.0f)
			{
				Vec3f cueDir = PossessedActor->GetTransform()->GetWorldTransform().GetPos() - WhiteBallActor->GetTransform()->GetWorldTransform().GetPos();
				cueDir.y = 0.0f;
				cueDir = glm::normalize(cueDir);

				const Vec3f velChange = (CueMaxGivenSpeed * static_cast<float>(CueDistanceAnim.GetT())) * -cueDir;
				const Vec3f torque = (CueMaxGivenSpeed * static_cast<float>(CueDistanceAnim.GetT())) * glm::cross(cueDir, -cueDir);
				std::cout << "#POOL#> Launching cue ball with vel change " << velChange << " and torque" << torque << "\n";

				Physics::ApplyForce(*WhiteBallActor->GetRoot()->GetCollisionObj(), velChange, Physics::ForceMode::VelocityChange);
				//WhiteBallActor->GetRoot()->GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>()->addTorque(Physics::Util::toPx(torque));

				WhiteBallActor->GetRoot()->GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>()->setStabilizationThreshold(0.0f);
				WhiteBallActor->GetRoot()->GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>()->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
				//WhiteBallActor->GetRoot()->GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>()->setRigidDynamicLockFlags(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y);

				std::cout << "#POOL#> Mass: " << WhiteBallActor->GetRoot()->GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>()->getMass() << "\n";

				CueDistanceAnim.Reset();
				dynamic_cast<ModelComponent*>(PossessedActor->GetRoot())->SetHide(true);
			}
	}
	void CueController::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Controller::GetEditorDescription(descBuilder);

		auto forAllBallsFunc = [this](std::function<void(Component*)> iterationFunc)
		{
			if (auto poolBallsActor = Scene.FindActor("Poolballs"))
			{
				std::vector<Actor*> actors;
				poolBallsActor->GetAllActors(&actors);

				for (auto actor : actors)
					iterationFunc(actor->GetRoot());
			}
		};

		descBuilder.AddField("White ball").GetTemplates().ObjectInput<Actor, Actor>(*Scene.GetRootActor(), WhiteBallActor);
		descBuilder.AddField("Cue").GetTemplates().ObjectInput<Actor, Actor>(*Scene.GetRootActor(), [this](Actor* cue) { SetPossessedActor(cue); }, PossessedActor);
		descBuilder.AddField("Cue hit power").CreateChild<UIInputBoxActor>("CueHitPowerInputBox", [this](float val) { CueMaxGivenSpeed = val; }, [this]() { return CueMaxGivenSpeed; });
		descBuilder.AddField("Reset balls").CreateChild<UIButtonActor>("ResetBallsButton", "Reset", [this]() { ResetBalls(); });
		descBuilder.AddField("Constrain Y axis").GetTemplates().TickBox([this, forAllBallsFunc](bool val)
			{
				forAllBallsFunc([val](Component* comp) { comp->GetCollisionObj()->ActorPtr->is<physx::PxRigidDynamic>()->setRigidDynamicLockFlags((val) ? (physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y) : (static_cast<physx::PxRigidDynamicLockFlag::Enum>(0)));  });
				ConstrainedBallMovement = val;
			}, [this]() { return ConstrainedBallMovement; });

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
		descBuilder.AddField("Ball static friction").GetTemplates().SliderUnitInterval([this, updateMatFunc](float coeff) {	updateMatFunc(GameHandle->GetPhysicsHandle()->CreateMaterial(BallStaticFriction = coeff, BallDynamicFriction, BallRestitution)); }, BallStaticFriction);
		descBuilder.AddField("Ball dynamic friction").GetTemplates().SliderUnitInterval([this, updateMatFunc](float coeff) { updateMatFunc(GameHandle->GetPhysicsHandle()->CreateMaterial(BallStaticFriction, BallDynamicFriction = coeff, BallRestitution)); }, BallDynamicFriction);
		descBuilder.AddField("Ball restitution").GetTemplates().SliderUnitInterval([this, updateMatFunc](float coeff) {	updateMatFunc(GameHandle->GetPhysicsHandle()->CreateMaterial(BallStaticFriction, BallDynamicFriction, BallRestitution = coeff)); }, BallRestitution);


		descBuilder.AddField("Ball linear damping").GetTemplates().SliderUnitInterval([this, forAllBallsFunc](float coeff) { forAllBallsFunc([this, coeff](Component* comp) { Physics::SetLinearDamping(*comp->GetCollisionObj(), BallLinearDamping = coeff); });	}, BallLinearDamping);
		descBuilder.AddField("Ball angular damping").GetTemplates().SliderUnitInterval([this, forAllBallsFunc](float coeff) { forAllBallsFunc([this, coeff](Component* comp) { Physics::SetAngularDamping(*comp->GetCollisionObj(), BallAngularDamping = coeff); });	}, BallAngularDamping);
	}
	void CueController::OnMouseMovement(const Vec2f&, const Vec2f& currentPosPx, const Vec2u& windowSize)
	{
		if (!PossessedActor || !WhiteBallActor || !Scene.GetActiveCamera())
			return;

		const Vec2f currentPosViewportSpace = static_cast<Vec2f>(glm::inverse(GameHandle->GetScene("GEE_Editor")->FindActor("SceneViewportActor")->GetTransform()->GetWorldTransformMatrix()) * Vec4f(pxConversion::PxToNDC(currentPosPx, windowSize), 0.0f, 1.0f));

		const Vec4f whiteBallProj = Scene.GetActiveCamera()->GetProjectionMat() * Scene.GetActiveCamera()->GetViewMat() * Vec4f(WhiteBallActor->GetTransform()->GetWorldTransform().GetPos(), 1.0f);
		const Vec2f whiteBallViewportSpace = (static_cast<Vec3f>(whiteBallProj) / whiteBallProj.z);

		if (currentPosViewportSpace == whiteBallViewportSpace)
			return;

		Vec2f mouseDir = glm::normalize(whiteBallViewportSpace - currentPosViewportSpace);
		float cueRotation = std::atan2(mouseDir.y, mouseDir.x);

		ResetCue(cueRotation);
	}
	void CueController::Update(Time dt)
	{
		Controller::Update(dt);

		if (WhiteBallActor && WhiteBallActor->IsBeingKilled())
			WhiteBallActor = nullptr;

		if (WhiteBallActor && WhiteBallActor->GetRoot()->GetCollisionObj() && WhiteBallActor->GetTransform()->GetWorldTransform().GetPos().y < 0.95f)
		{
			WhiteBallActor->SetTransform(Scene.FindHierarchyTree("Pooltable")->GetRoot().FindNode("PoolBall_White")->GetCompBaseType().GetTransform());
			Physics::SetLinearVelocity(*WhiteBallActor->GetRoot()->GetCollisionObj(), Vec3f(0.0f));
			Physics::SetAngularVelocity(*WhiteBallActor->GetRoot()->GetCollisionObj(), Vec3f(0.0f));
		}

		if (auto poolBallsActor = Scene.FindActor("Poolballs"))
		{
			std::vector<Actor*> actors;
			poolBallsActor->GetAllActors(&actors);


			for (auto actor : actors)
			{
				auto colObj = actor->GetRoot()->GetCollisionObj();
				if (!colObj || !colObj->ActorPtr)
					continue;

				const auto absPos = glm::abs(actor->GetTransform()->GetWorldTransform().GetPos());

				if (absPos.y < 0.95f)
					actor->MarkAsKilled();
				else if (absPos.x > 1.95f || absPos.z > 0.887f)
				{
					auto rigidDynamicPtr = colObj->ActorPtr->is<physx::PxRigidDynamic>();


					if (rigidDynamicPtr)
					{
						rigidDynamicPtr->setRigidDynamicLockFlags(static_cast<physx::PxRigidDynamicLockFlag::Enum>(0));

						physx::PxRaycastBuffer castBuffer;
						physx::PxU32 maxHits = 1;
						physx::PxHitFlags hitFlags = physx::PxHitFlag::ePOSITION | physx::PxHitFlag::eNORMAL | physx::PxHitFlag::eUV;
						Transform possessedWorld = actor->GetTransform()->GetWorldTransform();

						
						auto frontVec = Vec3f(0.0f, -1.0f, 0.0f);
						bool bHit = Scene.GetPhysicsData()->GetPxScene()->raycast(Physics::Util::toPx(possessedWorld.GetPos()), Physics::Util::toPx(frontVec), 0.08f, castBuffer,
							physx::PxHitFlags(physx::PxHitFlag::eDEFAULT), physx::PxQueryFilterData(physx::PxQueryFlag::eSTATIC));
						std::cout << "Hit" << actor->GetName() << "? " << bHit << '\n';
						if (bHit)
							std::cout << "hitowane: " << castBuffer.getAnyHit(0).distance << '\n';
						else
							std::cout << "z: " << absPos.z << '\n';

					}
				}
				else
				{
					auto rigidDynamicPtr = colObj->ActorPtr->is<physx::PxRigidDynamic>();


					if (rigidDynamicPtr)
					{
						//rigidDynamicPtr->setRigidDynamicLockFlags(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y);
					}
				}
			}

			if (actors.empty())
				ResetBalls();
		}

		if (GameHandle->GetCurrentMouseController() != this || !WhiteBallActor)
			return;

		if (GameHandle->GetDefInputRetriever().IsMouseButtonPressed(MouseButton::Right))
		{
			CueDistanceAnim.UpdateT(-dt);
			ResetCue();
		}
		else if (GameHandle->GetDefInputRetriever().IsMouseButtonPressed(MouseButton::Left))
		{
			CueDistanceAnim.UpdateT(dt);
			ResetCue();
		}

	}
	void CueController::ResetBalls()
	{

		if (auto poolBallsActor = Scene.FindActor("Poolballs"))
		{
			// Kill existing balls
			{
				std::vector<Actor*> actors;
				poolBallsActor->GetAllActors(&actors);

				for (auto actor : actors)
					actor->MarkAsKilled();
			}


			if (auto tableHierarchyTree = Scene.FindHierarchyTree("Pooltable"))
			{
				std::vector<Hierarchy::NodeBase*> selected;

				// Select only ball nodes and instantiate
				for (int i = 0; i < static_cast<int>(tableHierarchyTree->GetRoot().GetChildCount()); i++)
				{
					auto node = tableHierarchyTree->GetRoot().GetChild(i);
					std::string stringName = node->GetCompBaseType().GetName();
					std::transform(stringName.begin(), stringName.end(), stringName.begin(), [](unsigned char ch) { return std::tolower(ch); });

					if (stringName.find("ball") != std::string::npos)
					{
						selected.push_back(node);
					}
				}
				Hierarchy::Instantiation::TreeInstantiation(*tableHierarchyTree, true).ToActors(tableHierarchyTree->GetRoot(), *poolBallsActor, [](Actor& parent) -> Actor& { return parent.CreateChild<PoolBallActor>(""); }, selected);

				// Update collision objects
				std::vector<Actor*> actors;
				poolBallsActor->GetAllActors(&actors);
				for (auto actor: actors)
				{
					auto colObj = actor->GetRoot()->GetCollisionObj();
					//auto colObj = (stringName.find("white") == std::string::npos)
					//	? (poolBallsActor->AddComponent(std::move(child->GenerateComp(MakeShared<Hierarchy::Instantiation::Data>(*tableHierarchyTree), *poolBallsActor))).GetCollisionObj())
					//	: (WhiteBallActor->GetRoot()->GetCollisionObj());



					if (!colObj)
						continue;

					if (!colObj->ActorPtr)
					{
						//std::cout << "INFO: Actor " << actor->GetName() << " does not have a PhysX actor.\n";
						continue;
					}


					auto rigidDynamicPtr = colObj->ActorPtr->is<physx::PxRigidDynamic>();

					if (!rigidDynamicPtr)
					{
						std::cout << "INFO: Actor " << actor->GetName() << " is not a rigid dynamic.\n";
						continue;
					}

					// Set ball material
					GEE_CORE_ASSERT(GameHandle->GetPhysicsHandle());
					physx::PxShape* shapePtr;
					physx::PxMaterial* mat = GameHandle->GetPhysicsHandle()->CreateMaterial(BallStaticFriction, BallDynamicFriction, BallRestitution);

					rigidDynamicPtr->getShapes(&shapePtr, 1);

					rigidDynamicPtr->detachShape(*shapePtr);
					shapePtr->setContactOffset(0.00f);
					shapePtr->setMaterials(&mat, 1);
					rigidDynamicPtr->attachShape(*shapePtr);

					// Set damping
					rigidDynamicPtr->setLinearDamping(BallLinearDamping);
					rigidDynamicPtr->setAngularDamping(BallAngularDamping);

					rigidDynamicPtr->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
					rigidDynamicPtr->setRigidDynamicLockFlags((ConstrainedBallMovement) ? (physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y) : (static_cast<physx::PxRigidDynamicLockFlag::Enum>(0)));

					rigidDynamicPtr->setMass(0.19f);
					physx::PxRigidBodyExt::updateMassAndInertia(*rigidDynamicPtr, 188.0f);
				
				}
			}

			// Find the white ball
			{
				std::vector<Actor*> actors;
				poolBallsActor->GetAllActors(&actors);

				for (auto actor : actors)
				{
					if (actor->IsBeingKilled())
						continue;

					auto stringName = actor->GetName();
					std::transform(stringName.begin(), stringName.end(), stringName.begin(), [](unsigned char ch) { return std::tolower(ch); });

					std::cout << "POOL>"<< stringName << '\n';
					if (stringName.find("white") != std::string::npos)
					{
						WhiteBallActor = actor;
						break;
					}
				}
			}
		}

		if (WhiteBallActor)
		{
			//WhiteBallActor->SetTransform(Scene.FindHierarchyTree("Pooltable")->GetRoot().FindNode("PoolBall_White")->GetCompBaseType().GetTransform());

			if (WhiteBallActor->GetRoot()->GetCollisionObj()->ActorPtr && !WhiteBallActor->GetRoot()->GetCollisionObj()->IsStatic)
			{
				Physics::SetLinearVelocity(*WhiteBallActor->GetRoot()->GetCollisionObj(), Vec3f(0.0f));
				Physics::SetAngularVelocity(*WhiteBallActor->GetRoot()->GetCollisionObj(), Vec3f(0.0f));
			}
		}
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
		auto cueDistance = MinCueDistance + (MaxCueDistance - MinCueDistance) * CueDistanceAnim.GetT();
		auto cueTransform = glm::translate(Mat4f(1.0f), WhiteBallActor->GetTransform()->GetWorldTransform().GetPos()) * static_cast<Mat4f>(rotation);

		PossessedActor->GetTransform()->SetPosition(Vec3f(cueTransform * Vec4f(0.0f, 0.0f, -cueDistance, 1.0f)) + Vec3f(0.0f, 0.03f, 0.0f));
		PossessedActor->GetTransform()->SetRotation(rotation * toQuat(Vec3f(0.0f, -90.0f, 0.0f)));
		dynamic_cast<ModelComponent*>(PossessedActor->GetRoot())->SetHide(false);
	}
}
