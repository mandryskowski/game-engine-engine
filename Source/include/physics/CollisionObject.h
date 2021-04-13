#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <PhysX/PxPhysicsAPI.h>
#include <math/Transform.h>
#include <game/GameScene.h>

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
	GameScenePhysicsData* ScenePhysicsData;
	physx::PxRigidActor* ActorPtr;
	std::vector <std::shared_ptr<CollisionShape>> Shapes;
	Transform* TransformPtr;
	unsigned int TransformDirtyFlag;
	bool IgnoreRotation;

	bool IsStatic;

	CollisionObject(bool isStatic = true) :
		ScenePhysicsData(nullptr),
		ActorPtr(nullptr),
		TransformPtr(nullptr),
		IsStatic(isStatic),
		IgnoreRotation(false),
		TransformDirtyFlag(std::numeric_limits<unsigned int>::max())
	{
	}
	CollisionObject(bool isStatic, CollisionShapeType type) :
		CollisionObject(isStatic)
	{
		AddShape(type);
	}
	CollisionObject(const CollisionObject& obj):
		ScenePhysicsData(obj.ScenePhysicsData),
		ActorPtr(obj.ActorPtr),
		Shapes(obj.Shapes),
		TransformPtr(obj.TransformPtr),
		TransformDirtyFlag(std::numeric_limits<unsigned int>::max()),
		IgnoreRotation(obj.IgnoreRotation),
		IsStatic(obj.IsStatic)
	{
	}
	CollisionObject(CollisionObject&& obj):
		CollisionObject(static_cast<const CollisionObject&>(obj))
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
	~CollisionObject()
	{
		//std::cout << "attempting to remove collision object " << this << "\n";
		if (ScenePhysicsData)
			ScenePhysicsData->EraseCollisionObject(*this);
	}
};

glm::vec3 toGlm(physx::PxVec3);
glm::quat toGlm(physx::PxQuat);

physx::PxVec3 toPx(glm::vec3);
physx::PxQuat toPx(glm::quat);
physx::PxTransform toPx(Transform);
physx::PxTransform toPx(Transform*);