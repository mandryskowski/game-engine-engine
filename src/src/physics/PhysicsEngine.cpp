#include <physics/PhysicsEngine.h>
#include <physics/DynamicPhysicsObjects.h>
#include <game/GameScene.h>
#include <array>
#include <rendering/RenderEngine.h>
#include <physics/CollisionObject.h>
#include <rendering/Mesh.h>
#include <math/Transform.h>

using namespace physx;

namespace GEE
{
	namespace Physics
	{
		using namespace GEE::Physics::Util;

		physx::PxFoundation* PhysicsEngine::Foundation = nullptr;

		PhysicsEngine::PhysicsEngine(bool* debugmode) :
			Physics(nullptr),
			Dispatcher(nullptr),
			DefaultMaterial(nullptr),
			Pvd(nullptr),
			WasSetup(false)
		{
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);

			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(nullptr));
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 3));
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);

			DebugModePtr = debugmode;
		}

		void PhysicsEngine::Init()
		{
			if (!Foundation)
				Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, Allocator, ErrorCallback);

			Pvd = PxCreatePvd(*Foundation);

			PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("localhost", 5425, 1000);
			//PxPvdTransport* transport = PxDefaultPvdFileTransportCreate("plik.pvd");
			Pvd = PxCreatePvd(*Foundation);
			Pvd->connect(*transport, PxPvdInstrumentationFlag::eDEBUG);

			Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *Foundation, PxTolerancesScale(), true, Pvd);

			Cooking = PxCreateCooking(PX_PHYSICS_VERSION, *Foundation, PxCookingParams(PxTolerancesScale()));
			if (!Cooking)
				std::cerr << "ERROR! Can't initialize cooking.\n";

			DefaultMaterial = Physics->createMaterial(1.0f, 0.5f, 1.0f);
		}

		PxShape* PhysicsEngine::CreateTriangleMeshShape(CollisionShape* colShape, Vec3f scale)
		{
			if (colShape->VertData.empty() || colShape->IndicesData.empty())
			{
				std::cout << "ERROR: No VertData or IndicesData present in a triangle mesh collision shape. Vert count: " << colShape->VertData.size() << ". Index count: " << colShape->IndicesData.size() << "\n";
				return nullptr;
			}

			PxTriangleMeshDesc desc;
			desc.points.count = static_cast<physx::PxU32>(colShape->VertData.size());
			desc.points.stride = static_cast<physx::PxU32>(sizeof(Vec3f));
			desc.points.data = &colShape->VertData[0];

			desc.triangles.count = static_cast<physx::PxU32>(colShape->IndicesData.size() / 3);
			desc.triangles.stride = sizeof(unsigned int) * 3;
			desc.triangles.data = &colShape->IndicesData[0];

			PxDefaultMemoryOutputStream writeBuffer;
			PxTriangleMeshCookingResult::Enum result;
			bool status = Cooking->cookTriangleMesh(desc, writeBuffer, &result);
			if (!status)
			{
				std::cerr << "ERROR! Can't cook mesh with " << desc.points.count << " vertices.\n";
				return nullptr;
			}

			switch (result)
			{
			case PxTriangleMeshCookingResult::Enum::eSUCCESS: std::cout << "INFO: Succesfully cooked a mesh with " << desc.points.count << " vertices.\n"; break;
			case PxTriangleMeshCookingResult::Enum::eLARGE_TRIANGLE: std::cout << "INFO: Triangles are too large in a cooked mesh!\n"; break;
			case PxTriangleMeshCookingResult::Enum::eFAILURE: std::cout << "ERROR! Can't cook a mesh\n"; return nullptr;
			}

			PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
			PxTriangleMesh* mesh = Physics->createTriangleMesh(readBuffer);
			PxMeshScale meshScale(toPx(scale));

			return Physics->createShape(PxTriangleMeshGeometry(mesh, meshScale, PxMeshGeometryFlag::eDOUBLE_SIDED), *DefaultMaterial);
		}

		void PhysicsEngine::AddCollisionObjectToPxPipeline(GameScenePhysicsData& scenePhysicsData, CollisionObject& object)
		{
			if (object.ActorPtr)
			{
				std::cout << "INFO: The given CollisionObject is already associated with a PxActor object. No PxActor will be created.\n";
				return;
			}
			if (!object.TransformPtr)
			{
				std::cerr << "ERROR! The given CollisionObject does not have a pointer to any Transform object.\n";
				return;
			}
			Vec3f worldObjectScale = object.TransformPtr->GetWorldTransform().GetScale();

			for (int i = 0; i < static_cast<int>(object.Shapes.size()); i++)
				CreatePxShape(*object.Shapes[i], object);

			if (!object.ActorPtr)
			{
				std::cerr << "ERROR! Can't create PxActor. Number of shapes: " << object.Shapes.size() << ".\n";
				return;
			}

			scenePhysicsData.PhysXScene->addActor(*object.ActorPtr);
			//Scene->addActor(*object->ActorPtr);
		}

		void PhysicsEngine::AddScenePhysicsDataPtr(GameScenePhysicsData& scenePhysicsData)
		{
			ScenesPhysicsData.push_back(&scenePhysicsData);
		}

		void PhysicsEngine::RemoveScenePhysicsDataPtr(GameScenePhysicsData& scenePhysicsData)
		{
			ScenesPhysicsData.erase(std::remove_if(ScenesPhysicsData.begin(), ScenesPhysicsData.end(), [&scenePhysicsData](GameScenePhysicsData* scenePhysicsDataVec) { return scenePhysicsDataVec == &scenePhysicsData; }), ScenesPhysicsData.end());
		}


		void PhysicsEngine::CreatePxShape(CollisionShape& shape, CollisionObject& object)
		{
			Vec3f worldObjectScale = object.TransformPtr->GetWorldTransform().GetScale();

			const Transform& shapeT = shape.ShapeTransform;
			Vec3f shapeScale = shapeT.GetScale() * worldObjectScale;

			switch (shape.Type)
			{
			case CollisionShapeType::COLLISION_TRIANGLE_MESH:
				shape.ShapePtr = CreateTriangleMeshShape(&shape, shapeScale);
				break;
			case CollisionShapeType::COLLISION_BOX:
				shape.ShapePtr = Physics->createShape(PxBoxGeometry(toPx(shapeScale)), *DefaultMaterial);
				break;
			case CollisionShapeType::COLLISION_SPHERE:
				shape.ShapePtr = Physics->createShape(PxSphereGeometry(shapeScale.x), *DefaultMaterial);
				break;
			case CollisionShapeType::COLLISION_CAPSULE:
				shape.ShapePtr = Physics->createShape(PxCapsuleGeometry(shapeScale.x, shapeScale.y), *DefaultMaterial);
			}

			if (!shape.ShapePtr)
				return;

			shape.ShapePtr->setLocalPose(PxTransform(toPx(static_cast<Mat3f>(object.TransformPtr->GetWorldTransformMatrix()) * shapeT.GetPos()), (object.IgnoreRotation) ? (physx::PxQuat()) : (toPx(shapeT.GetRot()))));
			shape.ShapePtr->userData = &shape.ShapeTransform;

			if (!object.ActorPtr)
			{
				object.ActorPtr = (object.IsStatic) ?
					(static_cast<PxRigidActor*>(PxCreateStatic(*Physics, toPx(*object.TransformPtr), *shape.ShapePtr))) :
					(static_cast<PxRigidActor*>(PxCreateDynamic(*Physics, toPx(*object.TransformPtr), *shape.ShapePtr, 10.0f)));
				object.TransformDirtyFlag = object.TransformPtr->AddDirtyFlag();
			}
			else
				object.ActorPtr->attachShape(*shape.ShapePtr);
		}

		PxController* PhysicsEngine::CreateController(GameScenePhysicsData& scenePhysicsData, const Transform& t)
		{
			PxCapsuleControllerDesc desc;
			desc.radius = 0.12f;
			desc.height = (1.60f - 0.12f) * 2.0f;
			desc.material = DefaultMaterial;
			desc.position = PxExtendedVec3(0.0f, 0.0f, 0.0f);
			desc.position = PxExtendedVec3(t.GetPos().x, t.GetPos().y, t.GetPos().z);
			//desc.maxJumpHeight = 0.5f;
			//desc.invisibleWallHeight = 0.5f;
			desc.contactOffset = desc.radius * 0.1f;
			desc.stepOffset = 0.01f;
			//desc.slopeLimit = glm::cos(glm::radians(90.0f));

			PxController* controller = scenePhysicsData.PhysXControllerManager->createController(desc);

			PxRigidDynamic* actor = controller->getActor();
			actor->setName("controllercapsule");
			PxShape* shapes[1]; //There is only one shape in this controller
			actor->getShapes(shapes, 1, 0); //get that shape
			PxShape* shape = shapes[0];
			//shape->setLocalPose(physx::PxTransform(physx::PxVec3(0.0f), physx::PxQuat(physx::PxHalfPi, physx::PxVec3(0.0f, 0.0f, 1.0f))));	//rotate it so we get a vertical capsule (without this line the capsule would be oriented towards X+, I guess that's how physX defaults it)
			return controller;
		}

		physx::PxMaterial* PhysicsEngine::CreateMaterial(float staticFriction, float dynamicFriction, float restitutionCoeff)
		{
			return Physics->createMaterial(staticFriction, dynamicFriction, restitutionCoeff);
		}

		PxFilterFlags testCCDFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
		{
			pairFlags = PxPairFlag::eSOLVE_CONTACT;
			pairFlags |= PxPairFlag::eDETECT_DISCRETE_CONTACT;
			pairFlags |= PxPairFlag::eDETECT_CCD_CONTACT;
			return PxFilterFlags();
		}

		void PhysicsEngine::SetupScene(GameScenePhysicsData& scenePhysicsData)
		{
			if (!Physics)
				return;
			PxSceneDesc sceneDesc(Physics->getTolerancesScale());
			Dispatcher = PxDefaultCpuDispatcherCreate(2);

			sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
			sceneDesc.cpuDispatcher = Dispatcher;
			sceneDesc.filterShader = testCCDFilterShader;
			//sceneDesc.filterShader = testCCD;
			sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
			sceneDesc.bounceThresholdVelocity = 0.5f;
			scenePhysicsData.PhysXScene = Physics->createScene(sceneDesc);

			PxPvdSceneClient* pvdClient = scenePhysicsData.PhysXScene->getScenePvdClient();
			if (pvdClient)
			{
				std::cout << "Pvd successful.\n";
				pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
				pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
				pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
			}

			scenePhysicsData.PhysXControllerManager = PxCreateControllerManager(*scenePhysicsData.PhysXScene);

			PxRigidStatic* ground = PxCreatePlane(*Physics, PxPlane(0.0f, 1.0f, 0.0f, 0.0f), *DefaultMaterial);
			ground->setName("ground");
			scenePhysicsData.PhysXScene->addActor(*ground);

			if (*DebugModePtr)
			{
				scenePhysicsData.PhysXScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 0.3f);
				scenePhysicsData.PhysXScene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 1.0f);
				scenePhysicsData.PhysXScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
			}

			scenePhysicsData.WasSetup = true;

			for (int i = 0; i < static_cast<int>(scenePhysicsData.CollisionObjects.size()); i++)
				//if (!CollisionObjects[i]->ActorPtr)
				AddCollisionObjectToPxPipeline(scenePhysicsData, *scenePhysicsData.CollisionObjects[i]);

			AddScenePhysicsDataPtr(scenePhysicsData);
		}

		void PhysicsEngine::Update(Time deltaTime)
		{
			UpdatePxTransforms();

			for (int i = 0; i < static_cast<int>(ScenesPhysicsData.size()); i++)
			{
				ScenesPhysicsData[i]->PhysXScene->simulate(static_cast<physx::PxReal>(deltaTime));
				ScenesPhysicsData[i]->PhysXScene->fetchResults(true);
			}

			UpdateTransforms();
		}

		void PhysicsEngine::UpdateTransforms()
		{
			/*physx::PxRaycastBuffer castBuffer;
			physx::PxU32 maxHits = 1;
			physx::PxHitFlags hitFlags = physx::PxHitFlag::ePOSITION | physx::PxHitFlag::eNORMAL | physx::PxHitFlag::eUV;

			bool hit = ScenesPhysicsData[0]->GetPxScene()->raycast()*/

			for (int sceneIndex = 0; sceneIndex < static_cast<int>(ScenesPhysicsData.size()); sceneIndex++)
			{
				for (int i = 0; i < static_cast<int>(ScenesPhysicsData[sceneIndex]->CollisionObjects.size()); i++)
				{
					CollisionObject* obj = ScenesPhysicsData[sceneIndex]->CollisionObjects[i];
					if (!obj->ActorPtr || !obj->TransformPtr)
						continue;

					// Make sure that we do not change the flag which corresponds to updating px transforms
					bool flagBefore = obj->TransformPtr->GetDirtyFlag(obj->TransformDirtyFlag, false);

					PxTransform pxTransform = obj->ActorPtr->getGlobalPose();
					obj->TransformPtr->SetPositionWorld(toGlm(pxTransform.p));
					if (!obj->IgnoreRotation)
						obj->TransformPtr->SetRotationWorld(toGlm(pxTransform.q));

					if (!flagBefore)
						obj->TransformPtr->SetDirtyFlag(obj->TransformDirtyFlag, false);

					//obj->TransformPtr->SetMatrix(t.Matrix);
				}
			}
		}

		void PhysicsEngine::UpdatePxTransforms()
		{
			for (int sceneIndex = 0; sceneIndex < static_cast<int>(ScenesPhysicsData.size()); sceneIndex++)
			{
				for (int i = 0; i < static_cast<int>(ScenesPhysicsData[sceneIndex]->CollisionObjects.size()); i++)
				{
					CollisionObject* obj = ScenesPhysicsData[sceneIndex]->CollisionObjects[i];
					if (!obj->ActorPtr || !obj->TransformPtr || !obj->TransformPtr->GetDirtyFlag(obj->TransformDirtyFlag))
						continue;

					const Transform& worldTransform = obj->TransformPtr->GetWorldTransform();

					PxTransform pxTransform = obj->ActorPtr->getGlobalPose();
					pxTransform.p = toPx(worldTransform.GetPos());
					if (!obj->IgnoreRotation)
						pxTransform.q = toPx(worldTransform.GetRot());

					physx::PxShape** shapes = new physx::PxShape * [obj->ActorPtr->getNbShapes()];
					obj->ActorPtr->getShapes(shapes, obj->ActorPtr->getNbShapes());

					for (int j = 0; j < static_cast<int>(obj->ActorPtr->getNbShapes()); j++)
					{
						if (!shapes[j])
							continue;

						Transform* shapeTransform = (Transform*)shapes[j]->userData;
						if (!shapeTransform)
							continue;

					//	std::cout << "Updating shape of scale" << shapeTransform->GetScale().x << "...\n";

						switch (shapes[j]->getGeometryType())
						{
						case PxGeometryType::eTRIANGLEMESH:
						{
							physx::PxTriangleMeshGeometry meshGeom;
							shapes[j]->getTriangleMeshGeometry(meshGeom);
							meshGeom.scale = toPx(worldTransform.GetScale() * shapeTransform->GetScale());

							obj->ActorPtr->detachShape(*shapes[j]);
							shapes[j]->setGeometry(meshGeom);
							break;
						}
						case PxGeometryType::eBOX:
						{
							physx::PxBoxGeometry boxGeom;
							shapes[j]->getBoxGeometry(boxGeom);
							boxGeom.halfExtents = toPx(worldTransform.GetScale() * shapeTransform->GetScale());

							obj->ActorPtr->detachShape(*shapes[j]);
							shapes[j]->setGeometry(boxGeom);
							break;
						}
						case PxGeometryType::eSPHERE:
						{
							physx::PxSphereGeometry sphereGeom;
							shapes[j]->getSphereGeometry(sphereGeom);
							sphereGeom.radius = worldTransform.GetScale().x * shapeTransform->GetScale().x;

							obj->ActorPtr->detachShape(*shapes[j]);
							shapes[j]->setGeometry(sphereGeom);
							break;
						}
						case PxGeometryType::eCAPSULE:
							continue;
						default:
							std::cout << "Geometry type " << shapes[j]->getGeometryType() << " not supported.\n";
							continue; //skip iteration
							obj->ActorPtr->detachShape(*shapes[j]);
						}

						shapes[j]->setLocalPose(PxTransform(toPx(static_cast<Mat3f>(worldTransform.GetMatrix()) * shapeTransform->GetPos()), (obj->IgnoreRotation) ? (physx::PxQuat()) : (toPx(shapeTransform->GetRot()))));
						obj->ActorPtr->attachShape(*shapes[j]);
					}

					obj->ActorPtr->setGlobalPose(pxTransform);

					//delete[] shapes;
				}
			}
		}

		void PhysicsEngine::ConnectToPVD()
		{
			PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("localhost", 5425, 1000);
			Pvd->disconnect();
			Pvd->connect(*transport, PxPvdInstrumentationFlag::eDEBUG);
		}

		PhysicsEngine::~PhysicsEngine()
		{
			for (int i = 0; i < static_cast<int>(ScenesPhysicsData.size()); i++)
				ScenesPhysicsData[i]->PhysXScene->release();

			Dispatcher->release();
			Physics->release();
			Cooking->release();

			if (Pvd)
			{
				PxPvdTransport* transport = Pvd->getTransport();
				Pvd->release();
				Pvd = nullptr;
				if (transport)
					transport->release();
			}

			//Foundation->release();
			std::cout << "Physics engine successfully destroyed!\n";
		}
	}
}