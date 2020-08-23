#include "ModelComponent.h"
#include "MeshSystem.h"

using namespace MeshSystem;

ModelComponent::ModelComponent(GameManager* gameHandle, const Transform& transform, std::string name, Material* overrideMat) :
	Component(gameHandle, name, transform),
	LastFrameMVP(glm::mat4(1.0f))
{
}

ModelComponent::ModelComponent(GameManager* gameHandle, const MeshSystem::MeshNode& node, const Transform& transform, std::string name, Material* overrideMat) :
	ModelComponent(gameHandle, transform, name, overrideMat)
{
	GenerateFromNode(node, overrideMat);
}

ModelComponent::ModelComponent(const ModelComponent& model) :
	ModelComponent(model.GameHandle, model.ComponentTransform, model.Name)
{
	*this = model;
}

ModelComponent::ModelComponent(ModelComponent&& model) :
	ModelComponent(model.GameHandle, model.ComponentTransform, model.Name)
{
	*this = model;
}

void ModelComponent::OverrideInstancesMaterial(Material* overrideMat)
{
	for (int i = 0; i < static_cast<int>(MeshInstances.size()); i++)
		MeshInstances[i]->SetMaterial(overrideMat);
}

CollisionObject* ModelComponent::SetCollisionObject(std::unique_ptr<CollisionObject>& obj)
{
	if (!obj)
		return nullptr;
	obj.swap(CollisionObj);
	CollisionObj->TransformPtr = &ComponentTransform;
	GameHandle->GetPhysicsHandle()->AddCollisionObject(CollisionObj.get());

	return CollisionObj.get();
}

void ModelComponent::SetLastFrameMVP(const glm::mat4& lastMVP) const
{
	LastFrameMVP = lastMVP;
}

void ModelComponent::GenerateFromNode(const MeshSystem::MeshNode& node, Material* overrideMaterial)
{
	for (int i = 0; i < node.GetMeshCount(); i++)
		AddMeshInst(node.GetMesh(i));

	ComponentTransform *= node.GetTemplateTransform();

	SetCollisionObject(node.InstantiateCollisionObj());

	if (overrideMaterial)
		OverrideInstancesMaterial(overrideMaterial);
	else if (node.GetOverrideMaterial())
		OverrideInstancesMaterial(node.GetOverrideMaterial());
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
