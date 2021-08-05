#pragma once
#ifndef NDEBUG
#define NDEBUG
#endif
#include <PhysX/PxPhysicsAPI.h>

#include <math/Vec.h>
#include <glm/gtc/quaternion.hpp>

#include <game/GameScene.h>

#include <vector>
#include <game/GameManager.h>

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
			physx::PxShape* CreateTriangleMeshShape(CollisionShape*, Vec3f scale);
			virtual void AddCollisionObjectToPxPipeline(GameScenePhysicsData& scenePhysicsData, CollisionObject&) override;

		public:
			virtual void CreatePxShape(CollisionShape&, CollisionObject&) override;
			void AddScenePhysicsDataPtr(GameScenePhysicsData& scenePhysicsData);
			virtual void RemoveScenePhysicsDataPtr(GameScenePhysicsData& scenePhysicsData) override;

			virtual physx::PxController* CreateController(GameScenePhysicsData& scenePhysicsData, const Transform& t) override;

			virtual void ApplyForce(CollisionObject&, Vec3f force) override;
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
			Vec3f toVecColor(physx::PxDebugColor::Enum);
		}
	}
}