namespace physx
{
	class PxController;
	class PxMaterial;
}

namespace GEE
{
	class Transform;
	class GameRenderer;
	class GameScene;
	namespace Physics
	{
		class GameScenePhysicsData;
		struct CollisionObject;
		struct CollisionShape;

		class PhysicsEngineManager
		{
			friend class GEE::GameScene;
			friend class GEE::GameRenderer;
		public:
			virtual void CreatePxShape(CollisionShape&, CollisionObject&) = 0;
			virtual void AddCollisionObjectToPxPipeline(GameScenePhysicsData& scenePhysicsData, CollisionObject&) = 0;

			virtual physx::PxController* CreateController(GameScenePhysicsData& scenePhysicsData, const GEE::Transform& t) = 0;
			virtual physx::PxMaterial* CreateMaterial(float staticFriction, float dynamicFriction, float restitutionCoeff) = 0;

			virtual ~PhysicsEngineManager() = default;
		protected:
			virtual void RemoveScenePhysicsDataPtr(GameScenePhysicsData& scenePhysicsData) = 0;
		};
	}
}