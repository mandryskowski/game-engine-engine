#pragma once
#include <math/Vec.h>

namespace GEE
{
	struct CollisionObject;
	namespace Physics
	{
		void ApplyForce(CollisionObject&, const Vec3f& force);
		void SetLinearVelocity(CollisionObject&, const Vec3f& velocity);
		void SetAngularVelocity(CollisionObject&, const Vec3f& angularVelocity);
		void SetLinearDamping(CollisionObject&, float damping);
		void SetAngularDamping(CollisionObject&, float angularDamping);
	}
}