#pragma once
#include <game/GameManager.h>
#include <assimp/types.h>
#include <ft2build.h>
#include <math/Transform.h>
#include FT_FREETYPE_H

struct aiScene;
struct aiNode;
struct aiMesh;
struct aiBone;

namespace GEE
{
	class Material;
	struct MaterialLoadingData;
	struct Vertex;
	class Mesh;
	class Transform;
	class Component;
	class BoneMapping;
	class SkeletonInfo;
	namespace Physics
	{
		struct CollisionShape;
		struct CollisionObject;
	}
	class Font;

	enum MeshTreeInstancingType
	{
		MERGE,
		ROOTTREE
	};


	// This code is absolutely horrendous and should be rewritten; some of it is redundant.
	class EngineDataLoader
	{
	public:
		static bool SetupSceneFromFile(GameManager*, const std::string& path, const std::string& name);
		static void LoadModel(std::string path, Component& comp, MeshTreeInstancingType type, Material* overrideMaterial = nullptr);

		static Hierarchy::Tree* LoadHierarchyTree(GameScene&, std::string path, Hierarchy::Tree* treePtr = nullptr, bool keepVertsData = true);

		static SharedPtr<Font> LoadFont(GameManager& gameHandle, const std::string& regularPath, const std::string& boldPath = "", const std::string& italicPath = "", const std::string& boldItalicPath = "");
		template <class T = GameSettings> static T LoadSettingsFromFile(std::string path);

		static SharedPtr<Physics::CollisionShape> LoadTriangleMeshCollisionShape(Physics::PhysicsEngineManager* physicsHandle, const Mesh& mesh);

	private:
		static void LoadMaterials(RenderEngineManager*, std::string path, std::string directory);
		static void LoadShaders(RenderEngineManager*, std::stringstream&, std::string path);
		static void LoadComponentData(GameManager*, std::stringstream&, Actor* currentActor, GameScene& scene);
		static UniquePtr<Physics::CollisionObject> LoadCollisionObject(GameScene& scene, std::stringstream&);
		static SharedPtr<Physics::CollisionShape> LoadTriangleMeshCollisionShape(Physics::PhysicsEngineManager* physicsHandle, const aiScene* scene, aiMesh&);
		static void LoadLightProbes(GameScene&, std::stringstream&);

		static void LoadMeshFromAi(Mesh* meshPtr, const aiScene* scene, const aiMesh* mesh, const HTreeObjectLoc& treeObjLoc, const std::string& directory = std::string(), bool bLoadMaterial = true, MaterialLoadingData* matLoadingData = nullptr, BoneMapping* = nullptr, bool keepVertsData = false);

		static void LoadTransform(std::stringstream&, Transform&);
		static void LoadTransform(std::stringstream&, Transform&, std::string loadType);

		static Hierarchy::Tree* LoadCustomHierarchyTree(GameScene& scene, std::stringstream& filestr, bool loadPath = false);

		static void LoadCustomHierarchyNode(GameScene&, std::stringstream&, Hierarchy::NodeBase* parent = nullptr, Hierarchy::Tree* treeToEdit = nullptr);

		static void LoadHierarchyNodeFromAi(GameManager&, const aiScene*, const std::string& directory, MaterialLoadingData* matLoadingData, const HTreeObjectLoc& treeObjLoc, Hierarchy::NodeBase& hierarchyNode, aiNode* node, BoneMapping* boneMapping = nullptr, aiBone* bone = nullptr, const Transform& parentTransform = Transform(), bool keepVertsData = false);

		static void LoadComponentsFromHierarchyTree(Component& comp, const Hierarchy::Tree&, const Hierarchy::NodeBase&, SkeletonInfo& skeletonInfo, const std::vector<Hierarchy::NodeBase*>& selectedComponents = {}, Material* overrideMaterial = nullptr);

		static FT_Library* FTLib;
	};

	aiBone* CastAiNodeToBone(const aiScene* scene, aiNode* node, const aiMesh** ownerMesh = nullptr);
	Mat4f toGlm(const aiMatrix4x4&);
}