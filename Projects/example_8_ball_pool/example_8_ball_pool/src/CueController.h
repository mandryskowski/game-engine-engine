#pragma once
#include <scene/Controller.h>

namespace GEE
{
	class CueController : public Controller
	{
	public:
		CueController(GameScene& scene, Actor* parentActor, const std::string& name);

		virtual void OnStart() override;

		virtual void HandleEvent(const Event& ev) override;

		virtual void GetEditorDescription(EditorDescriptionBuilder) override;
		virtual void OnMouseMovement(const Vec2f& previousPosPx, const Vec2f& currentPosPx, const Vec2u& windowSize) override;
		virtual void Update(Time dt) override;

		template <typename Archive> void Save(Archive& archive) const
		{
			std::string whiteBallActorName = (WhiteBallActor) ? (WhiteBallActor->GetName()) : ("");
			archive(cereal::make_nvp("Controller", cereal::base_class<Controller>(this)), cereal::make_nvp("WhiteBallActorName", whiteBallActorName), CEREAL_NVP(CueHitPower));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			std::string whiteBallActorName;
			archive(cereal::make_nvp("Controller", cereal::base_class<Controller>(this)), cereal::make_nvp("WhiteBallActorName", whiteBallActorName), CEREAL_NVP(CueHitPower));
			Scene.AddPostLoadLambda([this, whiteBallActorName]() { if (!whiteBallActorName.empty()) WhiteBallActor = Scene.FindActor(whiteBallActorName); });
		}
	private:
		void ResetBalls();
		void ResetCue(float rotationRad);
		void ResetCue(Quatf rotation = Quatf());

		Actor* WhiteBallActor;
		float CueHitPower, BallStaticFriction, BallDynamicFriction, BallRestitution, BallLinearDamping, BallAngularDamping;
		bool ConstrainedBallMovement;

		float MinCueDistance, MaxCueDistance;
		bool PowerInverted;

		Interpolation CueDistanceAnim;
	};

}

GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(GEE::Controller, GEE::CueController)