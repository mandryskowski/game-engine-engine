#pragma once
#include "Actor.h"
#include "ModelComponent.h"

class GunActor: public Actor	//example class implented using engine's components
{
	AtlasMaterial* FireMaterial;
	MeshInstance* ParticleMeshInst;
	ModelComponent* GunModel;
	SoundSourceComponent* GunBlast;

	float FireCooldown;	//in seconds
	float CooldownLeft;	//also in seconds

public:
	GunActor(std::string name);
	virtual void LoadDataFromLevel(SearchEngine* searcher) override;
	virtual void Update(float) override;
	virtual void HandleInputs(GLFWwindow*, float) override;
};