#pragma once
#include "RenderableComponent.h"
#include "Mesh.h"
#include "CollisionObject.h"

namespace MeshSystem
{
	class MeshNodeInstance;
	class MeshNode;
}



class ModelComponent: public RenderableComponent
{
	std::vector<std::unique_ptr<MeshInstance>> MeshInstances;
	std::vector<ModelComponent*> TreeInheritedChildren;
	SkeletonInfo* SkelInfo;

	mutable glm::mat4 LastFrameMVP;	//for velocity buffer; this field is only updated when velocity buffer is needed (for temporal AA/motion blur), in any other case it will be set to an identity matrix

public:
	ModelComponent(GameManager*, std::string name = "undefinedModel", const Transform& = Transform(), SkeletonInfo* info = nullptr, Material* overrideMat = nullptr);
	ModelComponent(GameManager*, const MeshSystem::MeshNode&, std::string name = "undefinedModel", const Transform& = Transform(), SkeletonInfo* info = nullptr, Material* overrideMat = nullptr);
	ModelComponent(const ModelComponent&);
	ModelComponent(ModelComponent&&);

	void OverrideInstancesMaterial(Material*);
	void SetLastFrameMVP(const glm::mat4& lastMVP) const;
	void SetSkeletonInfo(SkeletonInfo*);
	virtual void GenerateFromNode(const MeshSystem::TemplateNode*, Material* overrideMaterial = nullptr) override;

	void DRAWBATCH() const;

	int GetMeshInstanceCount() const;
	const MeshInstance& GetMeshInstance(int index) const;
	const glm::mat4& GetLastFrameMVP() const;
	SkeletonInfo* GetSkeletonInfo() const;

	MeshInstance* FindMeshInstance(std::string);
	void AddMeshInst(Mesh*);

	static std::shared_ptr<ModelComponent> Of(const ModelComponent&);
	static std::shared_ptr<ModelComponent> Of(ModelComponent&&);

	virtual void Render(RenderInfo&, Shader* shader) override;

	ModelComponent& operator=(const ModelComponent&);
	ModelComponent& operator=(ModelComponent&&);

	~ModelComponent();
};