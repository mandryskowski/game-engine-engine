#pragma once
#include "Actor.h"
#include "ModelComponent.h"

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
	GunActor(GameManager*, std::string name);
	virtual void Setup(SearchEngine* searcher) override;
	virtual void Update(float) override;
	virtual void HandleInputs(GLFWwindow*, float) override;
};