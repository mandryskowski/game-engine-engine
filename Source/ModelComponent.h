#pragma once
#include "Mesh.h"
#include "CollisionObject.h"

namespace MeshSystem
{
	class MeshNodeInstance;
	class MeshNode;
}



class ModelComponent: public Component
{
	std::vector<std::unique_ptr<MeshInstance>> MeshInstances;
	std::unique_ptr<CollisionObject> CollisionObj;
	std::vector<ModelComponent*> TreeInheritedChildren;

	mutable glm::mat4 LastFrameMVP;	//for velocity buffer; this field is only updated when velocity buffer is needed (for temporal AA/motion blur), in any other case it will be set to an identity matrix

public:
	ModelComponent(GameManager*, const Transform& = Transform(), std::string name = "undefinedModel", Material* overrideMat = nullptr);
	ModelComponent(GameManager*, const MeshSystem::MeshNode&, const Transform& = Transform(), std::string name = "undefinedModel", Material* overrideMat = nullptr);
	ModelComponent(const ModelComponent&);
	ModelComponent(ModelComponent&&);

	void OverrideInstancesMaterial(Material*);
	CollisionObject* SetCollisionObject(std::unique_ptr<CollisionObject>&);
	void SetLastFrameMVP(const glm::mat4& lastMVP) const;
	void GenerateFromNode(const MeshSystem::MeshNode&, Material* overrideMaterial = nullptr);

	int GetMeshInstanceCount() const;
	const MeshInstance& GetMeshInstance(int index) const;
	const glm::mat4& GetLastFrameMVP() const;

	MeshInstance* FindMeshInstance(std::string);
	void AddMeshInst(Mesh*);

	ModelComponent& operator=(const ModelComponent&);
	ModelComponent& operator=(ModelComponent&&);

	~ModelComponent();
};