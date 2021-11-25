#pragma once
#include <rendering/Mesh.h>
#include <math/Vec.h>
#include <glm/gtc/quaternion.hpp>
#include <PhysX/PxPhysicsAPI.h>
#include <math/Transform.h>
#include <game/GameScene.h>
#include <game/GameManager.h>
#include <cereal/archives/json.hpp>

#include <scene/hierarchy/HierarchyTree.h>

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
			template <typename Archive> void Save(Archive& archive) const
			{
				std::string treeName, meshNodeName, meshSpecificName;
				if (OptionalLocalization)
				{
					treeName = OptionalLocalization->GetTreeName();
					meshNodeName = OptionalLocalization->ShapeMeshLoc.NodeName;
					meshSpecificName = OptionalLocalization->ShapeMeshLoc.SpecificName;
				}
				std::cout << "Zapisuje col shape " << treeName << "###" << meshNodeName << "###" << meshSpecificName << "///";
				if (OptionalLocalization)
					std::cout << OptionalLocalization->ShapeMeshLoc.SpecificName << " " << OptionalLocalization->OptionalCorrespondingMesh.GetVertexCount() << '\n';
				archive(CEREAL_NVP(Type), CEREAL_NVP(ShapeTransform), cereal::make_nvp("OptionalTreeName", treeName), cereal::make_nvp("OptionalMeshNodeName", meshNodeName), cereal::make_nvp("OptionalMeshSpecificName", meshSpecificName));
			}
			template <typename Archive> void Load(Archive& archive)
			{
				std::string treeName, meshNodeName, meshSpecificName;
				archive(CEREAL_NVP(Type), CEREAL_NVP(ShapeTransform), cereal::make_nvp("OptionalTreeName", treeName), cereal::make_nvp("OptionalMeshNodeName", meshNodeName), cereal::make_nvp("OptionalMeshSpecificName", meshSpecificName));
				if (Type == CollisionShapeType::COLLISION_TRIANGLE_MESH)
				{
					HierarchyTemplate::HierarchyTreeT* tree = EngineDataLoader::LoadHierarchyTree(*GameManager::DefaultScene, treeName);
					if (treeName.empty() || (meshNodeName.empty() && meshSpecificName.empty()))
					{
						std::cout << "ERROR: While serializing Triangle Mesh CollisionShape - No file path or no mesh name detected. Shape will not be added to the physics scene. Nr of verts: " << VertData.size() << "\n";
						return;
					}

					if (auto found = tree->FindTriangleMeshCollisionShape(meshNodeName, meshSpecificName))
						*this = *found;
					else if (auto foundMesh = tree->FindMesh(meshNodeName, meshSpecificName))
						*this = *EngineDataLoader::LoadTriangleMeshCollisionShape(GameManager::Get().GetPhysicsHandle(), *foundMesh);
					else
						std::cout << "ERROR: Could not load " << meshNodeName << "###" << meshSpecificName << " from " << treeName << ".\n";
				}
			}
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