#include <physics/CollisionObject.h>
#include <PhysX/PxPhysicsAPI.h>

using namespace physx;

std::string collisionShapeTypeToString(CollisionShapeType type)
{
	switch (type)
	{
	case CollisionShapeType::COLLISION_BOX: return "Box";
	case CollisionShapeType::COLLISION_SPHERE: return "Sphere";
	case CollisionShapeType::COLLISION_TRIANGLE_MESH: return "Triangle Mesh";
	case CollisionShapeType::COLLISION_CAPSULE: return "Capsule";
	}
	return "CannotCastCollisionShapeToString";
}

glm::vec3 toGlm(PxVec3 pxVec)
{
	return glm::vec3(pxVec.x, pxVec.y, pxVec.z);
}

glm::quat toGlm(PxQuat pxQuat)
{
	return glm::quat(pxQuat.w, pxQuat.x, pxQuat.y, pxQuat.z);
}

PxVec3 toPx(const glm::vec3& glmVec)
{
	return PxVec3(glmVec.x, glmVec.y, glmVec.z);
}

PxQuat toPx(const glm::quat& glmQuat)
{
	return PxQuat(glmQuat.x, glmQuat.y, glmQuat.z, glmQuat.w);
}

PxTransform toPx(const Transform& t)
{
	return PxTransform(toPx(t.PositionRef), toPx(t.RotationRef));
}

Transform toTransform(const physx::PxTransform& t)
{
	return Transform(toGlm(t.p), toGlm(t.q));
}
