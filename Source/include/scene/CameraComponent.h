#pragma once
#include <scene/Component.h>
#include <game/GameSettings.h>
#include <array>

namespace GEE
{
	class CameraComponent : public Component
	{
		Vec3f RotationEuler;
		Mat4f Projection;

	public:
		CameraComponent(Actor&, Component* parentComp, std::string name, const Mat4f& projectionMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f));
		CameraComponent(CameraComponent&&);
		Mat4f GetProjectionMat();		//returns the camera projection matrix - i've decided that each camera should have their own projection mat, because just as in the real world each cam has its own FoV and aspect ratio
		RenderInfo GetRenderInfo(RenderToolboxCollection& renderCollection);

		void RotateWithMouse(Vec2f);	//rotates camera - you should pass the mouse offset from the center

		virtual void Update(float);		//controls the component

		virtual MaterialInstance GetDebugMatInst(EditorIconState) override;

		virtual void GetEditorDescription(EditorDescriptionBuilder);
		template <typename Archive> void Save(Archive& archive) const
		{
			archive(CEREAL_NVP(RotationEuler), CEREAL_NVP(Projection), cereal::make_nvp("Active", Scene.GetActiveCamera() == this), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			bool active;
			archive(CEREAL_NVP(RotationEuler), CEREAL_NVP(Projection), cereal::make_nvp("Active", active), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
			if (active)
			{
				Scene.BindActiveCamera(this);
				GameHandle->BindAudioListenerTransformPtr(&GetTransform());
			}
		}
		~CameraComponent();
	};
}

CEREAL_REGISTER_TYPE(GEE::CameraComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GEE::Component, GEE::CameraComponent)