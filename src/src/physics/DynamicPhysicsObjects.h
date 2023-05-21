#pragma once
#include <math/Vec.h>

namespace GEE
{
	struct CollisionObject;
	namespace Physics
	{
		enum class ForceMode
		{
			Impulse,
			VelocityChange,
			Acceleration,
			Force
		};
		void ApplyForce(CollisionObject&, const Vec3f& force, ForceMode forceMode = ForceMode::Impulse);
		void SetLinearVelocity(CollisionObject&, const Vec3f& velocity);
		void SetAngularVelocity(CollisionObject&, const Vec3f& angularVelocity);
		void SetLinearDamping(CollisionObject&, float damping);
		void SetAngularDamping(CollisionObject&, float angularDamping);
	}
}