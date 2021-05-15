#pragma once
#include <scene/Actor.h>
#include <scene/ModelComponent.h>

class GunActor: public Actor	//example class implented using engine's components
{
	AtlasMaterial* FireMaterial;
	MeshInstance* ParticleMeshInst;
	ModelComponent* GunModel;
	SoundSourceComponent* GunBlast;

	int FiredBullets;

	float FireCooldown;	//in seconds
	float CooldownLeft;	//also in seconds

public:
	GunActor(GameScene& scene, Actor* parentActor, std::string name);
	virtual void Setup() override;
	virtual void Update(float) override;
	virtual void HandleEvent(const Event& ev) override;
	void FireWeapon();	//try to fire held weapon (if exists & it's not on cooldown)
	virtual void GetEditorDescription(EditorDescriptionBuilder) override;
};