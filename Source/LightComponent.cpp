#include "LightComponent.h"

LightComponent::LightComponent(std::string name, LightType type, unsigned int shadowNr, float far, glm::mat4 projection, glm::vec3 amb, glm::vec3 diff, glm::vec3 spec, glm::vec3 settings):
	Type(type),
	Ambient(amb),
	Diffuse(diff),
	Specular(spec),
	Attenuation(settings.x),
	ShadowMapNr(shadowNr),
	Far(far),
	Projection(projection),
	DirtyFlag(true),
	Component(name, Transform())
{

	CutOff = glm::cos(glm::radians(settings.y));
	OuterCutOffBeforeCos = glm::radians(settings.z);
	OuterCutOff = glm::cos(OuterCutOffBeforeCos);
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

glm::mat4 LightComponent::GetProjection()
{
	return Projection;
}

glm::mat4 LightComponent::GetVP(Transform* worldTransform)
{
	if (worldTransform)
		return Projection * worldTransform->GetViewMatrix();
	else
		return Projection * ComponentTransform.GetWorldTransform().GetViewMatrix();
}

EngineObjectTypes LightComponent::GetLightVolumeType()
{
	switch (Type)
	{
	case SPOT:
		if (Ambient == glm::vec3(0.0f))
			return EngineObjectTypes::CONE;
	case POINT:
		return EngineObjectTypes::SPHERE;
	case DIRECTIONAL:
		return EngineObjectTypes::QUAD;
	default:
		std::cerr << "ERROR! Unknown light type: " << (int)Type << ".\n";
		return EngineObjectTypes::SPHERE;
	}
}

void LightComponent::CalculateLightRadius()
{
	if (Type == LightType::DIRECTIONAL)
		return;

	DirtyFlag = true;

	///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////

	float constant = 1.0f;
	float linear = 0.0f;
	float quadratic = Attenuation;
	float lightMax = std::fmax(std::fmax(Diffuse.r, Diffuse.g), Diffuse.b);

	float radius = 10.0f;
	if (Attenuation > 0.0f)
		radius = (-linear + std::sqrtf(linear * linear - 4.0f * quadratic * (constant - lightMax * (256.0f / 5.0f)))) / (2.0f * quadratic);

	Far = radius;
	Projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, Far);
	ComponentTransform.SetScale(glm::vec3(radius));

	///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////

	if (Type == LightType::SPOT && Ambient == glm::vec3(0.0f))	//If we are a spotlight and we don't emit any ambient light, we can set the light volume to a cone instead of a sphere - more sweet FPS
	{
		float scale = radius * tan(OuterCutOffBeforeCos);	//radius == height in our code
		ComponentTransform.SetScale(glm::vec3(scale, scale, radius));
	}
}

void LightComponent::SetAdditionalData(glm::vec3 data)
{
	DirtyFlag = true;
	Attenuation = data.x;
	CutOff = glm::cos(glm::radians(data.y));
	OuterCutOffBeforeCos = glm::radians(data.z);
	OuterCutOff = glm::cos(OuterCutOffBeforeCos);
}

void LightComponent::UpdateUBOData(UniformBuffer* lightsUBO, const glm::mat4& view, size_t offset)
{
	if (offset != -1)
		lightsUBO->offsetCache = offset;
	
	float pierszy = lightsUBO->offsetCache;
	glm::mat4 viewInverse = glm::inverse(view);

	Transform worldTransform = ComponentTransform.GetWorldTransform();

	switch (Type)
	{
	case LightType::DIRECTIONAL:
		lightsUBO->offsetCache += sizeof(glm::vec4);
		lightsUBO->SubData4fv(glm::vec3(view * glm::vec4(worldTransform.FrontRef, 0.0f)), lightsUBO->offsetCache); break;
	case LightType::POINT:
		lightsUBO->SubData4fv(glm::vec3(view * glm::vec4(worldTransform.PositionRef, 1.0f)), lightsUBO->offsetCache);
		lightsUBO->offsetCache += sizeof(glm::vec4); break;
	case LightType::SPOT:
		lightsUBO->SubData4fv(glm::vec3(view * glm::vec4(worldTransform.PositionRef, 1.0f)), lightsUBO->offsetCache);
		lightsUBO->SubData4fv(glm::vec3(view * glm::vec4(worldTransform.FrontRef, 0.0f)), lightsUBO->offsetCache); break;
	}
	
	if (DirtyFlag)
	{
		float first = lightsUBO->offsetCache;
		lightsUBO->SubData4fv(std::vector <glm::vec3> {Ambient, Diffuse, Specular}, lightsUBO->offsetCache);
		float additionalData[3] = { Attenuation, CutOff, OuterCutOff };
		lightsUBO->SubData(12, additionalData, lightsUBO->offsetCache);
		lightsUBO->SubData1f(Type, lightsUBO->offsetCache);
		lightsUBO->SubData1f(ShadowMapNr, lightsUBO->offsetCache);
		lightsUBO->SubData1f(Far, lightsUBO->offsetCache);

		DirtyFlag = false;
	}
	else
		lightsUBO->offsetCache += 72;

	if (Type != LightType::POINT)
		lightsUBO->SubDataMatrix4fv(Projection * worldTransform.GetViewMatrix() * viewInverse, lightsUBO->offsetCache + 8);
	else
		lightsUBO->SubDataMatrix4fv(viewInverse, lightsUBO->offsetCache + 8);
	lightsUBO->PadOffset();
}

glm::vec3& LightComponent::operator[](unsigned int i)
{
	DirtyFlag = true;
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