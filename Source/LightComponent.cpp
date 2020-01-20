#include "LightComponent.h"

LightComponent::LightComponent(std::string name, LightType type, glm::vec3 amb, glm::vec3 diff, glm::vec3 spec, glm::vec3 settings, glm::mat4 projection):
	Component(name, Transform())
{
	Type = type;

	Ambient = amb;
	Diffuse = diff;
	Specular = spec;

	Attenuation = settings.x;
	CutOff = glm::cos(glm::radians(settings.y));
	OuterCutOff = glm::cos(glm::radians(settings.z));

	ProjectionMat = projection;
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
	lightsUBO->SubData4fv(glm::vec4(Attenuation, CutOff, OuterCutOff, Type), lightsUBO->offsetCache);
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