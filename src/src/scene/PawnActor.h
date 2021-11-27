#pragma once
#include <scene/GunActor.h>
#include <animation/AnimationManagerComponent.h>
#include <functional>

namespace GEE
{
	enum PawnState
	{
		Idle,
		Walking,
		Dying,
		Respawning
	};
	class PawnActor : public Actor
	{
	public:
		PawnActor(GameScene&, Actor* parentActor, const std::string& name);
		virtual void OnStart() override;
		virtual void Update(float deltaTime);
		virtual void GetEditorDescription(EditorDescriptionBuilder) override;

		void InflictDamage(float damage, const Vec3f& optionalDirection = Vec3f(0.0f));
		void MoveToPosition(const Vec3f& worldPos);
		void MoveAlongPath();

		template <typename Archive>
		void Save(Archive& archive) const
		{
			std::string animManagerName = (AnimManager) ? (AnimManager->GetName()) : (std::string()), gunName = (Gun) ? (Gun->GetName()) : (std::string()), playerTargetName = (PlayerTarget) ? (PlayerTarget->GetName()) : (std::string());

			archive(cereal::make_nvp("AnimManagerName", animManagerName), CEREAL_NVP(AnimIndex), cereal::make_nvp("GunName", gunName), CEREAL_NVP(SpeedPerSec), cereal::make_nvp("PlayerTargetName", playerTargetName), cereal::make_nvp("Actor", cereal::base_class<Actor>(this)));
		}
		template <typename Archive>
		void Load(Archive& archive)
		{
			std::string animManagerName, gunName, playerTargetName;
			archive(cereal::make_nvp("AnimManagerName", animManagerName), CEREAL_NVP(AnimIndex), cereal::make_nvp("GunName", gunName), CEREAL_NVP(SpeedPerSec), cereal::make_nvp("PlayerTargetName", playerTargetName), cereal::make_nvp("Actor", cereal::base_class<Actor>(this)));

			Scene.AddPostLoadLambda([this, animManagerName, gunName, playerTargetName]() {
					AnimManager = GetRoot()->GetComponent<AnimationManagerComponent>(animManagerName);
					Gun = dynamic_cast<GunActor*>(GetScene().FindActor(gunName));
					PlayerTarget = GetScene().FindActor(playerTargetName);
				});
		}
	private:
		void Respawn();
		//Component* RootBone;
		AnimationManagerComponent* AnimManager;
		int AnimIndex;
		Vec3f PreAnimBonePos;

		PawnState State;

		float Health;
		float RespawnTime, DeathTime;

		Audio::SoundSourceComponent* PawnSound;
		float SoundCooldown, LastSoundTime;

		Vec3f CurrentTargetPos;
		float SpeedPerSec;

		int PathIndex;
		Vec3f Waypoints[4];

		GunActor* Gun;
		Actor* PlayerTarget;

		std::function<void(AnimationManagerComponent*)> UpdateAnimsListInEditor;
	};
}

GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(GEE::Actor, GEE::PawnActor)