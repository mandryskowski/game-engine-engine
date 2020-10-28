#pragma once
#ifndef NDEBUG
#define NDEBUG
#endif
#include <PhysX/PxPhysicsAPI.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include "GameManager.h"

struct CollisionShape;
class RenderEngine;
class Transform;
struct RenderInfo;

class PhysicsEngine: public PhysicsEngineManager
{
	physx::PxDefaultAllocator Allocator;
	physx::PxDefaultErrorCallback ErrorCallback;

	physx::PxFoundation* Foundation;
	physx::PxPhysics* Physics;

	physx::PxDefaultCpuDispatcher* Dispatcher;
	physx::PxScene* Scene;
	physx::PxCooking* Cooking;

	physx::PxMaterial* DefaultMaterial;
	physx::PxControllerManager* ControllerManager;
	physx::PxPvd* Pvd;

	std::vector <CollisionObject*> CollisionObjects;
	unsigned int VAO, VBO;
	bool WasSetup;
	bool* DebugModePtr;

public:
	PhysicsEngine(bool* debugmode);
	void Init();

private:
	void CreatePxActorForObject(CollisionObject*);
	physx::PxShape* CreateTriangleMeshShape(CollisionShape*, glm::vec3 scale);

public:
	virtual CollisionObject* CreateCollisionObject(glm::vec3 pos) override;
	virtual physx::PxController* CreateController() override;

	virtual void ApplyForce(CollisionObject*, glm::vec3 force) override;
	virtual void AddCollisionObject(CollisionObject*) override;
	void Setup();

	void Update(float deltaTime);
	void UpdateTransforms();
	void UpdatePxTransforms();

	virtual void DebugRender(RenderEngine*, RenderInfo&) override;
	~PhysicsEngine();
};

glm::vec3 toVecColor(physx::PxDebugColor::Enum);

glm::vec3 toGlm(physx::PxVec3);
glm::quat toGlm(physx::PxQuat);

physx::PxVec3 toPx(glm::vec3);
physx::PxQuat toPx(glm::quat);
physx::PxTransform toPx(Transform);
physx::PxTransform toPx(Transform*);