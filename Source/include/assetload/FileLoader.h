#pragma once
#include <game/GameManager.h>
#include <assimp/types.h>
#include <ft2build.h>
#include FT_FREETYPE_H

struct aiScene;
struct aiNode;
struct aiMesh;
struct aiBone;
struct MaterialLoadingData;
struct Vertex;
class Mesh;
class Transform;
class Component;
class BoneMapping;
class SkeletonInfo;
struct CollisionShape;
namespace MeshSystem
{
	class TemplateNode;
	class MeshNode;
	class MeshTree;
}

class Font;

enum MeshTreeInstancingType
{
	MERGE,
	ROOTTREE
};


class EngineDataLoader
{
public:
	static std::unique_ptr<GameScene> LoadSceneFromFile(GameManager*, std::string path);
	static void LoadModel(std::string path, Component& comp, MeshTreeInstancingType type, Material* overrideMaterial = nullptr);
	static MeshSystem::MeshTree* LoadMeshTree(GameManager*, RenderEngineManager*, std::string path = std::string(), MeshSystem::MeshTree* treePtr = nullptr);
	static void InstantiateTree(Component& comp, MeshSystem::MeshTree&, MeshTreeInstancingType type = MeshTreeInstancingType::ROOTTREE, Material* overrideMaterial = nullptr);

	static std::shared_ptr<Font> LoadFont(GameManager& gameHandle, const std::string& path);
	template <class T = GameSettings> static T LoadSettingsFromFile(std::string path);

private:
	static void LoadMaterials(RenderEngineManager*, std::string path, std::string directory);
	static void LoadShaders(RenderEngineManager*, std::stringstream&, std::string path);
	static void LoadComponentData(GameManager*, std::stringstream&, Actor* currentActor, GameScene& scene);
	static std::unique_ptr<CollisionObject> LoadCollisionObject(PhysicsEngineManager* physicsHandle, std::stringstream&);
	static std::unique_ptr<CollisionShape> LoadTriangleMeshCollisionShape(PhysicsEngineManager* physicsHandle, const aiScene* scene, aiMesh&);
	static void LoadLightProbes(GameScene&, std::stringstream&);

	static void LoadMeshFromAi(Mesh* meshPtr, const aiScene* scene, aiMesh* mesh, std::string directory = std::string(), bool bLoadMaterial = true, MaterialLoadingData* matLoadingData = nullptr, BoneMapping* = nullptr, std::vector<glm::vec3>* vertsPosPtr = nullptr, std::vector<unsigned int>* indicesPtr = nullptr, std::vector<Vertex>* verticesPtr = nullptr);

	static void LoadTransform(std::stringstream&, Transform&);
	static void LoadTransform(std::stringstream&, Transform&, std::string loadType);

	static MeshSystem::MeshTree* LoadCustomMeshTree(GameManager*, std::stringstream&, bool loadPath = false);
	static void LoadCustomMeshNode(GameManager*, std::stringstream&, MeshSystem::TemplateNode* parent = nullptr, MeshSystem::MeshTree* treeToEdit = nullptr);
	static void LoadMeshNodeFromAi(GameManager*, const aiScene*, std::string directory, MaterialLoadingData* matLoadingData, MeshSystem::TemplateNode& meshSystemNode, aiNode* node, BoneMapping& boneMapping, aiBone* bone = nullptr, const Transform& parentTransform = Transform());
	static void LoadComponentsFromMeshTree(Component& comp, const MeshSystem::MeshTree&, const MeshSystem::TemplateNode&, SkeletonInfo& skeletonInfo, Material* overrideMaterial = nullptr);
	
	static FT_Library* FTLib;
};

aiBone* CastAiNodeToBone(const aiScene* scene, aiNode* node, const aiMesh** ownerMesh = nullptr);
glm::mat4 toGlm(const aiMatrix4x4&);