#pragma once
#include <PhysX/PxPhysicsAPI.h>

#include <math/Vec.h>

#include <vector>
#include <game/GameManager.h>

namespace GEE
{
	struct CollisionShape;
	class RenderEngine;
	class Transform;

	namespace Physics
	{
		class PhysicsEngine : public PhysicsEngineManager
		{
		public:
			PhysicsEngine(bool* debugmode);
			void Init();

		private:
			physx::PxShape* CreateTriangleMeshShape(CollisionShape*, Vec3f scale);
			void AddCollisionObjectToPxPipeline(GameScenePhysicsData& scenePhysicsData, CollisionObject&) override;

			void AddScenePhysicsDataPtr(GameScenePhysicsData& scenePhysicsData);
			void RemoveScenePhysicsDataPtr(GameScenePhysicsData& scenePhysicsData) override;

		public:
			void CreatePxShape(CollisionShape&, CollisionObject&) override;

			physx::PxController* CreateController(GameScenePhysicsData& scenePhysicsData, const Transform& t) override;
			physx::PxMaterial* CreateMaterial(float staticFriction, float dynamicFriction, float restitutionCoeff) override;

			void SetupScene(GameScenePhysicsData& scenePhysicsData);

			void Update(Time deltaTime);
			void UpdateTransforms();
			void UpdatePxTransforms();

			void ConnectToPVD();

			~PhysicsEngine();

		private:
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
		};




		namespace Util
		{
		}
	}
}