#pragma once
#include <math/Vec.h>

namespace physx
{
	class PxMaterial;
}

namespace GEE
{
	namespace Physics
	{
		struct CollisionObject;
		class PhysicsEngineManager;

		/**
		 * @brief Leave a parameter at -1.0f to get the value from the current material. If it does not exist, some default value will be taken. Only works for actors which do not share shapes.
		*/
		void ApplyNewMaterial(PhysicsEngineManager&, CollisionObject&, float staticFriction, float dynamicFriction = -1.0f, float restitution = -1.0f);
		void SetMaterial(CollisionObject&, physx::PxMaterial*);
	}
}