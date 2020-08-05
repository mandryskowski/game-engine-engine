#include "PhysicsEngine.h"
#include <array>
#include "RenderEngine.h"
#include "CollisionObject.h"
#include "Mesh.h"
#include "Transform.h"


using namespace physx;

PhysicsEngine::PhysicsEngine(bool* debugmode):
	Foundation(nullptr),
	Physics(nullptr),
	Dispatcher(nullptr),
	Scene(nullptr),
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
	Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, Allocator, ErrorCallback);

	Pvd = PxCreatePvd(*Foundation);

	//PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("localhost", 5425, 1000);
	PxPvdTransport* transport = PxDefaultPvdFileTransportCreate("plik.pxd2");
	Pvd = PxCreatePvd(*Foundation);
	Pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *Foundation, PxTolerancesScale(), true, Pvd);

	PxSceneDesc sceneDesc(Physics->getTolerancesScale());
	Dispatcher = PxDefaultCpuDispatcherCreate(2);

	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = Dispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	Scene = Physics->createScene(sceneDesc);

	Cooking = PxCreateCooking(PX_PHYSICS_VERSION, *Foundation, PxCookingParams(PxTolerancesScale()));
	if (!Cooking)
		std::cerr << "ERROR! Can't initialize cooking.\n";

	PxPvdSceneClient* pvdClient = Scene->getScenePvdClient();
	if (pvdClient)
	{
		std::cout << "Pvd successful.\n";
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	DefaultMaterial = Physics->createMaterial(0.5f, 1.0f, 0.6f);

	PxRigidStatic* ground = PxCreatePlane(*Physics, PxPlane(0.0f, 1.0f, 0.0f, 0.5f), *DefaultMaterial);
	Scene->addActor(*ground);

	if (*DebugModePtr)
	{
		Scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 0.3f);
		Scene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 1.0f);
		Scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
	}
}

void PhysicsEngine::CreatePxActorForObject(CollisionObject* object)
{
	glm::vec3 worldObjectScale = object->TransformPtr->GetWorldTransform().ScaleRef;

	for (int i = 0; i < static_cast<int>(object->Shapes.size()); i++)
	{
		PxShape* pxShape = nullptr;
		const Transform& shapeT = object->Shapes[i]->ShapeTransform;
		glm::vec3 shapeScale = shapeT.ScaleRef * worldObjectScale;

		switch (object->Shapes[i]->Type)
		{
			case CollisionShapeType::COLLISION_TRIANGLE_MESH:
				pxShape = CreateTriangleMeshShape(object->Shapes[i].get(), shapeScale);
				break;
			case CollisionShapeType::COLLISION_BOX:
				pxShape = Physics->createShape(PxBoxGeometry(toPx(shapeScale * 0.5f)), *DefaultMaterial);
				break;
			case CollisionShapeType::COLLISION_SPHERE:
				pxShape = Physics->createShape(PxSphereGeometry(shapeScale.x), *DefaultMaterial);
				break;

		}

		if (!pxShape)
			continue;
	
		if (i == 0)
		{
			object->ActorPtr = (object->IsStatic) ?
						       (static_cast<PxRigidActor*>(PxCreateStatic(*Physics, toPx(object->TransformPtr), *pxShape))) :
							   (static_cast<PxRigidActor*>(PxCreateDynamic(*Physics, toPx(object->TransformPtr), *pxShape, 10.0f)));
		}
		else
			object->ActorPtr->attachShape(*pxShape);

		pxShape->setLocalPose(PxTransform(toPx(object->TransformPtr->GetWorldTransform().ScaleRef * shapeT.PositionRef), toPx(shapeT.RotationRef)));
	}

	if (!object->ActorPtr)
	{
		std::cerr << "ERROR! Can't create PxActor.\n";
		return;
	}

	Scene->addActor(*object->ActorPtr);
}

