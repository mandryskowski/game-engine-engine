#include <physics/CollisionObject.h>
#include <PhysX/PxPhysicsAPI.h>
#include <assetload/FileLoader.h>

using namespace physx;

namespace GEE
{
	namespace Physics
	{
		CollisionShape::ColShapeLoc::ColShapeLoc(Mesh::MeshLoc shapeMeshLoc) :
			HTreeObjectLoc(shapeMeshLoc),
			ShapeMeshLoc(shapeMeshLoc),
			OptionalCorrespondingMesh(Mesh::MeshLoc(shapeMeshLoc))
		{}

		CollisionShape::CollisionShape(CollisionShapeType type) :
			OptionalLocalization(nullptr),
			Type(type)
		{
		}

		CollisionShape::CollisionShape(HTreeObjectLoc treeObjLoc, const std::string& meshName, CollisionShapeType type) :
			OptionalLocalization(std::make_unique<ColShapeLoc>(Mesh::MeshLoc(treeObjLoc, meshName, meshName))),
			Type(type)
		{
		}

		CollisionShape::ColShapeLoc* CollisionShape::GetOptionalLocalization()
		{
			return OptionalLocalization.get();
		}

		void CollisionShape::SetOptionalLocalization(ColShapeLoc loc)
		{
			OptionalLocalization = std::make_unique<ColShapeLoc>(loc);
		}

		CollisionObject::CollisionObject(bool isStatic) :
			ScenePhysicsData(nullptr),
			ActorPtr(nullptr),
			TransformPtr(nullptr),
			IsStatic(isStatic),
			IgnoreRotation(false),
			TransformDirtyFlag(std::numeric_limits<unsigned int>::max())
		{
		}

		CollisionObject::CollisionObject(bool isStatic, CollisionShapeType type) :
			CollisionObject(isStatic)
		{
			AddShape(type);
		}

		CollisionObject::CollisionObject(const CollisionObject& obj) :
			ScenePhysicsData(obj.ScenePhysicsData),
			ActorPtr(obj.ActorPtr),
			Shapes(obj.Shapes),
			TransformPtr(obj.TransformPtr),
			TransformDirtyFlag(std::numeric_limits<unsigned int>::max()),
			IgnoreRotation(obj.IgnoreRotation),
			IsStatic(obj.IsStatic)
		{

		}

		CollisionObject::CollisionObject(CollisionObject&& obj) :
			CollisionObject(static_cast<const CollisionObject&>(obj))
		{
		}

		CollisionShape& CollisionObject::AddShape(CollisionShapeType type)
		{
			return AddShape(std::make_shared<CollisionShape>(type));
		}

		CollisionShape& CollisionObject::AddShape(std::shared_ptr<CollisionShape> shape)
		{
			Shapes.push_back(shape);

			if (ActorPtr)
				ScenePhysicsData->GetPhysicsHandle()->CreatePxShape(*shape, *this);
			return *Shapes.back();
		}

		CollisionShape* CollisionObject::FindTriangleMeshCollisionShape(const std::string& meshNodeName, const std::string& meshSpecificName)
		{
			if (meshNodeName.empty() && meshSpecificName.empty())
				return nullptr;

			std::function<bool(const std::string&, const std::string&)> checkEqual = [](const std::string& str1, const std::string& str2) -> bool { return str1.empty() || str1 == str2; };
			for (auto& it : Shapes)
				if (it->GetOptionalLocalization() && (checkEqual(meshNodeName, it->GetOptionalLocalization()->ShapeMeshLoc.NodeName)) && checkEqual(meshSpecificName, it->GetOptionalLocalization()->ShapeMeshLoc.NodeName))
					return it.get();


			return nullptr;
		}

		CollisionObject::~CollisionObject()
		{
			//std::cout << "attempting to remove collision object " << this << "\n";
			if (ScenePhysicsData)
				ScenePhysicsData->EraseCollisionObject(*this);
		}

		namespace Util
		{
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

			Vec3f toGlm(PxVec3 pxVec)
			{
				return Vec3f(pxVec.x, pxVec.y, pxVec.z);
			}

			Quatf toGlm(PxQuat pxQuat)
			{
				return Quatf(pxQuat.w, pxQuat.x, pxQuat.y, pxQuat.z);
			}

			PxVec3 toPx(const Vec3f& glmVec)
			{
				return PxVec3(glmVec.x, glmVec.y, glmVec.z);
			}

			PxQuat toPx(const Quatf& glmQuat)
			{
				return PxQuat(glmQuat.x, glmQuat.y, glmQuat.z, glmQuat.w);
			}

			PxTransform toPx(const Transform& t)
			{
				return PxTransform(toPx(t.Pos()), toPx(t.Rot()));
			}
			Transform toTransform(const physx::PxTransform& t)
			{
				return Transform(toGlm(t.p), toGlm(t.q));
			}
		}
	}
}