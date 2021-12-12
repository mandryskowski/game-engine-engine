#pragma once
#if !defined(NDEBUG) && !defined(_DEBUG)
#define NDEBUG
#endif
#include <PhysX/PxPhysicsAPI.h>

#include <math/Vec.h>
#include <glm/gtc/quaternion.hpp>

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
			virtual void AddCollisionObjectToPxPipeline(GameScenePhysicsData& scenePhysicsData, CollisionObject&) override;

		public:
			virtual void CreatePxShape(CollisionShape&, CollisionObject&) override;
			void AddScenePhysicsDataPtr(GameScenePhysicsData& scenePhysicsData);
			virtual void RemoveScenePhysicsDataPtr(GameScenePhysicsData& scenePhysicsData) override;

			virtual physx::PxController* CreateController(GameScenePhysicsData& scenePhysicsData, const Transform& t) override;

			void SetupScene(GameScenePhysicsData& scenePhysicsData);

			void Update(float deltaTime);
			void UpdateTransforms();
			void UpdatePxTransforms();

			void DebugRender(GameScenePhysicsData& scenePhysicsData, RenderEngineManager&, SceneMatrixInfo&);
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
			Vec3f toVecColor(physx::PxDebugColor::Enum);
		}
	}
}