PxShape* PhysicsEngine::CreateTriangleMeshShape(CollisionShape* colShape, glm::vec3 scale)
{
	if (colShape->VertData.empty() || colShape->IndicesData.empty())
		return nullptr;

	PxTriangleMeshDesc desc;
	desc.points.count = colShape->VertData.size();
	desc.points.stride = sizeof(glm::vec3);
	desc.points.data = &colShape->VertData[0];

	desc.triangles.count = colShape->IndicesData.size() / 3;
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
		case PxTriangleMeshCookingResult::Enum::eSUCCESS: std::cout << "Successfully cooked a mesh!\n"; break;
		case PxTriangleMeshCookingResult::Enum::eLARGE_TRIANGLE: std::cout << "INFO: Triangles are too large in a cooked mesh!\n"; break;
		case PxTriangleMeshCookingResult::Enum::eFAILURE: std::cout << "ERROR! Can't cook a mesh\n"; return nullptr;
	}

	PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
	PxTriangleMesh* mesh = Physics->createTriangleMesh(readBuffer);
	PxMeshScale meshScale(toPx(scale));

	return Physics->createShape(PxTriangleMeshGeometry(mesh, meshScale, PxMeshGeometryFlag::eDOUBLE_SIDED), *DefaultMaterial);
}

CollisionObject* PhysicsEngine::CreateCollisionObject(glm::vec3 pos)
{
	CollisionObject* obj = new CollisionObject;
	obj->ActorPtr = PxCreateDynamic(*Physics, PxTransform(toPx(pos)), PxSphereGeometry(0.1f), *DefaultMaterial, 100.0f);
	CollisionObjects.push_back(obj);
	Scene->addActor(*obj->ActorPtr);

	return obj;
}

void PhysicsEngine::ApplyForce(CollisionObject* obj, glm::vec3 force)
{
	PxRigidDynamic* body = obj->ActorPtr->is<PxRigidDynamic>();

	if (body)
		body->addForce(toPx(force), PxForceMode::eIMPULSE);
}

void PhysicsEngine::AddCollisionObject(CollisionObject* object)
{
	CollisionObjects.push_back(object);

	if (WasSetup)
		CreatePxActorForObject(object);
}

void PhysicsEngine::Setup()
{
	for (int i = 0; i < static_cast<int>(CollisionObjects.size()); i++)
		CreatePxActorForObject(CollisionObjects[i]);

	WasSetup = true;
}

CollisionObject* PhysicsEngine::FindCollisionObject(std::string name)
{
	for (int i = 0; i < static_cast<int>(CollisionObjects.size()); i++)
		if (CollisionObjects[i]->Name == name)
			return CollisionObjects[i];

	return nullptr;
}

void PhysicsEngine::Update(float deltaTime)
{
	UpdatePxTransforms();

	Scene->simulate(deltaTime);
	Scene->fetchResults(true);

	UpdateTransforms();
}

void PhysicsEngine::UpdateTransforms()
{
	for (int i = 0; i < CollisionObjects.size(); i++)
	{
		CollisionObject* obj = CollisionObjects[i];
		if (!obj->ActorPtr || !obj->TransformPtr)
			continue;

		PxTransform& pxTransform = obj->ActorPtr->getGlobalPose();
		obj->TransformPtr->SetPositionWorld(toGlm(pxTransform.p));
		obj->TransformPtr->SetRotationWorld(toGlm(pxTransform.q));

		//obj->TransformPtr->SetMatrix(t.Matrix);

	}
}

void PhysicsEngine::UpdatePxTransforms()
{
	for (int i = 0; i < CollisionObjects.size(); i++)
	{
		CollisionObject* obj = CollisionObjects[i];
		if (!obj->ActorPtr || !obj->TransformPtr)// || (obj->TransformPtr->NotDirty())
			continue;
		

		const Transform& worldTransform = obj->TransformPtr->GetWorldTransform();

		PxTransform pxTransform;
		pxTransform.p = toPx(worldTransform.PositionRef);
		pxTransform.q = toPx(worldTransform.RotationRef);
		obj->ActorPtr->setGlobalPose(pxTransform);
	}
}

