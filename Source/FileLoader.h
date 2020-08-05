#pragma once
#include "GameManager.h"

class aiScene;
class aiNode;
class aiMesh;
class MaterialLoadingData;
class Vertex;
class Mesh;
class Transform;
class Component;
namespace MeshSystem
{
	class MeshNode;
	class MeshTree;
}

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

public:
	static void LoadMeshFromAi(Mesh* meshPtr, const aiScene* scene, aiMesh* mesh, std::string directory, bool bLoadMaterial, MaterialLoadingData* matLoadingData = nullptr, std::vector<glm::vec3>* vertsPosPtr = nullptr, std::vector<unsigned int>* indicesPtr = nullptr, std::vector<Vertex>* verticesPtr = nullptr);

	static void LoadTransform(std::stringstream&, Transform&);
	static void LoadTransform(std::stringstream&, Transform&, std::string loadType);

	static void LoadLevelFile(GameManager*, std::string path);
	static MeshSystem::MeshTree* LoadMeshTree(GameManager*, RenderEngineManager*, SearchEngine*, std::string path = std::string(), MeshSystem::MeshTree* treePtr = nullptr);
	static MeshSystem::MeshTree* LoadCustomMeshTree(GameManager*, std::stringstream&, bool loadPath = false);
	static void LoadCustomMeshNode(GameManager*, std::stringstream&, MeshSystem::MeshNode* parent = nullptr, MeshSystem::MeshTree* treeToEdit = nullptr);
	static void LoadMeshNodeFromAi(GameManager*, const aiScene*, std::string directory, aiNode* node, MaterialLoadingData* matLoadingData, MeshSystem::MeshNode& meshSystemNode);
	static void LoadComponentsFromMeshTree(GameManager*, Component* comp, const MeshSystem::MeshNode&, Material* overrideMaterial = nullptr);
	static void InstantiateTree(GameManager*, Component* comp, MeshSystem::MeshTree&, MeshTreeInstancingType type = MeshTreeInstancingType::ROOTTREE, Material* overrideMaterial = nullptr);
	static void LoadModel(GameManager*, std::string path, Component* comp, MeshTreeInstancingType type, Material* overrideMaterial = nullptr);
};