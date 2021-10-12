#pragma once
#include <scene/Component.h>
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
		Mat4f GetViewMat();
		Mat4f GetProjectionMat();		//returns the camera Projection matrix - i've decided that each camera should have their own Projection mat, because just as in the real world each cam has its own FoV and aspect ratio
		SceneMatrixInfo GetRenderInfo(RenderToolboxCollection& renderCollection);

		virtual void Update(float);		//controls the component

		virtual MaterialInstance LoadDebugMatInst(EditorIconState) override;

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

GEE_POLYMORPHIC_SERIALIZABLE_COMPONENT(GEE::Component, GEE::CameraComponent)