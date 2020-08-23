#include "LightsManager.h"

void LightsManager::SetupLights()
{
	LightsBuffer.Generate(1, sizeof(glm::vec4) * 2 + Lights.size() * 192);
	LightsBuffer.SubData1i((int)Lights.size(), (size_t)0);

	UpdateLightUniforms();
}

void LightsManager::UpdateLightUniforms()
{
	LightsBuffer.offsetCache = sizeof(glm::vec4) * 2;
	for (int i = 0; i < static_cast<int>(Lights.size()); i++)
		Lights[i]->UpdateUBOData(&LightsBuffer);
}

void LightsManager::AddLight(std::shared_ptr<LightComponent> light)
{
	Lights.push_back(light);
}

int LightsManager::GetAvailableLightIndex()
{
	return Lights.size();
}
