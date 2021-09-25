#pragma once
#include <physics/PhysicsEngine.h>
#include <rendering/RenderEngine.h>
#include <audio/AudioEngine.h>
#include "GameSettings.h"
#include "GameScene.h"
#include <input/Event.h>

namespace GEE
{
	/*
		todo: comment explaining how to use the Game class
		1. Call Init with the window that you created
		2. Call LoadSceneFromFile and load at least the Main Scene, otherwise there will be no scenes and the program will just shut down
		3. Call PreGameLoop
		4. Call GameLoopIteration on every iteration
	*/

	class Game : public GameManager
	{
	public:
		Game(const ShadingModel&, const GameSettings&);
		virtual void Init(SystemWindow* window);

		void LoadSceneFromFile(const std::string& path, const std::string& name = std::string());
		virtual GameScene& CreateScene(const std::string& name, bool isAnUIScene = false) override;
		void AddScene(UniquePtr <GameScene> scene);
		virtual void BindAudioListenerTransformPtr(Transform*) override;

		/**
		 * @brief Checks if the passed transform is bound and unbinds it if that's the case.
		 * @param a pointer to the Transform object to be checked (and potentially unbound)
		*/
		virtual void UnbindAudioListenerTransformPtr(Transform* transform) override;

		virtual void PassMouseControl(Controller* controller) override; //pass nullptr to unbind the current controller.
		virtual const Controller* GetCurrentMouseController() const override;
		virtual SharedPtr<Font> GetDefaultFont() override;
		virtual bool HasStarted() const override;
		InputDevicesStateRetriever GetInputRetriever() override;
		virtual double GetCurrentTime() const override;

		virtual Physics::PhysicsEngineManager* GetPhysicsHandle() override;
		virtual RenderEngineManager* GetRenderEngineHandle() override;
		virtual Audio::AudioEngineManager* GetAudioEngineHandle() override;

		virtual GameSettings* GetGameSettings() override;
		virtual GameScene* GetScene(const std::string& name) override;
		virtual GameScene* GetMainScene() override;
		virtual Actor* GetRootActor(GameScene* scene = nullptr) override;

		virtual void SetActiveScene(GameScene* scene) override;

		virtual void SetCursorIcon(DefaultCursorIcon) override;

		virtual HierarchyTemplate::HierarchyTreeT* FindHierarchyTree(const std::string& name, HierarchyTemplate::HierarchyTreeT* treeToIgnore = nullptr) override;
		virtual SharedPtr<Font> FindFont(const std::string& path) override;

		virtual void PreGameLoop();
		bool GameLoopIteration(float timeStep, float deltaTime);
		virtual void HandleEvents();

		virtual void Update(float deltaTime);

		virtual void Render() = 0;

	protected:
		void TerminateGame();
		void SetMainScene(GameScene*);
		virtual void DeleteScene(GameScene&) override;

		// If bGameTerminated is true, no more GameLoopIterations will occur.
		bool bGameTerminated;

		SystemWindow* Window;

		std::vector <UniquePtr <GameScene>> Scenes;	//The first scene (Scenes[0]) is referred to as the Main Scene. If you only use 1 scene in your application, don't bother with passing arround GameScene pointers - the Main Scene will be chosen automatically.
		GameScene* MainScene;
		GameScene* ActiveScene;	//which scene currently handles events
		EventHolder EventHolderObj;
		Controller* MouseController;

		UniquePtr<GameSettings> Settings;

		std::vector<SharedPtr<Font>> Fonts;
		SharedPtr<Font> DefaultFont;

		RenderEngine RenderEng;
		Physics::PhysicsEngine PhysicsEng;
		Audio::AudioEngine AudioEng;

		const ShadingModel Shading;
		bool GameStarted;
		bool DebugMode;
		float LoopBeginTime;
		float TimeAccumulator;
	};

	struct GLFWEventProcessor
	{
		static void CursorPosCallback(SystemWindow*, double, double);
		static void MouseButtonCallback(SystemWindow*, int button, int action, int mods);
		static void KeyPressedCallback(SystemWindow*, int key, int scancode, int action, int mods);
		static void CharEnteredCallback(SystemWindow*, unsigned int codepoint);
		static void ScrollCallback(SystemWindow*, double xoffset, double yoffset);
		static void FileDropCallback(SystemWindow*, int count, const char** paths);

		static EventHolder* TargetHolder;
	};

}