#pragma once
#include "ModelComponent.h"

class HUDModelComponent : public ModelComponent
{
public:
	HUDModelComponent(GameScene*, const std::string& name = "undefinedModel", const Transform & = Transform(), SkeletonInfo* info = nullptr, Material* overrideMat = nullptr);
	HUDModelComponent(GameScene*, const MeshSystem::MeshNode&, const std::string& name = "undefinedModel", const Transform & = Transform(), SkeletonInfo* info = nullptr, Material* overrideMat = nullptr);

	//virtual void Render(RenderInfo& info, Shader* shader) override;

	static std::shared_ptr<HUDModelComponent> Of(const HUDModelComponent&);
	static std::shared_ptr<HUDModelComponent> Of(HUDModelComponent&&);
};