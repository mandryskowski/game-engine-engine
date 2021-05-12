#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <PhysX/PxPhysicsAPI.h>
#include <math/Transform.h>
#include <game/GameScene.h>
#include <rendering/Mesh.h>
#include <assetload/FileLoader.h>

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

std::string collisionShapeTypeToString(CollisionShapeType type);

struct CollisionShape
{
	CollisionShapeType Type;
	struct ColShapeLoc : public HTreeObjectLoc
	{
		Mesh::MeshLoc ShapeMeshLoc;
		Mesh OptionalCorrespondingMesh;
		ColShapeLoc(Mesh::MeshLoc shapeMeshLoc) : HTreeObjectLoc(shapeMeshLoc), ShapeMeshLoc(shapeMeshLoc), OptionalCorrespondingMesh(Mesh::MeshLoc(shapeMeshLoc)) {}
	};
	std::shared_ptr<ColShapeLoc> OptionalLocalization;
	Transform ShapeTransform;
	std::vector<glm::vec3> VertData;
	std::vector<unsigned int> IndicesData;
	CollisionShape(CollisionShapeType type = CollisionShapeType::COLLISION_BOX) :
		OptionalLocalization(nullptr),
		Type(type)
	{
	}
	CollisionShape(HTreeObjectLoc treeObjLoc, const std::string& meshName, CollisionShapeType type = CollisionShapeType::COLLISION_BOX) :
		OptionalLocalization(std::make_unique<ColShapeLoc>(Mesh::MeshLoc(treeObjLoc, meshName, meshName))),
		Type(type)
	{
	}
	ColShapeLoc* GetOptionalLocalization()	//NOTE: Can be nullptr
	{
		return OptionalLocalization.get();
	}
	void SetOptionalLocalization(ColShapeLoc loc)
	{
		OptionalLocalization = std::make_unique<ColShapeLoc>(loc);
	}
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
				*this = *EngineDataLoader::LoadTriangleMeshCollisionShape(GameManager::DefaultScene->GetGameHandle()->GetPhysicsHandle(), *foundMesh);
			else
				std::cout << "ERROR: Could not load " << meshNodeName << "###" << meshSpecificName << " from " << treeName << ".\n";
		}
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
	CollisionShape& AddShape(CollisionShapeType type)
	{
		return AddShape(std::make_shared<CollisionShape>(type));
	}

	CollisionShape& AddShape(std::shared_ptr<CollisionShape> shape)
	{
		Shapes.push_back(shape);

		if (ActorPtr)
			ScenePhysicsData->GetPhysicsHandle()->CreatePxShape(*shape, *this);
		return *Shapes.back();
	}
	CollisionShape* FindTriangleMeshCollisionShape(const std::string& meshNodeName, const std::string& meshSpecificName)
	{
		if (meshNodeName.empty() && meshSpecificName.empty())
			return nullptr;

		std::function<bool(const std::string&, const std::string&)> checkEqual = [](const std::string& str1, const std::string& str2) -> bool { return str1.empty() || str1 == str2; };
		for (auto& it : Shapes)
			if (it->GetOptionalLocalization() && (checkEqual(meshNodeName, it->GetOptionalLocalization()->ShapeMeshLoc.NodeName)) && checkEqual(meshSpecificName, it->GetOptionalLocalization()->ShapeMeshLoc.NodeName))
				return it.get();


		return nullptr;
	}
	template <typename Archive> void Serialize(Archive& archive)
	{
		archive(CEREAL_NVP(IsStatic), CEREAL_NVP(IgnoreRotation), CEREAL_NVP(Shapes));
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

physx::PxVec3 toPx(const glm::vec3&);
physx::PxQuat toPx(const glm::quat&);
physx::PxTransform toPx(const Transform&);

Transform toTransform(const physx::PxTransform&);