#include "LightComponent.h"

LightComponent::LightComponent(std::string name, LightType type, unsigned int shadowNr, float far, glm::mat4 projection, glm::vec3 amb, glm::vec3 diff, glm::vec3 spec, glm::vec3 settings):
	Component(name, Transform())
{
	Type = type;

	ShadowMapNr = shadowNr;
	Far = far;
	ProjectionMat = projection;

	Ambient = amb;
	Diffuse = diff;
	Specular = spec;

	Attenuation = settings.x;
	CutOff = glm::cos(glm::radians(settings.y));
	OuterCutOff = glm::cos(glm::radians(settings.z));
}

LightType LightComponent::GetType()
{
	return Type;
}

float LightComponent::GetFar()
{
	return Far;
}

unsigned int LightComponent::GetShadowMapNr()
{
	return ShadowMapNr;
}

void LightComponent::SetAdditionalData(glm::vec3 data)
{
	Attenuation = data.x;
	CutOff = glm::cos(glm::radians(data.y));
	OuterCutOff = glm::cos(glm::radians(data.z));
}

glm::mat4 LightComponent::GetProjection()
{
	return ProjectionMat;
}

void LightComponent::UpdateUBOData(UniformBuffer* lightsUBO, size_t offset)
{
	if (offset != -1)
		lightsUBO->offsetCache = offset;

	Transform worldTransform = ComponentTransform.GetWorldTransform();

	switch (Type)
	{
	case LightType::DIRECTIONAL:
		lightsUBO->offsetCache += sizeof(glm::vec4);
		lightsUBO->SubData4fv(worldTransform.Front, lightsUBO->offsetCache); break;
	case LightType::POINT:
		lightsUBO->SubData4fv(worldTransform.Position, lightsUBO->offsetCache);
		lightsUBO->offsetCache += sizeof(glm::vec4); break;
	case LightType::SPOT:
		lightsUBO->SubData4fv(worldTransform.Position, lightsUBO->offsetCache);
		lightsUBO->SubData4fv(worldTransform.Front, lightsUBO->offsetCache); break;
	}
	
	lightsUBO->SubData4fv(std::vector <glm::vec3> {Ambient, Diffuse, Specular}, lightsUBO->offsetCache);
	float additionalData[3] = { Attenuation, CutOff, OuterCutOff };
	lightsUBO->SubData(12, additionalData, lightsUBO->offsetCache);
	lightsUBO->SubData1i(Type, lightsUBO->offsetCache);
	lightsUBO->SubData1i(ShadowMapNr, lightsUBO->offsetCache);
	lightsUBO->SubData1f(Far, lightsUBO->offsetCache);
	lightsUBO->SubDataMatrix4fv(ProjectionMat * worldTransform.GetViewMatrix(), lightsUBO->offsetCache + 8);
	lightsUBO->PadOffset();
}

glm::vec3& LightComponent::operator[](unsigned int i)
{
	switch (i)
	{
	case 0:
		return Ambient;
	case 1:
		return Diffuse;
	case 2:
		return Specular;
	}

	return Ambient;
}

LightType toLightType(std::string str)
{
	if (str == "direction")
		return LightType::DIRECTIONAL;
	if (str == "spot")
		return LightType::SPOT;
	if (str != "point")
		std::cerr << str << " is not a known light type.\n";
	return LightType::POINT;
}