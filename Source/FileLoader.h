#pragma once
#include "GameManager.h"
#include <assimp/types.h>

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
	static void LoadMaterials(RenderEngineManager*, SearchEngine*, std::string path, std::string directory);
	static void LoadShaders(RenderEngineManager*, std::stringstream&, std::string path);
	static void LoadComponentData(GameManager*, std::stringstream&, Actor* currentActor);
	static std::unique_ptr<CollisionObject> LoadCollisionObject(std::string path, PhysicsEngineManager* physicsHandle, std::stringstream&);
	static void LoadLightProbes(GameManager*, std::stringstream&);

	static void LoadMeshFromAi(Mesh* meshPtr, const aiScene* scene, aiMesh* mesh, std::string directory, bool bLoadMaterial, MaterialLoadingData* matLoadingData = nullptr, BoneMapping* = nullptr, std::vector<glm::vec3>* vertsPosPtr = nullptr, std::vector<unsigned int>* indicesPtr = nullptr, std::vector<Vertex>* verticesPtr = nullptr);

	static void LoadTransform(std::stringstream&, Transform&);
	static void LoadTransform(std::stringstream&, Transform&, std::string loadType);

	static MeshSystem::MeshTree* LoadCustomMeshTree(GameManager*, std::stringstream&, bool loadPath = false);
	static void LoadCustomMeshNode(GameManager*, std::stringstream&, MeshSystem::TemplateNode* parent = nullptr, MeshSystem::MeshTree* treeToEdit = nullptr);
	static void LoadMeshNodeFromAi(GameManager*, const aiScene*, std::string directory, MaterialLoadingData* matLoadingData, MeshSystem::TemplateNode& meshSystemNode, aiNode* node, BoneMapping& boneMapping, aiBone* bone = nullptr);
	static void LoadComponentsFromMeshTree(GameManager*, Component* comp, const MeshSystem::MeshTree&, const MeshSystem::TemplateNode*, SkeletonInfo& skeletonInfo, Material* overrideMaterial = nullptr);

public:
	static void LoadLevelFile(GameManager*, std::string path);
	static void LoadModel(GameManager*, std::string path, Component* comp, MeshTreeInstancingType type, Material* overrideMaterial = nullptr);
	static MeshSystem::MeshTree* LoadMeshTree(GameManager*, RenderEngineManager*, SearchEngine*, std::string path = std::string(), MeshSystem::MeshTree* treePtr = nullptr);
	static void InstantiateTree(GameManager*, Component* comp, MeshSystem::MeshTree&, MeshTreeInstancingType type = MeshTreeInstancingType::ROOTTREE, Material* overrideMaterial = nullptr);

	static std::shared_ptr<Font> LoadFont(std::string path);
};

aiBone* CastAiNodeToBone(const aiScene* scene, aiNode* node, const aiMesh** ownerMesh = nullptr);
glm::mat4 toGlm(const aiMatrix4x4&);