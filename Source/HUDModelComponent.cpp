#include "HUDModelComponent.h"

HUDModelComponent::HUDModelComponent(GameScene* scene, const std::string& name, const Transform& transform, SkeletonInfo* info, Material* overrideMat) :
	ModelComponent(scene, name, transform, info, overrideMat)
{
}

HUDModelComponent::HUDModelComponent(GameScene* scene, const MeshSystem::MeshNode& meshNode, const std::string& name, const Transform& transform, SkeletonInfo* info, Material* overrideMat) :
	ModelComponent(scene, meshNode, name, transform, info, overrideMat)
{
}

/*void HUDModelComponent::Render(RenderInfo& info, Shader* shader)
{
	RenderInfo info2D = info;
	glm::mat4 view(1.0f);
	glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);

	info2D.view = view;
	info2D.projection = projection;
	info2D.VP = projection;

	GameHandle->GetRenderEngineHandle()->RenderStaticMeshes(info2D, MeshInstances, GetTransform().GetWorldTransform(), shader, &LastFrameMVP, nullptr, RenderAsBillboard);
}*/

std::shared_ptr<HUDModelComponent> HUDModelComponent::Of(const HUDModelComponent& constructorVal)
{
	std::shared_ptr<HUDModelComponent> createdObj = std::make_shared<HUDModelComponent>(HUDModelComponent(constructorVal));
	constructorVal.Scene->GetRenderData()->AddRenderable(createdObj);
	return createdObj;
}

std::shared_ptr<HUDModelComponent> HUDModelComponent::Of(HUDModelComponent&& constructorVal)
{
	std::shared_ptr<HUDModelComponent> createdObj = std::make_shared<HUDModelComponent>(HUDModelComponent(constructorVal));
	constructorVal.Scene->GetRenderData()->AddRenderable(createdObj);
	return createdObj;
}