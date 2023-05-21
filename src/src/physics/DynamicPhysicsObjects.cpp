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
		void ApplyForce(CollisionObject& obj, const Vec3f& force, ForceMode forceMode)
		{
			PxRigidDynamic* body = obj.ActorPtr->is<PxRigidDynamic>();

			PxForceMode::Enum pxForceMode;
			switch (forceMode)
			{
			default: case ForceMode::Impulse: pxForceMode = PxForceMode::eIMPULSE; break;
			case ForceMode::VelocityChange: pxForceMode = PxForceMode::eVELOCITY_CHANGE; break;
			case ForceMode::Acceleration: pxForceMode = PxForceMode::eACCELERATION; break;
			case ForceMode::Force: pxForceMode = PxForceMode::eFORCE; break;
			}

			if (body)
				body->addForce(toPx(force), pxForceMode);
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