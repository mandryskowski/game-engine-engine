#pragma once
#include "RenderableComponent.h"
#include "Mesh.h"
#include "CollisionObject.h"

namespace MeshSystem
{
	class MeshNodeInstance;
	class MeshNode;
}

//Todo: Usun SkeletonInfo z ModelComponent
//Dodaj to do osobnej klasy SkeletalModelComponent ktora dziedziczy z ModelComponent

class Model
{
public:

	int GetMeshInstanceCount() const;
	const MeshInstance& GetMeshInstance(int index) const;

	MeshInstance* FindMeshInstance(std::string);
	void AddMeshInst(Mesh*);
protected:
	std::vector<std::unique_ptr<MeshInstance>> MeshInstances;
};


class ModelComponent: public RenderableComponent
{
public:
	ModelComponent(GameScene*, const std::string& name = "undefinedModel", const Transform& = Transform(), SkeletonInfo* info = nullptr, Material* overrideMat = nullptr);
	ModelComponent(GameScene*, const MeshSystem::MeshNode&, const std::string& name = "undefinedModel", const Transform& = Transform(), SkeletonInfo* info = nullptr, Material* overrideMat = nullptr);
	ModelComponent(const ModelComponent&);
	ModelComponent(ModelComponent&&);

	void OverrideInstancesMaterial(Material* material);	//Overrides MeshInstances' materials with the one passed as the argument
	void SetLastFrameMVP(const glm::mat4& lastMVP) const;
	void SetSkeletonInfo(SkeletonInfo*);
	void SetRenderAsBillboard(bool billboard);
	virtual void GenerateFromNode(const MeshSystem::TemplateNode*, Material* overrideMaterial = nullptr) override;

	void DRAWBATCH() const;

	int GetMeshInstanceCount() const;
	const MeshInstance& GetMeshInstance(int index) const;
	const glm::mat4& GetLastFrameMVP() const;
	SkeletonInfo* GetSkeletonInfo() const;

	MeshInstance* FindMeshInstance(std::string);
	void AddMeshInst(const MeshInstance&);

	static std::shared_ptr<ModelComponent> Of(const ModelComponent&);
	static std::shared_ptr<ModelComponent> Of(ModelComponent&&);

	virtual void Render(const RenderInfo&, Shader* shader) override;

	ModelComponent& operator=(const ModelComponent&);
	ModelComponent& operator=(ModelComponent&&);

protected:
	std::vector<std::unique_ptr<MeshInstance>> MeshInstances;
	SkeletonInfo* SkelInfo;
	bool RenderAsBillboard;

	mutable glm::mat4 LastFrameMVP;	//for velocity buffer; this field is only updated when velocity buffer is needed (for temporal AA/motion blur), in any other case it will be set to an identity matrix

};