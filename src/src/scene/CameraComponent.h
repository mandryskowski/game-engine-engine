#pragma once
#include <scene/Component.h>
#include <array>

namespace GEE
{
	typedef unsigned int RenderingContextID;

	struct Angle
	{
	public:
		static Angle FromDegrees(double deg) { return glm::radians(deg); }
		static Angle FromRadians(double rad) { return rad; }

		double GetDegrees() { return glm::degrees(AngleRad); }
		double GetRadians() { return AngleRad; }

	private:
		Angle(double angleRad) : AngleRad(angleRad) {}
		double AngleRad;
	};

	struct ProjectionMatData
	{
		Angle FovY;
		float Near, Far, AspectRatio;

		ProjectionMatData(Angle fovy, float near, float far, float aspectRatio) :
			FovY(fovy), Near(near), Far(far), AspectRatio(aspectRatio) {}
	};

	class CameraComponent : public Component
	{
	public:
		CameraComponent(Actor&, Component* parentComp, std::string name, const Mat4f& projectionMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f));
		CameraComponent(CameraComponent&&);
		Mat4f GetViewMat();
		Mat4f GetProjectionMat();		//returns the camera Projection matrix - i've decided that each camera should have their own Projection mat, because just as in the real world each cam has its own FoV and aspect ratio
		SceneMatrixInfo GetRenderInfo(RenderingContextID, RenderToolboxCollection& renderCollection);

		void Update(Time dt) override;		//controls the component

		MaterialInstance GetDebugMatInst(ButtonMaterialType) override;

		void GetEditorDescription(ComponentDescriptionBuilder) override;

		void SetProjectionMat(const ProjectionMatData& projMat);
		void SetProjectionMat(const Mat4f& projMat) { Projection = projMat; }

		template <typename Archive> void Save(Archive& archive) const
		{
			archive(CEREAL_NVP(Projection), cereal::make_nvp("Active", GetScene().GetActiveCamera() == this), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			bool active;
			archive(CEREAL_NVP(Projection), cereal::make_nvp("Active", active), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
			if (active)
			{
				GetScene().BindActiveCamera(this);
				GetGameHandle()->BindAudioListenerTransformPtr(&GetTransform());
			}
		}
		~CameraComponent() override;

	private:
		Mat4f Projection;
	};
}

GEE_POLYMORPHIC_SERIALIZABLE_COMPONENT(GEE::Component, GEE::CameraComponent)