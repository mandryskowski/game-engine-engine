#pragma once
#include <scene/GunActor.h>
#include <animation/AnimationManagerActor.h>
#include <functional>

namespace GEE
{
	class PawnActor : public Actor
	{
	public:
		PawnActor(GameScene&, Actor* parentActor, const std::string& name);
		virtual void OnStart() override;
		virtual void Update(float deltaTime);
		virtual void GetEditorDescription(EditorDescriptionBuilder) override;
		void MoveToPosition(const glm::vec3& worldPos);
		void MoveAlongPath();

		template <typename Archive>
		void Save(Archive& archive) const
		{
			std::string animManagerName = (AnimManager) ? (AnimManager->GetName()) : (std::string()), gunName = (Gun) ? (Gun->GetName()) : (std::string()), playerTargetName = (PlayerTarget) ? (PlayerTarget->GetName()) : (nullptr);

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
		//Component* RootBone;
		AnimationManagerComponent* AnimManager;
		int AnimIndex;
		glm::vec3 PreAnimBonePos;

		glm::vec3 CurrentTargetPos;
		float SpeedPerSec;

		int PathIndex;
		glm::vec3 Waypoints[4];

		GunActor* Gun;
		Actor* PlayerTarget;

		std::function<void(AnimationManagerComponent*)> UpdateAnimsListInEditor;
	};
}

CEREAL_REGISTER_TYPE(GEE::PawnActor)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GEE::Actor, GEE::PawnActor)