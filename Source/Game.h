#pragma once
#include "PhysicsEngine.h"
#include "RenderEngine.h"
#include "CollisionEngine.h"
#include "AudioEngine.h"
#include "SearchEngine.h"
#include "GameSettings.h"

class Game: public GameManager
{
	GLFWwindow *Window;
	GEE_FB::Framebuffer GFramebuffer;
	GEE_FB::Framebuffer MainFramebuffer;
	Postprocess GamePostprocess;

	Actor RootActor;
	CameraComponent* ActiveCamera;	//there are 2 similiar camera variables: ActiveCamera and global MouseContextCamera. the first one is basically the camera we use to see the world (view mat); the second one is updated by mouse controls.
	std::vector <LightComponent*> Lights;

	UniformBuffer MatricesBuffer, LightsBuffer;

	std::unique_ptr<GameSettings> Settings;
	RenderEngine RenderEng;
	PhysicsEngine PhysicsEng;
	CollisionEngine CollisionEng;
	AudioEngine AudioEng;
	SearchEngine Searcher;

	bool DebugMode;

public:
	Game();
	virtual void Init(GLFWwindow* window);

	void LoadLevel(std::string);

	void SetupLights();
	void UpdateLightUniforms();

	virtual void BindActiveCamera(CameraComponent*) override;
	virtual std::shared_ptr<Actor> AddActorToScene(std::shared_ptr<Actor> actor) override; //you use this function to make the game interact (update, draw...) with the actor; without adding it to the scene, the Actor instance isnt updated real-time by default
	virtual void AddLightToScene(LightComponent* light) override; // this function lets the engine know that the passed light component's data needs to be forwarded to shaders (therefore lighting our meshes)
	virtual int GetAvailableLightIndex() override;
	virtual SearchEngine* GetSearchEngine() override;

	virtual PhysicsEngineManager* GetPhysicsHandle() override;
	virtual RenderEngineManager* GetRenderEngineHandle() override;
	virtual AudioEngineManager* GetAudioEngineHandle() override;
	virtual PostprocessManager* GetPostprocessHandle() override;

	virtual const GameSettings* GetGameSettings() override;

	void Run();

	void Update(float);

	void Render();
};

void cursorPosCallback(GLFWwindow*, double, double);
void loadTransform(std::stringstream&, Transform*);	//load transform from a file (own format)