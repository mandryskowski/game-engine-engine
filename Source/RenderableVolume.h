#pragma once
#include "GameManager.h"
#include "Transform.h"

class RenderableVolume
{
public:
	virtual EngineBasicShape GetShape() const = 0;
	virtual Transform GetRenderTransform() const = 0;
	virtual Shader* GetRenderShader(const RenderToolboxCollection& renderCol) const = 0;
	virtual void SetupRenderUniforms(const Shader& shader) const {}
};