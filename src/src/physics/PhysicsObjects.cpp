#include <physics/PhysicsObjects.h>
#include <physics/CollisionObject.h>
#include <utility/Asserts.h>

#include "PhysicsEngineManager.h"

using namespace physx;
using namespace GEE::Physics::Util;

namespace GEE
{
	namespace Physics
	{
		void ApplyNewMaterial(PhysicsEngineManager& physicsHandle, CollisionObject& colObj, float staticFriction, float dynamicFriction, float restitution)
		{
			SetMaterial(colObj, physicsHandle.CreateMaterial(staticFriction, dynamicFriction, restitution));
		}
		void SetMaterial(CollisionObject& colObj, physx::PxMaterial* mat)
		{
			GEE_CORE_ASSERT(colObj.ActorPtr);

			physx::PxShape* shapePtr;

			colObj.ActorPtr->getShapes(&shapePtr, 1);
			colObj.ActorPtr->detachShape(*shapePtr);
			shapePtr->setMaterials(&mat, 1);
			colObj.ActorPtr->attachShape(*shapePtr);
		}
	}
}