void PhysicsEngine::DebugRender(RenderEngine* engPtr, RenderInfo& info)
{
	if (!(*DebugModePtr))
	{
		std::cerr << "ERROR! Physics debug render called, but DebugMode is false.\n";
		return;
	}
	
	const PxRenderBuffer& rb = Scene->getRenderBuffer();


	std::vector<std::array<glm::vec3, 2>> verts;
	int sizeSum = rb.getNbPoints() + rb.getNbLines() * 2 + rb.getNbTriangles() * 3;
	int v = 0;
	verts.resize(sizeSum);

	for (int i = 0; i < rb.getNbPoints(); i++)
	{
		const PxDebugPoint& point = rb.getPoints()[i];

		verts[v][0] = toGlm(point.pos);
		verts[v++][1] = toVecColor(static_cast<PxDebugColor::Enum>(point.color));
	}
	

	for (int i = 0; i < rb.getNbLines(); i++)
	{
		const PxDebugLine& line = rb.getLines()[i];

		verts[v][0] = toGlm(line.pos0);
		verts[v++][1] = toVecColor(static_cast<PxDebugColor::Enum>(line.color0));
		verts[v][0] = toGlm(line.pos1);
		verts[v++][1] = toVecColor(static_cast<PxDebugColor::Enum>(line.color1));
	}

	for (int i = 0; i < rb.getNbTriangles(); i++)
	{
		const PxDebugTriangle& triangle = rb.getTriangles()[i];

		verts[v][0] = toGlm(triangle.pos0);
		verts[v++][1] = toVecColor(static_cast<PxDebugColor::Enum>(triangle.color0));
		verts[v][0] = toGlm(triangle.pos1);
		verts[v++][1] = toVecColor(static_cast<PxDebugColor::Enum>(triangle.color1));
		verts[v][0] = toGlm(triangle.pos2);
		verts[v++][1] = toVecColor(static_cast<PxDebugColor::Enum>(triangle.color2));
	}


	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 2 * verts.size(), &verts[0][0], GL_STATIC_DRAW);
	
	engPtr->RenderBoundInDebug(info, GL_POINTS, 0, rb.getNbPoints(), glm::vec3(0.0f));
	engPtr->RenderBoundInDebug(info, GL_LINES, rb.getNbPoints(), rb.getNbLines() * 2, glm::vec3(0.0f));
	engPtr->RenderBoundInDebug(info, GL_TRIANGLES, rb.getNbPoints() + rb.getNbLines() * 2, rb.getNbTriangles() * 3, glm::vec3(0.0f));
}

PhysicsEngine::~PhysicsEngine()
{
	Scene->release();
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

	Foundation->release();
	std::cout << "Physics engine successfully destroyed!\n";
}


glm::vec3 toVecColor(PxDebugColor::Enum col)
{
	switch (col)
	{
	case PxDebugColor::eARGB_BLACK: return glm::vec3(0.0f);
	case PxDebugColor::eARGB_BLUE: return glm::vec3(0.0f, 0.0f, 1.0f);
	case PxDebugColor::eARGB_CYAN: return glm::vec3(0.5f, 0.5f, 1.0f);
	case PxDebugColor::eARGB_DARKBLUE: return glm::vec3(0.0f, 0.0f, 0.75f);
	case PxDebugColor::eARGB_DARKGREEN: return glm::vec3(0.0f, 0.75f, 0.0f);
	case PxDebugColor::eARGB_DARKRED: return glm::vec3(0.75f, 0.0f, 0.0f);
	case PxDebugColor::eARGB_GREEN: return glm::vec3(0.0f, 1.0f, 0.0f);
	case PxDebugColor::eARGB_GREY: return glm::vec3(0.5f, 0.5f, 0.5f);
	case PxDebugColor::eARGB_MAGENTA : return glm::vec3(1.0f, 0.0f, 1.0f);
	case PxDebugColor::eARGB_RED: return glm::vec3(1.0f, 0.0f, 0.0f);
	case PxDebugColor::eARGB_WHITE: return glm::vec3(1.0f);
	case PxDebugColor::eARGB_YELLOW: return glm::vec3(1.0f, 1.0f, 0.0f);
	}

	return glm::vec3(0.0f);
}

glm::vec3 toGlm(PxVec3 pxVec)
{
	return glm::vec3(pxVec.x, pxVec.y, pxVec.z);
}

glm::quat toGlm(PxQuat pxQuat)
{
	return glm::quat(pxQuat.w, pxQuat.x, pxQuat.y, pxQuat.z);
}


PxVec3 toPx(glm::vec3 glmVec)
{
	return PxVec3(glmVec.x, glmVec.y, glmVec.z);
}

PxQuat toPx(glm::quat glmQuat)
{
	return PxQuat(glmQuat.x, glmQuat.y, glmQuat.z, glmQuat.w);
}

PxTransform toPx(Transform t)
{
	return PxTransform(toPx(t.PositionRef), toPx(t.RotationRef));
}

PxTransform toPx(Transform* t)
{
	return PxTransform(toPx(t->PositionRef), toPx(t->RotationRef));
}