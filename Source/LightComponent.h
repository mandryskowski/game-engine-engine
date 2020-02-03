#pragma once
#include "Component.h"
enum LightType
{
	DIRECTIONAL,
	POINT,
	SPOT
};


class LightComponent: public Component
{
	LightType Type;

	glm::vec3 Ambient;
	glm::vec3 Diffuse;
	glm::vec3 Specular;

	float Attenuation; 
	float CutOff;			//"additional data"
	float OuterCutOff;

	unsigned int ShadowMapNr;
	float Far;
	glm::mat4 ProjectionMat;
public:
	LightComponent(std::string name = "error", LightType = LightType::POINT, unsigned int = 0, float = 10.0f, glm::mat4 = glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 10.0f), glm::vec3 = glm::vec3(0.1f), glm::vec3 = glm::vec3(1.0f), glm::vec3 = glm::vec3(0.5f), glm::vec3 = glm::vec3(0.0f));
	LightType GetType();
	float GetFar();
	unsigned int GetShadowMapNr();
	void SetAdditionalData(glm::vec3);
	glm::mat4 GetProjection();
	void UpdateUBOData(UniformBuffer*, size_t = -1);
	glm::vec3& operator[](unsigned int);
};

LightType toLightType(std::string);