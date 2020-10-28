#include "ModelComponent.h"
#include "MeshSystem.h"

using namespace MeshSystem;

ModelComponent::ModelComponent(GameManager* gameHandle, std::string name, const Transform& transform, SkeletonInfo* info, Material* overrideMat) :
	RenderableComponent(gameHandle, name, transform),
	LastFrameMVP(glm::mat4(1.0f)),
	SkelInfo(info)
{
}

ModelComponent::ModelComponent(GameManager* gameHandle, const MeshSystem::MeshNode& node, std::string name, const Transform& transform, SkeletonInfo* info, Material* overrideMat) :
	ModelComponent(gameHandle, name, transform, info, overrideMat)
{
	GenerateFromNode(&node, overrideMat);
}

ModelComponent::ModelComponent(const ModelComponent& model) :
	ModelComponent(model.GameHandle, model.Name, model.ComponentTransform)
{
	*this = model;
}

ModelComponent::ModelComponent(ModelComponent&& model) :
	ModelComponent(model.GameHandle, model.Name, model.ComponentTransform)
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

void ModelComponent::GenerateFromNode(const MeshSystem::TemplateNode* node, Material* overrideMaterial)
{
	Component::GenerateFromNode(node, overrideMaterial);
	const MeshSystem::MeshNode* meshNodeCast = dynamic_cast<const MeshSystem::MeshNode*>(node);
	for (int i = 0; i < meshNodeCast->GetMeshCount(); i++)
		AddMeshInst(meshNodeCast->GetMesh(i));

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

void ModelComponent::AddMeshInst(Mesh* mesh)
{
	MeshInstances.push_back(std::make_unique<MeshInstance>(MeshInstance(mesh)));
}

std::shared_ptr<ModelComponent> ModelComponent::Of(const ModelComponent& constructorVal)
{
	std::shared_ptr<ModelComponent> createdObj = std::make_shared<ModelComponent>(ModelComponent(constructorVal));
	constructorVal.GameHandle->GetRenderEngineHandle()->AddRenderable(createdObj);
	return createdObj;
}

std::shared_ptr<ModelComponent> ModelComponent::Of(ModelComponent&& constructorVal)
{
	std::shared_ptr<ModelComponent> createdObj = std::make_shared<ModelComponent>(ModelComponent(constructorVal));
	constructorVal.GameHandle->GetRenderEngineHandle()->AddRenderable(createdObj);
	return createdObj;
}

void ModelComponent::Render(RenderInfo& info, Shader* shader)
{
	GameHandle->GetRenderEngineHandle()->Render(info, *this, shader);
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

ModelComponent::~ModelComponent()
{

}
