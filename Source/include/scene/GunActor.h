#pragma once
#include <scene/Actor.h>
#include <scene/ModelComponent.h>
#include <scene/SoundSourceComponent.h>

namespace GEE
{
	class GunActor : public Actor	//example class implented using engine's components
	{
	public:
		GunActor(GameScene& scene, Actor* parentActor, std::string name);
		virtual void Setup() override;
		virtual void Update(float) override;
		virtual void HandleEvent(const Event& ev) override;
		void SetFireModel(ModelComponent*);
		void FireWeapon();	//try to fire held weapon (if exists & it's not on cooldown)
		virtual void GetEditorDescription(EditorDescriptionBuilder) override;
		template <typename Archive>
		void Save(Archive& archive) const
		{
			std::string fireModelName = (FireModel) ? (FireModel->GetName()) : (std::string()), gunBlastName = (GunBlast) ? (GunBlast->GetName()) : (std::string());
			archive(cereal::make_nvp("FireModelName", fireModelName), cereal::make_nvp("GunBlastName", gunBlastName), cereal::base_class<Actor>(this));
		}
		template <typename Archive>
		void Load(Archive& archive)
		{
			std::string fireModelName, gunBlastName;
			archive(cereal::make_nvp("FireModelName", fireModelName), cereal::make_nvp("GunBlastName", gunBlastName), cereal::base_class<Actor>(this));
			
			Scene.AddPostLoadLambda([this, fireModelName, gunBlastName]() {
				SetFireModel(GetRoot()->GetComponent<ModelComponent>(fireModelName));
				GunBlast = GetRoot()->GetComponent<Audio::SoundSourceComponent>(gunBlastName);
			});
		}

	private:
		MeshInstance* ParticleMeshInst;
		ModelComponent* FireModel;
		Audio::SoundSourceComponent* GunBlast;

		int FiredBullets;

		float FireCooldown;	//in seconds
		float CooldownLeft;	//also in seconds

		float BulletRadius, ImpulseFactor;
	};


}
GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(GEE::Actor, GEE::GunActor)