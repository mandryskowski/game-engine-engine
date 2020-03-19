#pragma once
#include "RenderEngine.h"
#include "Postprocess.h"
#include "Actor.h"
#include "CameraComponent.h"


class Game
{
	GLFWwindow *Window;
	Framebuffer GFramebuffer;
	Framebuffer MainFramebuffer;
	Framebuffer BlitFramebuffer;
	Postprocess GamePostprocess;

	Actor RootActor;
	CameraComponent* ActiveCamera;	//there are 2 similiar camera variables: ActiveCamera and global MouseContextCamera. the first one is basically the camera we use to see the world (view mat); the second one is updated by mouse controls.
	std::vector <LightComponent*> Lights;

	UniformBuffer MatricesBuffer, LightsBuffer;

protected:
	GameSettings Settings;
	RenderEngine RenderEng;
	CollisionEngine CollisionEng;

public:
	Game(GLFWwindow*);
	virtual void Init() = 0;
	void LoadLevel(std::string);
	Component* LoadComponentData(std::stringstream&, Actor*);
	void SetupLights();
	void UpdateLightUniforms();
	void BindActiveCamera(CameraComponent*);
	void AddActorToScene(Actor* actor); //you use this function to make the game interact (update, draw...) with the actor; without adding it to the scene, the Actor instance isnt updated real-time by default
	void AddLightToScene(LightComponent* light); // this function lets the game know that the passed light component's data needs to be forwarded to shaders (therefore lighting our meshes)
	void AddMeshToScene(MeshComponent* mesh);
	void Run();
	void Update(float);
	void Render();
};

void cursorPosCallback(GLFWwindow*, double, double);
void loadTransform(std::stringstream&, Transform*);	//load transform from a file (own format)