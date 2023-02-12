#pragma once
#include <scene/Actor.h>
#include <scene/ModelComponent.h>
#include <scene/SoundSourceComponent.h>
#include <scene/LightComponent.h>

namespace GEE
{
	class GunActor : public Actor	//example class implented using engine's components
	{
	public:
		GunActor(GameScene& scene, Actor* parentActor, std::string name);
		virtual void Setup() override;
		virtual void Update(Time dt) override;
		virtual void HandleEvent(const Event& ev) override;
		void SetFireModel(ModelComponent*);
		void FireWeapon();	//try to fire held weapon (if exists & it's not on cooldown)
		virtual void GetEditorDescription(EditorDescriptionBuilder) override;
		template <typename Archive> void Save(Archive& archive) const;
		template <typename Archive> void Load(Archive& archive);

	private:
		MeshInstance* ParticleMeshInst;
		ModelComponent* FireModel;
		Audio::SoundSourceComponent* BlastSound;
		LightComponent* GunLight;

		int FiredBullets;

		Time FireCooldown;	//in seconds
		Time CooldownLeft;	//also in seconds

		float BulletRadius, ImpulseFactor;
	};
}
GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(GEE::Actor, GEE::GunActor)