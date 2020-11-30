#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <PhysX/PxPhysicsAPI.h>
#include "Transform.h"

#include <vector>

enum CollisionShapeType
{
	COLLISION_BOX,
	COLLISION_SPHERE,
	COLLISION_CAPSULE,
	COLLISION_TRIANGLE_MESH,
	COLLISION_FIRST = COLLISION_BOX,
	COLLISION_LAST = COLLISION_TRIANGLE_MESH
};

struct CollisionShape
{
	CollisionShapeType Type;
	Transform ShapeTransform;
	std::vector<glm::vec3> VertData;
	std::vector<unsigned int> IndicesData;
	CollisionShape(CollisionShapeType type = CollisionShapeType::COLLISION_BOX)
	{
		Type = type;
	}
};

namespace physx
{
	class PxRigidActor;
}
class Transform;

struct CollisionObject
{
	physx::PxRigidActor* ActorPtr;
	std::vector <std::shared_ptr<CollisionShape>> Shapes;
	Transform* TransformPtr;
	bool IgnoreRotation;

	bool IsStatic;

	CollisionObject(bool isStatic = true) :
		ActorPtr(nullptr),
		TransformPtr(nullptr),
		IsStatic(isStatic),
		IgnoreRotation(false)
	{
	}
	CollisionObject(bool isStatic, CollisionShapeType type) :
		ActorPtr(nullptr),
		TransformPtr(nullptr),
		IsStatic(isStatic),
		IgnoreRotation(false)
	{
		AddShape(type);
	}
	CollisionObject(const CollisionObject& obj):
		ActorPtr(nullptr),
		Shapes(obj.Shapes),
		IsStatic(obj.IsStatic),
		IgnoreRotation(false)
	{
	}
	CollisionObject(CollisionObject&& obj):
		CollisionObject((const CollisionObject&)obj)
	{

	}
	
	CollisionShape* AddShape(CollisionShapeType type)
	{
		Shapes.push_back(std::make_shared<CollisionShape>(type));

		return Shapes.back().get();
	}

	CollisionShape* AddShape(std::shared_ptr<CollisionShape> shape)
	{
		Shapes.push_back(shape);

		return Shapes.back().get();
	}
};

glm::vec3 toGlm(physx::PxVec3);
glm::quat toGlm(physx::PxQuat);

physx::PxVec3 toPx(glm::vec3);
physx::PxQuat toPx(glm::quat);
physx::PxTransform toPx(Transform);
physx::PxTransform toPx(Transform*);