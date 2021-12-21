#include <physics/DynamicPhysicsObjects.h>
#include <physics/CollisionObject.h>
#include <PhysX/PxRigidDynamic.h>
#include <utility/Asserts.h>

using namespace physx;
using namespace GEE::Physics::Util;

namespace GEE
{
	namespace Physics
	{
		void ApplyForce(CollisionObject& obj, const Vec3f& force)
		{
			PxRigidDynamic* body = obj.ActorPtr->is<PxRigidDynamic>();

			if (body)
				body->addForce(toPx(force), PxForceMode::eIMPULSE);
		}

		void SetLinearVelocity(CollisionObject& obj, const Vec3f& velocity)
		{
			GEE_CORE_ASSERT(obj.ActorPtr && obj.ActorPtr->is<physx::PxRigidDynamic>());

			obj.ActorPtr->is<physx::PxRigidDynamic>()->setLinearVelocity(toPx(velocity));
		}

		void SetAngularVelocity(CollisionObject& obj, const Vec3f& angularVelocity)
		{
			GEE_CORE_ASSERT(obj.ActorPtr && obj.ActorPtr->is<physx::PxRigidDynamic>());

			obj.ActorPtr->is<physx::PxRigidDynamic>()->setAngularVelocity(toPx(angularVelocity));
		}

		void SetLinearDamping(CollisionObject& obj, float damping)
		{
			GEE_CORE_ASSERT(obj.ActorPtr && obj.ActorPtr->is<physx::PxRigidDynamic>());

			obj.ActorPtr->is<physx::PxRigidDynamic>()->setLinearDamping(damping);
		}

		void SetAngularDamping(CollisionObject& obj, float angularDamping)
		{
			GEE_CORE_ASSERT(obj.ActorPtr && obj.ActorPtr->is<physx::PxRigidDynamic>());

			obj.ActorPtr->is<physx::PxRigidDynamic>()->setAngularDamping(angularDamping);
		}
	}
}