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

		template <typename Archive> void Save(Archive& archive) const;

		template <typename Archive> void Load(Archive& archive);

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
		virtual void Update(Time dt) override;
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
		virtual void Update(Time dt) override;

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
		template <typename Archive> void Save(Archive& archive) const;

		template <typename Archive> void Load(Archive& archive);
		virtual void GetEditorDescription(EditorDescriptionBuilder) override;

	private:
		GunActor* PossessedGunActor;
	};
}
GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(GEE::Actor, GEE::Controller)
GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(GEE::Controller, GEE::FreeRoamingController)
GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(GEE::FreeRoamingController, GEE::FPSController)
GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(GEE::FPSController, GEE::ShootingController)