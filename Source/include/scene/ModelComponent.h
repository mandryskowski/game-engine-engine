#pragma once
#include <scene/RenderableComponent.h>
#include <rendering/Mesh.h>
#include "physics/CollisionObject.h"
#include <UI/UIElement.h>

namespace MeshSystem
{
	class MeshNodeInstance;
	class MeshNode;
}

//Todo: Usun SkeletonInfo z ModelComponent
//Dodaj to do osobnej klasy SkeletalModelComponent ktora dziedziczy z ModelComponent


class ModelComponent: public RenderableComponent, public UICanvasElement
{
public:
	ModelComponent(GameScene&, const std::string& name = "undefinedModel", const Transform& = Transform(), SkeletonInfo* info = nullptr, Material* overrideMat = nullptr);
	ModelComponent(GameScene&, const MeshSystem::MeshNode&, const std::string& name = "undefinedModel", const Transform& = Transform(), SkeletonInfo* info = nullptr, Material* overrideMat = nullptr);

	ModelComponent(const ModelComponent&) = delete;
	ModelComponent(ModelComponent&&);

protected:
	friend class HierarchyTemplate::ComponentTemplate<ModelComponent>;
	ModelComponent& operator=(const ModelComponent&);
public:
	ModelComponent& operator=(ModelComponent&&) = delete;	//TODO: de-delete this, it should be written but i am too lazy

	void OverrideInstancesMaterial(Material* material);	//Overrides MeshInstances' materials with the one passed as the argument
	void OverrideInstancesMaterialInstances(std::shared_ptr<MaterialInstance> matInst);	//Overrides MeshInstances' materials with the one passed as the argument
	void SetLastFrameMVP(const glm::mat4& lastMVP) const;
	void SetSkeletonInfo(SkeletonInfo*);
	void SetRenderAsBillboard(bool billboard);
	void SetHide(bool hide);
	virtual void GenerateFromNode(const MeshSystem::TemplateNode&, Material* overrideMaterial = nullptr) override;

	void DRAWBATCH() const;

	int GetMeshInstanceCount() const;
	const MeshInstance& GetMeshInstance(int index) const;
	const glm::mat4& GetLastFrameMVP() const;
	SkeletonInfo* GetSkeletonInfo() const;
	bool GetHide() const;

	MeshInstance* FindMeshInstance(std::string);
	void AddMeshInst(const MeshInstance&);

	virtual void Update(float deltaTime) override;

	virtual void Render(const RenderInfo&, Shader* shader) override;

	virtual void GetEditorDescription(UIActor& canvas, GameScene& editorScene);


protected:
	std::vector<std::unique_ptr<MeshInstance>> MeshInstances;
	SkeletonInfo* SkelInfo;
	bool RenderAsBillboard;
	bool Hide;

	mutable glm::mat4 LastFrameMVP;	//for velocity buffer; this field is only updated when velocity buffer is needed (for temporal AA/motion blur), in any other case it will be set to an identity matrix

};