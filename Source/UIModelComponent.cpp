#include "UIModelComponent.h"
#include "UICanvasActor.h"

UIModelComponent::UIModelComponent(GameScene* scene, Transform& canvasTransform, const std::string& name, const Transform& t, Material* overrideMat):
	ModelComponent(scene, name, t, nullptr, overrideMat)
{
}

void UIModelComponent::Render(const RenderInfo& infoPreConvert, Shader* shader)
{
	RenderInfo info = infoPreConvert;
	info.view = CanvasPtr->GetView();
	info.projection = CanvasPtr->GetProjection();
	GameHandle->GetRenderEngineHandle()->RenderStaticMeshes(info, MeshInstances, GetTransform().GetWorldTransform(), shader, &LastFrameMVP, nullptr, RenderAsBillboard);
}
