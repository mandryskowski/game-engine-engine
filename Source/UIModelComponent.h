#pragma once
#include "ModelComponent.h"

class UICanvas;

class UIModelComponent : public ModelComponent
{
public:
	UIModelComponent(GameScene*, Transform& canvasTransform, const std::string& name = "undefinedUiModel", const Transform& = Transform(), Material* overrideMat = nullptr);

	virtual void Render(const RenderInfo&, Shader* shader) override;
private:
	UICanvas* CanvasPtr;
};