#pragma once
#include "LightComponent.h"



class LightsManager
{
	std::vector<std::shared_ptr<LightComponent>> Lights;
	UniformBuffer LightsBuffer;

	friend class RenderEngine;

public:

	void SetupLights();
	void UpdateLightUniforms();

	void AddLight(std::shared_ptr<LightComponent> light); // this function lets the engine know that the passed light component's data needs to be forwarded to shaders (therefore lighting our meshes)
	int GetAvailableLightIndex();
};