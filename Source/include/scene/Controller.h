#pragma once
#include <scene/Actor.h>
#include <math/Transform.h>
#include <array>

template<class T> class Interpolator;

namespace physx
{
	class PxController;
}

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

	struct MovementAxis
	{
		std::unique_ptr<Interpolator<float>> MovementInterpolator;
		bool Inversed;
		glm::vec3 Direction;
	};

	physx::PxController* PxController;
	Actor* PossessedActor;
	bool Directions[DIRECTION_COUNT];
	Vec3f Velocity;
	Vec3f PreviousFramePos;
	Vec3f RotationEuler;

	std::array<MovementAxis, 4> MovementAxises;

public:
	Controller(GameScene& scene, Actor* parentActor, std::string name);

	void SetPossessedActor(Actor*);

	virtual void HandleEvent(const Event& ev) override;
	virtual void ReadMovementKeys();
	virtual void Update(float deltaTime) override;
	void RotateWithMouse(glm::vec2);	//rotates camera - you should pass the mouse offset from the center
	void HandleMovementAxis(bool pressed, MovementAxis& axis);

	template <typename Archive> void Save(Archive& archive) const
	{
		archive(cereal::make_nvp("PossessedActorName", (PossessedActor) ? (PossessedActor->GetName()) : (std::string())), cereal::make_nvp("Actor", cereal::base_class<Actor>(this)));
	}
	template <typename Archive> void Load(Archive& archive)
	{
		std::string possessedActorName;
		archive(cereal::make_nvp("PossessedActorName", possessedActorName), cereal::make_nvp("Actor", cereal::base_class<Actor>(this)));
		Scene.AddPostLoadLambda([this, possessedActorName]() { std::cout << "czy sie udalo?: " << Scene.FindActor(possessedActorName); SetPossessedActor(Scene.FindActor(possessedActorName)); });
	}

	virtual void GetEditorDescription(EditorDescriptionBuilder);
};

class GunActor;

class ShootingController : public Controller
{
	GunActor* PossessedGunActor;
public:
	ShootingController(GameScene& scene, Actor* parentActor, const std::string& name);
	virtual void HandleEvent(const Event& ev) override;
	template <typename Archive> void Save(Archive& archive) const
	{
		archive(cereal::make_nvp("Controller", cereal::base_class<Controller>(this)), cereal::make_nvp("PossesedGunActorName", (PossessedGunActor) ? (PossessedGunActor->GetName()) : (std::string(""))));
	}
	template <typename Archive> void Load(Archive& archive)
	{
		std::string possessedActorName;
		archive(cereal::make_nvp("Controller", cereal::base_class<Controller>(this)), cereal::make_nvp("PossesedGunActorName", possessedActorName));
		if (!possessedActorName.empty())
			PossessedGunActor = dynamic_cast<GunActor*>(Scene.GetRootActor()->FindActor(possessedActorName));
	}
	virtual void GetEditorDescription(EditorDescriptionBuilder);
};