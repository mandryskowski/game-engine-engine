#pragma once
#include <scene/Component.h>
#include <game/GameSettings.h>
#include <array>

class CameraComponent: public Component
{
	Vec3f RotationEuler;

	
	Mat4f Projection;	

public:
	CameraComponent(Actor&, Component* parentComp, std::string name, const glm::mat4& projectionMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f));
	CameraComponent(CameraComponent&&);
	glm::mat4 GetProjectionMat();		//returns the camera projection matrix - i've decided that each camera should have their own projection mat, because just as in the real world each cam has its own FoV and aspect ratio
	RenderInfo GetRenderInfo(RenderToolboxCollection& renderCollection);

	void RotateWithMouse(glm::vec2);	//rotates camera - you should pass the mouse offset from the center

	virtual void Update(float);		//controls the component

	virtual MaterialInstance GetDebugMatInst(EditorIconState) override;
	template <typename Archive> void Serialize(Archive& archive)
	{
		archive(CEREAL_NVP(RotationEuler), CEREAL_NVP(Projection), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
	}
	~CameraComponent();
};

namespace cereal
{
	template <class Archive>
	struct specialize<Archive, CameraComponent, cereal::specialization::member_serialize> {};
}