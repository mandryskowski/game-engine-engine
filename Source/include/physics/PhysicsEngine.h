#pragma once
#ifndef NDEBUG
#define NDEBUG
#endif
#include <PhysX/PxPhysicsAPI.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <game/GameScene.h>

#include <vector>
#include <game/GameManager.h>

#include <math/Vec.h>

namespace GEE
{
	struct CollisionShape;
	class RenderEngine;
	class Transform;
	class RenderInfo;

	namespace Physics
	{
		class PhysicsEngine : public PhysicsEngineManager
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
			virtual void AddCollisionObjectToPxPipeline(GameScenePhysicsData& scenePhysicsData, CollisionObject&) override;

		public:
			virtual void CreatePxShape(CollisionShape&, CollisionObject&) override;
			void AddScenePhysicsDataPtr(GameScenePhysicsData& scenePhysicsData);
			virtual void RemoveScenePhysicsDataPtr(GameScenePhysicsData& scenePhysicsData) override;

			virtual physx::PxController* CreateController(GameScenePhysicsData& scenePhysicsData, const Transform& t) override;

			virtual void ApplyForce(CollisionObject&, glm::vec3 force) override;
			void SetupScene(GameScenePhysicsData& scenePhysicsData);

			void Update(float deltaTime);
			void UpdateTransforms();
			void UpdatePxTransforms();

			virtual void DebugRender(GameScenePhysicsData& scenePhysicsData, RenderEngine&, RenderInfo&) override;
			~PhysicsEngine();
		};


		void ApplyForce(CollisionObject&, const Vec3f& force);

		namespace Util
		{
			glm::vec3 toVecColor(physx::PxDebugColor::Enum);
		}
	}
}