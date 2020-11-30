#pragma once
#ifndef NDEBUG
#define NDEBUG
#endif
#include <PhysX/PxPhysicsAPI.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GameScene.h"

#include <vector>
#include "GameManager.h"

struct CollisionShape;
class RenderEngine;
class Transform;
class RenderInfo;

class PhysicsEngine: public PhysicsEngineManager
{
	physx::PxDefaultAllocator Allocator;
	physx::PxDefaultErrorCallback ErrorCallback;

	static physx::PxFoundation* Foundation;
	physx::PxPhysics* Physics;

	physx::PxDefaultCpuDispatcher* Dispatcher;
	physx::PxCooking* Cooking;

	physx::PxMaterial* DefaultMaterial;
	physx::PxPvd* Pvd;

	std::vector <GameScenePhysicsData*> ScenesPhysicsData;
	unsigned int VAO, VBO;
	bool WasSetup;
	bool* DebugModePtr;

public:
	PhysicsEngine(bool* debugmode);
	void Init();

private:
	physx::PxShape* CreateTriangleMeshShape(CollisionShape*, glm::vec3 scale);
	void AddCollisionObjectToPx(GameScenePhysicsData* scenePhysicsData, CollisionObject*);

public:
	void AddScenePhysicsDataPtr(GameScenePhysicsData* scenePhysicsData);
	virtual void AddCollisionObject(GameScenePhysicsData* scenePhysicsData, CollisionObject*) override;

	virtual CollisionObject* CreateCollisionObject(GameScenePhysicsData* scenePhysicsData, glm::vec3 pos) override;
	virtual physx::PxController* CreateController(GameScenePhysicsData* scenePhysicsData) override;

	virtual void ApplyForce(CollisionObject*, glm::vec3 force) override;
	void SetupScene(GameScenePhysicsData* scenePhysicsData);

	void Update(float deltaTime);
	void UpdateTransforms();
	void UpdatePxTransforms();

	virtual void DebugRender(GameScenePhysicsData* scenePhysicsData, RenderEngine*, RenderInfo&) override;
	~PhysicsEngine();
};

glm::vec3 toVecColor(physx::PxDebugColor::Enum);