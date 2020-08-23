#pragma once
#include "PhysicsEngine.h"
#include "RenderEngine.h"
#include "CollisionEngine.h"
#include "AudioEngine.h"
#include "SearchEngine.h"
#include "GameSettings.h"
#include "Actor.h"

class Game: public GameManager
{
	GLFWwindow *Window;

	Actor RootActor;
	CameraComponent* ActiveCamera;	//there are 2 similiar camera variables: ActiveCamera and global MouseContextCamera. the first one is basically the camera we use to see the world (view mat); the second one is updated by mouse controls.

	UniformBuffer MatricesBuffer;

	std::unique_ptr<GameSettings> Settings;
	RenderEngine RenderEng;
	PhysicsEngine PhysicsEng;
	CollisionEngine CollisionEng;
	AudioEngine AudioEng;
	SearchEngine Searcher;

	const ShadingModel Shading;
	bool DebugMode;
	glm::mat4 PreviousFrameView;

public:
	Game(const ShadingModel&);
	virtual void Init(GLFWwindow* window);

	void LoadLevel(std::string);
	virtual void BindActiveCamera(CameraComponent*) override;
	virtual std::shared_ptr<Actor> AddActorToScene(std::shared_ptr<Actor> actor) override; //you use this function to make the game interact (update, draw...) with the actor; without adding it to the scene, the Actor instance isnt updated real-time by default
	virtual SearchEngine* GetSearchEngine() override;

	virtual PhysicsEngineManager* GetPhysicsHandle() override;
	virtual RenderEngineManager* GetRenderEngineHandle() override;
	virtual AudioEngineManager* GetAudioEngineHandle() override;

	virtual const GameSettings* GetGameSettings() override;

	void Run();

	void Update(float);

	void Render();
};

void cursorPosCallback(GLFWwindow*, double, double);
void loadTransform(std::stringstream&, Transform*);	//load transform from a file (own format)