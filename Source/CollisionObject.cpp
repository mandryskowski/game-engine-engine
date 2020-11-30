#include "CollisionObject.h"
#include <PhysX/PxPhysicsAPI.h>

using namespace physx;

glm::vec3 toGlm(PxVec3 pxVec)
{
	return glm::vec3(pxVec.x, pxVec.y, pxVec.z);
}

glm::quat toGlm(PxQuat pxQuat)
{
	return glm::quat(pxQuat.w, pxQuat.x, pxQuat.y, pxQuat.z);
}

PxVec3 toPx(glm::vec3 glmVec)
{
	return PxVec3(glmVec.x, glmVec.y, glmVec.z);
}

PxQuat toPx(glm::quat glmQuat)
{
	return PxQuat(glmQuat.x, glmQuat.y, glmQuat.z, glmQuat.w);
}

PxTransform toPx(Transform t)
{
	return PxTransform(toPx(t.PositionRef), toPx(t.RotationRef));
}

PxTransform toPx(Transform* t)
{
	return PxTransform(toPx(t->PositionRef), toPx(t->RotationRef));
}