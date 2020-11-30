#pragma once
#include "PhysicsEngine.h"
#include "RenderEngine.h"
#include "AudioEngine.h"
#include "GameSettings.h"
#include "GameScene.h"
#include "Actor.h"

class Font;
class Controller;

/*
	todo: comment explaining how to use the Game class
	1. Call Init with the window that you created
	2. Call LoadSceneFromFile and load at least the Main Scene, otherwise there will be no scenes and the program will just shut down
	3. Call PreGameLoop
	4. Call GameLoopIteration with every iteration
*/

class Game : public GameManager
{
public:
	Game(const ShadingModel&, const GameSettings&);
	virtual void Init(GLFWwindow* window);

	void LoadSceneFromFile(std::string path);
	void AddScene(std::unique_ptr <GameScene> scene);
	virtual void BindAudioListenerTransformPtr(Transform*) override;
	virtual void PassMouseControl(Controller* controller) override; //pass nullptr to unbind any controller and allow moving the mouse around freely

	virtual PhysicsEngineManager* GetPhysicsHandle() override;
	virtual RenderEngineManager* GetRenderEngineHandle() override;
	virtual AudioEngineManager* GetAudioEngineHandle() override;

	virtual GameSettings* GetGameSettings() override;
	virtual GameScene* GetMainScene() override;
	virtual Actor* GetRootActor(GameScene* scene = nullptr) override;

	virtual void PreGameLoop();
	virtual bool GameLoopIteration(float timeStep, float deltaTime);

	void Update(float);

	virtual void Render() = 0;

protected:
	GLFWwindow* Window;

	std::vector <std::unique_ptr <GameScene>> Scenes;	//The first scene (Scenes[0]) is referred to as the Main Scene. If you only use 1 scene in your application, don't bother with passing arround GameScene pointers - the Main Scene will be chosen automatically.
	Controller* MouseController;

	std::unique_ptr<GameSettings> Settings;
	std::shared_ptr<Font> MyFont;
	RenderEngine RenderEng;
	PhysicsEngine PhysicsEng;
	AudioEngine AudioEng;

	const ShadingModel Shading;
	bool DebugMode;
	float LoopBeginTime;
	float TimeAccumulator;
};

void cursorPosCallback(GLFWwindow*, double, double);
void loadTransform(std::stringstream&, Transform*);	//load transform from a file (own format)