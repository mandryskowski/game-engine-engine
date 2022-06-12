#pragma once
#include <scene/Actor.h>
#include <math/Transform.h>
#include <scene/GunActor.h>
#include <array>

namespace physx
{
	class PxController;
}

namespace GEE
{

	template<class T> class Interpolator;

	enum MovementDirections
	{
		DIRECTION_FORWARD,
		DIRECTION_BACKWARD,
		DIRECTION_LEFT,
		DIRECTION_RIGHT,
		DIRECTION_UP,
		DIRECTION_COUNT
	};

	class Controller : public Actor
	{
	public:
		Controller(GameScene& scene, Actor* parentActor, const std::string& name);
		bool GetHideCursor() const;
		bool GetLockMouseAtCenter() const;
		void SetHideCursor(bool);
		void SetLockMouseAtCenter(bool);

		virtual void SetPossessedActor(Actor*);
		virtual void OnMouseMovement(const Vec2f& previousPosPx, const Vec2f& currentPosPx, const Vec2u& windowSize) {}

		template <typename Archive> void Save(Archive& archive) const
		{
			archive(cereal::make_nvp("PossessedActorName", (PossessedActor) ? (PossessedActor->GetName()) : (std::string())), CEREAL_NVP(bHideCursor), CEREAL_NVP(bLockMouseAtCenter), cereal::make_nvp("Actor", cereal::base_class<Actor>(this)));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			std::string possessedActorName;
			archive(cereal::make_nvp("PossessedActorName", possessedActorName), CEREAL_NVP(bHideCursor), CEREAL_NVP(bLockMouseAtCenter), cereal::make_nvp("Actor", cereal::base_class<Actor>(this)));
			Scene.AddPostLoadLambda([this, possessedActorName]() { SetPossessedActor(Scene.FindActor(possessedActorName)); });
		}

		virtual void GetEditorDescription(EditorDescriptionBuilder) override;
	protected:
		Actor* PossessedActor;
		bool bHideCursor, bLockMouseAtCenter;
	};

	class FreeRoamingController : public Controller
	{
	public:
		FreeRoamingController(GameScene& scene, Actor* parentActor, const std::string& name);

		virtual void SetPossessedActor(Actor*) override;

		virtual Vec3i GetMovementDirFromPressedKeys();
		virtual void Update(float deltaTime) override;
		virtual void OnMouseMovement(const Vec2f& previousPosPx, const Vec2f& currentPosPx, const Vec2u& windowSize) override;

		virtual void GetEditorDescription(EditorDescriptionBuilder) override;
	private:
		Vec3f RotationEuler;
		float ControllerSpeed;
	};

	class FPSController : public FreeRoamingController
	{
	public:
		FPSController(GameScene& scene, Actor* parentActor, const std::string& name);

		virtual void SetPossessedActor(Actor*) override;

		virtual Vec3i GetMovementDirFromPressedKeys() override;
		virtual void Update(float deltaTime) override;

	private:
		physx::PxController* PxController;
		Vec3f Velocity;
	};

	class GunActor;

	class ShootingController : public FPSController
	{
	public:
		ShootingController(GameScene& scene, Actor* parentActor, const std::string& name);
		virtual void HandleEvent(const Event& ev) override;
		template <typename Archive> void Save(Archive& archive) const
		{
			std::string gunActorName = (PossessedGunActor) ? (PossessedGunActor->GetName()) : ("");
			archive(cereal::make_nvp("Controller", cereal::base_class<Controller>(this)), cereal::make_nvp("PossesedGunActorName", gunActorName));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			std::string gunActorName;
			archive(cereal::make_nvp("Controller", cereal::base_class<Controller>(this)), cereal::make_nvp("PossesedGunActorName", gunActorName));

			Scene.AddPostLoadLambda([this, gunActorName]() {
				if (!gunActorName.empty())
					PossessedGunActor = dynamic_cast<GunActor*>(Scene.GetRootActor()->FindActor(gunActorName));
				});
		}
		virtual void GetEditorDescription(EditorDescriptionBuilder) override;

	private:
		GunActor* PossessedGunActor;
	};


	class CueController : public Controller
	{
	public:
		CueController(GameScene& scene, Actor* parentActor, const std::string& name);

		virtual void OnStart() override;

		virtual void HandleEvent(const Event& ev) override;

		virtual void GetEditorDescription(EditorDescriptionBuilder) override;
		virtual void OnMouseMovement(const Vec2f& previousPosPx, const Vec2f& currentPosPx, const Vec2u& windowSize) override;
		virtual void Update(float deltaTime) override;

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
		bool PowerInversed;

		Interpolation CueDistanceAnim;
	};
}
GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(GEE::Actor, GEE::Controller)
GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(GEE::Controller, GEE::FreeRoamingController)
GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(GEE::FreeRoamingController, GEE::FPSController)
GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(GEE::FPSController, GEE::ShootingController)
GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(GEE::Controller, GEE::CueController)