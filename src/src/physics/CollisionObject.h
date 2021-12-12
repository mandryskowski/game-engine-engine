#pragma once
#include <rendering/Mesh.h>
#include <math/Vec.h>
#include <glm/gtc/quaternion.hpp>
#include <PhysX/PxPhysicsAPI.h>
#include <math/Transform.h>
#include <game/GameManager.h>
#include <cereal/archives/json.hpp>

#include <assetload/FileLoader.h>

#include <vector>

namespace physx
{
	class PxRigidActor;
}

namespace GEE
{
	class Transform;
	namespace Physics
	{
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
			physx::PxShape* ShapePtr;
			CollisionShapeType Type;
			struct ColShapeLoc : public HTreeObjectLoc
			{
				Mesh::MeshLoc ShapeMeshLoc;
				Mesh OptionalCorrespondingMesh;
				ColShapeLoc(Mesh::MeshLoc shapeMeshLoc);
			};
			SharedPtr<ColShapeLoc> OptionalLocalization;
			Transform ShapeTransform;
			std::vector<Vec3f> VertData;
			std::vector<unsigned int> IndicesData;

			CollisionShape(CollisionShapeType type = CollisionShapeType::COLLISION_BOX);
			CollisionShape(HTreeObjectLoc treeObjLoc, const std::string& meshName, CollisionShapeType type = CollisionShapeType::COLLISION_BOX);
			ColShapeLoc* GetOptionalLocalization();	//NOTE: Can be nullptr
			void SetOptionalLocalization(ColShapeLoc loc);
			template <typename Archive> void Save(Archive& archive)	const;
			template <typename Archive> void Load(Archive& archive);
		};

		struct CollisionObject
		{
			GameScenePhysicsData* ScenePhysicsData;
			physx::PxRigidActor* ActorPtr;
			std::vector <SharedPtr<CollisionShape>> Shapes;
			Transform* TransformPtr;
			unsigned int TransformDirtyFlag;
			bool IgnoreRotation;

			bool IsStatic;

			CollisionObject(bool isStatic = true);
			CollisionObject(bool isStatic, CollisionShapeType type);
			CollisionObject(const CollisionObject& obj);
			CollisionObject(CollisionObject&& obj);

			CollisionShape& AddShape(CollisionShapeType type);
			CollisionShape& AddShape(SharedPtr<CollisionShape> shape);

			void DetachShape(CollisionShape&);

			CollisionShape* FindTriangleMeshCollisionShape(const std::string& meshNodeName, const std::string& meshSpecificName);
			template <typename Archive> void Serialize(Archive& archive)
			{
				archive(CEREAL_NVP(IsStatic), CEREAL_NVP(IgnoreRotation), CEREAL_NVP(Shapes));
			}
			~CollisionObject();
		};

		namespace Util
		{
			std::string collisionShapeTypeToString(CollisionShapeType type);

			Vec3f toGlm(physx::PxVec3);
			Vec3f toGlm(physx::PxExtendedVec3);
			Quatf toGlm(physx::PxQuat);

			physx::PxVec3 toPx(const Vec3f&);
			physx::PxQuat toPx(const Quatf&);
			physx::PxTransform toPx(const Transform&);

			Transform toTransform(const physx::PxTransform&);
		}
	}
}