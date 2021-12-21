#include <physics/CollisionObject.h>
#include <game/GameScene.h>
#include <scene/hierarchy/HierarchyTree.h>
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
			Type(type),
			ShapePtr(nullptr)
		{
		}

		CollisionShape::CollisionShape(HTreeObjectLoc treeObjLoc, const std::string& meshName, CollisionShapeType type) :
			OptionalLocalization(MakeUnique<ColShapeLoc>(Mesh::MeshLoc(treeObjLoc, meshName, meshName))),
			Type(type),
			ShapePtr(nullptr)
		{
		}

		CollisionShape::ColShapeLoc* CollisionShape::GetOptionalLocalization()
		{
			return OptionalLocalization.get();
		}

		void CollisionShape::SetOptionalLocalization(ColShapeLoc loc)
		{
			OptionalLocalization = MakeUnique<ColShapeLoc>(loc);
		}

		template <typename Archive> void CollisionShape::Save(Archive& archive) const
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
		template <typename Archive> void CollisionShape::Load(Archive& archive)
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

		template void CollisionShape::Save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const;
		template void CollisionShape::Load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

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
			return AddShape(MakeShared<CollisionShape>(type));
		}

		CollisionShape& CollisionObject::AddShape(SharedPtr<CollisionShape> shape)
		{
			Shapes.push_back(shape);

			if (ActorPtr)
				ScenePhysicsData->GetPhysicsHandle()->CreatePxShape(*shape, *this);
			return *Shapes.back();
		}

		void CollisionObject::DetachShape(CollisionShape& shape)
		{
			Shapes.erase(std::remove_if(Shapes.begin(), Shapes.end(), [&](SharedPtr<CollisionShape> shapeVec) {
				if (shapeVec.get() != &shape) return false;
				if (shapeVec->ShapePtr) ActorPtr->detachShape(*shapeVec->ShapePtr);
			}), Shapes.end());
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

			Vec3f toGlm(physx::PxExtendedVec3 pxVec)
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
				return PxTransform(toPx(t.GetPos()), toPx(t.GetRot()));
			}
			Transform toTransform(const physx::PxTransform& t)
			{
				return Transform(toGlm(t.p), toGlm(t.q));
			}
		}
	}
}