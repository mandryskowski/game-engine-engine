#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
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
	std::string Name;	//Used for copying shapes from this object. Usually, Name should be set to the file path
	physx::PxRigidActor* ActorPtr;
	std::vector <std::shared_ptr<CollisionShape>> Shapes;
	Transform* TransformPtr;

	bool IsStatic;
	bool HasDefaultShapes;	//Used for shape loading. If the engine loads a model with the same path as a model that uses a "default" collision object, the pointers to the shapes from this CollisionObject will be copied

	CollisionObject(bool isStatic = true) :
		ActorPtr(nullptr),
		TransformPtr(nullptr),
		IsStatic(isStatic),
		HasDefaultShapes(false)
	{
	}
	CollisionObject(bool isStatic, CollisionShapeType type) :
		ActorPtr(nullptr),
		TransformPtr(nullptr),
		IsStatic(isStatic),
		HasDefaultShapes(false)
	{
		AddShape(type);
	}
	CollisionObject(const CollisionObject& obj):
		Name(obj.Name),
		ActorPtr(nullptr),
		Shapes(obj.Shapes),
		IsStatic(obj.IsStatic)
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
};