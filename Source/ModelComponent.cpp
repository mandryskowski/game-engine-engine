#include "ModelComponent.h"
#include "MeshSystem.h"
#include "GameScene.h"
#include "RenderInfo.h"

using namespace MeshSystem;

ModelComponent::ModelComponent(GameScene* scene, const std::string& name, const Transform& transform, SkeletonInfo* info, Material* overrideMat) :
	RenderableComponent(scene, name, transform),
	LastFrameMVP(glm::mat4(1.0f)),
	SkelInfo(info),
	RenderAsBillboard(false)
{
}

ModelComponent::ModelComponent(GameScene* scene, const MeshSystem::MeshNode& node, const std::string& name, const Transform& transform, SkeletonInfo* info, Material* overrideMat) :
	ModelComponent(scene, name, transform, info, overrideMat)
{
	GenerateFromNode(&node, overrideMat);
}

ModelComponent::ModelComponent(const ModelComponent& model) :
	ModelComponent(model.Scene, model.Name, model.ComponentTransform)
{
	*this = model;
}

ModelComponent::ModelComponent(ModelComponent&& model) :
	ModelComponent(model.Scene, model.Name, model.ComponentTransform)
{
	*this = model;
}

void ModelComponent::OverrideInstancesMaterial(Material* overrideMat)
{
	for (int i = 0; i < static_cast<int>(MeshInstances.size()); i++)
		MeshInstances[i]->SetMaterial(overrideMat);
}

void ModelComponent::SetLastFrameMVP(const glm::mat4& lastMVP) const
{
	LastFrameMVP = lastMVP;
}

void ModelComponent::SetSkeletonInfo(SkeletonInfo* info)
{
	SkelInfo = info;
}

void ModelComponent::SetRenderAsBillboard(bool billboard)
{
	RenderAsBillboard = billboard;
}

void ModelComponent::GenerateFromNode(const MeshSystem::TemplateNode* node, Material* overrideMaterial)
{
	Component::GenerateFromNode(node, overrideMaterial);
	const MeshSystem::MeshNode* meshNodeCast = dynamic_cast<const MeshSystem::MeshNode*>(node);
	for (int i = 0; i < meshNodeCast->GetMeshCount(); i++)
		AddMeshInst(*meshNodeCast->GetMesh(i));

	if (overrideMaterial)
		OverrideInstancesMaterial(overrideMaterial);
	else if (meshNodeCast->GetOverrideMaterial())
		OverrideInstancesMaterial(meshNodeCast->GetOverrideMaterial());
}

void ModelComponent::DRAWBATCH() const
{
	if (SkelInfo)
	{
		SkelInfo->DRAWBATCH();
	}
}

int ModelComponent::GetMeshInstanceCount() const
{
	return MeshInstances.size();
}

const MeshInstance& ModelComponent::GetMeshInstance(int index) const
{
	return *MeshInstances[index];
}

const glm::mat4& ModelComponent::GetLastFrameMVP() const
{
	return LastFrameMVP;
}

SkeletonInfo* ModelComponent::GetSkeletonInfo() const
{
	return SkelInfo;
}


MeshInstance* ModelComponent::FindMeshInstance(std::string name)
{
	auto found = std::find_if(MeshInstances.begin(), MeshInstances.end(), [name](const std::unique_ptr<MeshInstance>& meshInst) { return meshInst->GetMesh().GetName().find(name) != std::string::npos; });

	if (found != MeshInstances.end())
		return found->get();


	for (unsigned int i = 0; i < Children.size(); i++)
	{
		ModelComponent* modelCast = dynamic_cast<ModelComponent*>(Children[i]);
		MeshInstance* meshInst = modelCast->FindMeshInstance(name);
		if (meshInst)
			return meshInst;
	}


	return nullptr;
}

void ModelComponent::AddMeshInst(const MeshInstance& meshInst)
{
	MeshInstances.push_back(std::make_unique<MeshInstance>(meshInst));
}

std::shared_ptr<ModelComponent> ModelComponent::Of(const ModelComponent& constructorVal)
{
	std::shared_ptr<ModelComponent> createdObj = std::make_shared<ModelComponent>(ModelComponent(constructorVal));
	createdObj->Scene->GetRenderData()->AddRenderable(createdObj);

	return createdObj;
}

std::shared_ptr<ModelComponent> ModelComponent::Of(ModelComponent&& constructorVal)
{
	std::shared_ptr<ModelComponent> createdObj = std::make_shared<ModelComponent>(ModelComponent(constructorVal));
	createdObj->Scene->GetRenderData()->AddRenderable(createdObj);

	return createdObj;
}

void ModelComponent::Render(const RenderInfo& info, Shader* shader)
{
	if (SkelInfo && SkelInfo->GetBoneCount() > 0)
		GameHandle->GetRenderEngineHandle()->RenderSkeletalMeshes(info, MeshInstances, GetTransform().GetWorldTransform(), shader, *SkelInfo);
	else
		GameHandle->GetRenderEngineHandle()->RenderStaticMeshes(info, MeshInstances, GetTransform().GetWorldTransform(), shader, &LastFrameMVP, nullptr, RenderAsBillboard);
}


ModelComponent& ModelComponent::operator=(const ModelComponent& model)
{
	MeshInstances.reserve(model.MeshInstances.size());
	for (int i = 0; i < model.MeshInstances.size(); i++)	//Maybe STLize this?
		MeshInstances.push_back(std::make_unique<MeshInstance>(MeshInstance(*model.MeshInstances[i])));

	return *this;
}

ModelComponent& ModelComponent::operator=(ModelComponent&&)
{
	std::cout << "DZIALA222222222222222!!!!!!!!!!!!!!!!!!!!!!\n";
	return *this;
}